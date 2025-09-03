#pragma once

#include "common/type.hpp"

#include <any>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "special-packet.hpp"

namespace gdb_stub::command
{
	/**
	 * @brief `!` packet, enables persistant mode
	 *
	 */
	struct Enable_persistant
	{};

	/**
	 * @brief `?` packet, ask for stop/halt reason
	 *
	 */
	struct Ask_halt_reason
	{};

	/**
	 * @brief `c` packet, continue execution at @p address (if given)
	 *
	 */
	struct Continue
	{
		std::optional<u32> address;
	};

	/**
	 * @brief `g` packet, read all registers in the system
	 *
	 */
	struct Read_register
	{};

	/**
	 * @brief `G` packet, write all registers in the system
	 *
	 */
	struct Write_register
	{
		std::map<u16, u32> values;
	};

	/**
	 * @brief `s` packet
	 * @details Step execution at @p address (if given, current address otherwise), for @p cycle_count cycles
	 * (if given, 1 otherwise)
	 *
	 */
	struct Step_cycles
	{
		std::optional<u32> address;
		std::optional<u32> cycle_count;
	};

	/**
	 * @brief `k` packet, stop execution and kill the system
	 *
	 */
	struct Stop
	{};

	/**
	 * @brief `m` packet, read memory at @p address for @p length bytes
	 *
	 */
	struct Read_memory
	{
		u32 address;
		u32 length;
	};

	/**
	 * @brief `M` packet, write memory at @p address with @p data
	 *
	 */
	struct Write_memory
	{
		u32 address;
		std::vector<u8> data;
	};

	/**
	 * @brief `p` packet, Read single register with No. @p regno
	 *
	 */
	struct Read_single_register
	{
		u32 regno;
	};

	/**
	 * @brief `P` packet, Write single register with No. @p regno
	 *
	 */
	struct Write_single_register
	{
		u32 regno;
		u32 value;
	};

	/**
	 * @brief `R` packet, Restart the emulator by resetting
	 *
	 */
	struct Restart
	{};

	/**
	 * @brief Step one instruction resuming from @p address (if given, current PC otherwise)
	 *
	 */
	struct Step_single_inst
	{
		std::optional<u32> address;
	};

	/**
	 * @brief `qSupported` packet, query host features
	 *
	 */
	struct Query_supported
	{
		struct Host_feature_status
		{
			enum
			{
				Value,
				Supported,
				Unsupported,
				Unknown
			} property;

			std::optional<std::string> value = std::nullopt;
		};

		std::map<std::string, Host_feature_status> features;
	};

	/**
	 * @brief `qXfer:features:read:...` packet
	 * @details Read target description XML file ( @p annex ) from @p offset for @p length bytes
	 */
	struct Read_feature_xml
	{
		std::string annex;
		u32 offset;
		u32 length;
	};

	/**
	 * @brief `qXfer:memory-map:read:...` packet
	 * @details Read memory map XML from @p offset for @p length bytes
	 */
	struct Read_memory_map_xml
	{
		u32 offset;
		u32 length;
	};

	/**
	 * @brief `z0/1` packet, Add a new breakpoint
	 *
	 */
	struct Add_breakpoint
	{
		bool is_hardware;
		u32 address;
		u32 length;
		std::optional<std::vector<u8>> cond;
	};

	/**
	 * @brief `z2/3/4` packet, Add a new watchpoint
	 *
	 */
	struct Add_watchpoint
	{
		bool watch_write;
		bool watch_read;
		u32 address;
		u32 length;
	};

	/**
	 * @brief `Z0/1` packet, Remove a breakpoint
	 *
	 */
	struct Remove_breakpoint
	{
		bool is_hardware;
		u32 address;
		u32 length;
	};

	/**
	 * @brief `Z2/3/4` packet, Remove a watchpoint
	 *
	 */
	struct Remove_watchpoint
	{
		bool watch_write;
		bool watch_read;
		u32 address;
		u32 length;
	};

	/**
	 * @brief Parse command from string
	 *
	 * @param command Clean command string, escaped character removed
	 * @return `std::any` containing one of the command structs if successful, empty
	 */
	std::any parse(std::string_view command);
}
