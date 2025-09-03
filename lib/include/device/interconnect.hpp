#pragma once

#include "core/memory.hpp"

#include <cassert>
#include <cmath>
#include <cstring>

#include <array>
#include <memory>
#include <vector>

namespace device
{
	/**
	 * @brief Base class for a interconnect.
	 * @note To implement a interconnect, inherit from this class and implement `get_memory()` function.
	 */
	class Interconnect : public core::Memory_interface
	{
	  protected:

		/**
		 * @brief Result of memory query
		 * 
		 */
		struct Memory_query_result
		{
			// Reference to the memory device
			std::reference_wrapper<core::Memory_interface> entry;

			// Logical address relative to the memory device
			size_t offset;
		};

		/**
		 * @brief Query memory device that contains the given address.
		 * 
		 * @param address Logical address relative to the interconnect
		 * @return `Memory_query_result` if the address is valid, `Error::Invalid_address` otherwise.
		 */
		virtual std::expected<Memory_query_result, Error> get_memory(u64 address) const noexcept = 0;

	  public:

		std::expected<u32, Error> read(u64 address) override final;
		std::expected<void, Error> read_page(u64 address, std::span<u32, 1024> data) override final;
		std::expected<void, Error> write(u64 address, u32 data, core::Bitset<4> mask) override final;
		u64 size() const override final { return std::numeric_limits<u64>::max(); }
	};
}