#include "core/decode.hpp"
#include "core/alu.hpp"
#include "core/inst.hpp"

namespace core
{
	enum class Opcode : u8
	{
		Lui = 0b01101,
		Auipc = 0b00101,
		Jal = 0b11011,
		Jalr = 0b11001,
		Branch = 0b11000,
		Load = 0b00000,
		Store = 0b01000,
		Reg_imm_arithmetic = 0b00100,
		Reg_reg_arithmetic = 0b01100,
		Misc_mem = 0b00011,
		System = 0b11100
	};

	static Inst_decode_module::Result decode_lui(Bitset<32> instr) noexcept
	{
		const inst::Utype utype(instr);

		return Inst_decode_module::Result{
			.writeback_source = Register_source::Alu,
			.dest_register = utype.rd,
			.alu_opcode = ALU_module::Opcode::Add,
			.alu_num1 = static_cast<u32>(utype.imm),
			.alu_num2 = 0,
		};
	}

	static Inst_decode_module::Result decode_auipc(Bitset<32> instr, u32 pc) noexcept
	{
		const inst::Utype utype(instr);

		return Inst_decode_module::Result{
			.writeback_source = Register_source::Alu,
			.dest_register = utype.rd,
			.alu_opcode = ALU_module::Opcode::Add,
			.alu_num1 = pc,
			.alu_num2 = static_cast<u32>(utype.imm),
		};
	}

	static Inst_decode_module::Result decode_jal(Bitset<32> instr, u32 pc) noexcept
	{
		const inst::Jtype jtype(instr);

		return Inst_decode_module::Result{
			.writeback_source = Register_source::Pc_plus_4,
			.dest_register = jtype.rd,
			.alu_opcode = ALU_module::Opcode::Add,
			.alu_num1 = pc,
			.alu_num2 = static_cast<u32>(jtype.imm),
			.branch_opcode = Branch_module::Opcode::Eq,
			.branch_num1 = 0,
			.branch_num2 = 0,
		};
	}

	static Inst_decode_module::Result decode_jalr(
		const Register_file_module& registers,
		Bitset<32> instr
	) noexcept
	{
		const inst::Itype itype(instr);

		return Inst_decode_module::Result{
			.writeback_source = Register_source::Pc_plus_4,
			.dest_register = itype.rd,
			.alu_opcode = ALU_module::Opcode::Add,
			.alu_num1 = registers.get_register(itype.rs1),
			.alu_num2 = static_cast<u32>(itype.imm),
			.branch_opcode = Branch_module::Opcode::Eq,
			.branch_num1 = 0,
			.branch_num2 = 0,
		};
	}

	static Inst_decode_module::Result decode_load(
		const Register_file_module& registers,
		Bitset<32> instr
	) noexcept
	{
		const inst::Itype itype(instr);

		constexpr std::array memory_opcode_list = std::to_array(
			{Load_store_module::Funct::Load_byte,      // 000
			 Load_store_module::Funct::Load_halfword,  // 001
			 Load_store_module::Funct::Load_word,      // 010
			 Load_store_module::Funct::None,
			 Load_store_module::Funct::Load_byte_unsigned,      // 100
			 Load_store_module::Funct::Load_halfword_unsigned,  // 101
			 Load_store_module::Funct::None,
			 Load_store_module::Funct::None}
		);

		return Inst_decode_module::Result{
			.writeback_source = Register_source::Memory,
			.dest_register = itype.rd,
			.alu_opcode = ALU_module::Opcode::Add,
			.alu_num1 = registers.get_register(itype.rs1),
			.alu_num2 = static_cast<u32>(itype.imm),
			.memory_opcode = Load_store_module::Opcode::Load,
			.memory_funct = memory_opcode_list[static_cast<size_t>(itype.funct3)],
		};
	}

	static Inst_decode_module::Result decode_store(
		const Register_file_module& registers,
		Bitset<32> instr
	) noexcept
	{
		const inst::Stype stype(instr);

		Load_store_module::Funct memory_funct;

		switch (static_cast<u8>(stype.funct3))
		{
		case 0b000:
			memory_funct = Load_store_module::Funct::Store_byte;
			break;
		case 0b001:
			memory_funct = Load_store_module::Funct::Store_halfword;
			break;
		case 0b010:
			memory_funct = Load_store_module::Funct::Store_word;
			break;
		default:
			memory_funct = Load_store_module::Funct::None;
			break;
		}

		return Inst_decode_module::Result{
			.writeback_source = Register_source::None,
			.alu_opcode = ALU_module::Opcode::Add,
			.alu_num1 = registers.get_register(stype.rs1),
			.alu_num2 = static_cast<u32>(stype.imm),
			.memory_opcode = Load_store_module::Opcode::Store,
			.memory_funct = memory_funct,
			.memory_store_value = registers.get_register(stype.rs2),
		};
	}

