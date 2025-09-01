#include "gdb-stub/response.hpp"

#include <algorithm>
#include <csignal>
#include <ranges>

namespace gdb_stub::response
{
	std::string Single_register_content::to_string() const
	{
		if (!value.has_value()) return "xxxxxxxx";
		return std::format("{:08x}", *value);
	}

	std::string Register_content::to_string() const
	{
		return reg_values
			 | std::views::transform(
				   [](auto val)
				   {
					   return val.has_value() ? std::format("{:08x}", std::byteswap(val.value()))
											  : std::string("xxxxxxxx");
				   }
			 )
			 | std::views::join
			 | std::ranges::to<std::string>();
	}

	std::string Raw_byte_stream::to_string() const
	{
		if (data.empty()) return "E00";

		return data
			 | std::views::transform([](auto val) { return std::format("{:02x}", val); })
			 | std::views::join
			 | std::ranges::to<std::string>();
	}

	Stop_reason::Stop_reason(u8 signal) :
		signal(signal),
		hit(std::monostate())
	{}

	Stop_reason::Stop_reason(Watchpoint_hit watchpoint_hit) :
		signal(SIGTRAP),
		hit(watchpoint_hit)
	{}

	Stop_reason::Stop_reason(Breakpoint_hit breakpoint_hit) :
		signal(SIGTRAP),
		hit(breakpoint_hit)
	{}

	std::string Stop_reason::parse_hit_string(std::monostate hit [[maybe_unused]])
	{
		return "";
	}

	std::string Stop_reason::parse_hit_string(Watchpoint_hit hit)
	{
		const char* watch_str;

		if (hit.is_read && hit.is_write)
			watch_str = "awatch";
		else if (hit.is_read)
			watch_str = "rwatch";
		else if (hit.is_write)
			watch_str = "watch";
		else
			return "";

		return std::format("{}:{:x};", watch_str, hit.address);
	}

	std::string Stop_reason::parse_hit_string(Breakpoint_hit hit)
	{
		return std::format("{}:;", hit.is_hardware ? "hwbreak" : "swbreak");
	}

	std::string Stop_reason::to_string() const
	{
		const auto hit_string = std::visit([](auto x) { return parse_hit_string(x); }, hit);

		std::string signal_string = std::format("T{:02x}{}", signal, hit_string);

		return signal_string;
	}

	std::string Qxfer_response::to_string() const
	{
		using namespace std::string_literals;

		return (completed ? "l"s : "m"s) + std::string((const char*)data.data(), data.size());
	}

	std::string Qsupported_response::to_string() const
	{
		return
#include "stub-feature.inc"
			;
	}
}