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
			timer.low = mask.expand_byte_mask().choose_bits(data, timer.low);
			counter_templow = std::nullopt;
			break;

		case 1:
			timer.high = mask.expand_byte_mask().choose_bits(data, timer.high);
			counter_templow = std::nullopt;
			break;

		case 2:
			comp.low = mask.expand_byte_mask().choose_bits(data, comp.low);
			comp_templow = std::nullopt;
			break;

		case 3:
			comp.high = mask.expand_byte_mask().choose_bits(data, comp.high);
			comp_templow = std::nullopt;
			break;

		[[unlikely]] default:
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
				return timer.low;

		case 1:
			counter_templow = timer.low;
			return timer.high;

		case 2:
			if (comp_templow)
			{
				const auto result = *comp_templow;
				comp_templow = std::nullopt;
				return result;
			}
			else
				return comp.low;

		case 3:
			comp_templow = comp.low;
			return comp.high;

		default:
			wprintln("Clock.read: address out of range: 0x{:08x}", address);
			return std::unexpected(Error::Access_fault);
		}
	}

	void Clock::tick(core::csr::Mip& mip)
	{
		timer.set_64(timer.get_64() + 1);
		if (timer.get_64() > comp.get_64()) mip.value |= (1 << 7);  // mtimer interrupt
	}
}