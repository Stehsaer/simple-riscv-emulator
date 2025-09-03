// common/bitset.hpp
// -- Implements an alternative for std::bitset with extraction, concat, extension capabilities.

#pragma once

#include <bitset>
#include <format>

namespace core
{
	/**
	 * @brief Represents a fixed-width bitset
	 * 
	 * @tparam Nb Bit width
	 */
	template <std::size_t Nb>
		requires(Nb > 0 && Nb <= sizeof(0ull) * 8)
	class Bitset
	{
		unsigned long long value;

		static constexpr unsigned long long mask = ~(static_cast<unsigned long long>(-1ll) << Nb);

		friend std::formatter<Bitset>;

	  public:

		/* Constructors*/

		/**
		 * @brief Constructs a bitset. All bits are initialized to 0.
		 * 
		 */
		constexpr Bitset() noexcept :
			value(0)
		{
		}

		/**
		 * @brief Constructs a bitset from an integer value.
		 * 
		 * @tparam T Type of the integer.
		 * @param value Integer value. Only the least significant `Nb` bits are used.
		 */
		template <typename T>
			requires(std::is_integral_v<T>)
		constexpr Bitset(T value) noexcept :
			value(value & mask)
		{
		}

		/**
		 * @brief Constructs a bitset from another bitset with the same width.
		 * 
		 * @tparam Ny Width of the other bitset. **Must be equal to `Nb`**.
		 * @param other Other bitset.
		 */
		template <std::size_t Ny>
		constexpr Bitset(Bitset<Ny> other) noexcept :
			value(static_cast<unsigned long long>(other))
		{
			// Width must match.
			static_assert(
				Ny == Nb,
				"Can't construct a bitset from a bitset with different width. Expand/Extract before "
				"constructing!"
			);
		}

		constexpr Bitset(const Bitset& other) noexcept = default;
		constexpr Bitset(Bitset&& other) noexcept = default;

		/**
		 * @brief Constructs a 1-bit bitset from a `bool` value.
		 * 
		 */
		constexpr Bitset(bool value) noexcept
			requires(Nb == 1)
			:
			value(value)
		{
		}

		Bitset& operator=(const Bitset& other) noexcept = default;
		Bitset& operator=(Bitset&& other) noexcept = default;

		/* Implicit Conversion */

		template <typename T>
			requires(
				(sizeof(T) * 8 >= Nb && std::is_integral_v<T>)
				|| std::is_convertible_v<unsigned long long, T>
				|| std::is_enum_v<T>
			)
		explicit operator T() const noexcept
		{
			return static_cast<T>(value);
		}

		/* Bit-wise Operation */

		constexpr Bitset operator~() const noexcept { return Bitset(~value); }
		constexpr Bitset operator|(Bitset other) const noexcept { return Bitset(value | other.value); }
		constexpr Bitset operator&(Bitset other) const noexcept { return Bitset(value & other.value); }
		constexpr Bitset operator^(Bitset other) const noexcept { return Bitset(value ^ other.value); }

		constexpr Bitset& operator|=(Bitset other) noexcept { value |= other.value; }
		constexpr Bitset& operator&=(Bitset other) noexcept { value &= other.value; }
		constexpr Bitset& operator^=(Bitset other) noexcept { value ^= other.value; }

		/* Arithmetic */

		constexpr bool operator==(Bitset other) const noexcept { return value == other.value; }
		constexpr bool operator!=(Bitset other) const noexcept { return value != other.value; }

		template <typename T>
			requires(std::is_convertible_v<T, unsigned long long>)
		constexpr bool operator==(T value) const noexcept
		{
			return this->value == (static_cast<unsigned long long>(value) & mask);
		}

		template <typename T>
			requires(std::is_convertible_v<T, unsigned long long>)
		constexpr bool operator!=(T value) const noexcept
		{
			return this->value != (static_cast<unsigned long long>(value) & mask);
		}

		/* Bit Extraction */

		/**
		 * @brief Slice the bitset and produces `bitset[MSB:LSB]` (in Verilog notation)
		 * 
		 * @tparam MSB Most significant bit index.
		 * @tparam LSB Least significant bit index.
		 * @return Sliced bitset (`bitset[MSB:LSB]`)
		 */
		template <size_t MSB, size_t LSB>
			requires(MSB >= LSB && MSB <= Nb)
		constexpr Bitset<MSB - LSB + 1> slice() const noexcept
		{
			return Bitset<MSB - LSB + 1>(value >> LSB);
		}

		/**
		 * @brief Extract bit at the given index.
		 * 
		 * @tparam Bit Index of the bit
		 * @return Extracted bit as a 1-bit bitset.
		 */
		template <size_t Bit>
			requires(Bit <= Nb)
		constexpr Bitset<1> take_bit() const noexcept
		{
			return (value >> Bit) & 0x1;
		}

		/* Bit Concat */

		/**
		 * @brief Concatenates a `bool` value to the bitset. 
		 * @return Result (`{value, bitset}` in Verilog notation)
		 */
		constexpr Bitset<Nb + 1> operator+(bool value) const noexcept
			requires(Nb < sizeof(0uz) * 8)
		{
			return Bitset<Nb + 1>(value | ((unsigned long long)this->value << 1));
		}

