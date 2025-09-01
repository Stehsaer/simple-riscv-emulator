#pragma once

#include "core/memory.hpp"

namespace device::periph
{
	class Periph_base : public core::Memory_interface
	{
	  public:

		size_t size() const override { return 256; }

		std::expected<void, Error> read_page(
			u64 address [[maybe_unused]],
			std::span<u32, 1024> data [[maybe_unused]]
		) override
		{
			return std::unexpected(Error::Not_supported);
		}
	};
}