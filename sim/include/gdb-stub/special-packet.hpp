#pragma once

namespace gdb_stub
{
	/**
	 * @brief Special packet: `+` or `-`
	 * @details Acknowledge packet, sent from and to GDB to acknowledge the last received packet.
	 * `+` means success, `-` means failure.
	 */
	struct Acknowledge_packet
	{
		bool success;
	};

	/**
	 * @brief Special packet: `\x03` a.k.a Ctrl-C
	 * @details This packet is sent from GDB to the stub to interrupt execution.
	 */
	struct Interrupt_packet
	{};
};