#include <gtest/gtest.h>

#include "gdb-stub/packet.hpp"

TEST(PacketDecode, IncompletePacket)
{
	{
		const std::string_view input = {};
		gdb_stub::Packet_decoder decoder;
		decoder.push(input);
		ASSERT_FALSE(decoder.new_packet_available());
	}

	{
		const std::string_view input = "$";
		gdb_stub::Packet_decoder decoder;
		decoder.push(input);
		ASSERT_FALSE(decoder.new_packet_available());
	}

	{
		const std::string_view input = "$#";
		gdb_stub::Packet_decoder decoder;
		decoder.push(input);
		ASSERT_FALSE(decoder.new_packet_available());
	}
}

TEST(PacketDecode, Acknowledge)
{
	{
		const std::string_view input = "+";
		gdb_stub::Packet_decoder decoder;
		decoder.push(input);
		const auto packet = decoder.pop_packet();

		ASSERT_TRUE(packet.has_value());
		EXPECT_EQ(packet.value(), "+");
	}

	{
		const std::string_view input = "-";
		gdb_stub::Packet_decoder decoder;
		decoder.push(input);
		const auto packet = decoder.pop_packet();

		ASSERT_TRUE(packet.has_value());
		EXPECT_EQ(packet.value(), "-");
	}

	{
		const std::string_view input = "+$Hello#f4";
		gdb_stub::Packet_decoder decoder;
		decoder.push(input);

		const auto packet1 = decoder.pop_packet();
		ASSERT_TRUE(packet1.has_value());
		EXPECT_EQ(packet1.value(), "+");

		const auto packet2 = decoder.pop_packet();
		ASSERT_TRUE(packet2.has_value());
		EXPECT_EQ(packet2.value(), "Hello");
	}
}

TEST(PacketDecode, InvalidPacket)
{
	{
		const std::string_view input = "$#vv";
		gdb_stub::Packet_decoder decoder;
		decoder.push(input);
		const auto packet = decoder.pop_packet();
		ASSERT_FALSE(packet.has_value());
		EXPECT_EQ(packet.error(), gdb_stub::Packet_decoder::Error::Bad_packet);
	}

	{
		const std::string_view input = "$$";
		gdb_stub::Packet_decoder decoder;
		decoder.push(input);
		const auto packet = decoder.pop_packet();
		ASSERT_FALSE(packet.has_value());
		EXPECT_EQ(packet.error(), gdb_stub::Packet_decoder::Error::Bad_packet);
	}

	{
		const std::string_view input = "$##q";
		gdb_stub::Packet_decoder decoder;
		decoder.push(input);
		const auto packet = decoder.pop_packet();
		ASSERT_FALSE(packet.has_value());
		EXPECT_EQ(packet.error(), gdb_stub::Packet_decoder::Error::Bad_packet);
	}

	{
		const std::string_view input = "$#$";
		gdb_stub::Packet_decoder decoder;
		decoder.push(input);
		const auto packet = decoder.pop_packet();
		ASSERT_FALSE(packet.has_value());
		EXPECT_EQ(packet.error(), gdb_stub::Packet_decoder::Error::Bad_packet);
	}
}

TEST(PacketDecode, CheckSum)
{
	{
		const std::string_view input = "$#00";
		gdb_stub::Packet_decoder decoder;
		decoder.push(input);
		const auto packet = decoder.pop_packet();

		EXPECT_EQ(packet.value(), "");
	}

	{
		const std::string_view input = "$#f5";
		gdb_stub::Packet_decoder decoder;
		decoder.push(input);
		const auto packet = decoder.pop_packet();

		ASSERT_FALSE(packet.has_value());
	}

	{
		const std::string_view input = "$Hello#f4";
		gdb_stub::Packet_decoder decoder;
		decoder.push(input);
		const auto packet = decoder.pop_packet();

		EXPECT_EQ(packet.value(), "Hello");
	}

	{
		const std::string_view input = "$Hello#20";
		gdb_stub::Packet_decoder decoder;
		decoder.push(input);
		const auto packet = decoder.pop_packet();
		ASSERT_FALSE(packet.has_value());
		EXPECT_EQ(packet.error(), gdb_stub::Packet_decoder::Error::Bad_checksum);
	}
}

