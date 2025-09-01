## About

This project provides a 32-bit RISC-V emulator. The project is intended to work with my two other projects:
- RISC-V design on FPGA: https://github.com/Stehsaer/simple-riscv-cpu-design
- Corresponding software library: https://github.com/Stehsaer/simple-riscv-software-env

## Features

- Supports base ISA and extensions: `rv32im_zicond_zicsr_zifencei`
- Supports exception and interrupt
- Different configurations, modifiable via program run arguments
- Implements GDB stub, supports GDB remote debugging (Currently software breakpoint is not yet supported)
- Measures around maximum of 60M instr/s when not debugging (on Intel i7-13620H CPU)
- Small memory usage, only allocates memory space when needed

## Build

Before building the project, make sure you have *xmake* installed. After cloning the code, use the following commands to build this project: *(Only tested on Linux!)*

```bash
xmake f -m release
xmake build main
```

- The first line configures the project, where *xmake* will automatically download and install the dependencies
- The second line builds the code and generates the executable

## Usage

### Normal

Prepare the **RAW BINARY** flash file located in `<path>`, and execute the following command:

```bash
xmake run main --flash=<path>
```

> [!note]
> 1. The emulator only accepts raw binary flash file, and it'll put the raw content directly into flash as is. Use tools like `objcopy` to extract the desired ELF segments from ELF executables.
> 2. The emulator always starts execution at address `0x0010_0000`, which is also the start address of Boot ROM (See **Device** below). Use linkerscripts to place your startup function at `0x0010_0000`.

You can also run the executable in stand-alone way, with the same requirement of supplying the identical `--flash=<path>` argument. 

At default, when not debugging, the emulator stops when detecting an infinite-loop instruction, such as `j .`. Disable this behavior using argument `--stop-inf-loop=false`. There are also other options available, use `xmake run main -h` or see `main/src/option.cpp` for reference.

### Debugging

Prepare the **RAW BINARY** flash file `<flash_path>` and the corresponding **ELF** executable with debugs symbols `<elf_file>`. First run the emulator:

```bash
xmake run main --flash=<path> -g
```

Then run the GDB debugger (name or path of the GDB may vary):

```bash
riscv64-unknown-elf-gdb <elf_file>
```

Connect GDB to the emulator using the command:

```bash
target remote:16355
```

> [!note]
> The GDB stub listens on port 16355 at default. Append `-p <port>` to the argument list of emulator to designate a different port.

## Todo

- [ ] Support software breakpoints
- [ ] Add SD Card SPI emulation
- [ ] Add auto trace & compare capability

## Devices

- Standard RAM of 2GiB, placed on `0x8000_0000`
- Readonly Boot ROM of 128KiB, placed on `0x0010_0000`
- Several peripherals:
  - UART peripheral, fully emulates functionality of the actual physical one
  - Clock peripheral, use in conjunction with CSR


## Directory

- `sim`: Simulation support library
  - `core`: Core simulation, provides modules to emulate the CPU
  - `device`: Device implementation
  - `gdb-stub`: GDB stub implementation

- `main`: Main executable, handles various logic and put all above together

## Dependencies

The project has very few third-party dependencies, using *xmake* as the build system and also the package manager.

- `argparse`: provides easy command line argument parsing
- `asio`: provides network communication functionality
- `boost`: provides some useful containers
