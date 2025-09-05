#pragma once

#include <cstdint>
#include <map>
#include <span>
#include <string>

#include "command.hpp"
#include "response.hpp"

namespace gdb_stub
{
	struct Xml_file_slice
	{
		/**
		 * @brief `true` if this is the last slice of the file
		 *
		 */
		bool is_end;

		/**
		 * @brief Raw data of the slice. Doesn't contain null terminator.
		 *
		 */
		std::vector<u8> data;
	};

	/**
	 * @brief Get xml file slice from filename
	 *
	 * @param filename Filename of the xml file
	 * @param offset Offset in bytes
	 * @param size Size in bytes
	 * @return `Xml_file_slice` if `offset` is valid, `std::nullopt` otherwise.
	 * @note The actual returned slice may be smaller than `size` if the end of the file is reached.
	 */
	std::optional<Xml_file_slice> get_xml_file(const std::string& filename, size_t offset, size_t size);
}