	static Inst_decode_module::Result decode_register_imm(
		const Register_file_module& registers,
		Bitset<32> instr
	) noexcept
	{
		const inst::Itype itype(instr);

		const std::array opcode = std::to_array(
			{ALU_module::Opcode::Add,
			 ALU_module::Opcode::Sll,
			 ALU_module::Opcode::Slt,
			 ALU_module::Opcode::Sltu,
			 ALU_module::Opcode::Xor,
			 instr.take_bit<30>() ? ALU_module::Opcode::Sra : ALU_module::Opcode::Srl,
			 ALU_module::Opcode::Or,
			 ALU_module::Opcode::And}
		);

		return Inst_decode_module::Result{
			.writeback_source = Register_source::Alu,
			.dest_register = itype.rd,
			.alu_opcode = opcode[static_cast<size_t>(itype.funct3)],
			.alu_num1 = registers.get_register(itype.rs1),
			.alu_num2 = static_cast<u32>(itype.imm),
		};
	}

	static std::expected<Inst_decode_module::Result, Trap> decode_register_register(
		const Register_file_module& registers,
		Bitset<32> instr
	) noexcept
	{
		const inst::Rtype rtype(instr);

		ALU_module::Opcode opcode;
		const u8 funct7_select = static_cast<u8>(rtype.funct7.slice<2, 0>());

		switch (funct7_select)
		{
		case 0b000:  // RV32I
			opcode = std::to_array(
				{instr.take_bit<30>() ? ALU_module::Opcode::Sub : ALU_module::Opcode::Add,
				 ALU_module::Opcode::Sll,
				 ALU_module::Opcode::Slt,
				 ALU_module::Opcode::Sltu,
				 ALU_module::Opcode::Xor,
				 instr.take_bit<30>() ? ALU_module::Opcode::Sra : ALU_module::Opcode::Srl,
				 ALU_module::Opcode::Or,
				 ALU_module::Opcode::And}
			)[static_cast<size_t>(rtype.funct3)];
			break;

		case 0b001:  // RV32M
			opcode = std::to_array(
				{ALU_module::Opcode::Mul,
				 ALU_module::Opcode::Mulh,
				 ALU_module::Opcode::Mulhsu,
				 ALU_module::Opcode::Mulhu,
				 ALU_module::Opcode::Div,
				 ALU_module::Opcode::Divu,
				 ALU_module::Opcode::Rem,
				 ALU_module::Opcode::Remu}
			)[static_cast<size_t>(rtype.funct3)];
			break;

		case 0b111:  // Zicond
		{
			switch (static_cast<u8>(rtype.funct3))
			{
			case 0b101:
				opcode = ALU_module::Opcode::Czero_eqz;
				break;
			case 0b111:
				opcode = ALU_module::Opcode::Czero_nez;
				break;
			default:
				return std::unexpected(Trap::Illegal_instruction);
			}
			break;
		}

		default:
			return std::unexpected(Trap::Illegal_instruction);
		}

		return Inst_decode_module::Result{
			.writeback_source = Register_source::Alu,
			.dest_register = rtype.rd,
			.alu_opcode = opcode,
			.alu_num1 = registers.get_register(rtype.rs1),
			.alu_num2 = registers.get_register(rtype.rs2),
		};
	}

	static Inst_decode_module::Result decode_branch(
		const Register_file_module& registers,
		Bitset<32> instr,
		u32 pc
	) noexcept
	{
		const inst::Btype btype(instr);

		const std::array opcode = std::to_array(
			{Branch_module::Opcode::Eq,
			 Branch_module::Opcode::Ne,
			 Branch_module::Opcode::None,
			 Branch_module::Opcode::None,
			 Branch_module::Opcode::Lt,
			 Branch_module::Opcode::Ge,
			 Branch_module::Opcode::Ltu,
			 Branch_module::Opcode::Geu}
		);

		return Inst_decode_module::Result{
			.writeback_source = Register_source::None,
			.alu_opcode = ALU_module::Opcode::Add,
			.alu_num1 = pc,
			.alu_num2 = static_cast<u32>(btype.imm),
			.branch_opcode = opcode[static_cast<size_t>(btype.funct3)],
			.branch_num1 = registers.get_register(btype.rs1),
			.branch_num2 = registers.get_register(btype.rs2),
		};
	}

