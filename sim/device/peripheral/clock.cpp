#include "device/periph/clock.hpp"
#include "core/print.hpp"

#include <iostream>
#include <print>

namespace device::periph
{
	std::expected<void, core::Memory_interface::Error> Clock::write(
		u64 address,
		u32 data,
		core::Bitset<4> mask
	)
	{
		if ((address & 0x3) != 0) [[unlikely]]
		{
			wprintln("Clock.write: address unaligned: 0x{:08x}", address);
			return std::unexpected(Error::Unaligned);
		}

		const auto new_address = address / 4;

		switch (new_address)
		{
		case 0:
			counter_lo = mask.expand_byte_mask().choose_bits(data, counter_lo);
			counter_templow = std::nullopt;
			break;

		case 1:
			counter_hi = mask.expand_byte_mask().choose_bits(data, counter_hi);
			counter_templow = std::nullopt;
			break;

		case 2:
			cmp_lo = mask.expand_byte_mask().choose_bits(data, cmp_lo);
			cmp_templow = std::nullopt;
			break;

		case 3:
			cmp_hi = mask.expand_byte_mask().choose_bits(data, cmp_hi);
			cmp_templow = std::nullopt;
			break;

		default:
			wprintln("Clock.write: address out of range: 0x{:08x}", address);
			return std::unexpected(Error::Access_fault);
		}

		return {};
	}

	std::expected<u32, core::Memory_interface::Error> Clock::read(u64 address)
	{
		if ((address & 0x3) != 0) [[unlikely]]
		{
			wprintln("Clock.read: address unaligned: 0x{:08x}", address);
			return std::unexpected(Error::Unaligned);
		}

		const auto new_address = address / 4;

		switch (new_address)
		{
		case 0:
			if (counter_templow)
			{
				const auto result = *counter_templow;
				counter_templow = std::nullopt;
				return result;
			}
			else
				return counter_lo;

		case 1:
			counter_templow = counter_lo;
			return counter_hi;

		case 2:
			if (cmp_templow)
			{
				const auto result = *cmp_templow;
				cmp_templow = std::nullopt;
				return result;
			}
			else
				return cmp_lo;

		case 3:
			cmp_templow = cmp_lo;
			return cmp_hi;

		default:
			wprintln("Clock.read: address out of range: 0x{:08x}", address);
			return std::unexpected(Error::Access_fault);
		}
	}

	void Clock::tick(core::csr::Mip& mip)
	{
		++counter;
		if (counter > cmp) mip.value |= (1 << 7);  // mtimer interrupt
	}
}