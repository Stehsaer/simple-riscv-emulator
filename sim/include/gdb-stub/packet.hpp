#pragma once

#include "common/type.hpp"

#include <atomic>
#include <expected>
#include <mutex>
#include <optional>
#include <queue>
#include <sstream>
#include <string>
#include <string_view>

namespace gdb_stub
{
	namespace algo
	{
		/**
		 * @brief Get checksum of a string
		 * @note Empty string has checksum 0
		 *
		 * @param body Input string
		 * @return Checksum (modulo 256)
		 */
		u8 get_checksum(std::string_view body);

		/**
		 * @brief Decode a string by parsing escape characters
		 *
		 * @param body Input string
		 * @return Decoded string, or `std::nullopt` if the input was invalid
		 */
		std::optional<std::string> remove_escape(std::string_view body);
	}

	/**
	 * @brief Decoder for decoding packets from GDB. Supports streaming input.
	 *
	 */
	class Packet_decoder
	{
	  public:

		static constexpr size_t max_buffer_size = 1048576;

		enum class Error
		{
			Bad_checksum,
			Bad_packet,
			Buffer_overflow,
			Internal_error,
			No_new_packet
		};

		/**
		 * @brief Push new input data into the decoder
		 * @note Not every input will produce a new packet. Use `new_packet_available()` to query.
		 *
		 * @param new_input New input data
		 */
		void push(std::string_view new_input);

		/**
		 * @brief Pop a new packet from decoder
		 *
		 * @return `std::string` if successful, `Error` otherwise
		 */
		std::expected<std::string, Error> pop_packet();

		/**
		 * @brief Queries if a new packet is available
		 *
		 * @return `true` if a new packet is available, `false` otherwise
		 */
		bool new_packet_available();

	  private:

		std::string in_buffer;
		std::queue<std::expected<std::string, Error>> out_queue;

		std::mutex out_queue_mutex;
		std::atomic<bool> out_queue_available = false;

		enum class State
		{
			Waiting_dollar,
			Receiving_body,
			Receiving_checksum1,
			Receiving_checksum2
		} state = State::Waiting_dollar;

		std::array<char, 2> checksum_buffer;

		std::expected<std::string, Error> decode_in_buffer();
	};

	/**
	 * @brief Encoder for encoding packets to GDB.
	 *
	 */
	class Packet_encoder
	{
		std::stringstream stream;
		char last_char = '\0';
		u8 repeat = 0;

		void push_repeat();
		void push(char c);

		std::string internal_encode(std::string_view str);

	  public:

		/**
		 * @brief Encode a string into a GDB packet
		 *
		 * @param str Input string
		 * @return Run-length encoded packet string (including `$`, `#` and checksum)
		 */
		static std::string encode(std::string_view str);
	};

}