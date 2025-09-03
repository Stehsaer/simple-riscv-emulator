#pragma once

#include <array>

#include "common/bitset.hpp"
#include "common/type.hpp"

namespace core
{
	/**
	 * @brief Implements the register file module
	 *
	 */
	struct Register_file_module
	{
		std::array<u32, 32> registers = {0};

		/**
		 * @brief Get register value
		 *
		 * @param index Register index
		 * @return Value of the register
		 */
		u32 get_register(Bitset<5> index) const noexcept { return registers[static_cast<size_t>(index)]; }

		/**
		 * @brief Set register value
		 *
		 * @param index Register index
		 * @param value Write value
		 */
		void set_register(Bitset<5> index, u32 value) noexcept
		{
			if (index != 0u) registers[static_cast<size_t>(index)] = value;
		}

		/**
		 * @brief Get register readable name
		 *
		 * @param index Register index
		 * @return Register name
		 */
		static const char* get_name(Bitset<5> index) noexcept;
	};
}