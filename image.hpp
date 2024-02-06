#pragma once
#include <vulkan/vulkan.hpp>

#include "setup.hpp"
#include "command_pool.hpp"

class Image : IHasSetup {
public:
	vk::Image handle;
	vk::DeviceMemory memory;
	uint32_t width;
	uint32_t height;
	vk::ImageView view;
	vk::ImageLayout layout;

	Image(std::shared_ptr<Setup> setup, vk::Image handle, vk::Format format, uint32_t width, uint32_t height, vk::DeviceMemory memory=nullptr);

	void pipelineBarrier(std::shared_ptr<CommandBuffer> commandBuffer, vk::ImageLayout newLayout);

	void presentBarrier(std::shared_ptr<CommandBuffer> commandBuffer);

	void renderBarrier(std::shared_ptr<CommandBuffer> commandBuffer);

	void clear(std::shared_ptr<CommandBuffer> commandBuffer);

	~Image();

private:
	void clearBarrier(std::shared_ptr<CommandBuffer> commandBuffer);
};