add_requires("gtest")

function generate_tests(name)

	namespace("test")
	
		target(name)

			set_kind("binary")
			set_languages("c++23", {public=true})
			add_packages("gtest")

			add_files("../gtest-main.cpp")
			add_deps(name)

			for _, file in ipairs(os.files("*.cpp")) do
				local testname = path.basename(file)
				add_tests(testname, {files=file})
			end
		target_end()

		target(name .. "::all")

			set_kind("binary")
			set_languages("c++23", {public=true})
			add_packages("gtest")
			set_default("false")

			add_files("../gtest-main.cpp")
			add_deps(name)

			add_files("*.cpp")
		target_end()

	namespace_end()
end

includes("*")