		/**
		 * @brief Concatenates another bitset with this bitset.
		 * 
		 * @param other Another bitset
		 * @return Result (`{other, bitset}` in Verilog notation)
		 */
		template <size_t Ny>
		constexpr Bitset<Nb + Ny> operator+(Bitset<Ny> other) const noexcept
		{
			return Bitset<Nb + Ny>((value << Ny) | (unsigned long long)other);
		}

		/* Bit Manipulation */

		/**
		 * @brief Zero extends the bitset to a wider bitset.
		 * 
		 * @tparam Ny Target width. **Must be greater than `Nb`**.
		 * @return Zero-extended bitset.
		 */
		template <size_t Ny>
			requires(Ny > Nb)
		constexpr Bitset<Ny> zext() const noexcept
		{
			return Bitset<Ny>(value);
		}

		/**
		 * @brief Signed extends the bitset to a wider bitset.
		 * 
		 * @tparam Ny Target width. **Must be greater than `Nb`**.
		 * @return Sign-extended bitset.
		 */
		template <size_t Ny>
			requires(Ny > Nb)
		constexpr Bitset<Ny> sext() const noexcept
		{
			return (this->take_bit<Nb - 1>() ? Bitset<Ny - Nb>::ones() : Bitset<Ny - Nb>::zeros()) + *this;
		}

		/**
		 * @brief Expand byte-wise mask to bit-wise mask. e.g. `0b01` -> `0b0000000011111111`
		 * 
		 * @return Expanded bit-wise mask.
		 */
		constexpr Bitset<Nb <= 8 ? Nb * 8 : 1> expand_byte_mask() const noexcept
		{
			static_assert(Nb <= 8, "Width of expansion result would exceed limit of 64 bits.");

			if constexpr (Nb == 1)
				return slice<Nb - 1, Nb - 1>().template sext<8>();
			else if constexpr (Nb <= 8)
				return slice<Nb - 1, Nb - 1>().template sext<8>() + slice<Nb - 2, 0>().expand_byte_mask();
		}

		/**
		 * @brief Choose bits from two integer values according to the bitset.
		 * @details For each bit in the two integer values, select the bit from `if_one` 
		 *          if the corresponding bit in the bitset is 1, otherwise select the bit from `if_zero`.
		 * @param if_one Integer to be selected for `1` bits.
		 * @param if_zero Integer to be selected for `0` bits.
		 * @return The resulting integer.
		 */
		template <typename T>
			requires(std::is_integral_v<T> && sizeof(T) * 8 == Nb)
		constexpr T choose_bits(T if_one, T if_zero) const noexcept
		{
			const auto bit_mask = static_cast<T>(value);
			return (if_one & bit_mask) | (if_zero & ~bit_mask);
		}

		/**
		 * @brief Choose bits from two bitsets according to the bitset.
		 * 
		 * @param if_one Bitset to be selected for `1` bits.
		 * @param if_zero Bitset to be selected for `0` bits.
		 * @return The resulting bitset.
		 */
		constexpr Bitset<Nb> choose_bits(Bitset<Nb> if_one, Bitset<Nb> if_zero) const noexcept
		{
			return Bitset<Nb>((if_one.value & value) | (if_zero.value & ~value));
		}

		/* Constants */

		/**
		 * @brief All-zero bitset.
		 */
		static constexpr Bitset zeros() { return 0; }

		/**
		 * @brief All-one bitset.
		 */
		static constexpr Bitset ones() { return static_cast<unsigned long long>(-1ll); }

		/* String representation */

		/**
		 * @brief Represents the bitset as a Verilog hexadecimal.
		 * 
		 * @param is_capital `true` to use capital letters, `false` to use small letters.
		 * @return Hex string
		 */
		constexpr std::string as_hex_string(bool is_capital = false) const
		{
			const char* digits = is_capital ? "0123456789ABCDEF" : "0123456789abcdef";

			auto ull = static_cast<unsigned long long>(*this);
			std::string convert((Nb + 3) / 4, '0');

			for (size_t i = 0; i < convert.size(); ++i)
			{
				convert[convert.size() - i - 1] = digits[ull & 0x0F];
				ull >>= 4;
			}

			return std::format("{}'h{}", Nb, convert);
		}

		/**
		 * @brief Represents the bitset as a Verilog decimal.
		 * 
		 * @return Binary string
		 */
		constexpr std::string as_dec_string() const
		{
			return std::format("{}'d{}", Nb, static_cast<unsigned long long>(*this));
		}

		/**
		 * @brief Represents the bitset as a Verilog binary.
		 * 
		 * @return Binary string
		 */
		constexpr std::string as_bin_string() const
		{
			std::string convert(Nb, '0');
			for (size_t i = 0; i < Nb; ++i)
			{
				if (*this & (1ull << i))
				{
					convert[Nb - i - 1] = '1';
				}
			}
			return std::format("{}'b{}", Nb, convert);
		}
	};
}