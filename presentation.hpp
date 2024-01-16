#pragma once

#include<memory>

#define GLFW_INCLUDE_VULKAN
#include<GLFW/glfw3.h>
#include<vulkan/vulkan.hpp>

#include "builder.hpp"
#include "setup.hpp"

class Swapchain {
public:
	vk::SwapchainKHR handle;
	vk::Extent2D extent;
	vk::Format format;
	std::vector<vk::ImageView> imageViews;
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
	PresentationBuilder(std::shared_ptr<Setup> setup);

	static Requirements getRequirements();

	std::shared_ptr<Presentation> build();

private:
	std::shared_ptr<Presentation> presentation;

	GLFWwindow* createWindow();

	vk::SurfaceKHR createSurface();

	Swapchain createSwapchain();

	std::vector<vk::ImageView> createImageViews();
};