TEST(PacketDecode, BufferOverflow)
{
	{
		std::string input = "$";
		input.append((gdb_stub::Packet_decoder::max_buffer_size + 1), 'A');
		input.append("#");
		input.append(std::format("{:02x}", int('A') * (gdb_stub::Packet_decoder::max_buffer_size + 1) % 256));

		gdb_stub::Packet_decoder decoder;
		decoder.push(input);
		const auto packet = decoder.pop_packet();
		ASSERT_FALSE(packet.has_value());
		EXPECT_EQ(packet.error(), gdb_stub::Packet_decoder::Error::Buffer_overflow);
	}
}

TEST(PacketDecode, MultiplePackets)
{
	{
		const std::string_view input = "$Hello#f4$World#08";
		gdb_stub::Packet_decoder decoder;
		decoder.push(input);

		auto packet1 = decoder.pop_packet();
		{
			ASSERT_TRUE(packet1.has_value());
			EXPECT_EQ(packet1.value(), "Hello");
		}

		auto packet2 = decoder.pop_packet();
		{
			ASSERT_TRUE(packet2.has_value());
			EXPECT_EQ(packet2.value(), "World");
		}

		ASSERT_FALSE(decoder.new_packet_available());
	}

	{
		const std::string_view input = "$Hello#f0$World#08";
		gdb_stub::Packet_decoder decoder;
		decoder.push(input);

		auto packet1 = decoder.pop_packet();
		{
			ASSERT_FALSE(packet1.has_value());
			EXPECT_EQ(packet1.error(), gdb_stub::Packet_decoder::Error::Bad_checksum);
		}

		auto packet2 = decoder.pop_packet();
		{
			ASSERT_TRUE(packet2.has_value());
			ASSERT_TRUE(packet2.has_value());
			EXPECT_EQ(packet2.value(), "World");
		}

		ASSERT_FALSE(decoder.new_packet_available());
	}
}

TEST(PacketDecode, RealPackets)
{
	{
		const std::string_view input = "$vMustReplyEmpty#3a";
		gdb_stub::Packet_decoder decoder;
		decoder.push(input);
		const auto packet = decoder.pop_packet();

		ASSERT_TRUE(packet.has_value());
		EXPECT_EQ(packet.value(), "vMustReplyEmpty");
	}

	{
		const std::string_view input = "$qSupported:multiprocess+;swbreak+;hwbreak+;qRelocInsn+;fork-events+;"
									   "vfork-events+;exec-events+;vContSupported+;QThreadEvents+;"
									   "QThreadOptions+;no-resumed+;memory-tagging+;error-message+#89";
		gdb_stub::Packet_decoder decoder;
		decoder.push(input);
		const auto packet = decoder.pop_packet();

		ASSERT_TRUE(packet.has_value());
		EXPECT_EQ(
			packet.value(),
			"qSupported:multiprocess+;swbreak+;hwbreak+;qRelocInsn+;fork-events+;"
			"vfork-events+;exec-events+;vContSupported+;QThreadEvents+;"
			"QThreadOptions+;no-resumed+;memory-tagging+;error-message+"
		);
	}
}

