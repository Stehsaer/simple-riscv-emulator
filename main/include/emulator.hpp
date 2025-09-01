#pragma once

#include "core/cpu.hpp"
#include "gdb-stub/network.hpp"
#include "gdb-stub/stop-point.hpp"
#include "option.hpp"
#include "platform.hpp"

#include <map>
#include <set>

/**
 * @brief Basic emulator, no debugging
 *
 */
class Emulator
{
  public:

	/**
	 * @brief Create a basic emulator from options
	 *
	 * @param options Options
	 * @return Created Emulator
	 *
	 * @throws std::runtime_error If failed to read flash file
	 */
	static Emulator create(const Options& options);

	/**
	 * @brief Run the emulator, no debugging
	 *
	 */
	void run();

  protected:

	std::unique_ptr<Platform> platform;
	u64 inst_executed = 0;

	Options::Trap_capture_mode trap_capture_mode;
	bool stop_at_infinite_loop;

	// Tick one cycle of the CPU
	core::CPU_module::Result tick_one_cycle();
};
