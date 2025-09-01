#include <gtest/gtest.h>

int main(int argc, char** argv)
{
	// redirect std::cout to std::cerr
	std::cout.rdbuf(std::cerr.rdbuf());

	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}