#pragma once
#include <vulkan/vulkan.hpp>

#include "setup.hpp"
#include "command_pool.hpp"
#include "buffer.hpp"

class Image : IHasSetup {
public:
	vk::Image handle;
	vk::DeviceMemory memory;
	uint32_t width;
	uint32_t height;
	vk::ImageView view;
	vk::ImageLayout layout;
	vk::Format format;

	Image(std::shared_ptr<Setup> setup, vk::Image handle, vk::Format format, uint32_t width, uint32_t height, vk::DeviceMemory memory=nullptr);

	Image(std::shared_ptr<Setup> setup, std::shared_ptr<CommandBuffer> commandBuffer, std::string fileName);

	void pipelineBarrier(std::shared_ptr<CommandBuffer> commandBuffer, vk::ImageLayout newLayout);

	void presentBarrier(std::shared_ptr<CommandBuffer> commandBuffer);

	void renderBarrier(std::shared_ptr<CommandBuffer> commandBuffer);

	void layoutChangeBarrier(std::shared_ptr<CommandBuffer> commandBuffer, vk::ImageLayout newLayout);

	void clear(std::shared_ptr<CommandBuffer> commandBuffer);

	void copyBuffer(std::shared_ptr<CommandBuffer> commandBuffer, std::shared_ptr<Buffer> source);

	~Image();

private:
	bool selfDestroy;

	void createImageView();

	void clearBarrier(std::shared_ptr<CommandBuffer> commandBuffer);
};