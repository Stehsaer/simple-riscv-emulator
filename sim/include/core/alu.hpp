#pragma once

#include "common/type.hpp"

namespace core
{
	namespace native_math
	{
		namespace compare
		{
			bool eq(u32 x, u32 y) noexcept;
			bool lt(u32 x, u32 y) noexcept;
			bool ltu(u32 x, u32 y) noexcept;
		}

		u32 sll(u32 x, u32 shift_amount) noexcept;
		u32 srl(u32 x, u32 shift_amount) noexcept;
		u32 sra(u32 x, u32 shift_amount) noexcept;
		u32 mul(u32 x, u32 y) noexcept;
		u32 mulh(u32 x, u32 y) noexcept;
		u32 mulhu(u32 x, u32 y) noexcept;
		u32 mulhsu(u32 x, u32 y) noexcept;
		u32 div(u32 x, u32 y) noexcept;
		u32 divu(u32 x, u32 y) noexcept;
		u32 rem(u32 x, u32 y) noexcept;
		u32 remu(u32 x, u32 y) noexcept;
	}

	/**
	 * @brief Implements ALU module, executes arithmetic operations
	 *
	 */
	struct ALU_module
	{
		enum class Opcode
		{
			Add,
			Sub,

			Sll,
			Srl,
			Sra,

			Slt,
			Sltu,

			And,
			Or,
			Xor,

			Mul,
			Mulh,
			Mulhsu,
			Mulhu,

			Div,
			Divu,
			Rem,
			Remu,

			Czero_eqz,
			Czero_nez,
		};

		u32 operator()(Opcode opcode, u32 x, u32 y) noexcept;
	};

	/**
	 * @brief Implements Branch module
	 *
	 */
	struct Branch_module
	{
		enum class Opcode
		{
			None,

			Eq,
			Ne,

			Lt,
			Ge,

			Ltu,
			Geu
		};

		bool operator()(Opcode opcode, u32 x, u32 y) noexcept;
	};
}