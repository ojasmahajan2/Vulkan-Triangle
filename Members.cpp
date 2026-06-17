struct SDLWindowDeleter {
	void operator()(SDL_Window* w) const {
		if (w) SDL_DestroyWindow(w);
		SDL_Quit();
		std::cout << "SDL3 Utility Cleared!\n";
	}
};
std::unique_ptr<SDL_Window, SDLWindowDeleter> window = nullptr;

vk::raii::Context context;
vk::raii::Instance instance = nullptr;
vk::raii::SurfaceKHR surface = nullptr;
vk::raii::PhysicalDevice physicalDevice = nullptr;

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete() const { return graphicsFamily.has_value() && presentFamily.has_value(); }
};
QueueFamilyIndices indices;

vk::raii::Device device = nullptr;
vk::raii::Queue graphicsQueue = nullptr;
vk::raii::Queue presentQueue = nullptr;

struct VMAWrapper {
	VmaAllocator handle = nullptr;
	~VMAWrapper() {
		if (handle) {
			vmaDestroyAllocator(handle);
			std::cout << "VMA Allocator Cleared!\n";
		}
	}
};
VMAWrapper allocator;

struct VMABuffer {
	VkBuffer buffer = nullptr;
	VmaAllocation allocation = nullptr;
	VmaAllocator allocator = nullptr;

	VMABuffer() = default;

	VMABuffer(const VMABuffer&) = delete;
	VMABuffer& operator=(const VMABuffer&) = delete;

	~VMABuffer() {
		if (buffer && allocation && allocator) {
			vmaDestroyBuffer(allocator, buffer, allocation);
			std::cout << "RAII: VMA Buffer safely Cleared!\n";
		}
	}
};
VMABuffer vertexBuffer;

vk::raii::SwapchainKHR swapchain = nullptr;
vk::Format swapchainImageFormat;
vk::Extent2D swapchainExtent;
std::vector<vk::Image> swapchainImages;
std::vector<vk::raii::ImageView> swapchainImageViews;
vk::raii::PipelineLayout pipelineLayout = nullptr;
vk::raii::Pipeline graphicsPipeline = nullptr;
vk::raii::Pipeline wireframePipeline = nullptr;
vk::raii::Pipeline pointPipeline = nullptr;
vk::raii::CommandPool commandPool = nullptr;

const uint32_t MAX_FRAMES_IN_FLIGHT = 2;
uint32_t currentFrame = 0;
bool framebufferResized = false;

std::vector<vk::raii::CommandBuffer> commandBuffers;
std::vector<vk::raii::Semaphore> imageAvailableSemaphores;
std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
std::vector<vk::raii::Fence> inFlightFences;

const std::string SHADERS = "Shaders.hlsl";

/*______________________________________________________________*/
/*•••••••••••••••••••• App Design Variables ••••••••••••••••••••*/

const uint32_t WIDTH = 500;
const uint32_t HEIGHT = 500;
const char* const TITLE = "Vulkan Triangle";

enum PipelineFlags : uint8_t {
	PIPELINE_NONE	   = 0,
	PIPELINE_FILL	   = 1 << 0,
	PIPELINE_WIREFRAME = 1 << 1,
	PIPELINE_POINT	   = 1 << 2
};
uint8_t activePipelines = PIPELINE_FILL;

std::array<float, 4> backgroundColor = { 0.01f, 0.01f, 0.03f, 1.0f };

bool debugModeState = false;
bool vsyncToggle = true;
enum class VSyncSettings {
	VSyncOFF = vk::PresentModeKHR::eImmediate,
	VSyncON = vk::PresentModeKHR::eFifo
};
VSyncSettings VSyncMode = VSyncSettings::VSyncON;

/*______________________________________________________________*/
/*•••••••••••••••• Graphics management Variables •••••••••••••••*/

struct pushConstantData {
	glm::mat4 mvp;
};

struct Vertex {
	glm::vec2 position;
	glm::vec3 color;

	static vk::VertexInputBindingDescription getBindingDescription() {
		return vk::VertexInputBindingDescription(0, sizeof(Vertex), vk::VertexInputRate::eVertex);
	}

	static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions() {
		std::array<vk::VertexInputAttributeDescription, 2> attributes = {};
		attributes[0] = vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, position));
		attributes[1] = vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color));
		return attributes;
	}
};

const std::vector<Vertex> vertices = {
	{{ 0.5f, -0.5f},	{1.0f, 0.0f, 0.0f}},
	{{ 0.5f,  0.5f},	{0.0f, 1.0f, 0.0f}},
	{{-0.5f,  0.5f},	{0.0f, 0.0f, 1.0f}},
	{{ 0.0f, -0.5f},	{1.0f, 1.0f, 1.0f}}
};

std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
