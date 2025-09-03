#include "gdb-stub/stop-point.hpp"
#include "gdb-stub/accessor.hpp"
#include "gdb-stub/expression.hpp"

namespace gdb_stub
{
	bool Hw_breakpoint::is_triggered_by(core::CPU_module& cpu) const
	{
		if (cpu.pc != address) return false;
		if (!cond.has_value()) return true;

		const Memory_accessor mem_accessor{.memory = *cpu.interface};
		const CPU_register_accessor reg_accessor{.cpu = cpu};

		const auto result = expression::execute(
			[&mem_accessor](u32 val) { return mem_accessor.read(val); },
			[&reg_accessor](u32 val) { return reg_accessor.read(val); },
			*cond
		);

		if (!result) return false;
		return result->top;
	}

	Hw_breakpoint Hw_breakpoint::from(const command::Add_breakpoint& cmd)
	{
		return Hw_breakpoint{.address = cmd.address, .cond = cmd.cond};
	}
}