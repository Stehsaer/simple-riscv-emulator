add_rules("mode.release", "mode.debug", "mode.releasedbg", "mode.profile")

set_project("cpp-riscv-sim")

includes("main", "test", "sim")
