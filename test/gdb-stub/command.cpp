#include <algorithm>
#include <gtest/gtest.h>
#include <ranges>

#include "gdb-stub/command.hpp"

using namespace gdb_stub;

#define TEST_POSITIVE(str, _type, ...)                                                                       \
	{                                                                                                        \
		const auto result = command::parse(str);                                                             \
		ASSERT_TRUE(result.has_value());                                                                     \
		ASSERT_TRUE(result.type() == typeid(_type));                                                         \
		const auto& value [[maybe_unused]] = std::any_cast<_type>(result);                                   \
		__VA_ARGS__                                                                                          \
	}

#define TEST_NEGATIVE(str)                                                                                   \
	{                                                                                                        \
		const auto result = command::parse(str);                                                             \
		EXPECT_FALSE(result.has_value());                                                                    \
	}

#define OPTIONAL_EQ(val, expect)                                                                             \
	EXPECT_TRUE((val).has_value());                                                                          \
	if ((val).has_value()) EXPECT_EQ((val).value(), expect);

#define OPTIONAL_NOVAL(val) EXPECT_FALSE((val).has_value())

TEST(CommandParse, Acknowledge)
{
	TEST_POSITIVE("+", Acknowledge_packet, { EXPECT_EQ(value.success, true); });
	TEST_NEGATIVE("+www");

	TEST_POSITIVE("-", Acknowledge_packet, { EXPECT_EQ(value.success, false); });
	TEST_NEGATIVE("-bbb");
}

TEST(CommandParse, EnablePersistant)
{
	TEST_POSITIVE("!", command::Enable_persistant);
	TEST_NEGATIVE("!!!");
}

TEST(CommandParse, AskHaltReason)
{
	TEST_POSITIVE("?", command::Ask_halt_reason);
	TEST_NEGATIVE("?sdfas");
}

TEST(CommandParse, ReadGeneralRegister)
{
	TEST_POSITIVE("g", command::Read_register);
	TEST_NEGATIVE("ggg");
}

TEST(CommandParse, Continue)
{
	TEST_POSITIVE("c12345678", command::Continue, { OPTIONAL_EQ(value.address, 0x12345678); });
	TEST_POSITIVE("cdeadbeef", command::Continue, { OPTIONAL_EQ(value.address, 0xdeadbeef); });
	TEST_POSITIVE("cDEADBEEF", command::Continue, { OPTIONAL_EQ(value.address, 0xdeadbeef); });
	TEST_POSITIVE("cBEEF", command::Continue, { OPTIONAL_EQ(value.address, 0x0000beef); });
	TEST_POSITIVE("c", command::Continue, { OPTIONAL_NOVAL(value.address); });

	TEST_NEGATIVE("c1234w555");
}

TEST(CommandParse, WriteRegister)
{
	TEST_POSITIVE("G12345678", command::Write_register, {
		ASSERT_EQ(value.values.size(), 1);
		EXPECT_EQ(value.values.at(0), 0x12345678);
	});

	TEST_POSITIVE("Gdeadbeef", command::Write_register, {
		ASSERT_EQ(value.values.size(), 1);
		EXPECT_EQ(value.values.at(0), 0xdeadbeef);
	});

	TEST_POSITIVE("Gxxxxxxxx", command::Write_register, {
		ASSERT_EQ(value.values.size(), 0);
		EXPECT_FALSE(value.values.contains(0));
	});

	TEST_POSITIVE("G12345678xxxxxxxxdeadbeef", command::Write_register, {
		ASSERT_EQ(value.values.size(), 2);

		EXPECT_EQ(value.values.at(0), 0x12345678);
		EXPECT_EQ(value.values.at(2), 0xdeadbeef);
		EXPECT_FALSE(value.values.contains(1));
	});

	TEST_NEGATIVE("G1")
	TEST_NEGATIVE("G123548w3")
	TEST_NEGATIVE("GXXXXXXXX")
	TEST_NEGATIVE("GXX1234XX")
}

TEST(CommandParse, StepSingleCycle)
{
	TEST_POSITIVE("i", command::Step_cycles, {
		OPTIONAL_NOVAL(value.address);
		OPTIONAL_NOVAL(value.cycle_count);
	});

	TEST_POSITIVE("ideadbeef", command::Step_cycles, {
		OPTIONAL_EQ(value.address, 0xdeadbeef);
		OPTIONAL_NOVAL(value.cycle_count);
	});

	TEST_POSITIVE("ideadbeef,123", command::Step_cycles, {
		OPTIONAL_EQ(value.address, 0xdeadbeef);
		OPTIONAL_EQ(value.cycle_count, 0x123);
	});

	TEST_NEGATIVE("ixwx");
	TEST_NEGATIVE("i123,");
	TEST_NEGATIVE("i1234,xwx")
	TEST_NEGATIVE("i123,123,123")
}

