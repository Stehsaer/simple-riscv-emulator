#include "emulator.hpp"
#include "core/print.hpp"
#include "gdb-stub/accessor.hpp"
#include "gdb-stub/expression.hpp"
#include "gdb-stub/gdb-xml.hpp"

Emulator Emulator::create(const Options& options)
{
	std::vector<u8> rom_data;
	{
		std::ifstream rom_file(options.flash_file_path, std::ios::binary);
		if (!rom_file)
			throw std::runtime_error(std::format("Failed to open flash file ({})", std::strerror(errno)));

		rom_file.seekg(0, std::ios::end);
		const auto file_size = rom_file.tellg();
		rom_file.seekg(0, std::ios::beg);

		if (file_size <= 0) throw std::runtime_error("Flash file is empty");
		rom_data.resize(static_cast<size_t>(file_size));
		rom_file.read(reinterpret_cast<char*>(rom_data.data()), file_size);

		if (!rom_file || rom_file.gcount() != file_size)
			throw std::runtime_error(std::format("Failed to read flash file ({})", std::strerror(errno)));
	}

	Emulator emulator;
	emulator.platform = std::make_unique<Platform>(rom_data.data(), rom_data.size(), options.ram_fill_policy);
	emulator.trap_capture_mode = options.trap_capture;
	emulator.stop_at_infinite_loop = options.stop_at_infinite_loop;

	return emulator;
}

core::CPU_module::Result Emulator::tick_one_cycle()
{
	const auto result = platform->cpu->step();
	platform->memory->clock_periph->tick(platform->cpu->csr.mip);

	inst_executed++;

	return result;
}

void Emulator::run()
{
	while (true)
	{
		const auto result = tick_one_cycle();

		switch (trap_capture_mode)
		{
		case Options::Trap_capture_mode::No_capture:
			break;

		case Options::Trap_capture_mode::Exception_only:
			if (result.trap.has_value()
				&& core::is_interrupt(result.trap.value())
				&& result.trap.value() != core::Trap::Env_call_from_M_mode)
			{
				iprintln(
					"Exception detected at PC: 0x{:08x} (Inst=0x{:08x}). Trap code: {}",
					result.pc,
					result.inst,
					(u16)result.trap.value() & 0x0fff
				);
				return;
			}
			break;

		case Options::Trap_capture_mode::All:
			if (result.trap.has_value())
			{
				iprintln(
					"Trap captured at PC: 0x{:08x} (Inst=0x{:08x}). Trap type: {}; Trap code: {}",
					result.pc,
					result.inst,
					(u16)core::is_interrupt(result.trap.value()) ? "Interrupt" : "Exception",
					(u16)result.trap.value() & 0x0fff
				);
				return;
			}
			break;
		}

		if (stop_at_infinite_loop
			&& !result.trap.has_value()
			&& result.pc == result.alu_result
			&& result.branch_result)
		{
			iprintln("Infinite loop detected at PC: 0x{:08x}", result.pc);
			return;
		}
	}
}
