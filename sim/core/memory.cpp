#include "core/memory.hpp"

namespace core
{
	static bool is_aligned(u32 address, Load_store_module::Funct funct)
	{
		switch (funct)
		{
		case Load_store_module::Funct::Load_byte:
		case Load_store_module::Funct::Store_byte:
		case Load_store_module::Funct::Load_byte_unsigned:
			return true;
		case Load_store_module::Funct::Load_halfword:
		case Load_store_module::Funct::Store_halfword:
		case Load_store_module::Funct::Load_halfword_unsigned:
			return (address & 0x1) == 0;
		case Load_store_module::Funct::Load_word:
		case Load_store_module::Funct::Store_word:
			return (address & 0x3) == 0;
		default:
			return false;
		}
	}

	std::expected<u32, Trap> Load_store_module::operator()(
		Memory_interface& interface,
		Opcode opcode,
		Funct funct,
		u32 address,
		u32 store_value
	)
	{
		if (opcode == Opcode::None) return 0;

		const auto address_aligned = address & 0xfffffffc;

		const bool aligned = is_aligned(address, funct);
		const auto byte_offset = address & 0x3;

		if (opcode == Opcode::Load)
		{
			if (!aligned) [[unlikely]]
				return std::unexpected(Trap::Load_address_misaligned);

			const auto read_result = interface.read(address_aligned);

			if (!read_result) [[unlikely]]
				return std::unexpected(Trap::Load_access_fault);

			const u32 value = read_result.value();

			switch (funct)
			{
			case Funct::Load_byte:
				return static_cast<i32>(static_cast<i8>(value >> (byte_offset * 8)));
			case Funct::Load_halfword:
				return static_cast<i32>(static_cast<i16>(value >> (byte_offset * 8)));
			case Funct::Load_word:
				return value;
			case Funct::Load_byte_unsigned:
				return static_cast<u32>(static_cast<u8>(value >> (byte_offset * 8)));
			case Funct::Load_halfword_unsigned:
				return static_cast<u32>(static_cast<u16>(value >> (byte_offset * 8)));
			default:
				throw std::logic_error("Invalid funct for load operation");
			}
		}

		if (opcode == Opcode::Store)
		{
			if (!aligned) return std::unexpected(Trap::Store_address_misaligned);

			Bitset<4> mask;

			switch (funct)
			{
			case Funct::Store_byte:
				mask = 0b0001 << byte_offset;
				break;
			case Funct::Store_halfword:
				mask = 0b0011 << byte_offset;
				break;
			case Funct::Store_word:
				mask = 0b1111;
				break;
			default:
				throw std::logic_error("Invalid funct for store operation");
			}

			const auto write_result
				= interface.write(address_aligned, store_value << (byte_offset * 8), mask);

			if (!write_result) [[unlikely]]
			{
				switch (write_result.error())
				{
				case Memory_interface::Error::Out_of_range:
				case Memory_interface::Error::Access_fault:
				case core::Memory_interface::Error::Device_error:
					return std::unexpected(Trap::Store_access_fault);
				case core::Memory_interface::Error::Unaligned:
					return std::unexpected(Trap::Store_address_misaligned);
				default:
					throw std::logic_error("Invalid memory interface error");
				}
			}
			return 0;
		}

		throw std::logic_error("Invalid opcode");
	}

	size_t get_size(Load_store_module::Funct funct)
	{
		switch (funct)
		{
		case Load_store_module::Funct::Load_byte:
		case Load_store_module::Funct::Load_byte_unsigned:
		case Load_store_module::Funct::Store_byte:
			return 1;
		case Load_store_module::Funct::Load_halfword:
		case Load_store_module::Funct::Load_halfword_unsigned:
		case Load_store_module::Funct::Store_halfword:
			return 2;
		case Load_store_module::Funct::Load_word:
		case Load_store_module::Funct::Store_word:
			return 4;
		default:
			throw std::logic_error("Invalid funct");
		}
	}

	std::expected<u32, Trap> Inst_fetch_module::operator()(Memory_interface& interface, u32 pc)
	{
		if (pc & 0x3) [[unlikely]]
			return std::unexpected(Trap::Inst_address_misaligned);

		const auto cache_idx = (pc >> 12) % cache_num;

		if (!cache[cache_idx].valid || cache[cache_idx].address != (pc & 0xFFFFF000)) [[unlikely]]
		{
			const auto read_result = interface.read_page(pc & 0xfffff000, cache[cache_idx].data);

			if (!read_result) [[unlikely]]
			{
				cache[cache_idx].valid = false;

				switch (read_result.error())
				{
				case Memory_interface::Error::Out_of_range:
				case Memory_interface::Error::Access_fault:
					return std::unexpected(Trap::Inst_access_fault);
				case Memory_interface::Error::Device_error:
					return std::unexpected(Trap::Illegal_instruction);
				case Memory_interface::Error::Unaligned:
					return std::unexpected(Trap::Inst_address_misaligned);
				default:
					throw std::logic_error("Invalid memory interface error");
				}
			}

			cache[cache_idx].valid = true;
			cache[cache_idx].address = pc & 0xfffff000;
		}

		return cache[cache_idx].data[(pc & 0xfff) >> 2];
	}

	void Inst_fetch_module::fencei()
	{
		for (auto& entry : cache) entry.valid = false;
	}
}