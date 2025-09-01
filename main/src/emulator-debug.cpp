#include "emulator-debug.hpp"
#include "core/print.hpp"
#include "gdb-stub/accessor.hpp"
#include "gdb-stub/gdb-xml.hpp"
#include "memory-map.hpp"

using namespace gdb_stub;
namespace cmd = command;

Emulator_debug::Emulator_debug(Emulator&& emu, const Options& options) :
	Emulator(std::move(emu)),
	command_handlers({
		{&typeid(cmd::Query_supported),       [this](const std::any& cmd) { handle_qsupport(cmd); }          },
		{&typeid(cmd::Read_feature_xml),      [this](const std::any& cmd) { handle_qxfer_feature(cmd); }     },
		{&typeid(cmd::Read_memory_map_xml),   [this](const std::any& cmd) { handle_qxfer_memory_map(cmd); }  },
		{&typeid(cmd::Ask_halt_reason),       [this](const std::any& cmd) { handle_query(cmd); }             },
		{&typeid(cmd::Read_memory),           [this](const std::any& cmd) { handle_mem_read(cmd); }          },
		{&typeid(cmd::Write_memory),          [this](const std::any& cmd) { handle_mem_write(cmd); }         },
		{&typeid(cmd::Read_register),         [this](const std::any& cmd) { handle_reg_read(cmd); }          },
		{&typeid(cmd::Write_register),        [this](const std::any& cmd) { handle_reg_write(cmd); }         },
		{&typeid(cmd::Read_single_register),  [this](const std::any& cmd) { handle_single_reg_read(cmd); }   },
		{&typeid(cmd::Write_single_register), [this](const std::any& cmd) { handle_single_reg_write(cmd); }  },
		{&typeid(cmd::Continue),              [this](const std::any& cmd) { handle_continue(cmd); }          },
		{&typeid(cmd::Step_single_inst),      [this](const std::any& cmd) { handle_step(cmd); }              },
		{&typeid(cmd::Step_cycles),           [this](const std::any& cmd) { handle_step_single_cycles(cmd); }},
		{&typeid(cmd::Add_breakpoint),        [this](const std::any& cmd) { handle_add_breakpoint(cmd); }    },
		{&typeid(cmd::Remove_breakpoint),     [this](const std::any& cmd) { handle_remove_breakpoint(cmd); } },
		{&typeid(cmd::Add_watchpoint),        [this](const std::any& cmd) { handle_add_watchpoint(cmd); }    },
		{&typeid(cmd::Remove_watchpoint),     [this](const std::any& cmd) { handle_remove_watchpoint(cmd); } },
})
{
	network = std::make_unique<Network_handler>(options.debug_port);

	iprintln("GDB stub listening on port {}", options.debug_port);
}

bool Emulator_debug::check_breakpoint()
{
	return hw_breakpoints.contains(platform->cpu->pc)
		&& hw_breakpoints.at(platform->cpu->pc).is_triggered_by(*platform->cpu);
}

std::optional<std::pair<bool, bool>> Emulator_debug::check_watchpoint(const core::CPU_module::Result& result)
{
	const auto mode = result.memory_opcode;

	if (mode == core::Load_store_module::Opcode::None) [[likely]]
		return std::nullopt;

	const auto write = mode == core::Load_store_module::Opcode::Store;
	const auto read = mode == core::Load_store_module::Opcode::Load;

	const auto find = watchpoints.find(
		Address_range{.start = result.alu_result, .size = (u32)core::get_size(result.memory_funct)}
	);
	if (find == watchpoints.end()) [[likely]]
		return std::nullopt;

	if (!(write && find->second.watch_write) && !(read && find->second.watch_read)) return std::nullopt;

	return std::make_pair(read && find->second.watch_read, write && find->second.watch_write);
}

void Emulator_debug::send_response(const Response& response)
{
	if (!network->send(response).has_value()) network->close();
}

