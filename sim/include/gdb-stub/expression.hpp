#pragma once

#include "common/type.hpp"
#include "core/cpu.hpp"
#include "core/memory.hpp"

#include <expected>
#include <functional>
#include <optional>
#include <span>

namespace gdb_stub::expression
{
	enum class Bytecode : u8
	{
		Float = 0x01,
		Add = 0x02,
		Sub = 0x03,
		Mul = 0x04,
		Div_signed = 0x05,
		Div_unsigned = 0x06,
		Rem_signed = 0x07,
		Rem_unsigned = 0x08,
		Lsh = 0x09,
		Rsh_signed = 0x0a,
		Rsh_unsigned = 0x0b,
		Log_not = 0x0e,
		Bit_and = 0x0f,
		Bit_or = 0x10,
		Bit_xor = 0x11,
		Bit_not = 0x12,
		Equal = 0x13,
		Less_signed = 0x14,
		Less_unsigned = 0x15,
		Ext = 0x16,
		Zero_ext = 0x2a,
		Ref8 = 0x17,
		Ref16 = 0x18,
		Ref32 = 0x19,
		Ref64 = 0x1a,
		Ref_float = 0x1b,
		Ref_double = 0x1c,
		Ref_long_double = 0x1d,
		L_to_d = 0x1e,
		D_to_l = 0x1f,
		Dup = 0x28,
		Swap = 0x2b,
		Pop = 0x29,
		Pick = 0x32,
		Rot = 0x33,
		If_goto = 0x20,
		Goto = 0x21,
		Const8 = 0x22,
		Const16 = 0x23,
		Const32 = 0x24,
		Const64 = 0x25,
		Reg = 0x26,
		Getv = 0x2c,
		Setv = 0x2d,
		Trace = 0x0c,
		Trace_quick = 0x0d,
		Trace16 = 0x30,
		Tracev = 0x2e,
		Tracenz = 0x2f,
		Printf = 0x34,
		End = 0x27
	};

	using Stack_element = u32;

	/**
	 * @brief Error codes for expression execution
	 * 
	 */
	enum class Execute_error
	{
		Unsupported_bytecode,
		Bytecode_out_of_bound,
		Stack_out_of_bound,
		Division_by_zero,
		Memory_access_error,
		Register_access_error
	};

	/**
	 * @brief Result of bytecode execution
	 * 
	 */
	struct Execute_result
	{
		/**
		 * @brief Top element of the stack after execution. \n
		 *        Typically the result of the expression.
		 */
		Stack_element top;

		/**
		 * @brief The element below the top of the stack after execution, if any. \n
		 *        Not all expressions will leave a second element on the stack.
		 */
		std::optional<Stack_element> next_to_top = std::nullopt;
	};

	/**
	 * @brief Execute bytecode sequence
	 * 
	 * @param access_memory_func Function for accessing memory
	 * @param access_register_func Function for accessing registers
	 * @param bytecode_sequence `std::vector<u8>` containing the bytecode sequence
	 * @return `Execute_result` if successful, `Execute_error` otherwise
	 */
	std::expected<Execute_result, Execute_error> execute(
		const std::function<std::optional<u8>(u32)>& access_memory_func,
		const std::function<std::optional<u32>(u32)>& access_register_func,
		std::span<const u8> bytecode_sequence
	);
}