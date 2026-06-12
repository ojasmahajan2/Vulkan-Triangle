cmake_minimum_required(VERSION 3.20)

project(Vulkan LANGUAGES CXX)

# C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Executable
add_executable(Vulkan
    main.cpp
)

# Vulkan SDK
find_package(Vulkan REQUIRED)

target_include_directories(Vulkan PRIVATE
    $ENV{VULKAN_SDK}/Include/slang
)

target_link_directories(Vulkan PRIVATE
    $ENV{VULKAN_SDK}/Lib
)

target_link_libraries(Vulkan PRIVATE
    Vulkan::Vulkan
    sdl3
    slang
)

# Compiler definitions
target_compile_definitions(Vulkan PRIVATE
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
    _CONSOLE
)

# MSVC-specific settings
if (MSVC)
    target_compile_options(Vulkan PRIVATE
        /W3
        /permissive-
    )

    # Enable SDL checks
    target_compile_options(Vulkan PRIVATE /sdl)

    # Release optimizations
    target_compile_options(Vulkan PRIVATE
        $<$<CONFIG:Release>:/Oi>
        $<$<CONFIG:Release>:/Gy>
    )
endif()