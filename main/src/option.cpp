#include "option.hpp"
#include "core/print.hpp"

#include <argparse/argparse.hpp>
#include <map>

Options Options::parse_args(int argc, char* argv[])
{
	Options options;
	std::string fill_policy_str;
	std::string trap_capture_str;

	const std::map<std::string, device::Fill_policy> fill_policy_map = {
		{"zero",     device::Fill_policy::Zero    },
		{"one",      device::Fill_policy::One     },
		{"random",   device::Fill_policy::Random  },
		{"cdcdcdcd", device::Fill_policy::Cdcdcdcd},
	};

	const std::map<std::string, Options::Trap_capture_mode> trap_capture_map = {
		{"none",      Options::Trap_capture_mode::No_capture    },
		{"exception", Options::Trap_capture_mode::Exception_only},
		{"all",       Options::Trap_capture_mode::All           },
	};

	argparse::ArgumentParser program("<path>", "<alpha>");

	// Arguments
	{
		program.add_argument("--flash")
			.required()
			.help("Path to the flash file")
			.store_into(options.flash_file_path);

		program.add_argument("--fill")
			.choices("zero", "one", "random", "cdcdcdcd")
			.default_value("random")
			.help("Fill policy for the main memory")
			.store_into(fill_policy_str);

		program.add_argument("--trap")
			.choices("none", "exception", "all")
			.default_value("none")
			.help("Trap capture mode")
			.store_into(trap_capture_str);

		program.add_argument("-g", "--debug")
			.help("Enable GDB Debugging")
			.default_value(false)
			.implicit_value(true)
			.store_into(options.enable_debug);

		program.add_argument("--stop-inf-loop")
			.help("Stop simualtion when encountered an infinite loop")
			.default_value(true)
			.implicit_value(true)
			.store_into(options.stop_at_infinite_loop);

		program.add_argument("-p", "--remote-port")
			.help("TCP port of remote debugging connection")
			.default_value(u16(16355))
			.store_into(options.debug_port);
	}

	try
	{
		program.parse_args(argc, argv);
	}
	catch (const std::exception& e)
	{
#ifdef NDEBUG
		throw std::invalid_argument(e.what());
#else
		std::print(std::cerr, "Enter flash path:");
		std::string flash_path;
		std::getline(std::cin, flash_path);

		options.flash_file_path = flash_path;
#endif
	}

	if (options.flash_file_path.empty())
	{
		iprintln("Flash file path (-f, --flash) can't be empty, provide a path below:");
		std::print(std::cerr, "Enter flash path:");
		std::string flash_path;
		std::getline(std::cin, flash_path);
		options.flash_file_path = flash_path;
	}
	options.ram_fill_policy = fill_policy_map.at(fill_policy_str);
	options.trap_capture = trap_capture_map.at(trap_capture_str);

	return options;
}