void Emulator_debug::handle_qsupport(const std::any& command [[maybe_unused]])
{
	send_response(response::Qsupported_response());
}

void Emulator_debug::handle_qxfer_feature(const std::any& command)
{
	const auto cmd = std::any_cast<const command::Read_feature_xml>(command);
	const auto get_xml_result = get_xml_file(cmd.annex, cmd.offset, cmd.length);

	if (!get_xml_result)
		send_response(response::Error_message(std::format("Unknown annex: {}", cmd.annex)));
	else
		send_response(response::Qxfer_response(get_xml_result->is_end, get_xml_result->data));
}

void Emulator_debug::handle_qxfer_memory_map(const std::any& command)
{
	const auto cmd = std::any_cast<const command::Read_memory_map_xml>(command);
	const auto is_end = cmd.offset + cmd.length >= memory_map_xml.size();

	send_response(
		response::Qxfer_response(
			is_end,
			memory_map_xml
				| std::views::drop(cmd.offset)
				| std::views::take(cmd.length)
				| std::ranges::to<std::vector>()
		)
	);
}

void Emulator_debug::handle_query(const std::any& command [[maybe_unused]])
{
	send_response(response::Stop_reason(SIGINT));
}

void Emulator_debug::handle_mem_read(const std::any& command)
{
	const auto cmd = std::any_cast<const command::Read_memory>(command);

	std::vector<u8> data;
	data.reserve(cmd.length);

	for (const auto addr_range :
		 std::views::iota(cmd.address)
			 | std::views::take(cmd.length)
			 | std::views::chunk_by([](u32 prev, u32 next) { return prev % 4 != 3 || next % 4 != 0; }))
	{
		const auto read_result = platform->memory->read(addr_range.front());
		if (!read_result) break;

		const auto bytes = std::bit_cast<std::array<u8, 4>>(read_result.value());
		for (const auto addr : addr_range) data.push_back(bytes[addr % 4]);
	}

	send_response(response::Raw_byte_stream(data));
}

void Emulator_debug::handle_mem_write(const std::any& command)
{
	const auto cmd = std::any_cast<const command::Write_memory>(command);

	for (const auto addr : std::views::iota(cmd.address) | std::views::take(cmd.data.size()))
	{
		const core::Bitset<4> byte_enable = 1 << (addr % 4);
		const u32 write_data = static_cast<u32>(cmd.data[addr - cmd.address]) << ((addr % 4) * 8);
		if (!platform->memory->write(addr, write_data, byte_enable).has_value())
		{
			send_response(response::Error_code(0));
			return;
		}
	}

	send_response(response::OK());
}

void Emulator_debug::handle_reg_read(const std::any& command [[maybe_unused]])
{
	const CPU_register_accessor reg_accessor{.cpu = *platform->cpu};

	std::vector<u16> regnums;
	regnums.append_range(std::views::iota(0u, 33u));
	regnums.append_range(
		core::CSR_module::metadata.get<core::CSR_metadata::Key_address>()
		| std::views::transform([](const auto& meta) { return meta.address + 128; })
	);

	send_response(
		response::Register_content(
			regnums
			| std::views::transform([&reg_accessor](auto regno) { return reg_accessor.read(regno); })
			| std::ranges::to<std::vector>()
		)
	);
}

void Emulator_debug::handle_reg_write(const std::any& command)
{
	const auto cmd = std::any_cast<const command::Write_register>(command);
	const CPU_register_accessor reg_accessor{.cpu = *platform->cpu};

	for (const auto& [regno, value] : cmd.values) reg_accessor.write(regno, value);

	send_response(response::OK());
}

void Emulator_debug::handle_single_reg_read(const std::any& command)
{
	const auto cmd = std::any_cast<const command::Read_single_register>(command);
	const CPU_register_accessor reg_accessor{.cpu = *platform->cpu};
	const auto read_result = reg_accessor.read(cmd.regno);
	send_response(response::Single_register_content(read_result));
}

