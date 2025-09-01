add_requires("argparse")

target("main")
	set_kind("binary")

	add_rules("utils.bin2c", {extensions = {".xml",}})

	add_deps("core", "device", "gdb-stub")
	add_files("src/**.cpp", "xml/**.xml")
	add_includedirs("include", { public = true })
	add_packages("argparse")
