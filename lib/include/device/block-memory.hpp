#pragma once

#include "core/memory.hpp"

#include <atomic>
#include <cstring>
#include <memory>
#include <random>
#include <vector>

namespace device
{
	/**
	 * @brief Fill policy for uninitialized memory areas
	 *
	 */
	enum class Fill_policy
	{
		None,     // No filling. Use uninitialized memory (may contain garbage)
		Zero,     // Fill with zeros
		One,      // Fill with ones (`0xFF`)
		Random,   // Fill with random data
		Cdcdcdcd  // Fill with `0xCD` pattern
	};

	/**
	 * @brief Block memory device
	 *
	 */
	class Block_memory : public core::Memory_interface
	{
	  public:

		/**
		 * @brief Page size in bytes. Currently set to 64 KiB.
		 *
		 */
		static constexpr u64 page_size_bytes = 64 * 1024;
		static_assert(page_size_bytes % sizeof(u32) == 0);

		/**
		 * @brief Construct a new `Block_memory`
		 *
		 * @param size_bytes Size in bytes
		 * @param mode Fill mode for uninitialized memory areas
		 */
		Block_memory(u64 size_bytes, Fill_policy mode = Fill_policy::None);

		/**
		 * @brief Fill memory with data, starting at logic address `0`.
		 *
		 * @param data Data pointer
		 * @param size Data size in bytes. Must not exceed memory size.
		 * @return `true` if filled successfully, `false` otherwise.
		 */
		bool fill_data(const void* data, size_t size);

		std::expected<u32, Error> read(u64 address) override;
		std::expected<void, Error> read_page(u64 address, std::span<u32, 1024> data) override;
		std::expected<void, Error> write(u64 address, u32 data, core::Bitset<4> mask) override;
		size_t size() const override { return actual_size_bytes; }

		/**
		 * @brief Lock the memory and become read-only.
		 *
		 * @note
		 * - This function is thread-safe.
		 * - `unlock()` before writing to memory again, especially under debug mode.
		 */
		void lock() noexcept { write_lock = true; }

		/**
		 * @brief Unlock the memory and become writable.
		 *
		 */
		void unlock() noexcept { write_lock = false; }

		/**
		 * @brief Get used space in bytes.
		 * @note This is the upper-bound of the actual size. Count in granularity of pages.
		 *
		 * @return size_t Used space in bytes
		 */
		size_t used_space() const noexcept;

		/**
		 * @brief Reset all contents, keeping the fill policy.
		 *
		 */
		void reset_content() noexcept;

	  private:

		std::atomic<bool> write_lock = false;
		u64 actual_size_bytes;
		Fill_policy fill_policy;
		std::vector<std::unique_ptr<std::array<u32, page_size_bytes / sizeof(u32)>>> storage;

		void fill_page(std::array<u32, page_size_bytes / sizeof(u32)>& page);
		void touch_page(size_t page_index);
	};
}