add_requires("asio", "boost")

target("core")

	set_kind("static")
	set_languages("c++23", {public=true})

	add_files("core/**.cpp")
	add_headerfiles("include/core/**.hpp")
	add_includedirs("include", {public=true})
	add_packages("boost", {public=true})

target("device")

	set_kind("static")
	set_languages("c++23", {public=true})

	add_includedirs("include", {public=true})
	add_headerfiles("include/device/**.hpp")
	add_files("device/**.cpp")
	add_deps("core")

target("gdb-stub")

	set_kind("static")
	set_languages("c++23", {public=true})

	add_rules("utils.bin2c", {extensions = {".xml",}})

	add_includedirs("include", {public=true})
	add_files("gdb-stub/**.cpp")
	add_files("gdb-stub/gdb-xml/*.xml")
	add_headerfiles("include/gdb-stub/**.hpp")
	add_deps("core")
	add_packages("asio", {public=true})