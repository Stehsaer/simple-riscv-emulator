#include "core/cpu.hpp"

namespace core
{
	CPU_module::Result CPU_module::execute()
	{
		CPU_module::Result result;

		/* Inst Fetch */

		result.pc = pc;

		const auto fetch_result = inst_fetch(*interface, pc);
		if (!fetch_result) [[unlikely]]
		{
			result.trap = fetch_result.error();
			return result;
		}

		result.inst = fetch_result.value();

		/* Inst Decode */

		const auto decode_result = decoder(registers, result.inst, pc);
		if (!decode_result) [[unlikely]]
		{
			result.trap = decode_result.error();
			return result;
		}

		result = decode_result.value();

		/* Execute */

		if (result.ecall)
		{
			result.trap = Trap::Env_call_from_M_mode;
			return result;
		}

		if (csr.mstatus.mie)
		{
			const auto interrupt = csr.mip.value & csr.mie.value;
			if (interrupt != 0)
			{
				if (interrupt & (1 << 7))
				{
					result.trap = Trap::Machine_timer_interrupt;
					return result;
				}
			}
		}

		result.alu_result = alu(result.alu_opcode, result.alu_num1, result.alu_num2);
		result.branch_result = branch(result.branch_opcode, result.branch_num1, result.branch_num2);

		const auto csr_result = csr(result.csr_access_info);
		if (!csr_result) [[unlikely]]
		{
			result.trap = Trap::Illegal_instruction;
			return result;
		}

		result.csr_result = csr_result.value();

		const auto memory_result = memory(
			*interface,
			result.memory_opcode,
			result.memory_funct,
			result.alu_result,
			result.memory_store_value
		);

		if (!memory_result) [[unlikely]]
		{
			result.trap = memory_result.error();
			return result;
		}
		result.memory_load_value = memory_result.value();

		/* Writeback */

		switch (result.writeback_source)
		{
		case Register_source::None:
			break;
		case Register_source::Pc_plus_4:
			result.writeback_value = pc + 4;
			break;
		case Register_source::Alu:
			result.writeback_value = result.alu_result;
			break;
		case Register_source::Memory:
			result.writeback_value = result.memory_load_value;
			break;
		case Register_source::Csr:
			result.writeback_value = result.csr_result;
			break;
		}

		if (result.dest_register != 0) registers.set_register(result.dest_register, result.writeback_value);

		if (result.mret) [[unlikely]]
		{
			pc = csr.mepc.value;
			csr.mstatus.mie = csr.mstatus.mpie;
			csr.mstatus.mpie = false;
		}
		else if (result.branch_result) [[unlikely]]
			pc = result.alu_result;
		else
			pc += 4;

		return result;
	}

	void CPU_module::handle_trap(const Result& result)
	{
		if (!result.trap.has_value()) [[likely]]
			return;

		const auto trap = result.trap.value();

		// mstatus

		csr.mstatus.mpie = csr.mstatus.mie;
		csr.mstatus.mie = false;
		csr.mstatus.mpp = Priviledge::Machine;

		// mepc

		csr.mepc.value = pc;

		// mcause

		*reinterpret_cast<u32*>(&csr.mcause) = (u32)trap;

		// mtval

		switch (trap)
		{
		case Trap::Inst_address_misaligned:
		case Trap::Inst_access_fault:
		case Trap::Inst_page_fault:
		case Trap::Load_access_fault:
		case Trap::Store_access_fault:
		case Trap::Load_address_misaligned:
		case Trap::Store_address_misaligned:
		case Trap::Load_page_fault:
		case Trap::Store_page_fault:
			csr.mtval.value = result.alu_result;
			break;

		case Trap::Illegal_instruction:
			csr.mtval.value = result.inst;
			break;

		default:
			csr.mtval.value = 0;
			break;
		}

		// change pc

		if (is_interrupt(trap) && csr.mtvec.mode == csr::Mtvec::Mode::Vectored)
		{
			const auto vec = (u32)trap & 0x7fffffff;
			pc = (csr.mtvec.base_upper30 << 2) + 4 * vec;
		}
		else
		{
			pc = csr.mtvec.base_upper30 << 2;
		}
	}

	core::CPU_module::Result CPU_module::step()
	{
		const auto result = execute();
		handle_trap(result);
		csr.tick();
		return result;
	}
}