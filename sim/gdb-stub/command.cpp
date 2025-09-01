#include "gdb-stub/command.hpp"
#include "gdb-stub/special-packet.hpp"

#include <algorithm>
#include <charconv>
#include <format>
#include <ranges>

namespace gdb_stub::command
{
	static const std::any fail{};

	inline namespace util
	{
		template <std::ranges::input_range T>
		static std::optional<u32> parse_hex(T str)
		{
			if (str.empty()) return std::nullopt;

			u32 address;
			const auto end = std::from_chars(str.begin(), str.end(), address, 16);
			return end.ptr == str.end() ? std::optional(address) : std::nullopt;
		}

		static std::optional<u32> parse_hex(const char* begin, const char* end)
		{
			if (begin == end) return std::nullopt;

			u32 address;
			const auto _end = std::from_chars(begin, end, address, 16);
			return _end.ptr == end ? std::optional(address) : std::nullopt;
		}

		template <typename T>
		static std::optional<std::pair<u32, u32>> parse_u32_pair(T str)
		{
			const auto comma_pos = std::ranges::find(str, ',');
			if (comma_pos == str.end()) return std::nullopt;

			const auto first = parse_hex(str.begin(), comma_pos);
			const auto second = parse_hex(comma_pos + 1, str.end());

			if (!first.has_value() || !second.has_value()) return std::nullopt;

			return std::make_pair(first.value(), second.value());
		}

		static std::pair<std::string_view, std::string_view> split_once(std::string_view str, char delim)
		{
			const auto delim_pos = std::ranges::find(str, delim);
			if (delim_pos == str.end()) return std::make_pair(str.substr(0, 0), str.substr(0, 0));

			return std::make_pair(
				std::string_view(str.begin(), delim_pos),
				std::string_view(delim_pos + 1, str.end())
			);
		}

		template <typename T>
		static bool optional_has_value(std::optional<T> opt)
		{
			return opt.has_value();
		}

		template <typename T>
		static bool remove_optional(std::optional<T>&& opt)
		{
			return opt.value();
		}
	}

	static std::any parse_continue(std::string_view params)
	{
		if (params.empty()) return Continue{.address = std::nullopt};

		const auto address = parse_hex(params);
		if (!address.has_value()) return fail;
		return Continue{.address = address};
	}

	static std::any parse_write_registers(std::string_view params)
	{
		if (params.empty() || params.length() % 8 != 0) return fail;

		auto parse_func = std::views::transform(
			[](auto subrange) -> std::optional<u32>
			{
				if (std::ranges::equal(subrange, std::string_view("xxxxxxxx"))) return std::nullopt;
				const auto parse_result = parse_hex(subrange);
				if (!parse_result.has_value()) [[unlikely]]
					throw 0;
				return parse_result;
			}
		);

		try
		{
			return Write_register{
				.values
				= params
				| std::views::chunk(8)
				| parse_func
				| std::views::enumerate
				| std::views::filter([](auto val) { return std::get<1>(val).has_value(); })
				| std::views::transform(
					  [](auto tup) { return std::make_pair((u16)std::get<0>(tup), std::get<1>(tup).value()); }
				)
				| std::ranges::to<std::map>()
			};
		}
		catch (int)
		{
			return fail;
		}
	}

	static std::any parse_step_single_cycle(std::string_view params)
	{
		const auto subparams = params
							 | std::views::split(',')
							 | std::views::transform([](auto subrange) { return parse_hex(subrange); })
							 | std::ranges::to<std::vector>();

		if (!std::ranges::all_of(subparams, optional_has_value<u32>) > 0) return fail;

		switch (subparams.size())
		{
		case 0:
			return Step_cycles{};
		case 1:
			return Step_cycles{.address = subparams[0], .cycle_count = std::nullopt};
		case 2:
			return Step_cycles{.address = subparams[0], .cycle_count = subparams[1]};
		default:
			return fail;
		}
	}

	static std::any parse_read_memory(std::string_view params)
	{
		const auto parse_result = parse_u32_pair(params);
		if (!parse_result) return fail;
		const auto [addr, length] = parse_result.value();

		return Read_memory{.address = addr, .length = length};
	}

	static std::any parse_write_memory(std::string_view params)
	{
		const auto [addr_length_str, data_str] = split_once(params, ':');
		if (addr_length_str.empty() || data_str.empty()) return fail;

		// Parse `addr` and `length`
		const auto parse_pair_result = parse_u32_pair(addr_length_str);
		if (!parse_pair_result) return fail;
		const auto [addr, length] = parse_pair_result.value();

		// Parse `XXXX...`
		if (data_str.size() != length * 2) return fail;
		const auto data_view
			= data_str
			| std::views::chunk(2)
			| std::views::transform([](auto subrange) { return std::optional<u8>(parse_hex(subrange)); });
		if (!std::ranges::all_of(data_view, optional_has_value<u8>)) return fail;

		return Write_memory{
			.address = addr,
			.data = data_view
				  | std::views::transform([](auto opt) { return opt.value(); })
				  | std::ranges::to<std::vector>()
		};
	}

