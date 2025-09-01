#include <chrono>
#include <csignal>
#include <iostream>
#include <print>
#include <stacktrace>

#include "core/print.hpp"
#include "emulator-debug.hpp"
#include "emulator.hpp"
#include "option.hpp"

int main(int argc, char* argv[])
try
{
	/* Initialization */

	const Options options = Options::parse_args(argc, argv);

	/* Emulate */

	if (options.enable_debug)
	{
		Emulator_debug emulator_debug(Emulator_debug::create(options), options);
		emulator_debug.run();
	}
	else
	{
		Emulator emulator = Emulator::create(options);
		emulator.run();
	}

	return 0;
}
catch (const std::invalid_argument& e)
{
	eprintln("std::invalid_argument: {}", e.what());
	return 1;
}
catch (const std::out_of_range& e)
{
	eprintln("std::out_of_range: {}", e.what());
	return 1;
}
catch (const std::bad_alloc& e)
{
	eprintln("std::bad_alloc: {}", e.what());
	return 1;
}
catch (const std::runtime_error& e)
{
	eprintln("std::runtime_error: {}", e.what());
	return 1;
}
catch (const std::logic_error& e)
{
	eprintln("std::logic_error: {}", e.what());
	return 1;
}
catch (const std::exception& e)
{
	eprintln("std::exception: {}", e.what());
	return 1;
}
catch (...)
{
	eprintln("An unknown error occurred");
	return 1;
}