TEST(CommandParse, ReadMemory)
{
	TEST_POSITIVE("m123,456", command::Read_memory, {
		EXPECT_EQ(value.address, 0x123);
		EXPECT_EQ(value.length, 0x456);
	});

	TEST_NEGATIVE("m")
	TEST_NEGATIVE("m123")
	TEST_NEGATIVE("m123,")
	TEST_NEGATIVE("mxwx,xwx")
	TEST_NEGATIVE("mxwx,12345")
}

TEST(CommandParse, WriteMemory)
{
	TEST_POSITIVE("M123,2:1234", command::Write_memory, {
		EXPECT_EQ(value.address, 0x123);
		EXPECT_EQ(value.data, std::vector<u8>({0x12, 0x34}));
	});

	TEST_POSITIVE("MdeaDBeef,8:deadBEef12345678", command::Write_memory, {
		EXPECT_EQ(value.address, 0xdeadbeef);
		EXPECT_EQ(value.data, std::vector<u8>({0xde, 0xad, 0xbe, 0xef, 0x12, 0x34, 0x56, 0x78}));
	});

	TEST_NEGATIVE("M")
	TEST_NEGATIVE("M:")
	TEST_NEGATIVE("M:EEFFAA")
	TEST_NEGATIVE("Mdeadbeef")
	TEST_NEGATIVE("Mdeadbeef,")
	TEST_NEGATIVE("Mdeadbeef,16")
	TEST_NEGATIVE("M,2:dead")
	TEST_NEGATIVE("Mdeadbexx,2:EFEF")
	TEST_NEGATIVE("Mdeadbeef,2:EFEF:www:qq")
	TEST_NEGATIVE("Mdeadbeef,2:EFXX")
	TEST_NEGATIVE("Mdeadbeef,3:ABcd")
}

TEST(CommandParse, ReadWriteSingleRegister)
{
	TEST_POSITIVE("p0", command::Read_single_register, { EXPECT_EQ(value.regno, 0); });
	TEST_POSITIVE("pFFFF", command::Read_single_register, { EXPECT_EQ(value.regno, 0xFFFF); });

	TEST_NEGATIVE("pXXxx");
	TEST_NEGATIVE("p10000");
	TEST_NEGATIVE("p123156186489165156465456316");
	TEST_NEGATIVE("p");

	TEST_POSITIVE("P0=deadbeef", command::Write_single_register, {
		EXPECT_EQ(value.regno, 0);
		EXPECT_EQ(value.value, 0xdeadbeef);
	});
	TEST_POSITIVE("PFFFF=cafecafe", command::Write_single_register, {
		EXPECT_EQ(value.regno, 0xFFFF);
		EXPECT_EQ(value.value, 0xcafecafe);
	});

	TEST_NEGATIVE("P0=");
	TEST_NEGATIVE("P");
	TEST_NEGATIVE("P0");
	TEST_NEGATIVE("P10000=12345678");
	TEST_NEGATIVE("P0=12345sxx");
	TEST_NEGATIVE("P0=1234567");
	TEST_NEGATIVE("P0=123456789");
	TEST_NEGATIVE("P=12345678");
	TEST_NEGATIVE("Px=12345678")
}

TEST(CommandParse, StepSingle)
{
	TEST_POSITIVE("s12345678", command::Step_single_inst, { OPTIONAL_EQ(value.address, 0x12345678); });
	TEST_POSITIVE("sdeadbeef", command::Step_single_inst, { OPTIONAL_EQ(value.address, 0xdeadbeef); });
	TEST_POSITIVE("sDEADBEEF", command::Step_single_inst, { OPTIONAL_EQ(value.address, 0xdeadbeef); });
	TEST_POSITIVE("sBEEF", command::Step_single_inst, { OPTIONAL_EQ(value.address, 0x0000beef); });
	TEST_POSITIVE("s", command::Step_single_inst, { OPTIONAL_NOVAL(value.address); });

	TEST_NEGATIVE("s1234w555");
}

