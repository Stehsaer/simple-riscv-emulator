#pragma once

#include "alu.hpp"
#include "common/type.hpp"
#include "csr.hpp"
#include "memory.hpp"
#include "register-file.hpp"
#include "trap.hpp"

#include <expected>

namespace core
{
	/**
	 * @brief Register source for writeback
	 *
	 */
	enum class Register_source
	{
		None,
		Pc_plus_4,  // from `PC` + 4
		Alu,        // From ALU
		Memory,     // From memory read result
		Csr         // From CSR read result
	};

	/**
	 * @brief Instruction decode module
	 *
	 */
	struct Inst_decode_module
	{
		/**
		 * @brief Decode result
		 *
		 */
		struct Result
		{
			Register_source writeback_source = Register_source::None;
			Bitset<5> dest_register = 0;

			ALU_module::Opcode alu_opcode = ALU_module::Opcode::Add;
			u32 alu_num1 = 0, alu_num2 = 0;

			Branch_module::Opcode branch_opcode = Branch_module::Opcode::None;
			u32 branch_num1 = 0, branch_num2 = 0;

			Load_store_module::Opcode memory_opcode = Load_store_module::Opcode::None;
			Load_store_module::Funct memory_funct = Load_store_module::Funct::None;
			u32 memory_store_value = 0;

			bool fencei = false;
			bool ecall = false;
			bool mret = false;

			CSR_access_info csr_access_info = {};
		};

		/**
		 * @brief Decodes an instruction
		 *
		 * @param registers Register file object
		 * @param instr Instruction word
		 * @param pc Current `PC`
		 * @return `Result` if successful, `Trap` if illegal instruction
		 */
		std::expected<Result, Trap> operator()(
			const Register_file_module& registers,
			u32 instr,
			u32 pc
		) noexcept;
	};
}