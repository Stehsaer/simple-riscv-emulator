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
		union
		{
			struct
			{
				u32 counter_lo;
				u32 counter_hi;
			};

			u64 counter = 0;
		};

		union
		{
			struct
			{
				u32 cmp_lo;
				u32 cmp_hi;
			};

			u64 cmp = 0;
		};

		std::optional<u32> counter_templow;
		std::optional<u32> cmp_templow;

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