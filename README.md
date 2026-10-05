# Universal PS4 Native Runtime & Compiler on Linux

A universal runtime environment and compilation toolchain for executing PlayStation 4 games natively on Linux x86_64.

---

## Origin and Relation to Existing Projects

This project is an extension and generalization of the native PS4 port architecture demonstrated by [bbport](https://github.com/shadps4-emu/bbport) and [shadPS4](https://github.com/shadps4-emu/shadPS4).

While previous efforts focused on custom-tailored environments for specific titles, **universal_ps4_native** abstracts and generalizes the native execution model into an automated, title-agnostic toolchain:
- **Universal Package Extraction**: Automated unpacking of retail and fake PS4 `.pkg` containers.
- **Dynamic ELF Re-linking**: Discovers, analyzes, and binds arbitrary PlayStation 4 `eboot.bin` files and bundled system modules (`.prx`) directly into native host memory.
- **Permissive HLE Runtime**: Provides resilient Orbis OS system call and kernel shims with non-fatal stubbing for exploratory execution of unmapped services.
- **Standalone Linux Compilation**: Generates native runnable Linux launcher binaries from game dumps.

---

## How It Works

PlayStation 4 games are not foreign-architecture console ROMs. The PlayStation 4 runs an AMD Jaguar x86_64 CPU on an operating system derived from FreeBSD 9, utilizing the standard System V AMD64 ABI.

1. **Direct CPU Execution**: Because the guest binary is already compiled for x86_64, code runs directly on host CPU cores with zero CPU emulation and zero JIT compilation overhead.
2. **TLS Register Remapping**: On PS4 Orbis OS, user thread local storage resides in `%fs`. On Linux, glibc reserves `%fs` for host threading and stack protection. The linker rewrites initial-exec `mov rax, fs:[0]` instructions to `%gs` to ensure seamless coexistence with Linux glibc.
3. **40-bit Address Space Below 1 TiB**: PS4 GPU descriptors encode 40-bit pointers. The runtime memory manager places all guest allocations, stacks, and heaps within the lower 1 TiB address range.
4. **Vulkan GPU Translation**: Guest Gnm and Gnmx command buffers are translated into Vulkan 1.3 pipelines with asynchronous host shader recompilation.
5. **Native Audio and Input**: Audio pipelines interface directly with PipeWire / SDL3, and controller input supports background Wayland/X11 polling for DualSense, DualShock 4, and standard gamepads.

---

## Project Structure

- `bin/ps4-native`: Primary command line tool for extraction, inspection, compilation, and execution.
- `build.sh`: Host compiler script building the native runtime binary with native CPU optimizations (`-march=native -mtune=native -O3`).
- `src/`: Native C runtime shims for PS4 kernel, memory, threads, synchronization, files, audio, and pads.
- `scripts/prepare.py`: Decrypted SELF parser and memory segment builder.
- `scripts/link_modules.py`: Multi-PRX linker with TLS register fixup.
- `gpu/`: Vulkan video core, Gnm driver shim, and shader recompiler.

---

## Installation & Requirements

### System Dependencies
- Linux x86_64
- GCC or Clang with C11 and C++23 support
- Vulkan SDK / Vulkan loader and headers
- SDL3 (`libsdl3-dev` or `sdl3`)
- CMake and Ninja
- Python 3.8+
- `ps4-pkg-tool` (optional, for `.pkg` extraction)

### Setup & Dependencies
Run the setup script to initialize submodules, apply patches, and check/install external tools:
```bash
./setup.sh
```

### Building the Runtime
```bash
./build.sh
```

---

## Acknowledgments & Third-Party Credits
This project relies on and acknowledges numerous upstream projects, including **shadPS4**, **bbport**, **ps4-pkg-tool**, **LibAtrac9**, **FSR-Vulkan**, **Dear ImGui**, **sirit**, **Zydis**, **VMA**, **miniz**, and others.
For the complete list of licenses and authors, see [ACKNOWLEDGMENTS.md](file:///home/mash/game/universal_ps4_native/ACKNOWLEDGMENTS.md).

---

## Usage Guide

### 1. Extract a PS4 Package (.pkg)
```bash
./bin/ps4-native extract /path/to/game.pkg --out ./extracted/CUSAXXXXX
```

### 2. Inspect Title & Native Binary
```bash
./bin/ps4-native inspect ./extracted/CUSAXXXXX
```
Inspects metadata (`param.sfo`), entry point, loadable segments, and symbols in all bundled `.prx` modules.

### 3. Compile to Standalone Native Executable
```bash
./bin/ps4-native compile ./extracted/CUSAXXXXX --app-out ./my_game
```
Links `eboot.bin` with `sce_module/*.prx`, patches initial TLS loads, and outputs a standalone executable launcher.

### 4. Run Natively
```bash
# run compiled standalone executable directly
./my_game

# or run via the runner cli
./bin/ps4-native run ./extracted/CUSAXXXXX

# exploratory run with non-fatal stubs for unimplemented services
./bin/ps4-native run ./extracted/CUSAXXXXX --permissive

# headless test run
./bin/ps4-native run ./extracted/CUSAXXXXX --cpu-only --timeout 5
```
