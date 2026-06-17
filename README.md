# Vulkan : Rotating RGB Triangle
**Development Prerequisites** : 
  -   Vulkan 1.4 (SDK and Graphics API)
  -   SDL3 (Window management)
  -   GLM (Graphics Mathematics)
  -   VMA (Vulkan Memory Allocator)
  -   Slang (Shader compilation)
  -   **C++ 20 or above**

## How to use?
> [!WARNING]
> The `Shaders.hlsl` file needs to be in the same directory as the application (`Vulkan.exe`).
> <br>Making changes to Shader file may lead to unexpected visuals in window or exceptions in console.

Apparently, the executable for Windows is provided. The source code is 100% platform independent, thanks to libraries like SDL3 and Vulkan itself. Although, `CMakeLists.txt` build configuration does require some changes due to its OS specific nature in some parts and its MSVC inclusiveness.<br>
- Run the executable `Vulkan.exe` and find the window besides the console one.
### Features
- Vertex point View [Press '1' Key]
- Wireframe View [Press '2' Key]
- Fill or Normal [Press '3' Key]

## Explanation
> [!NOTE]
> Uses `<Vulkan.hpp>` and `<Vulkan_raii.hpp>`, explicitly indicating the C++ API and its RAII benefits.

This project was inspired from the 'Rasterized Triangle' generally developed in Graphics Programming community by novices. Usually, using OpenGL/WebGL graphics API stands out to be an easy way to learn graphics programming, this might be a different approach to that since Vulkan and other graphics APIs like D3D12 (DirectX 3D) are explicit and low-level APIs. Vulkan is incredibly verbose, it requires you to start from selecting a suitable GPU and assigning it logical instruction type such as Compute (GPGPU) or Graphics. Unlike developing on OpenGL, this project is more sophisticated and requires explicit code when using Vulkan which is why it is faster and performant.

To differentiate and expand some perspective, I added 2 new graphics pipelines to draw on the window surface such as Vertex Point and Wireframe besides Fill. Used RAII (Resource Acquisition is Initialisation) from Vulkan RAII and `std::unique_ptr` techniques as well as destructors for dynamic cleanup. Also added an FPS counter on Title bar.

### Components in the Application structure
**SDL3:** Window creation, Input Polling, Window surface creation for Vulkan and Event Watch (more real-time resizing impact on the spinning triangle)<br>
**GLM:** Only for rotation (generally used for graphical maths) and basic position + color attributes.<br>
**VMA:** Allocating memory<br>
**Slang:** Runtime compilation of HLSL shaders to SPIR-V Bytecode.
