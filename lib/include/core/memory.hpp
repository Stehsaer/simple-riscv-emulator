#pragma once

#include "common/bitset.hpp"
#include "common/type.hpp"
#include "trap.hpp"

#include <cstddef>
#include <expected>
#include <vector>

namespace core
{
	/**
	 * @brief General interface for memory access. Uses `u64` for address and `u32` for data.
	 * 
	 */
	class Memory_interface
	{
	  public:

		enum class Error
		{
			Out_of_range,
			Unaligned,
			Access_fault,
			Device_error,
			Not_supported
		};

		/**
		 * @brief Reads a word from memory.
		 * 
		 * @param address 64-bit address
		 * @return `u32` data if successful, `Error` otherwise.
		 */
		virtual std::expected<u32, Error> read(u64 address) = 0;

		/**
		 * @brief Reads a 4KiB page from memory. Address must be aligned to 4KiB.
		 * 
		 * @param address 64-bit address
		 * @param data Span of 1024 `u32` to store the read data
		 * @return `Error` if an error occurred, `void` otherwise.
		 */
		virtual std::expected<void, Error> read_page(u64 address, std::span<u32, 1024> data) = 0;

		/**
		 * @brief Writes a word to memory. Uses `mask` to determine which bytes to write.
		 * 
		 * @param address 64-bit address
		 * @param data 32-bit data to write
		 * @param mask Mask to determine which bytes to write. Bit 0 corresponds to LSB.
		 * @return `Error` if an error occurred, `void` otherwise.
		 */
		virtual std::expected<void, Error> write(u64 address, u32 data, Bitset<4> mask) = 0;

		/**
		 * @brief Queries the size of the memory in bytes.
		 * 
		 * @return Size in bytes
		 */
		virtual u64 size() const = 0;
	};

	/**
	 * @brief Memory access module
	 * 
	 */
	struct Load_store_module
	{
		enum class Funct
		{
			None,

			Load_byte,
			Load_halfword,
			Load_word,
			Load_byte_unsigned,
			Load_halfword_unsigned,

			Store_byte,
			Store_halfword,
			Store_word
		};

		enum class Opcode
		{
			None,
			Load,
			Store
		};

		std::expected<u32, Trap> operator()(
			Memory_interface& interface,
			Opcode opcode,
			Funct funct,
			u32 address,
			u32 store_value
		);
	};

	/**
	 * @brief Inst fetch module. Simple emulator-side cache is implemented.
	 * 
	 */
	struct Inst_fetch_module
	{
		struct Cache_entry
		{
			std::array<u32, 1024> data;
			u32 address = 0;
			bool valid = false;
		};

		static constexpr size_t cache_num = 1024;
		std::vector<Cache_entry> cache = std::vector<Cache_entry>(cache_num);

		std::expected<u32, Trap> operator()(Memory_interface& interface, u32 pc);

		/**
		 * @brief Execute `fence.i` on the cache
		 * 
		 */
		void fencei();
	};

	size_t get_size(Load_store_module::Funct funct);
}
