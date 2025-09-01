#pragma once

#include "common/bitset.hpp"
#include "common/type.hpp"
#include "trap.hpp"

#include <expected>
#include <flat_map>
#include <string>
#include <string_view>

#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index_container.hpp>

namespace core
{

	template <u16 Address>
	struct CSR_base
	{
		static constexpr u16 address = Address;
	};

	template <u16 Low, u16 High>
	struct CSR_64bit_empty
	{
		static constexpr u16 low_address = Low;
		static constexpr u16 high_address = High;

		u64 value = 0;
	};

	template <u16 Address>
	struct Readonly_csr
	{
		static constexpr u16 address = Address;
		u32 value = 0;

		Readonly_csr(u32 value) :
			value(value)
		{}
	};

	enum class Priviledge : u8
	{
		User = 0,
		Supervisor = 1,
		Machine = 3
	};

	/**
	 * @brief CSR Metadata
	 * @details Describes a CSR, its address, name and type. Mainly used for GDB integration.
	 */
	struct CSR_metadata
	{
		enum class Type
		{
			Int,       // Integer, can represent any data
			Data_ptr,  // A pointer to some data
			Code_ptr   // A pointer to code (e.g. mepc)
		};

		struct Key_address
		{};

		struct Key_name
		{};

		u16 address;            // CSR Address
		std::string_view name;  // Name of CSR
		Type type;              // Type of the CSR

		auto operator<=>(const CSR_metadata& other) const { return address <=> other.address; }
	};

	namespace csr
	{
		struct Mscratch : public CSR_base<0x340>
		{
			static constexpr CSR_metadata metadata
				= {.address = 0x340, .name = "mscratch", .type = CSR_metadata::Type::Int};

			u32 value = 0;
		};

		struct Mepc : public CSR_base<0x341>
		{
			static constexpr CSR_metadata metadata
				= {.address = 0x341, .name = "mepc", .type = CSR_metadata::Type::Code_ptr};

			u32 value = 0;
		};

		struct Mcause : public CSR_base<0x342>
		{
			static constexpr CSR_metadata metadata
				= {.address = 0x342, .name = "mcause", .type = CSR_metadata::Type::Int};

			u32 exception_code : 31 = 0;
			bool interrupt     : 1 = false;
		};

		struct Mtval : public CSR_base<0x343>
		{
			static constexpr CSR_metadata metadata
				= {.address = 0x343, .name = "mtval", .type = CSR_metadata::Type::Int};

			u32 value = 0;
		};

		struct Mip : public CSR_base<0x344>
		{
			static constexpr CSR_metadata metadata
				= {.address = 0x344, .name = "mip", .type = CSR_metadata::Type::Int};

			u32 value = 0;
		};

		struct Mie : public CSR_base<0x304>
		{
			static constexpr CSR_metadata metadata
				= {.address = 0x304, .name = "mie", .type = CSR_metadata::Type::Int};

			u32 value = 0;
		};

		struct Mtvec : public CSR_base<0x305>
		{
			static constexpr CSR_metadata metadata
				= {.address = 0x305, .name = "mtvec", .type = CSR_metadata::Type::Int};

			enum class Mode : u8
			{
				Direct = 0,
				Vectored = 1
			};

			Mode mode        : 2 = Mode::Direct;
			u32 base_upper30 : 30 = 0;  // Base address of the trap vector table
		};

		struct Misa : public CSR_base<0x301>
		{
			static constexpr CSR_metadata metadata
				= {.address = 0x301, .name = "misa", .type = CSR_metadata::Type::Int};

			enum class Base_width : uint8_t
			{
				Unknown = 0,
				Rv32 = 1,
				Rv64 = 2,
				Rv128 = 3
			};

			bool ext_a   : 1 = false;
			bool ext_b   : 1 = false;
			bool ext_c   : 1 = false;
			bool ext_d   : 1 = false;
			bool ext_e   : 1 = false;
			bool ext_f   : 1 = false;
			bool __ext_g : 1 = false;
			bool ext_h   : 1 = false;
			bool ext_i   : 1 = false;
			bool __ext_j : 1 = false;
			bool __ext_k : 1 = false;
			bool __ext_l : 1 = false;
			bool ext_m   : 1 = false;
			bool ext_n   : 1 = false;
			bool __ext_o : 1 = false;
			bool ext_p   : 1 = false;
			bool ext_q   : 1 = false;
			bool __ext_r : 1 = false;
			bool ext_s   : 1 = false;
			bool __ext_t : 1 = false;
			bool ext_u   : 1 = false;
			bool ext_v   : 1 = false;
			bool __ext_w : 1 = false;
			bool ext_x   : 1 = false;
			bool __ext_y : 1 = false;
			bool __ext_z : 1 = false;