	static std::expected<Inst_decode_module::Result, Trap> decode_misc_mem(Bitset<32> instr) noexcept
	{
		const inst::Itype itype(instr);

		if (itype.funct3 == 0b001) return Inst_decode_module::Result{.fencei = true};

		return std::unexpected(Trap::Illegal_instruction);
	}

	static std::expected<Inst_decode_module::Result, Trap> decode_system(
		const Register_file_module& registers,
		Bitset<32> instr
	) noexcept
	{
		const inst::Itype itype(instr);

		Inst_decode_module::Result result;

		if (itype.funct3 == 0b000)
		{
			switch (static_cast<u16>(itype.imm))
			{
			case 0b000000000000:  // ecall
				result.ecall = true;
				break;
			case 0b001100000010:  // mret
				result.mret = true;
				break;
			default:
				return std::unexpected(Trap::Illegal_instruction);
			}
			return result;
		}

		switch (static_cast<u8>(itype.funct3))
		{
		case 0b001:  // csrrw
		case 0b010:  // csrrs
		case 0b011:  // csrrc
		case 0b101:  // csrrwi
		case 0b110:  // csrrsi
		case 0b111:  // csrrci
			result.writeback_source = Register_source::Csr;
			break;
		default:
			return std::unexpected(Trap::Illegal_instruction);
		}

		const bool csr_do_write = itype.rs1 != 0;

		result.csr_access_info.address = itype.imm.slice<11, 0>();
		result.csr_access_info.write_value = itype.funct3.take_bit<2>()
											   ? static_cast<u32>(itype.rs1.sext<32>())
											   : registers.get_register(itype.rs1);
		result.dest_register = itype.rd;

		switch (static_cast<u8>(itype.funct3.slice<1, 0>()))
		{
		case 0b01:  // csrrw csrrwi
			result.csr_access_info.write_mode
				= csr_do_write ? CSR_write_mode::Overwrite : CSR_write_mode::None;
			result.csr_access_info.read = itype.rd != 0;
			break;
		case 0b10:  // csrrs csrrsi
			result.csr_access_info.write_mode = csr_do_write ? CSR_write_mode::Set : CSR_write_mode::None;
			result.csr_access_info.read = true;  // [INFO] CSRRS & CSRRSI always read
			break;
		case 0b11:  // csrrc csrrci
			result.csr_access_info.write_mode = csr_do_write ? CSR_write_mode::Clear : CSR_write_mode::None;
			result.csr_access_info.read = true;  // [INFO] CSRRC & CSRRCI always read
			break;
		default:
			return std::unexpected(Trap::Illegal_instruction);
		}

		return result;
	}

	std::expected<Inst_decode_module::Result, Trap> Inst_decode_module::operator()(
		const Register_file_module& registers,
		u32 instr,
		u32 pc
	) noexcept
	{
		const Bitset<32> instr_bitset(instr);

		const Bitset<5> opcode = instr_bitset.slice<6, 2>();
		const Bitset<2> len = instr_bitset.slice<1, 0>();

		// Detect RV32C
		if (len != 0b11) [[unlikely]]
			return std::unexpected(Trap::Illegal_instruction);

		switch (static_cast<Opcode>(opcode))
		{
		case Opcode::Lui:
			return decode_lui(instr_bitset);
		case Opcode::Auipc:
			return decode_auipc(instr_bitset, pc);
		case Opcode::Jal:
			return decode_jal(instr_bitset, pc);
		case Opcode::Jalr:
			return decode_jalr(registers, instr_bitset);
		case Opcode::Load:
			return decode_load(registers, instr_bitset);
		case Opcode::Store:
			return decode_store(registers, instr_bitset);
		case Opcode::Reg_imm_arithmetic:
			return decode_register_imm(registers, instr_bitset);
		case Opcode::Reg_reg_arithmetic:
			return decode_register_register(registers, instr_bitset);
		case Opcode::Branch:
			return decode_branch(registers, instr_bitset, pc);
		case Opcode::Misc_mem:
			return decode_misc_mem(instr_bitset);
		case Opcode::System:
			return decode_system(registers, instr_bitset);
		default:
			return std::unexpected(Trap::Illegal_instruction);
		}
	}
}