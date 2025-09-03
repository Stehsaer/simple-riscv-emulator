#include "gdb-stub/expression.hpp"

#include <print>

namespace gdb_stub::expression
{
	std::expected<Execute_result, Execute_error> execute(
		const std::function<std::optional<u8>(u32)>& access_memory_func,
		const std::function<std::optional<u32>(u32)>& access_register_func,
		std::span<const u8> bytecode_sequence
	)
	{
		size_t pc = 0;
		std::vector<Stack_element> stack;

#define POP_STACK(dst)                                                                                       \
	if (stack.empty()) return std::unexpected(Execute_error::Stack_out_of_bound);                            \
	(dst) = stack.back();                                                                                    \
	stack.pop_back();

#define PUSH_STACK(value) stack.push_back((value))

#define GET_STACK_AT_INDEX(dst, index)                                                                       \
	if ((index) >= stack.size()) return std::unexpected(Execute_error::Stack_out_of_bound);                  \
	(dst) = stack[stack.size() - (index) - 1];

#define GET_TOP_TWO_STACK                                                                                    \
	u32 a, b;                                                                                                \
	POP_STACK(b);                                                                                            \
	POP_STACK(a);

#define GET_OP_AT_INDEX(dst, index)                                                                          \
	if ((index) >= bytecode_sequence.size()) return std::unexpected(Execute_error::Bytecode_out_of_bound);   \
	(dst) = bytecode_sequence[index];

#define GET_MEMORY_AT_ADDR(dst, addr)                                                                        \
	if (const auto val = access_memory_func(addr); !val.has_value())                                         \
		return std::unexpected(Execute_error::Memory_access_error);                                          \
	else                                                                                                     \
		(dst) = *val;

		while (true)
		{
			u8 op_uint8;
			GET_OP_AT_INDEX(op_uint8, pc);

			const auto operation = static_cast<Bytecode>(op_uint8);

			switch (operation)
			{
			case Bytecode::Add:
			{
				GET_TOP_TWO_STACK;
				PUSH_STACK(a + b);
				pc++;
				break;
			}
			case Bytecode::Sub:
			{
				GET_TOP_TWO_STACK;
				PUSH_STACK(b - a);
				pc++;
				break;
			}
			case Bytecode::Mul:
			{
				GET_TOP_TWO_STACK;
				PUSH_STACK(a * b);
				pc++;
				break;
			}
			case Bytecode::Div_signed:
			{
				GET_TOP_TWO_STACK;
				if (b == 0) return std::unexpected(Execute_error::Division_by_zero);
				PUSH_STACK(static_cast<i32>(b) / static_cast<i32>(a));
				pc++;
				break;
			}
			case Bytecode::Div_unsigned:
			{
				GET_TOP_TWO_STACK;
				if (b == 0) return std::unexpected(Execute_error::Division_by_zero);
				PUSH_STACK(b / a);
				pc++;
				break;
			}
			case Bytecode::Rem_signed:
			{
				GET_TOP_TWO_STACK;
				if (b == 0) return std::unexpected(Execute_error::Division_by_zero);
				PUSH_STACK(static_cast<i32>(b) % static_cast<i32>(a));
				pc++;
				break;
			}
			case Bytecode::Rem_unsigned:
			{
				GET_TOP_TWO_STACK;
				if (b == 0) return std::unexpected(Execute_error::Division_by_zero);
				PUSH_STACK(b % a);
				pc++;
				break;
			}
			case Bytecode::Lsh:
			{
				GET_TOP_TWO_STACK;
				PUSH_STACK(b << (a & 0x1F));
				pc++;
				break;
			}
			case Bytecode::Rsh_signed:
			{
				GET_TOP_TWO_STACK;
				PUSH_STACK(static_cast<i32>(b) >> (a & 0x1F));
				pc++;
				break;
			}
			case Bytecode::Rsh_unsigned:
			{
				GET_TOP_TWO_STACK;
				PUSH_STACK(b >> (a & 0x1F));
				pc++;
				break;
			}
			case Bytecode::Log_not:
			{
				u32 a;
				POP_STACK(a);
				PUSH_STACK(a == 0 ? 1 : 0);
				pc++;
				break;
			}
			case Bytecode::Bit_and:
			{
				GET_TOP_TWO_STACK;
				PUSH_STACK(a & b);
				pc++;
				break;
			}
			case Bytecode::Bit_or:
			{
				GET_TOP_TWO_STACK;
				PUSH_STACK(a | b);
				pc++;
				break;
			}
			case Bytecode::Bit_xor:
			{
				GET_TOP_TWO_STACK;
				PUSH_STACK(a ^ b);
				pc++;
				break;
			}
			case Bytecode::Bit_not:
			{
				u32 a;
				POP_STACK(a);
				PUSH_STACK(~a);
				pc++;
				break;
			}
			case Bytecode::Equal:
			{
				GET_TOP_TWO_STACK;
				PUSH_STACK(a == b ? 1 : 0);
				pc++;
				break;
			}
			case Bytecode::Less_signed:
			{
				GET_TOP_TWO_STACK;
				PUSH_STACK(static_cast<i32>(b) < static_cast<i32>(a) ? 1 : 0);
				pc++;
				break;
			}
			case Bytecode::Less_unsigned:
			{
				GET_TOP_TWO_STACK;
				PUSH_STACK(b < a ? 1 : 0);
				pc++;
				break;
			}
			case Bytecode::Ext:
			{
				u8 n;
				GET_OP_AT_INDEX(n, pc + 1);
				u32 a;
				POP_STACK(a);
				PUSH_STACK((u32)(static_cast<i32>(static_cast<i32>(a) << (32 - n)) >> (32 - n)));
				pc += 2;
				break;
			}
			case Bytecode::Zero_ext:
			{
				u8 n;
				GET_OP_AT_INDEX(n, pc + 1);
				u32 a;
				POP_STACK(a);
				PUSH_STACK(a & ((1U << n) - 1));
				pc += 2;
				break;
			}
			case Bytecode::Ref8:
			{
				u32 addr;
				POP_STACK(addr);
				u32 value;
				GET_MEMORY_AT_ADDR(value, addr);
				PUSH_STACK(value);
				pc++;
				break;
			}
			case Bytecode::Ref16:
			{
				u32 addr;
				POP_STACK(addr);

				std::array<u8, 2> array;

				GET_MEMORY_AT_ADDR(array[0], addr);
				GET_MEMORY_AT_ADDR(array[1], addr + 1);

				PUSH_STACK(std::bit_cast<u16>(array));

				pc++;
				break;
			}
			case Bytecode::Ref32:
			{
				u32 addr;
				POP_STACK(addr);

				std::array<u8, 4> array;

				GET_MEMORY_AT_ADDR(array[0], addr);
				GET_MEMORY_AT_ADDR(array[1], addr + 1);
				GET_MEMORY_AT_ADDR(array[2], addr + 2);
				GET_MEMORY_AT_ADDR(array[3], addr + 3);

				PUSH_STACK(std::bit_cast<u32>(array));

				pc++;
				break;
			}
			case Bytecode::Dup:
			{
				u32 a;
				POP_STACK(a);
				PUSH_STACK(a);
				PUSH_STACK(a);
				pc++;
				break;
			}
			case Bytecode::Swap:
			{
				u32 a, b;
				POP_STACK(a);
				POP_STACK(b);
				PUSH_STACK(a);
				PUSH_STACK(b);
				pc++;
				break;
			}
			case Bytecode::Pop:
			{
				u32 a;
				POP_STACK(a);
				pc++;
				break;
			}
			case Bytecode::Pick:
			{
				u8 n;
				GET_OP_AT_INDEX(n, pc + 1);

				u32 val;
				GET_STACK_AT_INDEX(val, n);
				PUSH_STACK(val);
				pc += 2;
				break;
			}
			case Bytecode::Rot:
			{
				u32 a, b, c;
				POP_STACK(c);
				POP_STACK(b);
				POP_STACK(a);
				PUSH_STACK(c);
				PUSH_STACK(a);
				PUSH_STACK(b);
				pc++;
				break;
			}
			case Bytecode::If_goto:
			{
				u32 a;
				POP_STACK(a);

				if (a == 0)
				{
					pc += 3;
					break;
				}

				std::array<u8, 2> array;

				GET_OP_AT_INDEX(array[1], pc + 1);  // Most-significant byte first
				GET_OP_AT_INDEX(array[0], pc + 2);

				pc = std::bit_cast<u16>(array);
				break;
			}
			case Bytecode::Goto:
			{
				std::array<u8, 2> array;

				GET_OP_AT_INDEX(array[1], pc + 1);  // Most-significant byte first
				GET_OP_AT_INDEX(array[0], pc + 2);

				pc = std::bit_cast<u16>(array);
				break;
			}
			case Bytecode::Const8:
			{
				u8 value;
				GET_OP_AT_INDEX(value, pc + 1);
				PUSH_STACK(value);
				pc += 2;
				break;
			}
			case Bytecode::Const16:
			{
				std::array<u8, 2> array;

				GET_OP_AT_INDEX(array[1], pc + 1);  // Most-significant byte first
				GET_OP_AT_INDEX(array[0], pc + 2);

				PUSH_STACK(std::bit_cast<u16>(array));
				pc += 3;
				break;
			}
			case Bytecode::Const32:
			{
				std::array<u8, 4> array;

				GET_OP_AT_INDEX(array[3], pc + 1);  // Most-significant byte first
				GET_OP_AT_INDEX(array[2], pc + 2);
				GET_OP_AT_INDEX(array[1], pc + 3);
				GET_OP_AT_INDEX(array[0], pc + 4);

				PUSH_STACK(std::bit_cast<u32>(array));
				pc += 5;
				break;
			}
			case Bytecode::Reg:
			{
				std::array<u8, 2> array;

				GET_OP_AT_INDEX(array[1], pc + 1);  // Most-significant byte first
				GET_OP_AT_INDEX(array[0], pc + 2);

				if (const auto val = access_register_func(std::bit_cast<u16>(array)); !val.has_value())
				{
					return std::unexpected(Execute_error::Register_access_error);
				}
				else
				{
					PUSH_STACK(*val);
					pc += 3;
					break;
				}
			}
			case Bytecode::End:
			{
				u32 top;
				POP_STACK(top);
				if (stack.empty()) return Execute_result{.top = top};

				u32 next_to_top;
				POP_STACK(next_to_top);
				return Execute_result{.top = top, .next_to_top = next_to_top};
			}
			default:
				return std::unexpected(Execute_error::Unsupported_bytecode);
			}
		}
	}

}