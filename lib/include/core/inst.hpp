#pragma once

#include "common/bitset.hpp"

namespace core::inst
{
	struct Rtype
	{
		Bitset<7> funct7;
		Bitset<5> rs2, rs1;
		Bitset<3> funct3;
		Bitset<5> rd;

		Rtype(Bitset<32> instr) :
			funct7(instr.slice<31, 25>()),
			rs2(instr.slice<24, 20>()),
			rs1(instr.slice<19, 15>()),
			funct3(instr.slice<14, 12>()),
			rd(instr.slice<11, 7>())
		{}
	};

	struct Itype
	{
		Bitset<32> imm;
		Bitset<5> rs1;
		Bitset<3> funct3;
		Bitset<5> rd;

		Itype(Bitset<32> instr) :
			imm(instr.slice<31, 20>().sext<32>()),
			rs1(instr.slice<19, 15>()),
			funct3(instr.slice<14, 12>()),
			rd(instr.slice<11, 7>())
		{}
	};

	struct Stype
	{
		Bitset<32> imm;
		Bitset<5> rs1, rs2;
		Bitset<3> funct3;

		Stype(Bitset<32> instr) :
			imm((instr.slice<31, 25>() + instr.slice<11, 7>()).sext<32>()),
			rs1(instr.slice<19, 15>()),
			rs2(instr.slice<24, 20>()),
			funct3(instr.slice<14, 12>())
		{}
	};

	struct Btype
	{
		Bitset<32> imm;
		Bitset<5> rs1, rs2;
		Bitset<3> funct3;

		Btype(Bitset<32> instr) :
			imm((instr.take_bit<31>()
				 + instr.take_bit<7>()
				 + instr.slice<30, 25>()
				 + instr.slice<11, 8>()
				 + Bitset<1>::zeros())
					.sext<32>()),
			rs1(instr.slice<19, 15>()),
			rs2(instr.slice<24, 20>()),
			funct3(instr.slice<14, 12>())
		{}
	};

	struct Utype
	{
		Bitset<32> imm;
		Bitset<5> rd;

		Utype(Bitset<32> instr) :
			imm(instr.slice<31, 12>() + Bitset<12>::zeros()),
			rd(instr.slice<11, 7>())
		{}
	};

	struct Jtype
	{
		Bitset<32> imm;
		Bitset<5> rd;

		Jtype(Bitset<32> instr) :
			imm((instr.take_bit<31>()
				 + instr.slice<19, 12>()
				 + instr.take_bit<20>()
				 + instr.slice<30, 21>()
				 + Bitset<1>::zeros())
					.sext<32>()),
			rd(instr.slice<11, 7>())
		{}
	};
}