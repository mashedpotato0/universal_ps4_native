# Universal PS4 Native Runtime & Compiler on Linux

A universal runtime environment and compilation toolchain for executing PlayStation 4 games natively on Linux x86_64.

> [!NOTE]
> This repository is still in development. Only Bloodborne and Street Fighter 30th Anniversary Collection have been tested so far.

---

## Origin and Relation to Existing Projects

This project is an extension and generalization of the native PS4 port architecture demonstrated by [bbport](https://github.com/shadps4-emu/bbport) and [shadPS4](https://github.com/shadps4-emu/shadPS4).

While previous efforts focused on custom-tailored environments for specific titles, **universal_ps4_native** abstracts and generalizes the native execution model into an automated, title-agnostic toolchain:
- **Universal Package Extraction**: Automated unpacking of PS4 `.pkg` containers.
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
- `setup.sh`: Automated dependency manager that clones and updates git submodules, applies patches, and installs tool dependencies.
- `build.sh`: Host compiler script building the native runtime binary with native CPU optimizations (`-march=native -mtune=native -O3`).
- `src/`: Native C runtime shims for PS4 kernel, memory, threads, synchronization, files, audio, and pads.
- `scripts/prepare.py`: Decrypted SELF parser and memory segment builder.
- `scripts/link_modules.py`: Multi-PRX linker with TLS register fixup.
- `gpu/`: Vulkan video core, Gnm driver shim, and shader recompiler.

---

## Getting Started

### 1. System Requirements
- Linux x86_64 distribution (Arch, Fedora, Ubuntu, Debian, etc.)
- GCC or Clang with C11 and C++23 support
- Vulkan SDK or driver headers (`vulkan-headers`, `vulkan-icd-loader`)
- SDL3 library (`libsdl3-dev` or `sdl3`)
- CMake and Ninja
- Python 3.8+

### 2. Initial Setup
Clone the repository and run the setup script to initialize all submodules, apply required patches, and prepare extraction tools:
```bash
git clone --recursive https://github.com/mashedpotato0/universal_ps4_native.git
cd universal_ps4_native
./setup.sh
```

### 3. Build the Runtime
Compile the native runtime and GPU translation layers:
```bash
./build.sh
```

---

## Step-by-Step User Guide

### Quick Start: Automated One-Click Play
You don't need to fiddle with complex command line options to play. Simply run the automated launcher:
```bash
# auto-detects packages or extracted games and launches directly
./play

# or using run.sh
./run.sh
```
If you pass a `.pkg` or folder directly:
```bash
./play /path/to/game.pkg
```

### Direct Game Executables (e.g. `./bloodborne`)
When you compile or run a title, a standalone native launcher named after the game is automatically generated in the project root:
```bash
# run bloodborne directly with zero command line friction
./bloodborne
```

### Install Application Menu Shortcut (Steam Deck / GNOME / KDE)
Generate a native desktop shortcut with the official PlayStation 4 game icon in `~/.local/share/applications`:
```bash
./bin/ps4-native desktop ./extracted/CUSA03173
```

---

### Advanced Workflow

#### Step 1: Extract Packages (Single File, Multiple Files, or Folder)
Extract one or multiple `.pkg` files into a game directory. When pointing to a folder, the CLI automatically ignores non-PKG downloader metadata (`.sqlite`, `.xml`), orders base game and patches correctly, and extracts them in sequence:
```bash
# extract from a folder containing base game and update pkgs
./bin/ps4-native extract /path/to/pkg_folder/ --out ./extracted/CUSAXXXXX

# or pass multiple files explicitly
./bin/ps4-native extract base_game.pkg update_v109.pkg --out ./extracted/CUSAXXXXX
```

#### Step 2: Inspect Game Metadata and Packages
Inspect `.pkg` containers, package folders, or extracted directories to view title metadata, categories, versions, and entry points:
```bash
# inspect a folder containing packages
./bin/ps4-native inspect /path/to/pkg_folder/

# or inspect an already extracted game directory
./bin/ps4-native inspect ./extracted/CUSAXXXXX
```

#### Step 3: Compile into a Standalone Linux Executable
Compile and link `eboot.bin` and bundled PRX modules (`libc.prx`, `libSceFios2.prx`, etc.) into a standalone Linux application:
```bash
./bin/ps4-native compile ./extracted/CUSAXXXXX --app-out ./my_game
```
This produces an executable `./my_game` that can be launched directly without running through Python.

#### Step 4: Run the Application
Launch the standalone binary or use the runner CLI:
```bash
# launch the compiled standalone executable
./my_game

# or launch using the cli driver
./bin/ps4-native run ./extracted/CUSAXXXXX
```

#### Runtime Flags and Controls
- **Permissive Mode**: Stubs unhandled non-critical system imports with `0` so experimental titles can continue booting:
  ```bash
  ./bin/ps4-native run ./extracted/CUSAXXXXX --permissive
  ```
- **Headless / CPU-Only Mode**: Runs without opening a Vulkan window for testing compute, memory, and audio:
  ```bash
  ./bin/ps4-native run ./extracted/CUSAXXXXX --cpu-only
  ```
- **Timed Execution**: Run for a specified duration (in seconds) for automated benchmarking:
  ```bash
  ./bin/ps4-native run ./extracted/CUSAXXXXX --timeout 30
  ```
- **In-Game Settings Overlay**: Press `Insert` on keyboard or `L3 + R3` on gamepad to open the graphics and upscaler menu.

---

## Acknowledgments & Third-Party Credits

This project relies on and acknowledges numerous upstream projects, including **shadPS4**, **bbport**, **ps4-pkg-tool**, **LibAtrac9**, **FSR-Vulkan**, **Dear ImGui**, **sirit**, **Zydis**, **VMA**, **miniz**, and others.
For the complete list of licenses, contributors, and authors, see [ACKNOWLEDGMENTS.md](file:///home/mash/game/universal_ps4_native/ACKNOWLEDGMENTS.md).

---

## License

This project is licensed strictly for non-commercial, educational, and research use under the **Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0)**. See [LICENSE](file:///home/mash/game/universal_ps4_native/LICENSE) for the full text. Commercial use, redistribution for profit, or monetization of any derivative works is strictly prohibited.

Third-party libraries retain their original licenses as detailed in [ACKNOWLEDGMENTS.md](file:///home/mash/game/universal_ps4_native/ACKNOWLEDGMENTS.md).
