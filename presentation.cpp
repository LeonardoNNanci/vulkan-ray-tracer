#include "presentation.hpp"

#define WIDTH 600
#define HEIGHT 800

Presentation::Presentation(std::shared_ptr<Setup> setup) : IHasSetup(setup), window(NULL) {}

bool Presentation::windowIsOpen()
{
    glfwPollEvents();
    return !glfwWindowShouldClose(this->window);
}

Presentation::~Presentation()
{
    this->setup->device.destroySwapchainKHR(this->swapchain.handle);
    this->setup->instance.destroySurfaceKHR(this->surface);
    glfwDestroyWindow(this->window);
    glfwTerminate();
}

PresentationBuilder::PresentationBuilder(std::shared_ptr<Setup> setup) : IHasSetup(setup) {}

Requirements PresentationBuilder::getRequirements()
{	
	Requirements extensions;

	glfwInit();

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	for (int i = 0; i < glfwExtensionCount; i++)
		extensions.instanceExtensions.push_back(glfwExtensions[i]);

	extensions.deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	return extensions;
}

std::shared_ptr<Presentation> PresentationBuilder::build()
{
    this->presentation = std::make_shared<Presentation>(this->setup);

    this->presentation->window = this->createWindow();
    this->presentation->surface = this->createSurface();
    this->presentation->swapchain = this->createSwapchain();
    this->presentation->swapchain.images = this->createImages();
    int nImages = this->presentation->swapchain.images.size();
    this->presentation->albedoImages = this->createDenoiserImages(nImages);
    this->presentation->normalImages = this->createDenoiserImages(nImages);

    return this->presentation;
}

GLFWwindow* PresentationBuilder::createWindow()
{
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	return glfwCreateWindow(WIDTH, HEIGHT, "Vulkan window", nullptr, nullptr);
}

vk::SurfaceKHR PresentationBuilder::createSurface()
{
	VkSurfaceKHR handle;
	if (glfwCreateWindowSurface(this->setup->instance, this->presentation->window, nullptr, &handle) != VK_SUCCESS)
		throw std::runtime_error("Failed to create window surface!");

	return vk::SurfaceKHR(handle);
}

Swapchain PresentationBuilder::createSwapchain()
{
    Swapchain swapchain;

    std::vector<vk::SurfaceFormatKHR> surfaceFormats = this->setup->physicalDevice.getSurfaceFormatsKHR(this->presentation->surface);
    std::vector<vk::PresentModeKHR> surfacePresentModes = this->setup->physicalDevice.getSurfacePresentModesKHR(this->presentation->surface);
    vk::SurfaceCapabilitiesKHR surfaceCapabilities = this->setup->physicalDevice.getSurfaceCapabilitiesKHR(this->presentation->surface);

    vk::SurfaceFormatKHR format_;
    for (vk::SurfaceFormatKHR availableFormat : surfaceFormats)
        if (availableFormat.format == vk::Format::eR8G8B8A8Unorm && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            format_ = availableFormat;
            break;
        }
    swapchain.extent = surfaceCapabilities.currentExtent;
    swapchain.format = format_.format;

    vk::SwapchainCreateInfoKHR swapchainInfo{
        .surface = this->presentation->surface,
        .minImageCount = surfaceCapabilities.minImageCount,
        .imageFormat = swapchain.format,
        .imageColorSpace = format_.colorSpace,
        .imageExtent = swapchain.extent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &this->setup->graphicsQueue.familyIndex,
        .presentMode = vk::PresentModeKHR::eFifo,
        .clipped = vk::True
    };

    swapchain.handle = this->setup->device.createSwapchainKHR(swapchainInfo);

    return swapchain;
}

std::vector<std::shared_ptr<Image>> PresentationBuilder::createDenoiserImages(int nImages) {
    std::vector<std::shared_ptr<Image>> images(nImages);
    vk::ImageCreateInfo imageInfo{
            .imageType = vk::ImageType::e2D,
            .format = vk::Format::eR8G8B8A8Unorm,
            .extent = {
                .width = WIDTH,
                .height = HEIGHT,
                .depth = 1,
            },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = vk::SampleCountFlagBits::e1,
            .tiling = vk::ImageTiling::eOptimal,
            .usage = vk::ImageUsageFlagBits::eStorage,
            .sharingMode = vk::SharingMode::eExclusive,
            .queueFamilyIndexCount = 1,
            .pQueueFamilyIndices = &this->setup->graphicsQueue.familyIndex,
            .initialLayout = vk::ImageLayout::eGeneral
    };
    for (int i = 0; i < nImages; i++) {
        // albedo
        auto handle = this->setup->device.createImage(imageInfo);
        auto memory = this->createMemory(handle);
        this->setup->device.bindImageMemory(handle, memory, 0);
        images[i] = std::make_shared<Image>(this->setup, handle, vk::Format::eR8G8B8Unorm, WIDTH, HEIGHT, memory);
    }
    return images;
}

std::vector<std::shared_ptr<Image>> PresentationBuilder::createImages()
{
    std::vector<std::shared_ptr<Image>> images;

    std::vector<vk::Image> scImages = this->setup->device.getSwapchainImagesKHR(this->presentation->swapchain.handle);
    int nImages = scImages.size();

    images.resize(nImages);
    
    for (int i = 0; i < nImages; i++) {
        images[i] = std::make_shared<Image>(this->setup, scImages[i], this->presentation->swapchain.format, WIDTH, HEIGHT);
    }

    return images;
}

vk::DeviceMemory PresentationBuilder::createMemory(vk::Image image) {
    vk::MemoryRequirements memRequirements = this->setup->device.getImageMemoryRequirements(image);
    vk::MemoryAllocateInfo allocInfo{
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits),
    };
    return this->setup->device.allocateMemory(allocInfo);
}

uint32_t PresentationBuilder::findMemoryType(uint32_t typeFilter) {
    vk::PhysicalDeviceMemoryProperties memProperties = this->setup->physicalDevice.getMemoryProperties();
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal))
            return i;
    throw std::runtime_error("Could not find suitable memory type!");
}

