#include <gtest/gtest.h>
#include <print>
#include <ranges>

#include "device/block-memory.hpp"

TEST(BlockMemory, NormalWrite)
{
	device::Block_memory mem(16 * 1024 * 1024, device::Fill_policy::Random);

	ASSERT_TRUE(mem.write(0, 0x12345678, 0b1111).has_value());

	auto val = mem.read(0);
	ASSERT_TRUE(val.has_value());
	EXPECT_EQ(*val, 0x12345678);

	ASSERT_TRUE(mem.write(4, 0x12345678, 0b1010).has_value());
	ASSERT_TRUE(mem.write(4, 0xdeadbeef, 0b0101).has_value());
	val = mem.read(4);
	ASSERT_TRUE(val.has_value());
	EXPECT_EQ(*val, 0x12ad56ef);

	ASSERT_TRUE(mem.write(4, 0xFFFFFFFF, 0b0000).has_value());
	val = mem.read(4);
	ASSERT_TRUE(val.has_value());
	EXPECT_EQ(*val, 0x12ad56ef);

	EXPECT_EQ(mem.size(), 16 * 1024 * 1024);

	device::Block_memory small_mem(3, device::Fill_policy::Random);
	{
		auto write_result = small_mem.write(0, 0x12345678, 0b1111);
		ASSERT_FALSE(write_result.has_value());
		EXPECT_EQ(write_result.error(), core::Memory_interface::Error::Out_of_range);
	}
	{
		auto write_result = small_mem.write(0, 0x12345678, 0b0111);
		EXPECT_TRUE(write_result.has_value());
	}

	device::Block_memory locked_mem(16 * 1024 * 1024, device::Fill_policy::Random);
	locked_mem.lock();
	{
		auto write_result = locked_mem.write(0, 0x12345678, 0b1111);
		ASSERT_FALSE(write_result.has_value());
		EXPECT_EQ(write_result.error(), core::Memory_interface::Error::Access_fault);
	}
}

TEST(BlockMemory, FillValidation)
{
	device::Block_memory mem(16 * 1024 * 1024, device::Fill_policy::Cdcdcdcd);

	std::vector<u32> test_data(16 * 1024 * 1024 / sizeof(u32));
	EXPECT_TRUE(mem.fill_data(test_data.data(), test_data.size() * sizeof(u32)));

	std::array<u32, 1024> page_data;
	ASSERT_TRUE(mem.read_page(0, page_data).has_value());

	for (const auto [idx, src] : test_data | std::views::chunk(1024) | std::views::enumerate)
	{
		ASSERT_TRUE(mem.read_page(idx * 4096, page_data).has_value());

		auto is_eq = std::ranges::equal(page_data, src);
		EXPECT_TRUE(is_eq);

		if (!is_eq)
		{
			std::println("Mismatch at page {}", idx);
			break;
		}
	}

	device::Block_memory small_mem(16, device::Fill_policy::Random);
	ASSERT_FALSE(small_mem.read_page(0, page_data).has_value());
}

TEST(BlockMemory, OutOfBound)
{
	device::Block_memory mem(16 * 1024 * 1024, device::Fill_policy::Random);

	std::array<u32, 1024> page_data;
	EXPECT_TRUE(mem.read_page(0, page_data).has_value());
	EXPECT_TRUE(mem.read_page(16 * 1024 * 1024 - 4096, page_data).has_value());
	EXPECT_FALSE(mem.read_page(16 * 1024 * 1024, page_data).has_value());

	auto read_result = mem.read(16 * 1024 * 1024);
	{
		ASSERT_FALSE(read_result.has_value());
		EXPECT_EQ(read_result.error(), core::Memory_interface::Error::Out_of_range);
	}

	auto write_result = mem.write(16 * 1024 * 1024, 0x12345678, 0b1111);
	{
		ASSERT_FALSE(write_result.has_value());
		EXPECT_EQ(write_result.error(), core::Memory_interface::Error::Out_of_range);
	}

	read_result = mem.read(16 * 1024 * 1024 - 4);
	EXPECT_TRUE(read_result.has_value());

	write_result = mem.write(16 * 1024 * 1024 - 4, 0x12345678, 0b1111);
	EXPECT_TRUE(write_result.has_value());

	std::vector<u8> test_data(32 * 1024 * 1024);
	EXPECT_FALSE(mem.fill_data(test_data.data(), test_data.size()));
	EXPECT_TRUE(mem.fill_data(test_data.data(), 16 * 1024 * 1024));
}

TEST(BlockMemory, Unaligned)
{
	device::Block_memory mem(16 * 1024 * 1024, device::Fill_policy::Random);

	auto read_result = mem.read(1);
	{
		ASSERT_FALSE(read_result.has_value());
		EXPECT_EQ(read_result.error(), core::Memory_interface::Error::Unaligned);
	}

	auto write_result = mem.write(1, 0x12345678, 0b1111);
	{
		ASSERT_FALSE(write_result.has_value());
		EXPECT_EQ(write_result.error(), core::Memory_interface::Error::Unaligned);
	}
}