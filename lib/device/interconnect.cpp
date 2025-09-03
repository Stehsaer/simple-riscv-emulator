#include "device/interconnect.hpp"
#include "core/print.hpp"

#include <ranges>

namespace device
{
	std::expected<u32, Interconnect::Error> Interconnect::read(u64 address)
	{
		const auto memory_result = get_memory(address);
		if (!memory_result) return std::unexpected(memory_result.error());

		const auto& [entry, entry_address] = memory_result.value();
		return entry.get().read(entry_address);
	}

	std::expected<void, Interconnect::Error> Interconnect::read_page(u64 address, std::span<u32, 1024> data)
	{
		const auto memory_result = get_memory(address);
		if (!memory_result) return std::unexpected(memory_result.error());

		const auto& [entry, entry_address] = memory_result.value();

		return entry.get().read_page(entry_address, data);
	}

	std::expected<void, Interconnect::Error> Interconnect::write(u64 address, u32 data, core::Bitset<4> mask)
	{
		const auto memory_result = get_memory(address);
		if (!memory_result) return std::unexpected(memory_result.error());

		const auto& [entry, entry_address] = memory_result.value();

		return entry.get().write(entry_address, data, mask);
	}
}