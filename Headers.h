#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <SDL3/SDL.h>
#include <SDL3/SDL_Vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <slang.h>
#include <slang-com-ptr.h>
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#include <iostream>
#include <stdexcept>
#include <vector>
#include <string>
#include <memory>
#include <optional>
#include <set>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <limits>
#include <chrono>