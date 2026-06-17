#include "Headers.h"

using namespace std::chrono_literals;

struct Vulkan_TEST {
public:
	void run() {
		initWindow();
		initVulkan();
		mainLoop();

		device.waitIdle();
	}

private:
	//=====================================================================================================================
	#include "Members.cpp"
	//=====================================================================================================================

	void initWindow() {
		if (!SDL_Init(SDL_INIT_VIDEO)) {
			throw std::runtime_error("Failed to initialize SDL3: " + std::string(SDL_GetError()));
		}

		window.reset(SDL_CreateWindow(
			TITLE, WIDTH, HEIGHT,
			SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY
		));

		if (!window) {
			throw std::runtime_error("Failed to create a window: " + std::string(SDL_GetError()));
		}

		SDL_SetWindowResizable(window.get(), true);
		SDL_AddEventWatch(resizeEventWatcher, this);
	}

	static bool resizeEventWatcher(void* userdata, SDL_Event* event) {
		if (event->type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
			auto* app = static_cast<Vulkan_TEST*>(userdata);
			app->framebufferResized = true;
			app->drawFrame();
		}
		return true;
	}

	void initVulkan() {
		createInstance();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createAllocator();
		createSwapchain();
		createImageViews();
		createGraphicsPipeline();
		createCommandObjects();
		createVertexBuffer();
		createSyncObjects();
	}

	void mainLoop() {
		bool running = true;
		SDL_Event event;

		int numkeys;
		const bool* state = SDL_GetKeyboardState(&numkeys);

		auto lastTime = std::chrono::steady_clock::now();
		uint32_t frameCount = 0;
		uint32_t FPS = 0;

		while (running) {
			while (SDL_PollEvent(&event)) {
				if (event.type == SDL_EVENT_QUIT || state[SDL_SCANCODE_ESCAPE]) running = false;

				inputWindow(event, state);
			}

			drawFrame();

			manageFPS(lastTime, frameCount, FPS);

			std::string updatedTitle = std::string(TITLE) + " | FPS: " + std::to_string(FPS);
			SDL_SetWindowTitle(window.get(), updatedTitle.c_str());
		}

	}

	//=====================================================================================================================

	void createInstance() {
		vk::ApplicationInfo appInfo(
			TITLE,
			VK_MAKE_VERSION(1, 0, 0),
			"No Engine",
			VK_MAKE_VERSION(1, 0, 0),
			VK_API_VERSION_1_4
		);


		uint32_t sdlExtensionCount = 0;
		const char* const* sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);

		if (!sdlExtensions) {
			throw std::runtime_error("Failed to get SDL Vulkan extensions: " + std::string(SDL_GetError()));
		}

		std::vector<const char*> extensions(sdlExtensions, sdlExtensions + sdlExtensionCount);
		std::vector<const char*> validationLayers = {
			"VK_LAYER_KHRONOS_validation"
		};

		vk::InstanceCreateInfo createInfo(
			{},
			&appInfo,
			validationLayers,
			extensions
		);

