#pragma once

#include "command.hpp"
#include "common/type.hpp"
#include "packet.hpp"
#include "response.hpp"

#include <asio.hpp>
#include <chrono>

namespace gdb_stub
{
	/**
	 * @brief Network handler for GDB remote protocol
	 * @details This wrapper wraps: ASIO networking, packet encoding/decoding and command parsing. It
	 * automatically responds `+/-` to host, but doesn't handle the actual response logic.
	 */
	class Network_handler
	{
	  public:

		enum class Error
		{
			Internal_fail,     // Internal failure
			Connection_fault,  // Fault in the connection
			Protocol_fail,     // Protocol violation
			Protocol_retry,    // Protocol violation potentially caused by bad connection quality
			Decode_fail        // Unknown command
		};

		Network_handler(const Network_handler&) = delete;
		Network_handler(Network_handler&&) = delete;
		Network_handler& operator=(const Network_handler&) = delete;
		Network_handler& operator=(Network_handler&&) = delete;

		/**
		 * @brief Create a new network handler that listens on the given port
		 *
		 * @param port TCP port
		 */
		Network_handler(u16 port);

		/**
		 * @brief Sends a response to GDB and returns the status
		 *
		 * @param response Response object
		 */
		std::expected<void, Error> send(const Response& response);

		/**
		 * @brief Receives a command from GDB
		 *
		 * @return `Command` if successful, `Error` otherwise
		 * @note Handle `Error` carefully, call `close()` if necessary
		 */
		std::expected<std::any, Error> receive();

		/**
		 * @brief Forcefully close the connection to GDB
		 *
		 */
		void close();

	  private:

		static constexpr size_t max_retry_count = 5;
		static constexpr size_t timeout_ms = 5000;

		std::unique_ptr<asio::io_context> io_context;
		std::unique_ptr<asio::ip::tcp::acceptor> acceptor;
		std::unique_ptr<asio::ip::tcp::socket> socket;

		Packet_decoder decoder;

		/**
		 * @brief Setup the socket if it is not already open
		 *
		 */
		void setup_socket();

		/**
		 * @brief Get a packet from the decoder, waiting if necessary
		 *
		 * @retval std::string the packet string
		 * @retval Error On failure
		 */
		std::expected<std::string, Network_handler::Error> get_packet_from_decoder();
	};
}