	static std::any parse_read_single_register(std::string_view params)
	{
		if (params.empty() || params.length() > 4) return fail;

		const auto address = parse_hex(params);
		if (!address.has_value()) return fail;
		if (*address >= 65536) return fail;

		return Read_single_register{.regno = *address};
	}

	static std::any parse_write_single_register(std::string_view params)
	{
		if (params.empty()) return fail;

		const auto [address_str, value_str] = split_once(params, '=');
		if (address_str.empty() || value_str.empty() || address_str.length() > 4 || value_str.length() != 8)
			return fail;

		const auto address = parse_hex(address_str);
		if (!address.has_value()) return fail;
		if (*address >= 65536) return fail;

		const auto value = parse_hex(value_str);
		if (!value.has_value()) return fail;

		return Write_single_register{.regno = *address, .value = *value};
	}

	static std::any parse_step_single(std::string_view params)
	{
		if (params.empty()) return Step_single_inst{.address = std::nullopt};

		const auto address = parse_hex(params);
		if (!address.has_value()) return fail;
		return Step_single_inst{.address = address};
	}

	static std::any parse_q_supported(std::string_view params)
	{
		using namespace std::string_view_literals;

		return Query_supported{
			.features = params
					  | std::views::split(';')

					  /* Filter those doesn't contain one of `+-?=` */
					  | std::views::transform(
							[](auto subrange)
							{
								return std::pair(
									subrange,
									&*std::ranges::find_first_of(subrange | std::views::reverse, "+-?="sv)
								);
							}
					  )
					  | std::views::filter([](auto pair) { return pair.first.end() != pair.second; })

					  /* Parse every slice */
					  | std::views::transform(
							[](auto pair) -> std::pair<std::string, Query_supported::Host_feature_status>
							{
								const auto [subrange, sep] = pair;
								std::string name{subrange.begin(), sep};

								switch (*sep)
								{
								case '+':
									return std::make_pair(
										std::move(name),
										Query_supported::Host_feature_status{
											.property = Query_supported::Host_feature_status::Supported,
										}
									);

								case '-':
									return std::make_pair(
										std::move(name),
										Query_supported::Host_feature_status{
											.property = Query_supported::Host_feature_status::Unsupported,
										}
									);

								case '?':
									return std::make_pair(
										std::move(name),
										Query_supported::Host_feature_status{
											.property = Query_supported::Host_feature_status::Unknown,
										}
									);

								case '=':
									return std::make_pair(
										std::move(name),
										Query_supported::Host_feature_status{
											.property = Query_supported::Host_feature_status::Value,
											.value = std::string(sep + 1, subrange.end())
										}
									);

								default:
									throw std::logic_error(
										std::format("Unexpect switching character: \\x{:02x}", *sep)
									);
								}
							}
					  )

					  /* Convert to Map */
					  | std::ranges::to<std::map>()
		};
	}

	static std::any parse_qxfer_feature_read(std::string_view params)
	{
		const auto [annex_str, offset_length_str] = split_once(params, ':');

		const auto parse_offset_length_result = parse_u32_pair(offset_length_str);
		if (!parse_offset_length_result.has_value()) return fail;
		const auto [offset, length] = parse_offset_length_result.value();

		return Read_feature_xml{.annex = std::string(annex_str), .offset = offset, .length = length};
	}

	static std::any parse_qxfer_memorymap_read(std::string_view params)
	{
		const auto [annex_str, offset_length_str] = split_once(params, ':');

		const auto parse_offset_length_result = parse_u32_pair(offset_length_str);
		if (!parse_offset_length_result.has_value()) return fail;
		const auto [offset, length] = parse_offset_length_result.value();

		return Read_memory_map_xml{.offset = offset, .length = length};
	}

	static std::any parse_qxfer(std::string_view params)
	{
		if (params.starts_with("features:read"))
			return parse_qxfer_feature_read(params.substr(std::string_view("features:read:").length()));
		if (params.starts_with("memory-map:read"))
			return parse_qxfer_memorymap_read(params.substr(std::string_view("memory-map:read:").length()));

		return fail;
	}

	static std::any parse_q(std::string_view params)
	{
		const auto [sub_operand, sub_params] = split_once(params, ':');
		if (sub_operand.empty() || sub_params.empty()) return fail;

		if (sub_operand == "Supported") return parse_q_supported(sub_params);
		if (sub_operand == "Xfer") return parse_qxfer(sub_params);
		return fail;
	}