		instance = vk::raii::Instance(context, createInfo);
	}

	void createSurface() {
		VkSurfaceKHR c_surface;

		if (!SDL_Vulkan_CreateSurface(window.get(), *instance, nullptr, &c_surface)) {
			throw std::runtime_error("Failed to create SDL Vulkan Surface: " + std::string(SDL_GetError()));
		}

		surface = vk::raii::SurfaceKHR(instance, c_surface);
	}

	void pickPhysicalDevice() {
		vk::raii::PhysicalDevices devices(instance);
		if (devices.empty()) {
			throw std::runtime_error("Failed to find GPUs with Vulkan support!");
		}

		std::cout << "Found " << devices.size() << " Vulkan compatible GPUs!\n";

		for (const auto& device : devices) {
			vk::PhysicalDeviceProperties properties = device.getProperties();

			if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
				physicalDevice = device;
				std::cout << "Selected GPU: " << properties.deviceName << "\n";
				break;
			}
		}

		if (!*physicalDevice) {
			physicalDevice = devices.front();
			std::cout << "Selected fallback GPU: " << physicalDevice.getProperties().deviceName << "\n";
		}
	}

	void createLogicalDevice() {
		std::vector<vk::QueueFamilyProperties> queueFamilies = physicalDevice.getQueueFamilyProperties();

		int i = 0;
		for (const auto& queueFamily : queueFamilies) {
			if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
				indices.graphicsFamily = i;
			}

			if (physicalDevice.getSurfaceSupportKHR(i, *surface)) {
				indices.presentFamily = i;
			}
			if (indices.isComplete()) break;
			i++;
		}

		if (!indices.isComplete()) {
			throw std::runtime_error("Failed to find queue families!");
		}

		std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies) {
			queueCreateInfos.push_back(vk::DeviceQueueCreateInfo(
				{}, queueFamily, 1, &queuePriority
			));
		}

		std::vector<const char*> deviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		vk::PhysicalDeviceVulkan14Features features14;
		features14.maintenance5 = VK_TRUE;
		vk::PhysicalDeviceVulkan13Features features13;
		features13.dynamicRendering = VK_TRUE;
		features13.pNext = &features14;

		vk::PhysicalDeviceVulkan11Features features11;
		features11.shaderDrawParameters = VK_TRUE;
		features11.pNext = &features13;

		vk::PhysicalDeviceFeatures2 deviceFeatures;
		deviceFeatures.features.fillModeNonSolid = VK_TRUE;
		deviceFeatures.features.largePoints = VK_TRUE;
		deviceFeatures.pNext = &features11;

		vk::DeviceCreateInfo createInfo(
			{}, queueCreateInfos, {}, deviceExtensions, nullptr
		);
		createInfo.pNext = &deviceFeatures;

		device = vk::raii::Device(physicalDevice, createInfo);

		graphicsQueue = vk::raii::Queue(device, indices.graphicsFamily.value(), 0);
		presentQueue = vk::raii::Queue(device, indices.presentFamily.value(), 0);
	}

	void createAllocator() {
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_4;
		allocatorInfo.physicalDevice = *physicalDevice;
		allocatorInfo.device = *device;
		allocatorInfo.instance = *instance;

		if (vmaCreateAllocator(&allocatorInfo, &allocator.handle) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create VMA Allocator!");
		}
	}

	std::string readShaderFile(const std::string& filename) {
		std::ifstream shadersFile(filename);
		if (!shadersFile.is_open()) {
			throw std::runtime_error("Failed to open Shader file: " + filename);
		}

		std::stringstream buffer;
		buffer << shadersFile.rdbuf();
		return buffer.str();
	}

	std::vector<uint32_t> compileShadersToSPIRV(const std::string& hlslSource, const char* entryPointName) {
		Slang::ComPtr<slang::IGlobalSession> globalSession;
		slang::createGlobalSession(globalSession.writeRef());

		slang::TargetDesc targetDesc = {};
		targetDesc.format = SLANG_SPIRV;
		targetDesc.profile = globalSession->findProfile("sm_6_5");

		slang::SessionDesc sessionDesc = {};
		sessionDesc.targets = &targetDesc;
		sessionDesc.targetCount = 1;

		Slang::ComPtr<slang::ISession> session;
		globalSession->createSession(sessionDesc, session.writeRef());

		Slang::ComPtr<slang::IBlob> diagnosticBlob;

		slang::IModule* module = session->loadModuleFromSourceString(
			"Shaders",
			"path.hlsl",
			hlslSource.c_str(),
			diagnosticBlob.writeRef()
		);

		if (diagnosticBlob) {
			std::cout << "Slang Compiler message: " << (const char*)diagnosticBlob->getBufferPointer() << "\n";
		}
		if (!module) {
			throw std::runtime_error("Failed to load Slang shader module!");
		}

		Slang::ComPtr<slang::IEntryPoint> entryPoint;
		module->findEntryPointByName(entryPointName, entryPoint.writeRef());

		if (!entryPoint) {
			throw std::runtime_error("Failed to find entry point '" + std::string(entryPointName) + "' in HLSL file!");
		}

		std::vector<slang::IComponentType*> components = { module, entryPoint };
		Slang::ComPtr<slang::IComponentType> program;
		session->createCompositeComponentType(
			components.data(), components.size(), program.writeRef(), diagnosticBlob.writeRef()
		);

		Slang::ComPtr<slang::IBlob> spirvBlob;
		program->getEntryPointCode(0, 0, spirvBlob.writeRef(), diagnosticBlob.writeRef());

		if (!spirvBlob) {
			throw std::runtime_error("Failed to generate SPIR-V bytecode!");
		}

		const uint32_t* codeStart = (const uint32_t*)spirvBlob->getBufferPointer();
		size_t codeSize = spirvBlob->getBufferSize() / sizeof(uint32_t);

		std::cout << (entryPointName == "vertexMain" ? "Vertex" : "Fragment") << " shader compilation successful!\n";

		return std::vector<uint32_t>(codeStart, codeStart + codeSize);
	}

	void createSwapchain() {
		vk::SurfaceCapabilitiesKHR capabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface);
		std::vector<vk::SurfaceFormatKHR> formats = physicalDevice.getSurfaceFormatsKHR(*surface);
		std::vector<vk::PresentModeKHR> presentModes = physicalDevice.getSurfacePresentModesKHR(*surface);

		vk::SurfaceFormatKHR surfaceFormat = formats[0];
		for (const auto& availableFormat : formats) {
			if (availableFormat.format == vk::Format::eB8G8R8A8Srgb &&
				availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
				surfaceFormat = availableFormat;
				break;
			}
		}
		swapchainImageFormat = surfaceFormat.format;

		vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;
		for (const auto& availablePresentMode : presentModes) {
			if (availablePresentMode == static_cast<vk::PresentModeKHR>(VSyncMode)) {
				presentMode = availablePresentMode;
				break;
			}
		}

		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			swapchainExtent = capabilities.currentExtent;
		}
		else {
			swapchainExtent = vk::Extent2D{ WIDTH, HEIGHT };
			swapchainExtent.width = std::clamp(swapchainExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			swapchainExtent.height = std::clamp(swapchainExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
		}

		uint32_t imageCount = capabilities.minImageCount + 1;
		if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
			imageCount = capabilities.maxImageCount;
		}

		vk::SwapchainCreateInfoKHR createInfo(
			{}, *surface, imageCount,
			surfaceFormat.format, surfaceFormat.colorSpace,
			swapchainExtent, 1, vk::ImageUsageFlagBits::eColorAttachment
		);

		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };
		if (indices.graphicsFamily != indices.presentFamily) {
			createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			createInfo.imageSharingMode = vk::SharingMode::eExclusive;
		}

		createInfo.preTransform = capabilities.currentTransform;
		createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;

		swapchain = vk::raii::SwapchainKHR(device, createInfo);
		swapchainImages = swapchain.getImages();
	}

	void createImageViews() {
		swapchainImageViews.reserve(swapchainImages.size());

		for (vk::Image image : swapchainImages) {
			vk::ImageViewCreateInfo createInfo(
				{}, image, vk::ImageViewType::e2D, swapchainImageFormat,
				{}, { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
			);

			swapchainImageViews.emplace_back(device, createInfo);
		}
	}

	void createGraphicsPipeline() {
		std::string shaderCode = readShaderFile(SHADERS);
		std::vector<uint32_t> vertSpirv = compileShadersToSPIRV(shaderCode, "vertexMain");
		std::vector<uint32_t> fragSpirv = compileShadersToSPIRV(shaderCode, "fragmentMain");

		vk::ShaderModuleCreateInfo vertInfo({}, vertSpirv);
		vk::raii::ShaderModule vertModule(device, vertInfo);

		vk::ShaderModuleCreateInfo fragInfo({}, fragSpirv);
		vk::raii::ShaderModule fragModule(device, fragInfo);

		vk::PipelineShaderStageCreateInfo shaderStages[] = {
			{{}, vk::ShaderStageFlagBits::eVertex, *vertModule, "main"},
			{{}, vk::ShaderStageFlagBits::eFragment, *fragModule, "main"}
		};

		std::vector<vk::DynamicState> dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
		vk::PipelineDynamicStateCreateInfo dynamicStateInfo({}, dynamicStates);
		vk::PipelineViewportStateCreateInfo viewportState({}, 1, nullptr, 1, nullptr);

		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();
		vk::PipelineVertexInputStateCreateInfo vertexInputInfo(
			{}, 1, &bindingDescription,
			static_cast<uint32_t>(attributeDescriptions.size()), attributeDescriptions.data()
		);
		vk::PipelineInputAssemblyStateCreateInfo inputAssembly({}, vk::PrimitiveTopology::eTriangleList, VK_FALSE);

		vk::PipelineRasterizationStateCreateInfo rasterizer(
			{}, VK_FALSE, VK_FALSE, vk::PolygonMode::eFill,
			vk::CullModeFlagBits::eNone, vk::FrontFace::eClockwise,
			VK_FALSE, 0.0f, 0.0f, 0.0f, 1.0f
		);

		vk::PipelineMultisampleStateCreateInfo multisampling({}, vk::SampleCountFlagBits::e1, VK_FALSE);
		vk::PipelineColorBlendAttachmentState colorBlendAttachment(
			VK_FALSE, vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
			vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
			vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
		);
		vk::PipelineColorBlendStateCreateInfo colorBlending({}, VK_FALSE, vk::LogicOp::eCopy, 1, &colorBlendAttachment);

		vk::PushConstantRange pushRange(vk::ShaderStageFlagBits::eVertex, 0, sizeof(pushConstantData));
		vk::PipelineLayoutCreateInfo pipelineLayoutInfo({}, 0, nullptr, 1, &pushRange);
		pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);

		vk::PipelineRenderingCreateInfo pipelineRenderingInfo(0, 1, &swapchainImageFormat);

		vk::GraphicsPipelineCreateInfo pipelineInfo(
			{}, 2, shaderStages, &vertexInputInfo, &inputAssembly, nullptr,
			&viewportState, &rasterizer, &multisampling, nullptr, &colorBlending,
			&dynamicStateInfo, *pipelineLayout, nullptr
		);

		pipelineInfo.pNext = &pipelineRenderingInfo;

		graphicsPipeline = vk::raii::Pipeline(device, nullptr, pipelineInfo);

		rasterizer.polygonMode = vk::PolygonMode::eLine;
		wireframePipeline = vk::raii::Pipeline(device, nullptr, pipelineInfo);

		rasterizer.polygonMode = vk::PolygonMode::ePoint;
		pointPipeline = vk::raii::Pipeline(device, nullptr, pipelineInfo);
	}

	void createCommandObjects() {
		vk::CommandPoolCreateInfo poolInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, indices.graphicsFamily.value());
		commandPool = vk::raii::CommandPool(device, poolInfo);

		vk::CommandBufferAllocateInfo allocInfo(*commandPool, vk::CommandBufferLevel::ePrimary, MAX_FRAMES_IN_FLIGHT);

		vk::raii::CommandBuffers buffers(device, allocInfo);
		for (uint8_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			commandBuffers.push_back(std::move(buffers[i]));
		}
	}

	void createVertexBuffer() {
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

		VMABuffer stagingBuffer;
		stagingBuffer.allocator = allocator.handle;

		VkBufferCreateInfo stagingBufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		stagingBufferInfo.size = bufferSize;
		stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		VmaAllocationCreateInfo stagingAllocInfo = {};
		stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
		stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

		VmaAllocationInfo stagingAllocationInfo;
		vmaCreateBuffer(allocator.handle, &stagingBufferInfo, &stagingAllocInfo, &stagingBuffer.buffer, &stagingBuffer.allocation, &stagingAllocationInfo);

		memcpy(stagingAllocationInfo.pMappedData, vertices.data(), (size_t)bufferSize);

		vertexBuffer.allocator = allocator.handle;

		VkBufferCreateInfo vertexBufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		vertexBufferInfo.size = bufferSize;
		vertexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

		VmaAllocationCreateInfo vertexAllocInfo = {};
		vertexAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;

		vmaCreateBuffer(allocator.handle, &vertexBufferInfo, &vertexAllocInfo, &vertexBuffer.buffer, &vertexBuffer.allocation, nullptr);

		vk::CommandBufferAllocateInfo allocInfo(*commandPool, vk::CommandBufferLevel::ePrimary, 1);
		vk::raii::CommandBuffer transferCmd = std::move(vk::raii::CommandBuffers(device, allocInfo).front());

		transferCmd.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
		vk::BufferCopy copyRegion(0, 0, bufferSize);
		transferCmd.copyBuffer(stagingBuffer.buffer, vertexBuffer.buffer, copyRegion);
		transferCmd.end();

		vk::SubmitInfo submitInfo(0, nullptr, nullptr, 1, &*transferCmd, 0, nullptr);
		graphicsQueue.submit(submitInfo, nullptr);

		graphicsQueue.waitIdle();

		std::cout << "Vertex Buffer successfully uploaded to GPU VRAM!\n";
	}

	void createSyncObjects() {
		vk::SemaphoreCreateInfo semaphoreInfo;
		vk::FenceCreateInfo fenceInfo(vk::FenceCreateFlagBits::eSignaled);

		for (uint8_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			imageAvailableSemaphores.emplace_back(device, semaphoreInfo);
			inFlightFences.emplace_back(device, fenceInfo);
		}

		for (size_t i = 0; i < swapchainImages.size(); i++) {
			renderFinishedSemaphores.emplace_back(device, semaphoreInfo);
		}
	}

	void recreateSwapchain() {
		int width = 0, height = 0;
		SDL_GetWindowSizeInPixels(window.get(), &width, &height);
		while (width == 0 || height == 0) {
			SDL_GetWindowSizeInPixels(window.get(), &width, &height);
			SDL_WaitEvent(nullptr);
		}

		device.waitIdle();

		swapchainImageViews.clear();
		swapchain.clear();
		imageAvailableSemaphores.clear();
		renderFinishedSemaphores.clear();
		inFlightFences.clear();

		createSwapchain();
		createImageViews();
		createSyncObjects();
		currentFrame = 0;
	}

	/*______________________________________________________________*/
	/*•••••••••••••••••••• App Design Functions ••••••••••••••••••••*/

	void drawFrame() {
		auto waitResult = device.waitForFences({ *inFlightFences[currentFrame] }, VK_TRUE, UINT64_MAX);
		if (waitResult != vk::Result::eSuccess) {
			throw std::runtime_error("Failed to wait for fence!");
		}

		uint32_t imageIndex;
		try {
			auto [result, index] = swapchain.acquireNextImage(UINT64_MAX, *imageAvailableSemaphores[currentFrame], nullptr);
			imageIndex = index;

			if (result == vk::Result::eSuboptimalKHR || framebufferResized) {
				framebufferResized = false;
				recreateSwapchain();
				auto [result2, index2] = swapchain.acquireNextImage(UINT64_MAX, *imageAvailableSemaphores[currentFrame], nullptr);
				imageIndex = index2;
			}
		}
		catch (vk::OutOfDateKHRError&) {
			framebufferResized = false;
			recreateSwapchain();
			auto [result2, index2] = swapchain.acquireNextImage(UINT64_MAX, *imageAvailableSemaphores[currentFrame], nullptr);
			imageIndex = index2;
		}

		device.resetFences({ *inFlightFences[currentFrame] });

		commandBuffers[currentFrame].reset();
		commandBuffers[currentFrame].begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

		vk::ImageMemoryBarrier drawBarrier(
			{}, vk::AccessFlagBits::eColorAttachmentWrite,
			vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
			swapchainImages[imageIndex],
			{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
		);
		commandBuffers[currentFrame].pipelineBarrier(
			vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput,
			{}, nullptr, nullptr, drawBarrier
		);

		vk::RenderingAttachmentInfo colorAttachment(
			*swapchainImageViews[imageIndex], vk::ImageLayout::eColorAttachmentOptimal,
			vk::ResolveModeFlagBits::eNone, nullptr, vk::ImageLayout::eUndefined,
			vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
			vk::ClearValue(vk::ClearColorValue(backgroundColor))
		);

		vk::RenderingInfo renderingInfo({}, vk::Rect2D({ 0, 0 }, swapchainExtent), 1, 0, 1, &colorAttachment);

		commandBuffers[currentFrame].beginRendering(renderingInfo);

		vk::Viewport viewport(0.0f, 0.0f, (float)swapchainExtent.width, (float)swapchainExtent.height, 0.0f, 1.0f);
		commandBuffers[currentFrame].setViewport(0, viewport);
		vk::Rect2D scissor({ 0, 0 }, swapchainExtent);
		commandBuffers[currentFrame].setScissor(0, scissor);

		commandBuffers[currentFrame].bindVertexBuffers(0, { vertexBuffer.buffer }, { 0 });

		if (activePipelines & PIPELINE_FILL) {
			commandBuffers[currentFrame].bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);
			commandBuffers[currentFrame].draw(3, 1, 0, 0);
		}
		if (activePipelines & PIPELINE_WIREFRAME) {
			commandBuffers[currentFrame].bindPipeline(vk::PipelineBindPoint::eGraphics, *wireframePipeline);
			commandBuffers[currentFrame].draw(3, 1, 0, 0);
		}
		if (activePipelines & PIPELINE_POINT) {
			commandBuffers[currentFrame].bindPipeline(vk::PipelineBindPoint::eGraphics, *pointPipeline);
			commandBuffers[currentFrame].draw(3, 1, 0, 0);
		}

		commandBuffers[currentFrame].endRendering();

		vk::ImageMemoryBarrier presentBarrier(
			vk::AccessFlagBits::eColorAttachmentWrite, {},
			vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
			swapchainImages[imageIndex],
			{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
		);
		commandBuffers[currentFrame].pipelineBarrier(
			vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe,
			{}, nullptr, nullptr, presentBarrier
		);

		commandBuffers[currentFrame].end();

		vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
		vk::SubmitInfo submitInfo(
			1, &*imageAvailableSemaphores[currentFrame], waitStages,
			1, &*commandBuffers[currentFrame],
			1, &*renderFinishedSemaphores[imageIndex]
		);
		graphicsQueue.submit(submitInfo, *inFlightFences[currentFrame]);

		vk::PresentInfoKHR presentInfo(
			1, &*renderFinishedSemaphores[imageIndex],
			1, &*swapchain,
			&imageIndex
		);

		try {
			vk::Result presentResult = presentQueue.presentKHR(presentInfo);
			if (presentResult == vk::Result::eSuboptimalKHR || framebufferResized) {
				framebufferResized = false;
				recreateSwapchain();
			}
		}
		catch (vk::OutOfDateKHRError&) {
			framebufferResized = false;
			recreateSwapchain();
		}

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void inputWindow(SDL_Event& event, const bool*& state) {
		if (event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_F3 && !event.key.repeat) {
			debugModeState = !debugModeState;
			if (debugModeState) {
				std::cout << "debug mode [ON]" << std::endl;
			}
			else {
				std::cout << "debug mode [OFF]" << std::endl;
			}
		}
		if (debugModeState) debugMode(event, state);
		else {
			backgroundColor = { 0.01f, 0.01f, 0.02f, 1.0f };
			activePipelines = PIPELINE_FILL;
		}
	}

	void debugMode(SDL_Event& event, const bool*& state) {
		backgroundColor = { 0.005f, 0.005f, 0.005f, 1.0f };

		if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
			bool shiftHeld = (event.key.mod & SDL_KMOD_SHIFT);

			if (event.key.scancode == SDL_SCANCODE_1) {
				if (shiftHeld) {
					activePipelines ^= PIPELINE_POINT;
					if (activePipelines == PIPELINE_NONE) activePipelines = PIPELINE_POINT;
				}
				else {
					activePipelines = PIPELINE_POINT;
				}
			}

			else if (event.key.scancode == SDL_SCANCODE_2) {
				if (shiftHeld) {
					activePipelines ^= PIPELINE_WIREFRAME;
					if (activePipelines == PIPELINE_NONE) activePipelines = PIPELINE_WIREFRAME;
				}
				else {
					activePipelines = PIPELINE_WIREFRAME;
				}
			}

			else if (event.key.scancode == SDL_SCANCODE_3) {
				if (shiftHeld) {
					activePipelines ^= PIPELINE_FILL;
					if (activePipelines == PIPELINE_NONE) activePipelines = PIPELINE_FILL;
				}
				else {
					activePipelines = PIPELINE_FILL;
				}
			}
		}

		if (event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_V && !event.key.repeat) {
			vsyncToggle = !vsyncToggle;

			if (vsyncToggle) {
				VSyncMode = VSyncSettings::VSyncON;
				std::cout << "V-Sync [ON]\n";
			}
			else {
				VSyncMode = VSyncSettings::VSyncOFF;
				std::cout << "V-Sync [OFF]\n";
			}

			recreateSwapchain();
		}
	}

	void manageFPS(auto& lastTime, uint32_t& frameCount, uint32_t& FPS) {
		auto currentTime = std::chrono::steady_clock::now();
		std::chrono::duration<double> elapsed = currentTime - lastTime;

		frameCount++;
		if (elapsed >= 1s) {
			FPS = static_cast<uint32_t>(frameCount / elapsed.count());
			frameCount = 0;
			lastTime = currentTime;
		}
	}

};

int main() {
	try {
		Vulkan_TEST app;
		app.run();
	}
	catch (const std::exception& error) {
		std::cerr << error.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}