#include "device/block-memory.hpp"
#include "core/print.hpp"

#include <execution>
#include <ranges>

namespace device
{
	Block_memory::Block_memory(u64 size_bytes, Fill_policy mode) :
		actual_size_bytes(size_bytes),
		fill_policy(mode)
	{
		const size_t page_count = (size_bytes + page_size_bytes - 1) / page_size_bytes;
		storage.resize(page_count);
	}

	void Block_memory::fill_page(std::array<u32, page_size_bytes / sizeof(u32)>& page)
	{
		switch (fill_policy)
		{
		case Fill_policy::None:
			break;

		case Fill_policy::Zero:
			std::ranges::fill(page, 0);
			break;

		case Fill_policy::One:
			std::ranges::fill(page, ~0);
			break;

		case Fill_policy::Random:
		{
			std::random_device rd;
			std::mt19937_64 eng(rd());
			std::uniform_int_distribution<u32> distr;

			std::ranges::generate(page, [&distr, &eng]() { return distr(eng); });
			break;
		}

		case Fill_policy::Cdcdcdcd:
			std::ranges::fill(page, 0xCDCDCDCD'CDCDCDCD);
			break;
		}
	}

	void Block_memory::touch_page(size_t page_index)
	{
		if (page_index >= storage.size()) [[unlikely]]
			return;

		if (storage[page_index] != nullptr) [[likely]]
			return;

		storage[page_index] = std::make_unique<std::array<u32, page_size_bytes / sizeof(u32)>>();
		fill_page(*storage[page_index]);
	}

	bool Block_memory::fill_data(const void* data, size_t size)
	{
		if (size > this->size()) [[unlikely]]
			return false;

		const std::span<const u8> byte_data = {reinterpret_cast<const u8*>(data), size};

		for (const auto [page_idx, data_chunk] :
			 byte_data | std::views::chunk(page_size_bytes) | std::views::enumerate)
		{
			touch_page(page_idx);
			std::ranges::copy(data_chunk, reinterpret_cast<u8*>(storage[page_idx]->data()));
		}

		return true;
	}

	std::expected<u32, core::Memory_interface::Error> Block_memory::read(u64 address)
	{
		if (address >= this->size()) [[unlikely]]
			return std::unexpected(Error::Out_of_range);

		if (address & 0x3) [[unlikely]]
			return std::unexpected(Error::Unaligned);

		if (actual_size_bytes < 4) [[unlikely]]
			return std::unexpected(Error::Out_of_range);

		const u64 page_index = address / page_size_bytes;
		const u64 page_offset = address % page_size_bytes;
		const u64 page_offset_word = page_offset / sizeof(u32);

		touch_page(page_index);
		const auto& page = *storage[page_index];

		return page[page_offset_word];
	}

	std::expected<void, Block_memory::Error> Block_memory::read_page(u64 address, std::span<u32, 1024> data)
	{
		if ((address & 0x00000FFF) != 0) [[unlikely]]
			return std::unexpected(Error::Unaligned);

		if (address + data.size() * sizeof(u32) > this->size()) [[unlikely]]
			return std::unexpected(Error::Out_of_range);

		const u64 page_index = address / page_size_bytes;
		const u64 page_offset = address % page_size_bytes;
		const u64 page_offset_word = page_offset / sizeof(u32);

		touch_page(page_index);
		const auto& page = *storage[page_index];

		std::ranges::copy(std::span<const u32, 1024>(page.data() + page_offset_word, 1024), data.begin());

		return {};
	}

	std::expected<void, core::Memory_interface::Error> Block_memory::write(
		u64 address,
		u32 data,
		core::Bitset<4> mask
	)
	{
		if (address >= this->size()) [[unlikely]]
			return std::unexpected(Error::Out_of_range);

		if (write_lock) [[unlikely]]
			return std::unexpected(Error::Access_fault);

		if (mask == 0) [[unlikely]]
			return {};

		if (address & 0x3) [[unlikely]]
			return std::unexpected(Error::Unaligned);

		if (actual_size_bytes < 4) [[unlikely]]
		{
			if (actual_size_bytes < 4 && mask.take_bit<3>()) return std::unexpected(Error::Out_of_range);
			if (actual_size_bytes < 3 && mask.take_bit<2>()) return std::unexpected(Error::Out_of_range);
			if (actual_size_bytes < 2 && mask.take_bit<1>()) return std::unexpected(Error::Out_of_range);
			if (actual_size_bytes < 1 && mask.take_bit<0>()) return std::unexpected(Error::Out_of_range);
		}

		const u64 page_index = address / page_size_bytes;
		const u64 page_offset = address % page_size_bytes;

		touch_page(page_index);
		const auto& page = *storage[page_index];

		u8* byte_ptr = (u8*)page.data() + page_offset;

		byte_ptr[0] = mask.take_bit<0>() ? static_cast<u8>(data) : byte_ptr[0];
		byte_ptr[1] = mask.take_bit<1>() ? static_cast<u8>(data >> 8) : byte_ptr[1];
		byte_ptr[2] = mask.take_bit<2>() ? static_cast<u8>(data >> 16) : byte_ptr[2];
		byte_ptr[3] = mask.take_bit<3>() ? static_cast<u8>(data >> 24) : byte_ptr[3];

		return {};
	}

	size_t Block_memory::used_space() const noexcept
	{
		return std::ranges::count_if(storage, [](const auto& page) { return page != nullptr; })
			 * page_size_bytes;
	}

	void Block_memory::reset_content() noexcept
	{
		std::ranges::fill(storage, nullptr);
	}
}