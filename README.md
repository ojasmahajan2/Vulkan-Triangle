# Vulkan : Spinning RGB Triangle

A spinning, color-interpolated triangle rendered with the **Vulkan 1.4** graphics API.  
Built around **RAII** principles using `<vulkan/vulkan_raii.hpp>` and custom `std::unique_ptr` deleters for automatic, deterministic resource cleanup.

> [!NOTE]
> Uses **Dynamic Rendering** (Vulkan 1.3) and **Maintenance5** (Vulkan 1.4) features — there are no `VkRenderPass` or `VkFramebuffer` objects in the codebase.

---

## Tech Stack

- **Vulkan 1.4**: Graphics API (dynamic rendering, sync, pipelines)
- **SDL3**: Window management, input polling, and real-time resize events
- **GLM**: Matrix math and MVP push constants
- **VMA**: Vulkan Memory Allocator for vertex buffers
- **Slang**: Runtime compilation of HLSL shaders to SPIR-V bytecode
- **C++20**: RAII, structured bindings, and modern STL features

---

## Building from Source

**Requirements:** Vulkan SDK 1.4+ (with Slang and VMA), SDL3, GLM, CMake 3.20+, and a C++20 compiler.

```bash
git clone https://github.com/ojasmahajan2/Vulkan-Triangle.git
cd Vulkan-Triangle
cmake -B build -S .
cmake --build build --config Release
```

> [!IMPORTANT]  
> The `CMakeLists.txt` expects include paths at `$VULKAN_SDK/Include/slang` and link directories at `$VULKAN_SDK/Lib`. You may need to adjust these paths for your environment.

---

## Usage & Controls

The **`Shaders.hlsl`** file must be in the same directory as the compiled executable (`Vulkan.exe`).

When launched, you'll see a spinning triangle and a live FPS counter in the title bar. The application renders in Fill mode by default.

### Key Bindings

| Key | Action |
|---|---|
| `Esc` | Quit Application |
| `F3` | Toggle **Debug Mode** (unlocks advanced rendering controls) |

**Debug Mode Controls:**
| Key | Action |
|---|---|
| `1` | Switch to Point view (vertices only) |
| `2` | Switch to Wireframe view (edges only) |
| `3` | Switch to Fill view (solid, default) |
| `Shift` + `1/2/3` | Toggle Point/Wireframe/Fill pipeline (layer multiple modes simultaneously) |
| `V` | Toggle V-Sync on/off (Immediate vs. FIFO present mode) |

---

## Explanation

This project was inspired by the classic "Rasterized Triangle" typically built by novices learning graphics programming. While APIs like OpenGL and WebGL provide a gentler learning curve, explicit, low-level APIs like Vulkan and Direct3D 12 offer greater control and the potential for higher performance. Developing with Vulkan is highly verbose, requiring you to manually select a suitable GPU, set up logical queues (e.g., Graphics or Compute), and explicitly manage resources and memory.

To expand on the standard triangle, this project includes two additional graphics pipelines—Vertex Point and Wireframe—alongside the default Fill mode. It leverages C++20 and the `vulkan_raii.hpp` module to ensure all Vulkan handles and memory allocations are automatically and safely destroyed.

---

## Platform Notes

The source code is platform-independent via SDL3 and Vulkan. The current `CMakeLists.txt` build configuration is MSVC-oriented but can be adapted for GCC/Clang.  