void Emulator_debug::handle_single_reg_write(const std::any& command)
{
	const auto cmd = std::any_cast<const command::Write_single_register>(command);
	const CPU_register_accessor reg_accessor{.cpu = *platform->cpu};
	reg_accessor.write(cmd.regno, cmd.value);
	send_response(response::OK());
}

void Emulator_debug::handle_add_breakpoint(const std::any& command)
{
	const auto cmd = std::any_cast<const command::Add_breakpoint>(command);

	if (!cmd.is_hardware)
	{
		send_response(response::Unsupported_command());
		return;
	}

	if (cmd.length != 4)
	{
		send_response(response::Error_message("Only 4-byte breakpoints are supported"));
		return;
	}

	hw_breakpoints.emplace(cmd.address, Hw_breakpoint::from(cmd));
	send_response(response::OK());
}

void Emulator_debug::handle_remove_breakpoint(const std::any& command)
{
	const auto cmd = std::any_cast<const command::Remove_breakpoint>(command);
	if (!cmd.is_hardware)
	{
		send_response(response::Unsupported_command());
		return;
	}

	if (hw_breakpoints.erase(cmd.address) == 0)
	{
		send_response(response::Error_message("No such breakpoint"));
		return;
	}

	send_response(response::OK());
}

void Emulator_debug::handle_add_watchpoint(const std::any& command)
{
	const auto cmd = std::any_cast<const command::Add_watchpoint>(command);

	if (cmd.length == 0)
	{
		send_response(response::Error_message("Watchpoint length must be greater than 0"));
		return;
	}

	if (!cmd.watch_read && !cmd.watch_write)
	{
		send_response(response::Error_message("Watchpoint must watch read or write"));
		return;
	}

	watchpoints.emplace(
		Address_range{
			.start = cmd.address,
			.size = cmd.length
    },
		Watchpoint{
			.watch_write = cmd.watch_write,
			.watch_read = cmd.watch_read,
			.addr_range = {.start = cmd.address, .size = cmd.length}
		}
	);

	send_response(response::OK());
}

void Emulator_debug::handle_remove_watchpoint(const std::any& command)
{
	const auto cmd = std::any_cast<const command::Remove_watchpoint>(command);
	const auto hit = watchpoints.find(Address_range{.start = cmd.address, .size = cmd.length});

	if (hit == watchpoints.end())
	{
		send_response(response::Error_message("No such watchpoint"));
		return;
	}

	watchpoints.erase(hit);
	send_response(response::OK());
}

void Emulator_debug::async_run(
	const std::function<gdb_stub::response::Stop_reason(const std::atomic<bool>&)>& run_func
)
{
	std::atomic<bool> interrupt = false;
	auto result = std::async(std::launch::async, run_func, std::ref(interrupt));

	while (true)
	{
		std::this_thread::yield();

		if (!result.valid()) continue;
		const auto status = result.wait_for(std::chrono::milliseconds(50));
		if (status == std::future_status::ready) break;

		const auto recv_command_result = network->receive();
		if (!recv_command_result) continue;

		const auto& command = *recv_command_result;
		if (command.type() == typeid(Interrupt_packet) || command.type() == typeid(command::Ask_halt_reason))
			interrupt = true;
		else
		{
			network->close();
			return;
		}
	}

	send_response(result.get());
}

void Emulator_debug::handle_continue(const std::any& command)
{
	const auto cmd = std::any_cast<const command::Continue>(command);
	if (cmd.address.has_value()) platform->cpu->pc = *cmd.address;

	async_run([this](const std::atomic<bool>& interrupt) { return run_until_trap(interrupt); });
}

void Emulator_debug::handle_step(const std::any& command)
{
	const auto cmd = std::any_cast<const command::Step_single_inst>(command);
	if (cmd.address.has_value()) platform->cpu->pc = *cmd.address;

	async_run([this](const std::atomic<bool>& interrupt) { return run_steps(1, interrupt); });
}