TEST(CommandParse, qSupported)
{
	using namespace std::string_view_literals;
	using Host_feature_status = command::Query_supported::Host_feature_status;

	const auto real_example = "qSupported:multiprocess+;swbreak+;hwbreak+;qRelocInsn+;fork-events+;vfork-"
							  "events+;exec-events+;vContSupported+;QThreadEvents+;QThreadOptions+;no-"
							  "resumed+;memory-tagging+;xmlRegisters=i386;error-message+";

	TEST_POSITIVE(real_example, command::Query_supported, {
		EXPECT_EQ(value.features.at("multiprocess").property, Host_feature_status::Supported);
		EXPECT_EQ(value.features.at("swbreak").property, Host_feature_status::Supported);
		EXPECT_EQ(value.features.at("hwbreak").property, Host_feature_status::Supported);
		EXPECT_EQ(value.features.at("qRelocInsn").property, Host_feature_status::Supported);
		EXPECT_EQ(value.features.at("fork-events").property, Host_feature_status::Supported);
		EXPECT_EQ(value.features.at("vfork-events").property, Host_feature_status::Supported);
		EXPECT_EQ(value.features.at("exec-events").property, Host_feature_status::Supported);
		EXPECT_EQ(value.features.at("vContSupported").property, Host_feature_status::Supported);
		EXPECT_EQ(value.features.at("QThreadEvents").property, Host_feature_status::Supported);
		EXPECT_EQ(value.features.at("QThreadOptions").property, Host_feature_status::Supported);
		EXPECT_EQ(value.features.at("no-resumed").property, Host_feature_status::Supported);
		EXPECT_EQ(value.features.at("memory-tagging").property, Host_feature_status::Supported);
		EXPECT_EQ(value.features.at("error-message").property, Host_feature_status::Supported);

		EXPECT_EQ(value.features.at("xmlRegisters").property, Host_feature_status::Value);
		OPTIONAL_EQ(value.features.at("xmlRegisters").value, "i386");
	});
}

TEST(CommandParse, qXferFeatureRead)
{
	TEST_POSITIVE("qXfer:features:read:target.xml:0,ffb", command::Read_feature_xml, {
		EXPECT_EQ(value.annex, "target.xml");
		EXPECT_EQ(value.offset, 0);
		EXPECT_EQ(value.length, 0xffb);
	});

	TEST_POSITIVE("qXfer:features:read:riscv-xxx.xml:1333,13", command::Read_feature_xml, {
		EXPECT_EQ(value.annex, "riscv-xxx.xml");
		EXPECT_EQ(value.offset, 0x1333);
		EXPECT_EQ(value.length, 0x13);
	});

	TEST_NEGATIVE("qXfer:features:read:")
	TEST_NEGATIVE("qXfer:features:read:target.xml")
	TEST_NEGATIVE("qXfer:features:read:target.xml:0")
	TEST_NEGATIVE("qXfer:features:read:target.xml:0,ffq")
	TEST_NEGATIVE("qXfer:features:read:target.xml:qq,ffb")
	TEST_NEGATIVE("qXfer:features:read:target.xml:0,")
	TEST_NEGATIVE("qXfer:features:read:target.xml:,ffb")
}

TEST(CommandParse, qXferMemoryMapRead)
{
	TEST_POSITIVE("qXfer:memory-map:read::0,ffb", command::Read_memory_map_xml, {
		EXPECT_EQ(value.offset, 0);
		EXPECT_EQ(value.length, 0xffb);
	});

	TEST_POSITIVE("qXfer:memory-map:read::1333,13", command::Read_memory_map_xml, {
		EXPECT_EQ(value.offset, 0x1333);
		EXPECT_EQ(value.length, 0x13);
	});

	TEST_NEGATIVE("qXfer:memory-map:read::")
	TEST_NEGATIVE("qXfer:memory-map:read::0")
	TEST_NEGATIVE("qXfer:memory-map:read::0,ffq")
	TEST_NEGATIVE("qXfer:memory-map:read::qq,ffb")
	TEST_NEGATIVE("qXfer:memory-map:read::0,")
	TEST_NEGATIVE("qXfer:memory-map:read::,ffb")
}

