#pragma once

#include <core/cpu.hpp>
#include <device/block-memory.hpp>
#include <device/interconnect.hpp>
#include <device/peripheral.hpp>

#include <fstream>
#include <memory>

/**
 * @brief Holds all components that compose the emulated platform
 *
 */
struct Platform
{
	/**
	 * @brief Main memory map of the platform
	 *
	 */
	struct Memory : public device::Interconnect
	{
		std::shared_ptr<device::Block_memory> rom;
		std::shared_ptr<device::Block_memory> ram;
		std::shared_ptr<device::periph::Uart> uart;
		std::shared_ptr<device::periph::Clock> clock_periph;

	  protected:

		std::expected<Memory_query_result, Error> get_memory(u64 address) const noexcept override final;
	};

	std::shared_ptr<Memory> memory;
	std::shared_ptr<core::CPU_module> cpu;

	Platform(const Platform&) = delete;
	Platform(Platform&&) = default;
	Platform& operator=(const Platform&) = delete;
	Platform& operator=(Platform&&) = default;

	Platform(const void* rom_init_data, size_t rom_init_size, device::Fill_policy fill_policy);

	~Platform();
};