#include "gdb-stub/packet.hpp"

#include <algorithm>
#include <iomanip>
#include <print>
#include <ranges>

namespace gdb_stub
{
	namespace algo
	{
		u8 get_checksum(std::string_view body)
		{
			return std::ranges::fold_left(
				body | std::views::transform([](char c) { return static_cast<u8>(c); }),
				u8(0),
				std::plus()
			);
		}

		std::optional<std::string> remove_escape(std::string_view body)
		{
			std::string result;

			for (size_t i = 0; i < body.length(); i++)
			{
				if (body[i] == '}')
				{
					i++;
					if (i >= body.length()) return std::nullopt;
					if (body[i] == '}') return std::nullopt;

					result.push_back(static_cast<char>(body[i] ^ 0x20));
				}
				else
				{
					result.push_back(body[i]);
				}
			}

			return result;
		}
	}

	std::expected<std::string, Packet_decoder::Error> Packet_decoder::decode_in_buffer()
	{
		try
		{
			if (!std::ranges::all_of(checksum_buffer, [](char c) { return std::isxdigit(c); }))
				return std::unexpected(Error::Bad_packet);

			const std::string checksum_str = {checksum_buffer[0], checksum_buffer[1]};
			const u8 received_checksum = static_cast<u8>(std::stoi(checksum_str, nullptr, 16));
			const u8 computed_checksum = algo::get_checksum(in_buffer);

			if (received_checksum != computed_checksum) return std::unexpected(Error::Bad_checksum);

			const auto remove_escape_result = algo::remove_escape(in_buffer);
			if (!remove_escape_result) return std::unexpected(Error::Bad_packet);
			return remove_escape_result.value();
		}
		catch (...)
		{
			return std::unexpected(Error::Internal_error);
		}
	}

	void Packet_decoder::push(std::string_view new_input)
	{
		auto push_result = [&](std::expected<std::string, Error> result)
		{
			const std::scoped_lock lock(out_queue_mutex);
			out_queue.emplace(std::move(result));
			out_queue_available = true;
			in_buffer.clear();
		};

		for (const char c : new_input)
		{
			switch (state)
			{
			case State::Waiting_dollar:
				if (c == '$')
					state = State::Receiving_body;
				else if (c == '+' || c == '-' || c == '\x03')  // ACK, NAK, Ctrl-C
					push_result(std::string({c}));
				break;

			case State::Receiving_body:
				if (c == '#')
				{
					state = State::Receiving_checksum1;
					break;
				}

				if (c == '$')
				{
					push_result(std::unexpected(Error::Bad_packet));
					state = State::Receiving_body;
					break;
				}

				if (in_buffer.size() >= max_buffer_size)
				{
					push_result(std::unexpected(Error::Buffer_overflow));
					state = State::Waiting_dollar;
					break;
				}

				in_buffer.push_back(c);
				break;

			case State::Receiving_checksum1:
				if (c == '$')
				{
					push_result(std::unexpected(Error::Bad_packet));
					state = State::Receiving_body;
					break;
				}

				if (c == '+' || c == '-' || c == '\x03')
				{
					push_result(std::unexpected(Error::Bad_packet));
					push_result(std::string({c}));
					state = State::Waiting_dollar;
					break;
				}

				checksum_buffer[0] = c;
				state = State::Receiving_checksum2;
				break;

			case State::Receiving_checksum2:
				if (c == '$')
				{
					push_result(std::unexpected(Error::Bad_packet));
					state = State::Receiving_body;
					break;
				}

				if (c == '+' || c == '-' || c == '\x03')
				{
					push_result(std::unexpected(Error::Bad_packet));
					push_result(std::string({c}));
					state = State::Waiting_dollar;
					break;
				}

				checksum_buffer[1] = c;
				push_result(decode_in_buffer());
				state = State::Waiting_dollar;
				break;
			}
		}
	}

	bool Packet_decoder::new_packet_available()
	{
		return out_queue_available;
	}

	std::expected<std::string, Packet_decoder::Error> Packet_decoder::pop_packet()
	{
		const std::scoped_lock lock(out_queue_mutex);

		if (out_queue.empty()) return std::unexpected(Error::No_new_packet);

		auto packet = std::move(out_queue.front());
		out_queue.pop();
		out_queue_available = !out_queue.empty();

		return packet;
	}

	void Packet_encoder::push_repeat()
	{
		if (repeat == 0 || last_char == '\0') return;

		if (repeat <= 2)
		{
			stream << std::string(repeat, last_char);
			return;
		}

		if (repeat == 6)
		{
			stream << std::format("*\"{}", last_char);
			return;
		}
		if (repeat == 7)
		{
			stream << std::format("*\"{0}{0}", last_char);
			return;
		}

		stream << std::format("*{}", char(repeat + 29));
	}

	void Packet_encoder::push(char c)
	{
		if (c == '\0') return;

		if (c != last_char || repeat >= 126 - 29)
		{
			push_repeat();
			last_char = c;
			repeat = 0;

			if (c == '*' || c == '$' || c == '}' || c == '#')  // escape characters
			{
				stream << '}';
				stream << char(u8(c) ^ 0x20);
			}
			else  // normal characters
				stream << c;
		}
		else
		{
			repeat++;
		}
	}

	std::string Packet_encoder::internal_encode(std::string_view str)
	{
		for (auto c : str) push(c);
		push_repeat();
		const auto result_str = stream.str();
		const auto checksum = algo::get_checksum(result_str);

		return std::format("${}#{:02x}", result_str, checksum);
	}

	std::string Packet_encoder::encode(std::string_view str)
	{
		Packet_encoder encoder;
		return encoder.internal_encode(str);
	}
}