TEST(CommandParse, AddBreakWatch)
{
	for (const auto num : {0, 1})
	{
		TEST_POSITIVE(std::format("Z{},deadbeef,2", num), command::Add_breakpoint, {
			EXPECT_EQ(value.is_hardware, num == 1);
			EXPECT_EQ(value.address, 0xdeadbeef);
			EXPECT_EQ(value.length, 2);
			EXPECT_FALSE(value.cond.has_value());
		});

		TEST_POSITIVE(std::format("Z{},deadbeef,2;X8,00112233AABBCCDD", num), command::Add_breakpoint, {
			EXPECT_EQ(value.is_hardware, num == 1);
			EXPECT_EQ(value.address, 0xdeadbeef);
			EXPECT_EQ(value.length, 2);
			ASSERT_TRUE(value.cond.has_value());
			EXPECT_EQ(value.cond.value(), std::vector<u8>({0x00, 0x11, 0x22, 0x33, 0xAA, 0xBB, 0xCC, 0xDD}));
		});

		TEST_NEGATIVE(std::format("Z{},deadbeef,2;X8,00112233AABBCC", num));
		TEST_NEGATIVE(std::format("Z{},deadbeef,2;X8,00112233AABBCXDD", num));
		TEST_NEGATIVE(std::format("Z{},deadbeef,2;X,00112233AABBCCDD", num));
		TEST_NEGATIVE(std::format("Z{},deadbeef,2;X8", num));
		TEST_NEGATIVE(std::format("Z{},deadbeef,2;X", num));
		TEST_NEGATIVE(std::format("Z{},deadbeef,2;X,", num));
	}

	for (const auto num : {2, 3, 4})
	{
		TEST_POSITIVE(std::format("Z{},cafecafe,4", num), command::Add_watchpoint, {
			EXPECT_EQ(value.watch_read, num == 3 || num == 4);
			EXPECT_EQ(value.watch_write, num == 2 || num == 4);
			EXPECT_EQ(value.address, 0xcafecafe);
			EXPECT_EQ(value.length, 4);
		});

		TEST_POSITIVE(std::format("Z{},cafe,12388", num), command::Add_watchpoint, {
			EXPECT_EQ(value.watch_read, num == 3 || num == 4);
			EXPECT_EQ(value.watch_write, num == 2 || num == 4);
			EXPECT_EQ(value.address, 0xcafe);
			EXPECT_EQ(value.length, 0x12388);
		});
	}

	for (const auto num : {0, 1, 2, 3, 4})
	{
		TEST_NEGATIVE(std::format("Z{},cafecafe,", num));
		TEST_NEGATIVE(std::format("Z{},cafecafe,123xx", num));
		TEST_NEGATIVE(std::format("Z{},cafeqwq,12388", num));
		TEST_NEGATIVE(std::format("Z{},,12388", num));
		TEST_NEGATIVE(std::format("Z{},,", num));
		TEST_NEGATIVE(std::format("Z{},", num));
		TEST_NEGATIVE(std::format("Z{}", num));
	}

	for (const auto c : {'x', 'b', '7', '5'}) TEST_NEGATIVE(std::format("Z{},cafe,12388", c));
}

TEST(CommandParse, RemoveBreakWatch)
{
	for (const auto num : {0, 1})
	{
		TEST_POSITIVE(std::format("z{},deadbeef,2", num), command::Remove_breakpoint, {
			EXPECT_EQ(value.is_hardware, num == 1);
			EXPECT_EQ(value.address, 0xdeadbeef);
			EXPECT_EQ(value.length, 2);
		});
	}

	for (const auto num : {2, 3, 4})
	{
		TEST_POSITIVE(std::format("z{},cafecafe,4", num), command::Remove_watchpoint, {
			EXPECT_EQ(value.watch_read, num == 3 || num == 4);
			EXPECT_EQ(value.watch_write, num == 2 || num == 4);
			EXPECT_EQ(value.address, 0xcafecafe);
			EXPECT_EQ(value.length, 4);
		});
	}

	for (const auto num : {0, 1, 2, 3, 4})
	{
		TEST_NEGATIVE(std::format("z{},cafecafe,", num));
		TEST_NEGATIVE(std::format("z{},cafecafe,123xx", num));
		TEST_NEGATIVE(std::format("z{},cafeqwq,12388", num));
		TEST_NEGATIVE(std::format("z{},,12388", num));
		TEST_NEGATIVE(std::format("z{},,", num));
		TEST_NEGATIVE(std::format("z{},", num));
		TEST_NEGATIVE(std::format("z{}", num));
	}

	for (const auto c : {'x', 'b', '7', '5'}) TEST_NEGATIVE(std::format("z{},cafe,12388", c));
}
