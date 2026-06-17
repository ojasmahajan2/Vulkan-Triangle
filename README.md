# Vulkan : Spinning RGB Triangle
A spinning, color‑interpolated triangle rendered with the **Vulkan 1.4** graphics API.  
Built around **RAII** principles using `<vulkan/vulkan_raii.hpp>` and `std::unique_ptr` custom deleters, so every GPU resource cleans itself up automatically in reverse creation order.
> [!NOTE]
> Uses **Dynamic Rendering** (Vulkan 1.3) instead of traditional render passes, and **Maintenance5** (Vulkan 1.4) features — no `VkRenderPass` or `VkFramebuffer` objects anywhere in the code.
---
## Tech Stack
| Library / API | Role |
|---|---|
| **Vulkan 1.4** | Graphics API — instance, device, swapchain, pipelines, command buffers |
| **SDL3** | Window creation, input polling, Vulkan surface, and real‑time resize via event watcher |
| **GLM** | Rotation matrix (`glm::rotate`) and MVP push constants |
| **VMA** (Vulkan Memory Allocator) | GPU memory allocation for the vertex buffer |
| **Slang** (NVIDIA) | Runtime compilation of HLSL shaders → SPIR‑V bytecode |
| **C++20** | `std::optional`, chrono literals, structured bindings |
---
## Project Structure
```
Vulkan-Triangle/
├── Source.cpp        # Application entry, Vulkan initialisation pipeline, main loop & rendering
├── Headers.h         # Consolidated includes (Vulkan, SDL3, GLM, Slang, STL)
├── Members.cpp       # Member variables, RAII wrappers (SDLWindowDeleter, VMAWrapper, VMABuffer)
├── Shaders.hlsl      # HLSL vertex & fragment shaders with push‑constant MVP matrix
├── CMakeLists.txt    # CMake 3.20 build configuration (C++20, MSVC‑oriented)
└── README.md
```
---
## Architecture Overview
The entire application lives in a single `Vulkan_TEST` struct. Initialisation follows the standard Vulkan bringup order:
```
initWindow()          SDL3 window + Vulkan surface + resize event watcher
    │
initVulkan()
    ├── createInstance()          Vulkan instance with validation layers
    ├── createSurface()           SDL‑backed VkSurfaceKHR
    ├── pickPhysicalDevice()      Prefers discrete GPU, falls back to first available
    ├── createLogicalDevice()     Graphics + present queues, enables fillModeNonSolid & largePoints
    ├── createAllocator()         VMA allocator (Vulkan 1.4 API version)
    ├── createSwapchain()         Swapchain with preferred format & present mode
    ├── createImageViews()        One image view per swapchain image
    ├── createGraphicsPipeline()  3 pipelines — Fill, Line (wireframe), Point (vertices)
    ├── createCommandObjects()    Command pool + command buffers
    ├── createVertexBuffer()      VMA‑allocated vertex buffer (position + color)
    └── createSyncObjects()       Semaphores & fences for frame synchronisation
    │
mainLoop()            Poll events → drawFrame() → FPS counter on title bar
```
### RAII & Resource Management
- **`SDLWindowDeleter`** — Custom deleter for `std::unique_ptr<SDL_Window>`, calls `SDL_DestroyWindow` + `SDL_Quit`
- **`VMAWrapper`** — Destructor calls `vmaDestroyAllocator`
- **`VMABuffer`** — Destructor calls `vmaDestroyBuffer`; copy operations are deleted
- All Vulkan handles use **`vk::raii::*`** types from the Vulkan‑Hpp RAII module
### Shaders (HLSL → SPIR‑V)
Shaders are written in **HLSL** and compiled at runtime to **SPIR‑V** bytecode via **Slang**:
| Stage | Entry | Description |
|---|---|---|
| Vertex | `vertexMain` | Transforms position by an MVP matrix passed as a **push constant** (`float4x4`), forwards per‑vertex colour, sets `SV_PointSize` |
| Fragment | `fragmentMain` | Outputs the interpolated colour |
### Three Graphics Pipelines
The application builds **three separate `VkPipeline` objects** at startup, one for each polygon mode:
| Key | Mode | Pipeline |
|---|---|---|
| `1` | **Point** | `VK_POLYGON_MODE_POINT` — renders only vertices as large points |
| `2` | **Wireframe** | `VK_POLYGON_MODE_LINE` — renders triangle edges |
| `3` | **Fill** *(default)* | `VK_POLYGON_MODE_FILL` — standard solid rendering |
---
## Prerequisites
- **Vulkan SDK 1.4+** (includes validation layers and Slang)
- **SDL3** development libraries
- **GLM** header‑only math library
- **VMA** (bundled with the Vulkan SDK or available separately)
- **CMake 3.20+**
- **C++20** compatible compiler (MSVC recommended; the CMakeLists.txt uses MSVC‑specific settings)
---
## Building from Source
```bash
# Clone the repository
git clone https://github.com/ojasmahajan2/Vulkan-Triangle.git
cd Vulkan-Triangle
# Configure & build (ensure VULKAN_SDK environment variable is set)
cmake -B build -S .
cmake --build build --config Release
```
> [!IMPORTANT]
> The `CMakeLists.txt` expects include paths at `$VULKAN_SDK/Include/slang` and link directories at `$VULKAN_SDK/Lib`.
> On non‑Windows platforms or with non‑MSVC compilers, you may need to adjust these paths and compiler definitions.
---
## Usage
> [!WARNING]
> The **`Shaders.hlsl`** file must be in the **same directory** as the executable (`Vulkan.exe`).
> Modifying the shader file may cause unexpected visuals or runtime exceptions.
```
your‑build‑folder/
├── Vulkan.exe
└── Shaders.hlsl      ← copy here if not already present
```
1. Run `Vulkan.exe` — a window with the spinning triangle will appear alongside a console window.
2. The title bar displays a live **FPS counter**.
3. The window is **resizable** in real time (handled via `SDL_AddEventWatch`).
### Controls
| Key | Action |
|---|---|
| `1` | Vertex point view |
| `2` | Wireframe view |
| `3` | Fill / normal view |
| `Esc` | Quit |
---
## Explanation
> [!NOTE]
> Uses `<vulkan.hpp>` and `<vulkan_raii.hpp>`, explicitly indicating the C++ API and its RAII benefits.
This project was inspired from the 'Rasterized Triangle' generally developed in Graphics Programming community by novices. Usually, using OpenGL/WebGL graphics API stands out to be an easy way to learn graphics programming, this might be a different approach to that since Vulkan and other graphics APIs like D3D12 (DirectX 3D) are explicit and low-level APIs. Vulkan is incredibly verbose, it requires you to start from selecting a suitable GPU and assigning it logical instruction type such as Compute (GPGPU) or Graphics. Unlike developing on OpenGL, this project is more sophisticated and requires explicit code when using Vulkan which is why it is faster and performant.
To differentiate and expand some perspective, I added 2 new graphics pipelines to draw on the window surface such as Vertex Point and Wireframe besides Fill. Used RAII (Resource Acquisition is Initialisation) from Vulkan RAII and `std::unique_ptr` techniques as well as destructors for dynamic cleanup. Also added an FPS counter on Title bar.
**SDL3**: Window creation, Input Polling, Window surface creation for Vulkan and Event Watch (more real-time resizing impact on the spinning triangle) · **GLM**: Only for rotation (generally used for graphical maths) and basic position + color attributes · **VMA**: Allocating memory · **Slang**: Runtime compilation of HLSL shaders to SPIR-V Bytecode.
---
## Platform Notes
The source code is **100% platform‑independent** thanks to SDL3 and the cross‑platform Vulkan API. However, the current `CMakeLists.txt` build configuration is MSVC‑oriented and may require adjustments for GCC/Clang or non‑Windows platforms (library paths, compiler definitions).
A pre‑built Windows executable is provided in the [Releases](https://github.com/ojasmahajan2/Vulkan-Triangle/releases) section.
---
## Languages
| Language | Share |
|---|---|
| C++ | 93.6% |
| CMake | 4.0% |
| HLSL | 2.4% |