TEST(PacketDecode, Acknowledgement)
{
	{
		const std::string_view input = "+";
		gdb_stub::Packet_decoder decoder;
		decoder.push(input);
		const auto packet = decoder.pop_packet();

		ASSERT_TRUE(packet.has_value());
		EXPECT_EQ(packet.value(), "+");
	}

	{
		const std::string_view input = "-";
		gdb_stub::Packet_decoder decoder;
		decoder.push(input);
		const auto packet = decoder.pop_packet();

		ASSERT_TRUE(packet.has_value());
		EXPECT_EQ(packet.value(), "-");
	}

	{
		const std::string_view input = "$vMustReplyEmpty#3a-";
		gdb_stub::Packet_decoder decoder;
		decoder.push(input);

		auto packet = decoder.pop_packet();

		ASSERT_TRUE(packet.has_value());
		EXPECT_EQ(packet.value(), "vMustReplyEmpty");

		packet = decoder.pop_packet();

		ASSERT_TRUE(packet.has_value());
		EXPECT_EQ(packet.value(), "-");
	}

	{
		const std::string_view input = "$vMustReplyEmpty#-";
		gdb_stub::Packet_decoder decoder;
		decoder.push(input);

		auto packet = decoder.pop_packet();

		ASSERT_FALSE(packet.has_value());
		EXPECT_EQ(packet.error(), gdb_stub::Packet_decoder::Error::Bad_packet);

		packet = decoder.pop_packet();

		ASSERT_TRUE(packet.has_value());
		EXPECT_EQ(packet.value(), "-");
	}
}

TEST(PacketDecode, RemoveEscape)
{
	using namespace gdb_stub;

	auto expand_body = gdb_stub::algo::remove_escape;

	{
		const std::string input = {'H', 'e', 'l', 'l', 'o'};
		const auto result = expand_body(input);
		ASSERT_TRUE(result.has_value());
		EXPECT_EQ(result.value(), input);
	}

	{
		const std::string input = {'}', 'H' ^ 0x20, 'e', 'l', 'l', 'o'};
		const auto result = expand_body(input);
		ASSERT_TRUE(result.has_value());
		EXPECT_EQ(result.value(), "Hello");
	}

	{
		const std::string input = {'}', 'H' ^ 0x20, 'e', 'l', 'l', 'o', '}'};
		const auto result = expand_body(input);
		ASSERT_FALSE(result.has_value());
	}

	{
		const std::string input = {'}', '}', 'H' ^ 0x20, 'e', 'l', 'l', 'o'};
		const auto result = expand_body(input);
		ASSERT_FALSE(result.has_value());
	}

	{
		const std::string input = {'}', '}' ^ 0x20};
		const auto result = expand_body(input);
		ASSERT_TRUE(result.has_value());
		EXPECT_EQ(result.value(), "}");
	}
}

TEST(PacketEncode, Encoding)
{
	using namespace gdb_stub;

	auto encode = [](std::string_view str)
	{
		return Packet_encoder::encode(str);
	};

	auto encode_repeat = [](int n)
	{
		return Packet_encoder::encode(std::string(n, '0'));
	};

	EXPECT_EQ(encode_repeat(1), "$0#30");
	EXPECT_EQ(encode_repeat(2), "$00#60");
	EXPECT_EQ(encode_repeat(3), "$000#90");
	EXPECT_EQ(encode_repeat(4), "$0* #7a");
	EXPECT_EQ(encode_repeat(5), "$0*!#7b");
	EXPECT_EQ(encode_repeat(6), "$0*\"#7c");
	EXPECT_EQ(encode_repeat(7), "$0*\"0#ac");
	EXPECT_EQ(encode_repeat(8), "$0*\"00#dc");
	EXPECT_EQ(encode_repeat(9), "$0*%#7f");

	EXPECT_EQ(encode_repeat(98), "$0*~#d8");
	EXPECT_EQ(encode_repeat(99), "$0*~0#08");
	EXPECT_EQ(encode_repeat(100), "$0*~00#38");
	EXPECT_EQ(encode_repeat(101), "$0*~000#68");
	EXPECT_EQ(encode_repeat(102), "$0*~0* #52");

	EXPECT_EQ(encode("My favourite    number is 00001234"), "$My favourite * number is 0* 1234#0e");
}