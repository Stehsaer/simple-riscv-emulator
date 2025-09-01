#include "platform.hpp"
#include "core/print.hpp"

std::expected<Platform::Memory::Memory_query_result, Platform::Memory::Error> Platform::Memory::get_memory(
	u64 address
) const noexcept
{
	constexpr u64 rom_start = 0x0010'0000, ram_start = 0x8000'0000, uart_start = 0x0001'0000,
				  clock_start = 0x0001'1000;

	if (address >= ram_start && address < ram_start + ram->size())
		return Memory_query_result{.entry = *ram, .offset = static_cast<u64>(address - ram_start)};

	if (address >= rom_start && address < rom_start + rom->size())
		return Memory_query_result{.entry = *rom, .offset = static_cast<u64>(address - rom_start)};

	if (address >= uart_start && address < uart_start + uart->size())
		return Memory_query_result{.entry = *uart, .offset = static_cast<u64>(address - uart_start)};

	if (address >= clock_start && address < clock_start + clock_periph->size())
		return Memory_query_result{.entry = *clock_periph, .offset = static_cast<u64>(address - clock_start)};

	return std::unexpected(Error::Out_of_range);
}

Platform::Platform(const void* rom_init_data, size_t rom_init_size, device::Fill_policy fill_policy)
{
	memory = std::make_shared<Memory>();

	memory->rom = std::make_shared<device::Block_memory>(128 * 1024);
	if (!memory->rom->fill_data(rom_init_data, rom_init_size))
		throw std::runtime_error(
			std::format(
				"ROM init data size ({} Bytes) exceeds ROM size ({} Bytes)",
				rom_init_size,
				memory->rom->size()
			)
		);
	memory->rom->lock();

	memory->ram = std::make_shared<device::Block_memory>(2u * 1024 * 1024 * 1024, fill_policy);
	memory->uart = std::make_shared<device::periph::Uart>();
	memory->clock_periph = std::make_shared<device::periph::Clock>();

	cpu = std::make_shared<core::CPU_module>(0x0010'0000, memory);
}

Platform::~Platform() = default;