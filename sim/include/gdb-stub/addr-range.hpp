#pragma once

#include "common/type.hpp"
#include <algorithm>
#include <array>
#include <compare>
#include <concepts>
#include <ranges>

namespace gdb_stub
{
	/**
	 * @brief Represents an address range
	 * @details Designed to be used as keys in ordered containers (e.g. `std::map`).
	 */
	struct Address_range
	{
		u32 start;
		u32 size;

		std::partial_ordering operator<=>(const Address_range& other) const
		{
			const u32 end = start + size;
			const u32 other_end = other.start + other.size;

			if (start <= other.start && other_end <= end) return std::partial_ordering::equivalent;
			if (other.start <= start && end <= other_end) return std::partial_ordering::equivalent;
			if (other_end < start) return std::partial_ordering::greater;
			if (end < other.start) return std::partial_ordering::less;

			return std::partial_ordering::unordered;
		}
	};
}