			int __padding29_26 : 4 = 0;

			Base_width base : 2 = Base_width::Unknown;
		};

		struct Mstatus
		{
			static constexpr CSR_metadata metadata_low
				= {.address = 0x300, .name = "mstatus", .type = CSR_metadata::Type::Int};
			static constexpr CSR_metadata metadata_high
				= {.address = 0x310, .name = "mstatush", .type = CSR_metadata::Type::Int};

			static constexpr u16 low_address = 0x300;
			static constexpr u16 high_address = 0x310;

			/* [0:0] */ bool __padding0_0 : 1 = false;

			/* [1:1] */ bool sie : 1 = false;  // (Not Implemented) Supervisor Interrupt Enable

			/* [2:2] */ bool __padding2_2 : 1 = false;

			/* [3:3] */ bool mie : 1 = false;  // Machine Interrupt Enable

			/* [4:4] */ bool __padding4_4 : 1 = false;

			/* [5:5] */ bool spie : 1 = false;  // (Not Implemented) Supervisor Previous Interrupt Enable
			/* [6:6] */ bool ube  : 1 = false;  // (Not Implemented) User Big Endian
			/* [7:7] */ bool mpie : 1 = false;  // Machine Previous Interrupt Enable
			/* [8:8] */ bool spp  : 1 = false;  // (Not Implemented) Supervisor Previous Privilege
			/* [10:9] */ u8 vs    : 2 = 0;      // (Not implemented) Vector Extension Status

			/* [12:11] */ Priviledge mpp : 2 = Priviledge::Machine;  // Machine Previous Privilege

			/* [14:13] */ u8 fs     : 2 = 0;  // (Not implemented) Floating Point Status
			/* [15:15] */ bool xs1  : 1 = false;
			/* [16:16] */ bool xs2  : 1 = false;  // (Not implemented) eXtension Status
			/* [17:17] */ bool mprv : 1 = false;  // (Not implemented) Memory Access Privilege Mode
			/* [18:18] */ bool sum  : 1 = false;  // (Not implemented) Supervisor User Memory Access
			/* [19:19] */ bool mxr  : 1 = false;  // (Not implemented) Make Execute Readable
			/* [20:20] */ bool tvm  : 1 = false;  // (Not implemented) Trap Virtual Memory
			/* [21:21] */ bool tw   : 1 = false;  // (Not implemented) Timeout Wait
			/* [22:22] */ bool tsr  : 1 = false;  // (Not implemented) Trap SRET

			/* [30:23] */ int __padding30_23 : 8 = false;

			/* [31:31] */ bool sd : 1 = false;  // (Not implemented) State Dirty

			/* [3:0] */ int __paddingh3_0 : 4 = 0;

			/* [4:4] */ bool sbe : 1 = false;  // (Not implemented) Supervisor Big Endian
			/* [5:5] */ bool mbe : 1 = false;  // (Not implemented) Machine Big Endian

			/* [31:6] */ int __paddingh31_6 : 26 = 0;

			/**
			 * @brief Make all fields legal (for WARL)
			 *
			 */
			void force_legal();
		};

		struct Mcycles : public CSR_64bit_empty<0xB00, 0xB80>
		{
			static constexpr CSR_metadata metadata_low
				= {.address = 0xB00, .name = "mcycle", .type = CSR_metadata::Type::Int};
			static constexpr CSR_metadata metadata_high
				= {.address = 0xB80, .name = "mcycleh", .type = CSR_metadata::Type::Int};
		};

		struct Minstret : public CSR_64bit_empty<0xB02, 0xB82>
		{
			static constexpr CSR_metadata metadata_low
				= {.address = 0xB02, .name = "minstret", .type = CSR_metadata::Type::Int};
			static constexpr CSR_metadata metadata_high
				= {.address = 0xB82, .name = "minstreth", .type = CSR_metadata::Type::Int};
		};

		struct Mvendorid : public Readonly_csr<0xF11>
		{
			static constexpr CSR_metadata metadata
				= {.address = 0xF11, .name = "mvendorid", .type = CSR_metadata::Type::Int};
		};

		struct Marchid : public Readonly_csr<0xF12>
		{
			static constexpr CSR_metadata metadata
				= {.address = 0xF12, .name = "marchid", .type = CSR_metadata::Type::Int};
		};

