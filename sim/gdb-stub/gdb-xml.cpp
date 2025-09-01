#include "gdb-stub/gdb-xml.hpp"
#include "core/csr.hpp"
#include "core/print.hpp"

#include <array>
#include <ranges>

namespace gdb_stub
{
	static std::string gen_csr_registers()
	{
		const auto& metadata = core::CSR_module::metadata;

		const auto type_to_str = [](core::CSR_metadata::Type type)
		{
			switch (type)
			{
			case core::CSR_metadata::Type::Int:
				return "int";
			case core::CSR_metadata::Type::Code_ptr:
				return "code_ptr";
			case core::CSR_metadata::Type::Data_ptr:
				return "data_ptr";
			}

			std::unreachable();
		};

		const auto to_xml = [&type_to_str](const core::CSR_metadata& meta)
		{
			return std::format(
				"<reg name=\"{}\" bitsize=\"32\" type=\"{}\" regnum=\"{}\"/>\n",
				meta.name,
				type_to_str(meta.type),
				meta.address + 128
			);
		};

		const auto result = metadata.get<core::CSR_metadata::Key_address>()
						  | std::views::transform(to_xml)
						  | std::views::join
						  | std::ranges::to<std::string>();

		return std::format(
			"<?xml version=\"1.0\"?>"
			"<!DOCTYPE feature SYSTEM \"gdb-target.dtd\">"
			"<feature name=\"org.gnu.gdb.riscv.csr\">\n"
			"{}"
			"</feature>\n",
			result
		);
	}

	static const std::array riscv_32bit_cpu_xml = std::to_array<uint8_t>({
#include "riscv-32bit-cpu.xml.h"
		}
	);

	static const std::array target_xml = std::to_array<uint8_t>({
#include "target.xml.h"
		}
	);

	static const std::string csr_generated_xml = gen_csr_registers();

	static const std::map<std::string, std::span<const uint8_t>> gdb_xml_files = {
		{"riscv-32bit-cpu.xml", std::span(riscv_32bit_cpu_xml)},
		{"target.xml", std::span(target_xml)},
		{"riscv-32bit-csr-generated.xml",
		 std::span<const uint8_t>((const uint8_t*)csr_generated_xml.data(), csr_generated_xml.length() + 1)}
	};

	std::optional<Xml_file_slice> get_xml_file(const std::string& filename, size_t offset, size_t size)
	{
		const auto find = gdb_xml_files.find(filename);
		if (find == gdb_xml_files.end()) return std::nullopt;

		const auto span = find->second;
		return Xml_file_slice{
			.is_end = offset + size >= span.size(),
			.data = span | std::views::drop(offset) | std::views::take(size) | std::ranges::to<std::vector>()
		};
	}
}