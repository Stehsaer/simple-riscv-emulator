#pragma once

#include "core/cpu.hpp"

namespace gdb_stub
{
	/**
	 * @brief General accessor wrapper for memory interface
	 *
	 */
	struct Memory_accessor
	{
		core::Memory_interface& memory;
		std::optional<u8> read(u32 address) const noexcept;
		bool write(u32 address, u8 value) const noexcept;
	};

	/**
	 * @brief General register accessor wrapper for CPU registers
	 *
	 */
	struct CPU_register_accessor
	{
		core::CPU_module& cpu;
		std::optional<u32> read(u32 reg_num) const noexcept;
		bool write(u32 reg_num, u32 value) const noexcept;
	};
}