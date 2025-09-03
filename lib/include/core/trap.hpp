#pragma once
#include <cstdint>

namespace core
{
	enum class Trap : uint32_t
	{
		// Exceptions

		Inst_address_misaligned = 0x0000,
		Inst_access_fault = 0x0001,
		Illegal_instruction = 0x0002,
		Breakpoint = 0x0003,
		Load_address_misaligned = 0x0004,
		Load_access_fault = 0x0005,
		Store_address_misaligned = 0x0006,
		Store_access_fault = 0x0007,
		Env_call_from_U_mode = 0x0008,
		Env_call_from_S_mode = 0x0009,
		Env_call_from_M_mode = 0x000B,
		Inst_page_fault = 0x000C,
		Load_page_fault = 0x000D,
		Store_page_fault = 0x000F,

		// Interrupts

		Supervisor_software_interrupt = 0x80000001,
		Machine_software_interrupt = 0x80000003,
		Supervisor_timer_interrupt = 0x80000005,
		Machine_timer_interrupt = 0x80000007,
		Supervisor_external_interrupt = 0x80000009,
		Machine_external_interrupt = 0x8000000B,
		Counter_overflow_interrupt = 0x8000000C,
	};

	/**
	 * @brief Helper for determining if a trap is an interrupt
	 * 
	 * @param trap Trap value
	 * @return `true` if the trap is an interrupt, `false` otherwise
	 */
	static constexpr bool is_interrupt(Trap trap)
	{
		return (uint32_t)trap >= 0x80000000;
	}
}