#include <array>
#include <gtest/gtest.h>
#include <print>

#include "gdb-stub/expression.hpp"

#define BYTECODE(op, ...) static_cast<u8>(gdb_stub::expression::Bytecode::op) __VA_OPT__(, ) __VA_ARGS__

TEST(Expresion, Basic)
{
	const std::array<u32, 4> registers = {0, 2, 3, 5};
	const std::array<u32, 4> memory = {7, 11, 13, 17};

	const std::array bytecode = std::to_array<u8>(
		{BYTECODE(Reg, 0x00, 0x01),
		 BYTECODE(Reg, 0x00, 0x02),
		 BYTECODE(Const32, 0x00, 0x00, 0x00, 0x03 * 4),
		 BYTECODE(Ref32),
		 BYTECODE(Ext, 32),
		 BYTECODE(Mul),
		 BYTECODE(Add),
		 BYTECODE(End)}
	);

	auto access_register = [&registers](u32 index) -> std::optional<u32>
	{
		if (index >= registers.size()) return std::nullopt;

		return registers[index];
	};

	auto access_memory = [&memory](u32 addr) -> std::optional<u8>
	{
		if (addr >= memory.size() * 4) return std::nullopt;

		const u8* byte_ptr = reinterpret_cast<const u8*>(memory.data());
		return byte_ptr[addr];
	};

	const auto result = gdb_stub::expression::execute(access_memory, access_register, bytecode);

	EXPECT_TRUE(result.has_value());
	EXPECT_EQ(result->top, registers[1] + memory[3] * registers[2]);
	EXPECT_FALSE(result->next_to_top.has_value());
}

TEST(Expression, MemoryOutOfBound)
{
	const std::array<u32, 4> registers = {0, 2, 3, 5};
	const std::array<u32, 4> memory = {7, 11, 13, 17};

	// Const32 -> address 16 (out of bound for memory of 4 u32 = 16 bytes => valid addresses 0..15)
	const std::array bytecode
		= std::to_array<u8>({BYTECODE(Const32, 0x00, 0x00, 0x00, 0x10), BYTECODE(Ref32), BYTECODE(End)});

	auto access_register = [&registers](u32 index) -> std::optional<u32>
	{
		if (index >= registers.size()) return std::nullopt;
		return registers[index];
	};

	auto access_memory = [&memory](u32 addr) -> std::optional<u8>
	{
		if (addr >= memory.size() * 4) return std::nullopt;
		const u8* byte_ptr = reinterpret_cast<const u8*>(memory.data());
		return byte_ptr[addr];
	};

	const auto result = gdb_stub::expression::execute(access_memory, access_register, bytecode);

	EXPECT_FALSE(result.has_value());
	EXPECT_EQ(result.error(), gdb_stub::expression::Execute_error::Memory_access_error);
}

TEST(Expression, RegisterOutOfBound)
{
	const std::array<u32, 4> registers = {0, 2, 3, 5};
	const std::array<u32, 4> memory = {7, 11, 13, 17};

	// Reg index 9 (out of bound)
	const std::array bytecode = std::to_array<u8>({BYTECODE(Reg, 0x00, 0x09), BYTECODE(End)});

	auto access_register = [&registers](u32 index) -> std::optional<u32>
	{
		if (index >= registers.size()) return std::nullopt;
		return registers[index];
	};

	auto access_memory = [&memory](u32 addr) -> std::optional<u8>
	{
		if (addr >= memory.size() * 4) return std::nullopt;
		const u8* byte_ptr = reinterpret_cast<const u8*>(memory.data());
		return byte_ptr[addr];
	};

	const auto result = gdb_stub::expression::execute(access_memory, access_register, bytecode);

	EXPECT_FALSE(result.has_value());
	EXPECT_EQ(result.error(), gdb_stub::expression::Execute_error::Register_access_error);
}

TEST(Expression, DivisionByZero)
{
	const std::array<u32, 4> registers = {0, 2, 3, 5};
	const std::array<u32, 4> memory = {7, 11, 13, 17};

	// 10 / 0 -> division by zero
	const std::array bytecode = std::to_array<u8>(
		{BYTECODE(Const32, 0x00, 0x00, 0x00, 0x0A),
		 BYTECODE(Const32, 0x00, 0x00, 0x00, 0x00),
		 BYTECODE(Div_unsigned),
		 BYTECODE(End)}
	);

	auto access_register = [&registers](u32 index) -> std::optional<u32>
	{
		if (index >= registers.size()) return std::nullopt;
		return registers[index];
	};

	auto access_memory = [&memory](u32 addr) -> std::optional<u8>
	{
		if (addr >= memory.size() * 4) return std::nullopt;
		const u8* byte_ptr = reinterpret_cast<const u8*>(memory.data());
		return byte_ptr[addr];
	};

	const auto result = gdb_stub::expression::execute(access_memory, access_register, bytecode);

	EXPECT_FALSE(result.has_value());
	EXPECT_EQ(result.error(), gdb_stub::expression::Execute_error::Division_by_zero);
}

