#include <gtest/gtest.h>

#include "gdb-stub/response.hpp"

using namespace gdb_stub::response;

TEST(ResponseGen, RegisterContent)
{
	{
		const std::vector<std::optional<u32>> reg_values = {0, 0xdeadbeef, 0x12345678};
		const Register_content response(reg_values);
		EXPECT_EQ(response.to_string(), "00000000efbeadde78563412");
	}

	{
		const std::vector<std::optional<u32>> reg_values = {std::nullopt};
		const Register_content response(reg_values);
		EXPECT_EQ(response.to_string(), "xxxxxxxx");
	}
}

TEST(ResponseGen, RawByteStream)
{
	{
		const std::vector<u8> data = {0x0, 0x1, 0x2, 0x3};
		const Raw_byte_stream response(data);
		EXPECT_EQ(response.to_string(), "00010203");
	}

	{
		const std::vector<u8> data = {};
		const Raw_byte_stream response(data);
		EXPECT_EQ(response.to_string(), "E00");
	}
}

TEST(ResponseGen, ErrorCode)
{
	EXPECT_EQ(Error_code(0).to_string(), "E00");
	EXPECT_EQ(Error_code(255).to_string(), "Eff");
}

TEST(ResponseGen, ErrorMessage)
{
	EXPECT_EQ(Error_message("Test").to_string(), "E.Test");
	EXPECT_EQ(Error_message("$$$").to_string(), "E.$$$");
}

TEST(ResponseGen, StopReason)
{
	EXPECT_EQ(Stop_reason(0x3f).to_string(), "T3f");

	{
		const auto response = Stop_reason(Stop_reason::Breakpoint_hit{.is_hardware = true});
		EXPECT_EQ(response.to_string(), "T05hwbreak:;");
	}

	{
		const auto response = Stop_reason(Stop_reason::Breakpoint_hit{.is_hardware = false});
		EXPECT_EQ(response.to_string(), "T05swbreak:;");
	}

	{
		const auto response = Stop_reason(
			Stop_reason::Watchpoint_hit{
				.address = 0x123,
				.is_write = true,
				.is_read = false,
			}
		);
		EXPECT_EQ(response.to_string(), "T05watch:123;");
	}

	{
		const auto response = Stop_reason(
			Stop_reason::Watchpoint_hit{
				.address = 0x123,
				.is_write = false,
				.is_read = true,
			}
		);
		EXPECT_EQ(response.to_string(), "T05rwatch:123;");
	}

	{
		const auto response = Stop_reason(
			Stop_reason::Watchpoint_hit{
				.address = 0x123,
				.is_write = true,
				.is_read = true,
			}
		);
		EXPECT_EQ(response.to_string(), "T05awatch:123;");
	}
}

TEST(ResponseGen, Qxfer)
{
	const std::vector<u8> data = {'2', '3', 'e', 'a'};

	EXPECT_EQ(Qxfer_response(false, data).to_string(), "m23ea");
	EXPECT_EQ(Qxfer_response(true, data).to_string(), "l23ea");
	EXPECT_EQ(Qxfer_response(true, {}).to_string(), "l");
}