		struct Mimpid : public Readonly_csr<0xF13>
		{
			static constexpr CSR_metadata metadata
				= {.address = 0xF13, .name = "mimpid", .type = CSR_metadata::Type::Int};
		};

		struct Mhartid : public Readonly_csr<0xF14>
		{
			static constexpr CSR_metadata metadata
				= {.address = 0xF14, .name = "mhartid", .type = CSR_metadata::Type::Int};
		};

		struct Mconfigptr : public Readonly_csr<0xF15>
		{
			static constexpr CSR_metadata metadata
				= {.address = 0xF15, .name = "mconfigptr", .type = CSR_metadata::Type::Data_ptr};
		};

	}

	/**
	 * @brief CSR Write mode. Used for CSR accessing.
	 *
	 */
	enum class CSR_write_mode
	{
		None,       // No operation
		Overwrite,  // Overwrite the entire CSR with the write value
		Set,        // Set bits in the CSR according to the write value
		Clear,      // Clear bits in the CSR according to the write value
	};

	/**
	 * @brief CSR access info struct. Fill it to access a CSR.
	 *
	 */
	struct CSR_access_info
	{
		CSR_write_mode write_mode = CSR_write_mode::None;
		Bitset<12> address = 0;
		u32 write_value = 0;
		bool read = false;
	};

	struct CSR_module
	{
		csr::Mvendorid mvendorid = {0x00000000};
		csr::Marchid marchid = {0x00000000};
		csr::Mimpid mimpid = {0x00000000};
		csr::Mhartid mhartid = {0x00000000};
		csr::Mconfigptr mconfigptr = {0x00000000};
		csr::Misa misa = {.ext_i = true, .ext_m = true, .base = csr::Misa::Base_width::Rv32};
		csr::Mscratch mscratch;
		csr::Mcycles mcycles;
		csr::Minstret minstret;
		csr::Mstatus mstatus;
		csr::Mepc mepc;
		csr::Mcause mcause;
		csr::Mtval mtval;
		csr::Mip mip;
		csr::Mie mie;
		csr::Mtvec mtvec;

		// ===== NOTE =====
		// medeleg and mideleg not implemented (No S mode)
		// mcounteren not exists (No U mode)

		/*===== ACCESS =====*/

		/**
		 * @brief CSR access error
		 *
		 */
		enum class Error
		{
			Insufficient_priviledge,
			Write_readonly,
			Not_exists
		};

		/**
		 * @brief Access CSR using given info
		 *
		 * @param access_info Write info
		 * @return
		 * - `u32` read value if `access_info.read` is `true`
		 * - `0` if `access_info.read` is `false`
		 * - `Error` if an error occurred
		 */
		std::expected<u32, Error> operator()(const CSR_access_info& access_info) noexcept;

		/*===== UPDATE =====*/

		/**
		 * @brief Tick the CSR module. Increments `mcycle` and `minstret` by 1.
		 *
		 */
		void tick();

		/*===== METADATA =====*/

		using Metadata_set = boost::multi_index_container<
			CSR_metadata,
			boost::multi_index::indexed_by<
				boost::multi_index::ordered_unique<
					boost::multi_index::tag<CSR_metadata::Key_address>,
					boost::multi_index::member<CSR_metadata, u16, &CSR_metadata::address>
				>,
				boost::multi_index::ordered_unique<
					boost::multi_index::tag<CSR_metadata::Key_name>,
					boost::multi_index::member<CSR_metadata, std::string_view, &CSR_metadata::name>
				>
			>
		>;

#define METADATA(class_name) csr::class_name::metadata
#define METADATA_WIDE(class_name) csr::class_name::metadata_low, csr::class_name::metadata_high

		/**
		 * @brief Table of all metadata of implemented CSR
		 *
		 */
		inline static const CSR_module::Metadata_set metadata = {
			METADATA(Mvendorid),
			METADATA(Marchid),
			METADATA(Mimpid),
			METADATA(Mhartid),
			METADATA(Mconfigptr),
			METADATA(Misa),
			METADATA(Mscratch),
			METADATA_WIDE(Mcycles),
			METADATA_WIDE(Minstret),
			METADATA_WIDE(Mstatus),
			METADATA(Mepc),
			METADATA(Mcause),
			METADATA(Mtval),
			METADATA(Mip),
			METADATA(Mie),
			METADATA(Mtvec),
		};

#undef METADATA
#undef METADATA_WIDE
	};
}
