add_rules("mode.release", "mode.debug", "mode.releasedbg", "mode.profile")

if not is_plat("windows") then
	add_cxflags("-Wpedantic")
end

set_project("cpp-riscv-sim")

includes("main", "lib")
