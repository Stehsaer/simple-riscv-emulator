#include "device/periph/uart.hpp"
#include "core/print.hpp"
#include <iostream>

namespace device::periph
{
	std::expected<void, core::Memory_interface::Error> Uart::write(
		u64 address,
		u32 data,
		core::Bitset<4> mask
	)
	{
		if ((address & 0x3) != 0) return std::unexpected(Error::Unaligned);

		const auto new_address = address / 4;

		switch (new_address)
		{
		case 0:  // TX
		{
			if (mask & 0x1) std::cerr << static_cast<char>(data & 0xff);
			break;
		}
		case 2:  // CFG
		{
			config_reg = mask.expand_byte_mask().choose_bits(data, config_reg);
			iprintln("Uart.write: CFG <==[write]== 0x{:08x}", config_reg);
			break;
		}
		default:
			wprintln("Uart.write: address out of range: 0x{:08x}", address);
			return std::unexpected(Error::Access_fault);
		}

		return {};
	}

	std::expected<u32, core::Memory_interface::Error> Uart::read(u64 address)
	{
		if ((address & 0x3) != 0) return std::unexpected(Error::Unaligned);

		const auto new_address = address / 4;

		switch (new_address)
		{
		case 1:  // RX
			return static_cast<u32>(input_stream->get());
		case 2:  // CFG
			return config_reg;
		case 3:  // STA
			return 0b10 | (!input_stream->eof() && random() % 2 == 1 ? 0b01 : 0b00);
		default:
			wprintln("Uart.read: address out of range: 0x{:08x}", address);
			return std::unexpected(Error::Access_fault);
		}
	}

	void Uart::set_input_stream(std::istream& input_stream)
	{
		this->input_stream = &input_stream;
	}
}