TEST(Expression, UnalignedMemoryAccessSucceeds)
{
	// bytes memory to exercise unaligned 4-byte reads
	const std::array<u8, 8> memory_bytes = {1, 2, 3, 4, 5, 6, 7, 8};
	const std::array<u32, 0> registers = {};

	// Read 4 bytes starting at byte address 1 (unaligned). Expect little-endian assembly:
	// value = 2 + (3<<8) + (4<<16) + (5<<24)
	const std::array bytecode
		= std::to_array<u8>({BYTECODE(Const32, 0x00, 0x00, 0x00, 0x01), BYTECODE(Ref32), BYTECODE(End)});

	auto access_register = [&registers](u32 index) -> std::optional<u32>
	{
		if (index >= registers.size()) return std::nullopt;
		return registers[index];
	};

	auto access_memory = [&memory_bytes](u32 addr) -> std::optional<u8>
	{
		if (addr >= memory_bytes.size()) return std::nullopt;
		return memory_bytes[addr];
	};

	const auto result = gdb_stub::expression::execute(access_memory, access_register, bytecode);

	EXPECT_TRUE(result.has_value());
	const u32 expected = static_cast<u32>(memory_bytes[1]) | (static_cast<u32>(memory_bytes[2]) << 8)
					   | (static_cast<u32>(memory_bytes[3]) << 16)
					   | (static_cast<u32>(memory_bytes[4]) << 24);
	EXPECT_EQ(result->top, expected);
}

TEST(Expression, StackUnderflow)
{
	// Try to perform Add with only one value on stack
	const std::array<u8, 7> bytecode
		= {BYTECODE(Const32, 0x00, 0x00, 0x00, 0x01), BYTECODE(Add), BYTECODE(End)};
	auto access_register = [](u32) -> std::optional<u32>
	{
		return 0;
	};
	auto access_memory = [](u32) -> std::optional<u8>
	{
		return 0;
	};

	const auto result = gdb_stub::expression::execute(access_memory, access_register, bytecode);

	EXPECT_FALSE(result.has_value());
	EXPECT_EQ(result.error(), gdb_stub::expression::Execute_error::Stack_out_of_bound);
}

TEST(Expression, StackUnderflowPop)
{
	// Try to pop from empty stack
	const std::array<u8, 2> bytecode = {BYTECODE(Pop), BYTECODE(End)};
	auto access_register = [](u32) -> std::optional<u32>
	{
		return 0;
	};
	auto access_memory = [](u32) -> std::optional<u8>
	{
		return 0;
	};

	const auto result = gdb_stub::expression::execute(access_memory, access_register, bytecode);

	EXPECT_FALSE(result.has_value());
	EXPECT_EQ(result.error(), gdb_stub::expression::Execute_error::Stack_out_of_bound);
}

TEST(Expression, JumpOutOfBound)
{
	// Goto to an address out of bytecode range
	const std::array<u8, 9> bytecode
		= {BYTECODE(Const32, 0x00, 0x00, 0x00, 0x00),
		   BYTECODE(Goto),
		   0xFF,
		   0xFF,  // jump to 0xFFFF
		   BYTECODE(End)};
	auto access_register = [](u32) -> std::optional<u32>
	{
		return 0;
	};
	auto access_memory = [](u32) -> std::optional<u8>
	{
		return 0;
	};

	const auto result = gdb_stub::expression::execute(access_memory, access_register, bytecode);

	EXPECT_FALSE(result.has_value());
	EXPECT_EQ(result.error(), gdb_stub::expression::Execute_error::Bytecode_out_of_bound);
}

TEST(Expression, IfGotoOutOfBound)
{
	// If_goto to an address out of bytecode range
	const std::array<u8, 10> bytecode
		= {BYTECODE(Const32, 0x00, 0x00, 0x00, 0x01),  // push 1 (true)
		   BYTECODE(If_goto),
		   0xFF,
		   0xFF,  // jump to 0xFFFF
		   BYTECODE(End)};
	auto access_register = [](u32) -> std::optional<u32>
	{
		return 0;
	};
	auto access_memory = [](u32) -> std::optional<u8>
	{
		return 0;
	};

	const auto result = gdb_stub::expression::execute(access_memory, access_register, bytecode);

	EXPECT_FALSE(result.has_value());
	EXPECT_EQ(result.error(), gdb_stub::expression::Execute_error::Bytecode_out_of_bound);
}

