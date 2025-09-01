#pragma once

#include "base.hpp"
#include "common/bitset.hpp"

#include <iostream>

namespace device::periph
{
	/**
	 * @brief UART Peripheral class
	 * 
	 */
	class Uart : public Periph_base
	{
		u32 config_reg;
		std::istream* input_stream = &std::cin;

	  public:

		std::expected<u32, Error> read(u64 address) override;
		std::expected<void, Error> write(u64 address, u32 data, core::Bitset<4> mask) override;

		/**
		 * @brief Set input stream for UART peripheral. Used when not inputting data from `std::cin`
		 * 
		 * @param input_stream Input stream reference
		 */
		void set_input_stream(std::istream& input_stream);
	};
}