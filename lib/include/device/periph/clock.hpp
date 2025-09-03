#pragma once

#include "base.hpp"
#include "common/bitset.hpp"
#include "core/csr.hpp"
#include "core/memory.hpp"

namespace device::periph
{
	/**
	 * @brief Clock Peripheral
	 * @note Use @p tick() to increment the counter.
	 */
	class Clock : public Periph_base
	{
		struct Reg
		{
			u32 low;
			u32 high;

			u64 get_64() const { return (static_cast<u64>(high) << 32) | low; }
			void set_64(u64 value)
			{
				low = static_cast<u32>(value & 0xFFFFFFFF);
				high = static_cast<u32>((value >> 32) & 0xFFFFFFFF);
			}
		};

		Reg timer;
		Reg comp;

		std::optional<u32> counter_templow;
		std::optional<u32> comp_templow;

	  public:

		std::expected<u32, Error> read(u64 address) override;
		std::expected<void, Error> write(u64 address, u32 data, core::Bitset<4> mask) override;

		/**
		 * @brief Do a clock tick. Increments the internal counter by 1.
		 *
		 * @param mip `MIP` register of the CPU. Used to signal M-mode timer interrupt.
		 */
		void tick(core::csr::Mip& mip);
	};
}