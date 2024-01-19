#include "presentation.hpp"

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

    return this->presentation;
}

GLFWwindow* PresentationBuilder::createWindow()
{
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	return glfwCreateWindow(600, 800, "Vulkan window", nullptr, nullptr);
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
        .imageUsage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &this->setup->graphicsQueue.familyIndex,
        .presentMode = vk::PresentModeKHR::eFifo,
        .clipped = vk::True
    };

    swapchain.handle = this->setup->device.createSwapchainKHR(swapchainInfo);

    return swapchain;
}

std::vector<std::shared_ptr<Image>> PresentationBuilder::createImages()
{
    std::vector<std::shared_ptr<Image>> images;

    std::vector<vk::Image> scImages = this->setup->device.getSwapchainImagesKHR(this->presentation->swapchain.handle);
    int nImages = scImages.size();

    images.resize(nImages);
    
    for (int i = 0; i < nImages; i++) {
        images[i] = std::make_shared<Image>(this->setup, scImages[i], this->presentation->swapchain.format);
    }

    return images;
}
