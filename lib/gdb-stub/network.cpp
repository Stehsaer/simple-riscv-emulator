#include "gdb-stub/network.hpp"

#include <print>

namespace gdb_stub
{
	Network_handler::Network_handler(u16 port)
	{
		io_context = std::make_unique<asio::io_context>();

		acceptor = std::make_unique<asio::ip::tcp::acceptor>(
			*io_context,
			asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)
		);
	}

	void Network_handler::setup_socket()
	{
		if (socket && socket->is_open()) return;

		socket = std::make_unique<asio::ip::tcp::socket>(*io_context);
		acceptor->accept(*socket);
	}

	std::expected<std::string, Network_handler::Error> Network_handler::get_packet_from_decoder()
	{
		const auto timeout_begin = std::chrono::system_clock::now();

		while (!decoder.new_packet_available())
		{
			std::array<char, 1024> buf = {0};
			const auto len = socket->read_some(asio::buffer(buf));

			// Received **some** data
			if (len > 0)
			{
				decoder.push(std::string_view(buf.data(), len));
				continue;
			}

			// Timeout exceeded
			if (std::chrono::system_clock::now() - timeout_begin > std::chrono::milliseconds(timeout_ms))
				return std::unexpected(Error::Protocol_retry);

			std::this_thread::yield();
		}

		return decoder.pop_packet().or_else(
			[](auto err) -> std::expected<std::string, Network_handler::Error>
			{
				switch (err)
				{
				case Packet_decoder::Error::Bad_checksum:
				case Packet_decoder::Error::Bad_packet:
					return std::unexpected(Error::Protocol_retry);
				case Packet_decoder::Error::Buffer_overflow:
					return std::unexpected(Error::Protocol_fail);
				default:
					return std::unexpected(Error::Internal_fail);
				}
			}
		);
	}

	std::expected<void, Network_handler::Error> Network_handler::send(const Response& response)
	{
		const auto data = Packet_encoder::encode(response.to_string());

		try
		{
			if (socket == nullptr) setup_socket();

			for (const auto _ : std::views::iota(0zu, max_retry_count))
			{
				// Step 1: Send the packet
				asio::write(*socket, asio::buffer(data));

				// Step 2: Wait for ACK
				const auto get_packet_result = get_packet_from_decoder();
				if (!get_packet_result)
				{
					if (get_packet_result.error() == Error::Protocol_retry)
						continue;
					else
						return std::unexpected(get_packet_result.error());
				}

				// Step 3: Parse ACK
				const auto parse_command_result = command::parse(get_packet_result.value());
				if (!parse_command_result.has_value()) return std::unexpected(Error::Protocol_fail);
				if (parse_command_result.type() != typeid(Acknowledge_packet))
					return std::unexpected(Error::Protocol_fail);  // Not ACK

				const auto ack_packet = std::any_cast<Acknowledge_packet>(parse_command_result);
				if (!ack_packet.success) continue;  // NAK, resend
				return {};
			}

			return std::unexpected(Error::Protocol_fail);
		}
		catch (const std::exception&)
		{
			close();
			return std::unexpected(Error::Connection_fault);
		}
	}

	std::expected<std::any, Network_handler::Error> Network_handler::receive()
	{
		try
		{
			if (socket == nullptr) setup_socket();

			for (const auto _ : std::views::iota(0zu, max_retry_count))
			{
				// Step 1: Wait for packet
				const auto get_packet_result = get_packet_from_decoder();
				if (!get_packet_result)
				{
					if (get_packet_result.error() == Error::Protocol_retry)
					{
						asio::write(*socket, asio::buffer("-"));  // Send NAK
						continue;
					}
					else
						return std::unexpected(get_packet_result.error());
				}

				// Step 2: Receive successfully, send ACK
				asio::write(*socket, asio::buffer("+"));

				// Step 3: Parse command
				auto parse_command_result = command::parse(get_packet_result.value());
				if (!parse_command_result.has_value()) return std::unexpected(Error::Decode_fail);

				return parse_command_result;
			}

			return std::unexpected(Error::Protocol_fail);
		}
		catch (const std::exception&)
		{
			close();
			return std::unexpected(Error::Connection_fault);
		}
	}

	void Network_handler::close()
	{
		if (socket == nullptr) return;
		if (socket->is_open()) socket->close();
		socket.reset();
	}
}