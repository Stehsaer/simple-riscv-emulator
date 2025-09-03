#pragma once

#include "common/type.hpp"

#include <flat_map>
#include <format>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace gdb_stub
{
	/**
	 * @brief Base class for all responses to GDB
	 *
	 */
	class Response
	{
	  public:

		/**
		 * @brief Stub function to convert response to string
		 *
		 * @return Converted string, not escaped or runlength-encoded yet
		 */
		virtual std::string to_string() const = 0;

		virtual ~Response() = default;
	};

	namespace response
	{
		/**
		 * @brief `OK` response
		 *
		 */
		class OK : public Response
		{
		  public:

			std::string to_string() const override { return "OK"; }
		};

		/**
		 * @brief Content of a single register.
		 *
		 */
		class Single_register_content : public Response
		{
			std::optional<u32> value;

		  public:

			Single_register_content(std::optional<u32> value) :
				value(value)
			{}

			std::string to_string() const override;
		};

		/**
		 * @brief Returns register contents.
		 *
		 */
		class Register_content : public Response
		{
			std::vector<std::optional<u32>> reg_values;

		  public:

			/**
			 * @brief Construct a new `Register_content` from map of register values
			 *
			 * @param reg_values Map of register number to register value
			 */
			Register_content(std::vector<std::optional<u32>> reg_values) :
				reg_values(std::move(reg_values))
			{}

			std::string to_string() const override;
		};

		/**
		 * @brief Raw byte stream response. Used in `m` response.
		 *
		 */
		class Raw_byte_stream : public Response
		{
			std::vector<u8> data;

		  public:

			/**
			 * @brief Construct a new `Raw_byte_stream` from raw data bytes
			 *
			 * @param data Raw data span
			 */
			Raw_byte_stream(std::span<const u8> data) :
				data(data.begin(), data.end())
			{}

			std::string to_string() const override;
		};

		/**
		 * @brief Unsupported command response.
		 *
		 */
		class Unsupported_command : public Response
		{
		  public:

			std::string to_string() const override { return ""; }
		};

		/**
		 * @brief Error code response.
		 *
		 */
		class Error_code : public Response
		{
			u8 code;

		  public:

			Error_code(u8 code) :
				code(code)
			{}

			std::string to_string() const override { return std::format("E{:02x}", code); }
		};

		/**
		 * @brief Error message response.
		 *
		 */
		class Error_message : public Response
		{
			std::string message;

		  public:

			Error_message(std::string message) :
				message(std::move(message))
			{}

			std::string to_string() const override { return std::format("E.{}", message); }
		};

		/**
		 * @brief Stop reason response.
		 *
		 */
		class Stop_reason : public Response
		{
		  public:

			/**
			 * @brief Watchpoint hit information
			 *
			 */
			struct Watchpoint_hit
			{
				u32 address;             // Address of the watchpoint
				bool is_write, is_read;  // Watchpoint access info
			};

			/**
			 * @brief Breakpoint hit information
			 *
			 */
			struct Breakpoint_hit
			{
				bool is_hardware;  // `true` if is hardware breakpoint
			};

			/**
			 * @brief Construct a new `Stop_reason` with only signals and register contents
			 *
			 * @param signal Signal number. Follows POSIX signal numbers.
			 */
			Stop_reason(u8 signal);

			/**
			 * @brief Construct a new `Stop_reason` for watchpoint hit
			 *
			 * @param watchpoint_hit Watchpoint hit information
			 */
			Stop_reason(Watchpoint_hit watchpoint_hit);

			/**
			 * @brief Construct a new `Stop_reason` for breakpoint hit
			 *
			 * @param breakpoint_hit Breakpoint hit information
			 */
			Stop_reason(Breakpoint_hit breakpoint_hit);

			std::string to_string() const override;

		  private:

			u8 signal;
			std::variant<std::monostate, Watchpoint_hit, Breakpoint_hit> hit;

			static std::string parse_hit_string(std::monostate hit);
			static std::string parse_hit_string(Watchpoint_hit hit);
			static std::string parse_hit_string(Breakpoint_hit hit);
		};

		/**
		 * @brief Response for `qXfer` command.
		 *
		 */
		class Qxfer_response : public Response
		{
			bool completed;
			std::vector<u8> data;

		  public:

			/**
			 * @brief Construct a new `Qxfer_response`
			 *
			 * @param completed `true` if this is the last slice of the file, `false` otherwise
			 * @param data Raw data of the slice. Can be empty.
			 */
			Qxfer_response(bool completed, std::vector<u8> data) :
				completed(completed),
				data(std::move(data))
			{}

			std::string to_string() const override;
		};

		/**
		 * @brief Response for `qSupported` command.
		 * @note Current this response is hardcoded. See `stub-feature.inc` file.
		 */
		class Qsupported_response : public Response
		{
		  public:

			std::string to_string() const override;
		};
	}

}