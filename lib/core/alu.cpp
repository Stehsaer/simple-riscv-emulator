#include "core/alu.hpp"

namespace core
{
	namespace native_math
	{
		namespace compare
		{
			bool eq(u32 x, u32 y) noexcept
			{
				return x == y;
			}

			bool lt(u32 x, u32 y) noexcept
			{
				return static_cast<i32>(x) < static_cast<i32>(y);
			}

			bool ltu(u32 x, u32 y) noexcept
			{
				return x < y;
			}
		}

		u32 sll(u32 x, u32 shift_amount) noexcept
		{
			return x << (shift_amount & 0b11111);
		};

		u32 srl(u32 x, u32 shift_amount) noexcept
		{
			return x >> (shift_amount & 0b11111);
		};

		u32 sra(u32 x, u32 shift_amount) noexcept
		{
			return static_cast<i32>(x) >> (shift_amount & 0b11111);
		};

		u32 mul(u32 x, u32 y) noexcept
		{
			return x * y;
		}

		u32 mulh(u32 x, u32 y) noexcept
		{
			return (static_cast<i64>(x) * static_cast<i64>(y)) >> 32;
		}

		u32 mulhu(u32 x, u32 y) noexcept
		{
			return (static_cast<u64>(x) * static_cast<u64>(y)) >> 32;
		}

		u32 mulhsu(u32 x, u32 y) noexcept
		{
			return (static_cast<i64>(x) * static_cast<u64>(y)) >> 32;
		}

		u32 div(u32 x, u32 y) noexcept
		{
			if (y == 0)  // Divided by zero
				return -1;
			else if (x == 0x80000000 && y == 0xFFFFFFFF)  // Division overflow
				return x;
			else  // Normal
				return static_cast<i32>(x) / static_cast<i32>(y);
		}

		u32 divu(u32 x, u32 y) noexcept
		{
			if (y == 0)
				return -1;
			else
				return x / y;
		}

		u32 rem(u32 x, u32 y) noexcept
		{
			if (y == 0)  // Divided by zero
				return x;
			else if (x == 0x80000000 && y == 0xFFFFFFFF)  // Division overflow
				return 0;
			else  // Normal
				return static_cast<i32>(x) % static_cast<i32>(y);
		}

		u32 remu(u32 x, u32 y) noexcept
		{
			if (y == 0)
				return x;
			else
				return x % y;
		}
	}

	u32 ALU_module::operator()(Opcode opcode, u32 x, u32 y) noexcept
	{
		switch (opcode)
		{
		case Opcode::Add:
			return x + y;
		case Opcode::Sub:
			return x - y;

		case Opcode::Sll:
			return native_math::sll(x, y);
		case Opcode::Srl:
			return native_math::srl(x, y);
		case Opcode::Sra:
			return native_math::sra(x, y);

		case Opcode::Slt:
			return native_math::compare::lt(x, y) ? 1 : 0;
		case Opcode::Sltu:
			return native_math::compare::ltu(x, y) ? 1 : 0;

		case Opcode::And:
			return x & y;
		case Opcode::Or:
			return x | y;
		case Opcode::Xor:
			return x ^ y;

		case Opcode::Mul:
			return native_math::mul(x, y);
		case Opcode::Mulh:
			return native_math::mulh(x, y);
		case Opcode::Mulhsu:
			return native_math::mulhsu(x, y);
		case Opcode::Mulhu:
			return native_math::mulhu(x, y);

		case Opcode::Div:
			return native_math::div(x, y);
		case Opcode::Rem:
			return native_math::rem(x, y);
		case Opcode::Divu:
			return native_math::divu(x, y);
		case Opcode::Remu:
			return native_math::remu(x, y);

		case Opcode::Czero_eqz:
			return y == 0 ? 0 : x;
		case Opcode::Czero_nez:
			return y != 0 ? 0 : x;

		default:
			return 0;
		}
	}

	bool Branch_module::operator()(Opcode opcode, u32 x, u32 y) noexcept
	{
		switch (opcode)
		{
		case Opcode::None:
			return false;
		case Opcode::Eq:
			return native_math::compare::eq(x, y);
		case Opcode::Ne:
			return !native_math::compare::eq(x, y);
		case Opcode::Lt:
			return native_math::compare::lt(x, y);
		case Opcode::Ge:
			return !native_math::compare::lt(x, y);
		case Opcode::Ltu:
			return native_math::compare::ltu(x, y);
		case Opcode::Geu:
			return !native_math::compare::ltu(x, y);
		default:
			return false;
		}
	}
}