void Emulator_debug::handle_step_single_cycles(const std::any& command)
{
	const auto cmd = std::any_cast<const command::Step_cycles>(command);
	if (cmd.address.has_value()) platform->cpu->pc = *cmd.address;

	async_run([this, cycle_count = cmd.cycle_count.value_or(1)](const std::atomic<bool>& interrupt)
			  { return run_steps(cycle_count, interrupt); });
}

auto Emulator_debug::handle_special_commands(const std::any& command) -> Special_command_handle_result
{
	if (command.type() == typeid(Acknowledge_packet)) return Special_command_handle_result::Continue;
	if (command.type() == typeid(command::Restart))
	{
		iprintln("Emulator restarting, requested by GDB");
		platform->memory->ram->reset_content();
		return Special_command_handle_result::Continue;
	}
	if (command.type() == typeid(command::Stop))
	{
		iprintln("Emulator stopping, requested by GDB");
		return Special_command_handle_result::Stop;
	}

	return Special_command_handle_result::Unhandled;
}

void Emulator_debug::run()
{
	if (network == nullptr) throw std::logic_error("Network Handler not initialized");

	while (true)
	{
		const auto recv_command_result = network->receive();

		if (!recv_command_result)
		{
			using Error = Network_handler::Error;
			switch (recv_command_result.error())
			{
			case Error::Internal_fail:
				wprintln("Internal error!");
				network->close();
				return;

			case Error::Connection_fault:
				wprintln("Connection to GDB lost");
				network->close();
				continue;

			case Error::Protocol_fail:
				wprintln("GDB protocol violation detected");
				network->close();
				continue;

			case Error::Decode_fail:
				send_response(response::Unsupported_command());
				continue;

			default:
				wprintln("Unknown network error");
				continue;
			}
		}

		const auto& command = *recv_command_result;

		const auto find_handler = command_handlers.find(&command.type());
		if (find_handler == command_handlers.end())
		{
			const auto special_handle_result = handle_special_commands(command);

			switch (special_handle_result)
			{
			case Special_command_handle_result::Unhandled:
				break;
			case Special_command_handle_result::Continue:
				continue;
			case Special_command_handle_result::Stop:
				return;
			}

			wprintln("Unhandled command: {}", command.type().name());
			network->close();
			return;
		}

		find_handler->second(command);
	}
}

response::Stop_reason Emulator_debug::run_until_trap(const std::atomic<bool>& interrupt)
{
	while (true)
	{
		const auto result = tick_one_cycle();

		const bool is_breakpoint = check_breakpoint();
		const auto watchpoint_hit = check_watchpoint(result);

		if (is_breakpoint)
		{
			return response::Stop_reason::Breakpoint_hit{.is_hardware = true};
		}

		if (watchpoint_hit.has_value())
		{
			return response::Stop_reason::Watchpoint_hit{
				.address = result.alu_result,
				.is_write = watchpoint_hit->second,
				.is_read = watchpoint_hit->first,
			};
		}

		if (interrupt.load()) [[unlikely]]
			return SIGINT;
	}

	return SIGTRAP;
}

response::Stop_reason Emulator_debug::run_steps(size_t cycle_count, const std::atomic<bool>& interrupt)
{
	for (const auto _ : std::views::iota(0zu, cycle_count))
	{
		const auto result = tick_one_cycle();

		const bool is_breakpoint = check_breakpoint();
		const auto watchpoint_hit = check_watchpoint(result);

		if (is_breakpoint) [[unlikely]]
		{
			return response::Stop_reason::Breakpoint_hit{.is_hardware = true};
		}

		if (watchpoint_hit.has_value()) [[unlikely]]
		{
			return response::Stop_reason::Watchpoint_hit{
				.address = result.alu_result,
				.is_write = watchpoint_hit->second,
				.is_read = watchpoint_hit->first,
			};
		}

		if (interrupt.load()) [[unlikely]]
			return SIGINT;
	}

	return SIGTRAP;
}
