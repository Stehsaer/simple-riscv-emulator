#include "core/csr.hpp"

namespace core
{
	void csr::Mstatus::force_legal()
	{
		sie = false;
		spie = false;
		ube = false;
		spp = false;
		vs = 0;
		fs = 0;
		xs1 = false;
		xs2 = false;
		mprv = false;
		sum = false;
		mxr = false;
		tvm = false;
		tw = false;
		tsr = false;
		sd = false;
		sbe = false;
		mbe = false;

		__padding0_0 = 0;
		__padding2_2 = 0;
		__padding4_4 = 0;
		__padding30_23 = 0;
		__paddingh3_0 = 0;
		__paddingh31_6 = 0;
	}

	std::expected<u32, CSR_module::Error> CSR_module::operator()(const CSR_access_info& access_info) noexcept
	{
		const auto [write_value, write_mask] = [access_info] -> std::tuple<u32, u32>
		{
			switch (access_info.write_mode)
			{
			case CSR_write_mode::None:
				return {0, 0};
			case CSR_write_mode::Overwrite:
				return {access_info.write_value, 0xffffffff};
			case CSR_write_mode::Clear:
				return {0, access_info.write_value};
			case CSR_write_mode::Set:
				return {0xffffffff, access_info.write_value};
			}

			std::unreachable();
		}();

		const bool writable = access_info.address.slice<11, 10>() != 0b11;

		if (!writable && write_mask != 0) return std::unexpected(Error::Write_readonly);

		auto write = [](void* csr, u32 mask, u32 value) -> u32
		{
			u32& dst = *reinterpret_cast<u32*>(csr);
			const u32 prev = dst;
			dst = (dst & ~mask) | (value & mask);
			return prev;
		};

		// No csr access
		if (!access_info.read && access_info.write_mode == CSR_write_mode::None) return 0;

#define CSR_WRITE_CASE(class_name, member)                                                                   \
	case csr::class_name::address:                                                                           \
		return write(&this->member, write_mask, write_value);

#define WIDE_CSR_WRITE_CASE(class_name, member)                                                              \
	case csr::class_name::low_address:                                                                       \
		return write((u32*)&this->member, write_mask, write_value);                                          \
	case csr::class_name::high_address:                                                                      \
		return write((u32*)&this->member + 1, write_mask, write_value);

		switch (static_cast<u16>(access_info.address))
		{
			CSR_WRITE_CASE(Mscratch, mscratch)
			CSR_WRITE_CASE(Misa, misa)
			CSR_WRITE_CASE(Mvendorid, mvendorid)
			CSR_WRITE_CASE(Marchid, marchid)
			CSR_WRITE_CASE(Mimpid, mimpid)
			CSR_WRITE_CASE(Mhartid, mhartid)
			CSR_WRITE_CASE(Mconfigptr, mconfigptr)
			WIDE_CSR_WRITE_CASE(Mcycles, mcycles)
			WIDE_CSR_WRITE_CASE(Minstret, minstret)
			CSR_WRITE_CASE(Mepc, mepc)
			CSR_WRITE_CASE(Mcause, mcause)
			CSR_WRITE_CASE(Mtval, mtval)
			CSR_WRITE_CASE(Mip, mip)
			CSR_WRITE_CASE(Mie, mie)
			CSR_WRITE_CASE(Mtvec, mtvec)

		case csr::Mstatus::low_address:
		{
			write((u32*)&this->mstatus, write_mask, write_value);
			mstatus.force_legal();
			return *(u32*)&this->mstatus;
		}
		case csr::Mstatus::high_address:
		{
			write((u32*)&this->mstatus + 1, write_mask, write_value);
			mstatus.force_legal();
			return *((u32*)&this->mstatus + 1);
		}
		default:
			return std::unexpected(Error::Not_exists);
		}
	}

	void CSR_module::tick()
	{
		mcycles.value++;
		minstret.value++;
	}

}