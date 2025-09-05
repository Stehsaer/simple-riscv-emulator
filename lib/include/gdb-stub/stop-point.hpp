#pragma once

#include "common/type.hpp"
#include "core/cpu.hpp"
#include "gdb-stub/addr-range.hpp"
#include "gdb-stub/command.hpp"

#include <optional>
#include <vector>

namespace gdb_stub
{
	/**
	 * @brief Struct representing a watchpoint
	 *
	 */
	struct Watchpoint
	{
		bool watch_write;
		bool watch_read;
		Address_range addr_range;

		auto operator<=>(const Watchpoint& other) const { return addr_range <=> other.addr_range; }
	};

	/**
	 * @brief Struct representing a hardware breakpoint
	 *
	 */
	struct Hw_breakpoint
	{
		u32 address;
		std::optional<std::vector<u8>> cond;

		/**
		 * @brief Checks if the breakpoint is triggered by the CPU. If `cond` is set, it will be evaluated.
		 *
		 * @param cpu The CPU instance
		 * @return `true` if triggered, `false` otherwise
		 */
		bool is_triggered_by(core::CPU_module& cpu) const;

		/**
		 * @brief Create a `Hw_breakpoint` from a `Add_breakpoint` command
		 *
		 * @param cmd `Add_breakpoint` command
		 * @return New `Hw_breakpoint`
		 */
		static Hw_breakpoint from(const command::Add_breakpoint& cmd);
	};
}