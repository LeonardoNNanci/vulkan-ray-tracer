#pragma once

#include<memory>

#define GLFW_INCLUDE_VULKAN
#include<GLFW/glfw3.h>
#include<vulkan/vulkan.hpp>

#include "builder.hpp"
#include "image.hpp"
#include "setup.hpp"

class Swapchain {
public:
	vk::SwapchainKHR handle;
	vk::Extent2D extent;
	vk::Format format;
	std::vector<std::shared_ptr<Image>> images;
};

class Presentation : private IHasSetup {
public:
	GLFWwindow* window;
	vk::SurfaceKHR surface;
	Swapchain swapchain;

	Presentation(std::shared_ptr<Setup> setup);

	bool windowIsOpen();

	~Presentation();
};

class PresentationBuilder : public Builder<std::shared_ptr<Presentation>>, private IHasSetup {
public:
	PresentationBuilder(std::shared_ptr<Setup> setup, unsigned int width, unsigned int height);

	static Requirements getRequirements();

	std::shared_ptr<Presentation> build();

private:
	std::shared_ptr<Presentation> presentation;

	unsigned int width;

	unsigned int height;

	GLFWwindow* createWindow();

	vk::SurfaceKHR createSurface();

	Swapchain createSwapchain();

	std::vector<std::shared_ptr<Image>> createImages();

	std::vector<std::shared_ptr<Image>> createDenoiserImages(int nImages);

	vk::DeviceMemory createMemory(vk::Image image);

	uint32_t findMemoryType(uint32_t typeFilter);
};