#pragma once

#include "device/block-memory.hpp"
#include <string>

/**
 * @brief Describes options for the simulator
 *
 */
struct Options
{
	enum class Trap_capture_mode
	{
		No_capture,      // Don't capture traps
		Exception_only,  // Only catch exceptions (excluding ecall/ebreak)
		All              // Catch all traps
	};

	/* Platform settings */

	/**
	 * @brief Path to the RAW BINARY flash file
	 * @warning Maximum 128KiB (can be less)
	 */
	std::string flash_file_path;

	/**
	 * @brief Fill policy of main RAM
	 * @note To emulate DDR, use `device::Fill_policy::Random`
	 */
	device::Fill_policy ram_fill_policy;

	/* Simulation Settings */

	/**
	 * @brief Trap capture mode
	 * @note `Trap_capture_mode::Exception_only` is recommended for emulating use, and
	 * `Trap_capture_mode::No_capture` for debugging
	 */
	Trap_capture_mode trap_capture = Trap_capture_mode::No_capture;

	/**
	 * @brief Whether to stop the emulator when an infinite loop is detected (e.g. `j .`)
	 * @note Strongly recommended to disable when debugging or the program use interrupts -- it may cause the
	 * emulator to stop too early!
	 */
	bool stop_at_infinite_loop = true;

	/* Debug Settings */

	/**
	 * @brief Whether to enable the GDB stub for debugging
	 *
	 */
	bool enable_debug = false;

	/**
	 * @brief The remote TCP port for debugging
	 *
	 */
	u16 debug_port = 16355;

	/* Parse Methods */

	static Options parse_args(int argc, char* argv[]);
};