	static std::optional<std::vector<u8>> parse_bytecode(std::string_view str_rep)
	{
		if (str_rep.empty()) return std::nullopt;
		if (str_rep[0] != 'X') return std::nullopt;

		const auto [length_str, data_str] = split_once(str_rep.substr(1), ',');
		if (length_str.empty() || data_str.empty()) return std::nullopt;

		const auto parse_length_result = parse_hex(length_str);
		if (!parse_length_result) return std::nullopt;
		if (data_str.length() != parse_length_result.value() * 2) return std::nullopt;

		const auto data_vec
			= data_str
			| std::views::chunk(2)
			| std::views::transform([](auto subrange) { return std::optional<u8>(parse_hex(subrange)); });
		if (!std::ranges::all_of(data_vec, optional_has_value<u8>)) return std::nullopt;

		return data_vec
			 | std::views::transform([](auto opt) { return opt.value(); })
			 | std::ranges::to<std::vector>();
	}

	static std::any parse_z(std::string_view params)
	{
		if (params.length() <= 2) return fail;
		if (params[1] != ',') return fail;

		const auto parse_addr_length_result = parse_u32_pair(params.substr(2));
		if (!parse_addr_length_result) return fail;
		const auto [addr, length] = parse_addr_length_result.value();

		switch (params[0])
		{
		case '0':
			return Remove_breakpoint{.is_hardware = false, .address = addr, .length = length};
		case '1':
			return Remove_breakpoint{.is_hardware = true, .address = addr, .length = length};
		case '2':
			return Remove_watchpoint{
				.watch_write = true,
				.watch_read = false,
				.address = addr,
				.length = length
			};
		case '3':
			return Remove_watchpoint{
				.watch_write = false,
				.watch_read = true,
				.address = addr,
				.length = length
			};

		case '4':
			return Remove_watchpoint{
				.watch_write = true,
				.watch_read = true,
				.address = addr,
				.length = length
			};

		default:
			return fail;
		}
	}

	static std::any parse_Z(std::string_view params)
	{
		if (params.length() <= 2) return fail;
		if (params[1] != ',') return fail;

		const auto subparams = params.substr(2);

		switch (params[0])
		{
		case '0':
		case '1':
		{
			const auto subparam_list = subparams | std::views::split(';') | std::ranges::to<std::vector>();
			if (subparam_list.size() > 3 || subparam_list.empty()) return fail;

			const auto parse_addr_length_result = parse_u32_pair(subparam_list[0]);
			if (!parse_addr_length_result) return fail;
			const auto [addr, length] = parse_addr_length_result.value();

			std::optional<std::vector<u8>> bytecode = std::nullopt;
			if (subparam_list.size() >= 2)
			{
				bytecode = parse_bytecode(std::string_view(subparam_list[1]));
				if (!bytecode) return fail;
			}

			return Add_breakpoint{
				.is_hardware = params[0] == '1',
				.address = addr,
				.length = length,
				.cond = std::move(bytecode)
			};
		}
		break;
		case '2':
		case '3':
		case '4':
		{
			const auto parse_addr_length_result = parse_u32_pair(subparams);
			if (!parse_addr_length_result) return fail;
			const auto [addr, length] = parse_addr_length_result.value();

			return Add_watchpoint{
				.watch_write = (params[0] == '2' || params[0] == '4'),
				.watch_read = (params[0] == '3' || params[0] == '4'),
				.address = addr,
				.length = length
			};
		}
		break;
		default:
			break;
		}

		return fail;
	}

	using Parse_func = std::any(std::string_view);

	static const std::map<char, Parse_func*> param_char_command_parsers = {
		{'c', parse_continue             },
		{'G', parse_write_registers      },
		{'i', parse_step_single_cycle    },
		{'m', parse_read_memory          },
		{'M', parse_write_memory         },
		{'p', parse_read_single_register },
		{'P', parse_write_single_register},
		{'s', parse_step_single          },
		{'q', parse_q                    },
		{'z', parse_z                    },
		{'Z', parse_Z                    },
	};

	static const std::map<char, std::any> single_char_command = {
		{'+',    Acknowledge_packet{.success = true} },
		{'-',    Acknowledge_packet{.success = false}},
		{'\x03', Interrupt_packet{}                  },
		{'!',    Enable_persistant{}                 },
		{'?',    Ask_halt_reason{}                   },
		{'g',    Read_register{}                     },
		{'k',    Stop{}							  }
	};

	std::any parse(std::string_view command)
	{
		if (command.empty()) return fail;

		if (command.length() == 1)
		{
			const auto find_single_char = single_char_command.find(command[0]);
			if (find_single_char != single_char_command.end()) return find_single_char->second;
		}

		const auto find_parser = param_char_command_parsers.find(command[0]);
		if (find_parser != param_char_command_parsers.end()) return find_parser->second(command.substr(1));

		return fail;
	}

}