#pragma once

#include "emulator.hpp"

/**
 * @brief Debug extension for emulator with GDB stub
 *
 */
class Emulator_debug : public Emulator
{
  public:

	/**
	 * @brief Extend emulator with GDB stub
	 *
	 * @param emu Original emulator
	 * @param options Options
	 */
	Emulator_debug(Emulator&& emu, const Options& options);

	/**
	 * @brief Run the emulator with GDB stub enabled
	 *
	 */
	void run();

  private:

	std::unique_ptr<gdb_stub::Network_handler> network;                   // Main network handler
	std::map<u32, gdb_stub::Hw_breakpoint> hw_breakpoints;                // Map of hardware breakpoints
	std::map<gdb_stub::Address_range, gdb_stub::Watchpoint> watchpoints;  // Map of watchpoints

	/*===== CHECK TRAP =====*/

	/**
	 * @brief Check if breakpoint is hit
	 *
	 * @return `true` if hit, `false` otherwise
	 */
	bool check_breakpoint();

	/**
	 * @brief Check if watchpoint is hit
	 *
	 * @param result Input result
	 * @return The outer `std::optional` has value if a hit occurs. The inner `std::pair` has 2 booleans
	 * meaning `{<is_read>, <is_write>}` respectively
	 */
	std::optional<std::pair<bool, bool>> check_watchpoint(const core::CPU_module::Result& result);

	/*===== RUN FUNCTIONS =====*/

	/**
	 * @brief Run until a trap is hit (breakpoint, watchpoint, exception, ecall, ebreak) or interrupted
	 *
	 * @param interrupt Interrupt signal flag
	 * @return Resulted stop reason
	 */
	gdb_stub::response::Stop_reason run_until_trap(const std::atomic<bool>& interrupt);

	/**
	 * @brief Run given clock cycles (instructions). May return early if interrupted.
	 *
	 * @param cycle_count Expected cycle count
	 * @param interrupt Interrupt signal flag
	 * @return Resulted stop reason
	 */
	gdb_stub::response::Stop_reason run_steps(size_t cycle_count, const std::atomic<bool>& interrupt);

	/**
	 * @brief Run the given run function asynchronously, until it returns
	 * @details This function creates a separate thread to run the given function, and handles incoming GDB
	 * interrupt signals in the main thread. It also sends the response to the GDB host.
	 * @param run_func
	 */
	void async_run(const std::function<gdb_stub::response::Stop_reason(const std::atomic<bool>&)>& run_func);

	/*===== COMMAND HANDLING =====*/
	// This section contains handlers for GDB commands
	// See https://sourceware.org/gdb/current/onlinedocs/gdb.html/Packets.html

	// `qSupported` Command
	void handle_qsupport(const std::any& command);

	// `qXfer:feature:read` Command
	void handle_qxfer_feature(const std::any& command);

	// `qXfer:memory-map:read` Command
	void handle_qxfer_memory_map(const std::any& command);

	// `?` Command
	void handle_query(const std::any& command);

	// `m` Command
	void handle_mem_read(const std::any& command);

	// `M` Command
	void handle_mem_write(const std::any& command);

	// `g` Command
	void handle_reg_read(const std::any& command);

	// `G` Command
	void handle_reg_write(const std::any& command);

	// `p` Command
	void handle_single_reg_read(const std::any& command);

	// `P` Command
	void handle_single_reg_write(const std::any& command);

	// `Z0/1` Command, currently only supports hardware breakpoints
	void handle_add_breakpoint(const std::any& command);

	// `z0/1` Command, currently only supports hardware breakpoints
	void handle_remove_breakpoint(const std::any& command);

	// `Z2/3/4` Command
	void handle_add_watchpoint(const std::any& command);

	// `z2/3/4` Command
	void handle_remove_watchpoint(const std::any& command);

	// `c` Command
	void handle_continue(const std::any& command);

	// `s` Command
	void handle_step(const std::any& command);

	// `i` Command
	void handle_step_single_cycles(const std::any& command);

	enum class Special_command_handle_result
	{
		Unhandled,  // Command not handled
		Continue,   // Emulator should continue
		Stop        // Emulator should stop
	};

	/**
	 * @brief Handle special commands
	 *
	 * @param command Command input
	 * @return See `Special_command_handle_result`
	 */
	Special_command_handle_result handle_special_commands(const std::any& command);

	/**
	 * @brief Table that stores the mapping between command types and their handlers
	 *
	 */
	const std::map<const std::type_info*, std::function<void(const std::any& command)>> command_handlers;

	/*===== AUXILIARY FUNCTIONS =====*/

	/**
	 * @brief Send response to GDB, close connection if failed
	 *
	 * @param response Respons object
	 */
	void send_response(const gdb_stub::Response& response);
};