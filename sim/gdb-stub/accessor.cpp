#include "gdb-stub/accessor.hpp"

namespace gdb_stub
{
	std::optional<u8> Memory_accessor::read(u32 address) const noexcept
	{
		if (const auto val = memory.read(address & ~0x3); val.has_value())
			return std::bit_cast<std::array<u8, 4>>(val.value())[address & 0x3];
		else
			return std::nullopt;
	}

	bool Memory_accessor::write(u32 address, u8 value) const noexcept
	{
		const u32 aligned_addr = address & ~0x3;
		const core::Bitset<4> byte_enable = 1 << (address & 0x3);
		const u32 write_data = static_cast<u32>(value) << ((address & 0x3) * 8);

		return (bool)memory.write(aligned_addr, write_data, byte_enable);
	}

	std::optional<u32> CPU_register_accessor::read(u32 reg_num) const noexcept
	{
		// GPR
		if (reg_num < 32) return cpu.registers.get_register(reg_num);

		// PC
		if (reg_num == 32) return cpu.pc;

		// CSR
		if (reg_num >= 128)
		{
			const auto csr_addr = reg_num - 128;
			const auto result = cpu.csr(
				core::CSR_access_info{
					.write_mode = core::CSR_write_mode::None,
					.address = csr_addr,
					.read = true
				}
			);

			return result.has_value() ? std::optional<u32>(result.value()) : std::nullopt;
		}

		return std::nullopt;
	}

	bool CPU_register_accessor::write(u32 reg_num, u32 value) const noexcept
	{
		// GPR
		if (reg_num < 32)
		{
			cpu.registers.set_register(reg_num, value);
			return true;
		}

		// PC
		if (reg_num == 32)
		{
			cpu.pc = value;
			return true;
		}

		// CSR
		if (reg_num >= 128)
		{
			const u32 csr_address = reg_num - 128;

			const core::CSR_access_info info{
				.write_mode = core::CSR_write_mode::Overwrite,
				.address = csr_address,
				.write_value = value,
				.read = true
			};

			const auto result = cpu.csr(info);

			return result.has_value();
		}

		return false;
	}
}