TEST(Expression, UnsupportedBytecode)
{
	// Use an invalid/unsupported bytecode
	const std::array<u8, 2> bytecode = {0xFF, BYTECODE(End)};
	auto access_register = [](u32) -> std::optional<u32>
	{
		return 0;
	};
	auto access_memory = [](u32) -> std::optional<u8>
	{
		return 0;
	};

	const auto result = gdb_stub::expression::execute(access_memory, access_register, bytecode);

	EXPECT_FALSE(result.has_value());
	EXPECT_EQ(result.error(), gdb_stub::expression::Execute_error::Unsupported_bytecode);
}

TEST(Expression, BytecodeOutOfBound)
{
	// Bytecode sequence ends unexpectedly (e.g., Const32 expects 4 bytes, but only 2 provided)
	const std::array<u8, 4> bytecode = {BYTECODE(Const32, 0x00, 0x00), BYTECODE(End)};
	auto access_register = [](u32) -> std::optional<u32>
	{
		return 0;
	};
	auto access_memory = [](u32) -> std::optional<u8>
	{
		return 0;
	};

	const auto result = gdb_stub::expression::execute(access_memory, access_register, bytecode);

	EXPECT_FALSE(result.has_value());
	EXPECT_EQ(result.error(), gdb_stub::expression::Execute_error::Bytecode_out_of_bound);
}

TEST(Expression, PickStackUnderflow)
{
	// Pick with empty stack
	const std::array<u8, 3> bytecode = {BYTECODE(Pick), 0x01, BYTECODE(End)};
	auto access_register = [](u32) -> std::optional<u32>
	{
		return 0;
	};
	auto access_memory = [](u32) -> std::optional<u8>
	{
		return 0;
	};

	const auto result = gdb_stub::expression::execute(access_memory, access_register, bytecode);

	EXPECT_FALSE(result.has_value());
	EXPECT_EQ(result.error(), gdb_stub::expression::Execute_error::Stack_out_of_bound);
}

TEST(Expression, RotStackUnderflow)
{
	// Rot with less than 3 elements
	const std::array<u8, 7> bytecode
		= {BYTECODE(Const32, 0x00, 0x00, 0x00, 0x01), BYTECODE(Rot), BYTECODE(End)};

	auto access_register = [](u32) -> std::optional<u32>
	{
		return 0;
	};
	auto access_memory = [](u32) -> std::optional<u8>
	{
		return 0;
	};

	const auto result = gdb_stub::expression::execute(access_memory, access_register, bytecode);

	EXPECT_FALSE(result.has_value());
	EXPECT_EQ(result.error(), gdb_stub::expression::Execute_error::Stack_out_of_bound);
}

TEST(Expression, Const8AndConst16)
{
	// Const8 -> 0xAB ; Const16 -> 0x1234 (most-significant byte first)
	{
		const std::array bytecode = std::to_array<u8>({BYTECODE(Const8, 0xAB), BYTECODE(End)});

		auto access_register = [](u32) -> std::optional<u32>
		{
			return 0;
		};
		auto access_memory = [](u32) -> std::optional<u8>
		{
			return 0;
		};

		const auto result = gdb_stub::expression::execute(access_memory, access_register, bytecode);
		ASSERT_TRUE(result.has_value());
		EXPECT_EQ(result->top, 0xABu);
	}

	{
		// MSB first: 0x1234 -> 0x12, 0x34
		const std::array bytecode = std::to_array<u8>({BYTECODE(Const16, 0x12, 0x34), BYTECODE(End)});

		auto access_register = [](u32) -> std::optional<u32>
		{
			return 0;
		};
		auto access_memory = [](u32) -> std::optional<u8>
		{
			return 0;
		};

		const auto result = gdb_stub::expression::execute(access_memory, access_register, bytecode);
		ASSERT_TRUE(result.has_value());
		EXPECT_EQ(result->top, 0x1234u);
	}
}

