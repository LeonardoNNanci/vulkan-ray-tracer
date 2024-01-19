#pragma once
#include <vulkan/vulkan.hpp>

#include "setup.hpp"
#include "command_pool.hpp"

class Image : IHasSetup {
public:
	vk::Image handle;
	vk::ImageView view;
	vk::ImageLayout layout;

	Image(std::shared_ptr<Setup> setup, vk::Image handle, vk::Format format);

	void presentBarrier(std::shared_ptr<CommandBuffer> commandBuffer);

	void renderBarrier(std::shared_ptr<CommandBuffer> commandBuffer);

	void clear(std::shared_ptr<CommandBuffer> commandBuffer);

	~Image();

private:
	void clearBarrier(std::shared_ptr<CommandBuffer> commandBuffer);
};