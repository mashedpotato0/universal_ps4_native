# Acknowledgments & Third-Party Credits

This project builds upon, integrates, and interfaces with numerous open-source libraries and projects. We gratefully acknowledge every single tool, library, and author contributing to the ecosystem.

---

### Core Architecture & Execution

- **[deadinside28/bloodborne_pc](https://github.com/deadinside28/bloodborne_pc)** (GPL-2.0)  
  *Author*: deadinside28  
  *Contribution*: Base native port, initial execution environment, linker scripts, patch compiler, and runtime harness from which this multi-title generalized runtime is directly derived.

- **[shadPS4](https://github.com/shadps4-emu/shadPS4)** & **[bbport](https://github.com/shadps4-emu/bbport)** (GPL-2.0 / MIT)  
  *Authors*: shadPS4 contributors, FireBurn, and the bbport development team  
  *Contribution*: PS4 Gnm/Gnmx Vulkan video core, GCN/RDNA shader recompiler, HLE runtime system contracts, and the foundational concept of running PS4 x86_64 code natively on Linux.

- **[ps4-pkg-tool](https://github.com/hippie68/ps4-pkg-tool)** (GPL-3.0)  
  *Author*: hippie68  
  *Contribution*: PlayStation 4 PKG container parsing, unpacking, decryption integration, and asset extraction.

---

### Audio, Video & Shaders

- **[LibAtrac9](https://github.com/Thealexbarney/LibAtrac9)** (MIT)  
  *Author*: Thealexbarney  
  *Contribution*: Native C decoder for Sony's proprietary ATRAC9 audio format used across PlayStation 4 titles.

- **[FSR-Vulkan](https://github.com/FireBurn/FSR-Vulkan)** (MIT / AMD FidelityFX)  
  *Authors*: FireBurn, Advanced Micro Devices, Inc.  
  *Contribution*: Portable Vulkan implementation of AMD FidelityFX Super Resolution (FSR 3.1) temporal upscaling.

- **[sirit](https://github.com/ReinUsesLisp/sirit)** (MIT)  
  *Authors*: ReinUsesLisp, shadPS4 contributors  
  *Contribution*: Lightweight runtime SPIR-V assembly and intermediate code generation library.

- **[VulkanMemoryAllocator (VMA)](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)** (MIT)  
  *Author*: AMD GPUOpen  
  *Contribution*: Efficient allocation algorithms and memory management for Vulkan buffers and images.

---

### Instruction Decoding & Math

- **[Zydis](https://github.com/zyantific/zydis)** & **[Zycore](https://github.com/zyantific/zycore)** (MIT)  
  *Author*: Zyantific  
  *Contribution*: High-performance x86 and x86_64 disassembler, instruction length decoder, and core runtime support.

- **[half](https://half.sourceforge.net/)** (MIT)  
  *Author*: Christian Rau  
  *Contribution*: IEEE 754-2008 compliant 16-bit half-precision floating point type for C++.

---

### Data Structures, Hashing & Utilities

- **[Dear ImGui](https://github.com/ocornut/imgui)** (MIT)  
  *Author*: Omar Cornut (ocornut)  
  *Contribution*: Immediate-mode graphical user interface for in-game configuration, profiling, and debug overlays.

- **[miniz](https://github.com/richgel999/miniz)** (MIT)  
  *Author*: Rich Geldreich and contributors  
  *Contribution*: Single-source lossless data compression library implementing zlib-compatible Deflate/Inflate and ZIP reading.

- **[tsl-robin-map](https://github.com/Tessil/robin-map)** (MIT)  
  *Author*: Thibaut Goetghebuer-Planchon  
  *Contribution*: Fast robin-hood hashing hash map and hash set implementations.

- **[magic_enum](https://github.com/Neargye/magic_enum)** (MIT)  
  *Author*: Daniil Goncharov  
  *Contribution*: Static reflection and string conversion for C++ enums.

- **[xxHash](https://github.com/Cyan4973/xxHash)** (BSD-2-Clause)  
  *Author*: Yann Collet  
  *Contribution*: Extremely fast non-cryptographic hash algorithm.

- **[fmt](https://github.com/fmtlib/fmt)** (MIT)  
  *Author*: Victor Zverovich and contributors  
  *Contribution*: Safe, fast, and modern formatting library for C++.

- **[DejaVu Fonts](https://dejavu-fonts.github.io/)** (Bitstream Vera / DejaVu License)  
  *Authors*: DejaVu Fonts team  
  *Contribution*: TrueType Cyrillic and Latin typography embedded for user interface text rendering.