TEST(Expression, ExtAndZeroExtWidths)
{
	// Ext 8: sign-extend 0xFF -> 0xFFFFFFFF
	{
		const std::array bytecode
			= std::to_array<u8>({BYTECODE(Const8, 0xFF), BYTECODE(Ext, 8), BYTECODE(End)});
		auto access_register = [](u32) -> std::optional<u32>
		{
			return 0;
		};
		auto access_memory = [](u32) -> std::optional<u8>
		{
			return 0;
		};

		const auto result = gdb_stub::expression::execute(access_memory, access_register, bytecode);
		ASSERT_TRUE(result.has_value());
		EXPECT_EQ(result->top, 0xFFFFFFFFu);
	}

	// Zero_ext 8: zero-extend 0xFF -> 0x000000FF
	{
		const std::array bytecode
			= std::to_array<u8>({BYTECODE(Const8, 0xFF), BYTECODE(Zero_ext, 8), BYTECODE(End)});
		auto access_register = [](u32) -> std::optional<u32>
		{
			return 0;
		};
		auto access_memory = [](u32) -> std::optional<u8>
		{
			return 0;
		};

		const auto result = gdb_stub::expression::execute(access_memory, access_register, bytecode);
		ASSERT_TRUE(result.has_value());
		EXPECT_EQ(result->top, 0x000000FFu);
	}

	// Ext 16: sign-extend 0x8000 -> 0xFFFF8000
	{
		// MSB first: 0x8000 -> 0x80, 0x00
		const std::array bytecode
			= std::to_array<u8>({BYTECODE(Const16, 0x80, 0x00), BYTECODE(Ext, 16), BYTECODE(End)});
		auto access_register = [](u32) -> std::optional<u32>
		{
			return 0;
		};
		auto access_memory = [](u32) -> std::optional<u8>
		{
			return 0;
		};

		const auto result = gdb_stub::expression::execute(access_memory, access_register, bytecode);
		ASSERT_TRUE(result.has_value());
		EXPECT_EQ(result->top, 0xFFFF8000u);
	}

	// Zero_ext 16: start from 0xFFFF8001 -> zero_ext 16 -> 0x00008001
	{
		// MSB first: 0xFFFF8001 -> 0xFF, 0xFF, 0x80, 0x01
		const std::array bytecode = std::to_array<u8>(
			{BYTECODE(Const32, 0xFF, 0xFF, 0x80, 0x01), BYTECODE(Zero_ext, 16), BYTECODE(End)}
		);
		auto access_register = [](u32) -> std::optional<u32>
		{
			return 0;
		};
		auto access_memory = [](u32) -> std::optional<u8>
		{
			return 0;
		};

		const auto result = gdb_stub::expression::execute(access_memory, access_register, bytecode);
		ASSERT_TRUE(result.has_value());
		EXPECT_EQ(result->top, 0x00008001u);
	}
}

TEST(Expression, RefWidths)
{
	const std::array<u8, 8> memory_bytes = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
	const std::array<u32, 0> registers = {};

	auto access_register = [&registers](u32 index) -> std::optional<u32>
	{
		if (index >= registers.size()) return std::nullopt;
		return registers[index];
	};

	auto access_memory = [&memory_bytes](u32 addr) -> std::optional<u8>
	{
		if (addr >= memory_bytes.size()) return std::nullopt;
		return memory_bytes[addr];
	};

	// Ref8 at address 2 -> 0x33
	{
		// MSB first: address 2 -> 0x00,0x00,0x00,0x02
		const std::array bytecode
			= std::to_array<u8>({BYTECODE(Const32, 0x00, 0x00, 0x00, 0x02), BYTECODE(Ref8), BYTECODE(End)});
		const auto result = gdb_stub::expression::execute(access_memory, access_register, bytecode);
		ASSERT_TRUE(result.has_value());
		EXPECT_EQ(result->top, static_cast<u32>(0x33));
	}

	// Ref16 at address 1 -> 0x3322 (little-endian: low=0x22, high=0x33)
	{
		// MSB first: address 1 -> 0x00,0x00,0x00,0x01
		const std::array bytecode
			= std::to_array<u8>({BYTECODE(Const32, 0x00, 0x00, 0x00, 0x01), BYTECODE(Ref16), BYTECODE(End)});
		const auto result = gdb_stub::expression::execute(access_memory, access_register, bytecode);
		ASSERT_TRUE(result.has_value());
		EXPECT_EQ(result->top, static_cast<u32>(0x3322));
	}

	// Ref32 at address 0 -> 0x44332211 (little-endian)
	{
		const std::array bytecode
			= std::to_array<u8>({BYTECODE(Const32, 0x00, 0x00, 0x00, 0x00), BYTECODE(Ref32), BYTECODE(End)});
		const auto result = gdb_stub::expression::execute(access_memory, access_register, bytecode);
		ASSERT_TRUE(result.has_value());
		EXPECT_EQ(result->top, static_cast<u32>(0x44332211));
	}
}