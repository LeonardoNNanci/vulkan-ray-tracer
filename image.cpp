#include "image.hpp"

Image::Image(std::shared_ptr<Setup> setup, vk::Image handle, vk::Format format, uint32_t width, uint32_t height, vk::DeviceMemory memory)
	: IHasSetup(setup), handle(handle), width(width), height(height), memory(memory) {
	vk::ImageViewCreateInfo imageViewInfo{
		.image = this->handle,
		.viewType = vk::ImageViewType::e2D,
		.format = format,
		.components = {
			.r = vk::ComponentSwizzle::eIdentity,
			.g = vk::ComponentSwizzle::eIdentity,
			.b = vk::ComponentSwizzle::eIdentity,
			.a = vk::ComponentSwizzle::eIdentity
		},
		.subresourceRange = {
			.aspectMask = vk::ImageAspectFlagBits::eColor,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};
	this->view = this->setup->device.createImageView(imageViewInfo);
	this->layout = vk::ImageLayout::eGeneral;
}

void Image::pipelineBarrier(std::shared_ptr<CommandBuffer> commandBuffer, vk::ImageLayout newLayout) {
	vk::ImageMemoryBarrier imageBarrier{
		.srcAccessMask = vk::AccessFlagBits::eMemoryWrite,
		.dstAccessMask = vk::AccessFlagBits::eMemoryRead,
		.oldLayout = this->layout,
		.newLayout = newLayout,
		.srcQueueFamilyIndex = this->setup->graphicsQueue.familyIndex,
		.dstQueueFamilyIndex = this->setup->graphicsQueue.familyIndex,
		.image = this->handle,
		.subresourceRange = {
			.aspectMask = vk::ImageAspectFlagBits::eColor,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};
	commandBuffer->handle.pipelineBarrier(
		vk::PipelineStageFlagBits::eRayTracingShaderKHR,
		vk::PipelineStageFlagBits::eAllCommands,
		{},
		{},
		{},
		{ imageBarrier }
	);

	this->layout = newLayout;
}

void Image::presentBarrier(std::shared_ptr<CommandBuffer> commandBuffer) {
	auto newLayout = vk::ImageLayout::ePresentSrcKHR;

	vk::ImageMemoryBarrier imageBarrier{
		.srcAccessMask = vk::AccessFlagBits::eMemoryWrite,
		.dstAccessMask = vk::AccessFlagBits::eMemoryRead,
		.oldLayout = this->layout,
		.newLayout = newLayout,
		.srcQueueFamilyIndex = this->setup->graphicsQueue.familyIndex,
		.dstQueueFamilyIndex = this->setup->graphicsQueue.familyIndex,
		.image = this->handle,
		.subresourceRange = {
			.aspectMask = vk::ImageAspectFlagBits::eColor,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};
	commandBuffer->handle.pipelineBarrier(
		vk::PipelineStageFlagBits::eComputeShader,
		vk::PipelineStageFlagBits::eAllCommands,
		{},
		{},
		{},
		{ imageBarrier }
	);

	this->layout = newLayout;
}

void Image::renderBarrier(std::shared_ptr<CommandBuffer> commandBuffer) {
	auto newLayout = vk::ImageLayout::eGeneral;

	vk::ImageMemoryBarrier imageBarrier{
		.srcAccessMask = vk::AccessFlagBits::eMemoryWrite,
		.dstAccessMask = vk::AccessFlagBits::eMemoryWrite,
		.oldLayout = this->layout,
		.newLayout = newLayout,
		.srcQueueFamilyIndex = this->setup->graphicsQueue.familyIndex,
		.dstQueueFamilyIndex = this->setup->graphicsQueue.familyIndex,
		.image = this->handle,
		.subresourceRange = {
			.aspectMask = vk::ImageAspectFlagBits::eColor,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};
	commandBuffer->handle.pipelineBarrier(
		vk::PipelineStageFlagBits::eAllGraphics,
		vk::PipelineStageFlagBits::eAllCommands,
		{},
		{},
		{},
		{ imageBarrier }
	);

	this->layout = newLayout;
}

void Image::clearBarrier(std::shared_ptr<CommandBuffer> commandBuffer) {
	auto newLayout = vk::ImageLayout::eGeneral;

	vk::ImageMemoryBarrier imageBarrier{
		.srcAccessMask = vk::AccessFlagBits::eMemoryRead,
		.dstAccessMask = vk::AccessFlagBits::eMemoryWrite,
		.oldLayout = vk::ImageLayout::eUndefined,
		.newLayout = newLayout,
		.srcQueueFamilyIndex = this->setup->graphicsQueue.familyIndex,
		.dstQueueFamilyIndex = this->setup->graphicsQueue.familyIndex,
		.image = this->handle,
		.subresourceRange = {
			.aspectMask = vk::ImageAspectFlagBits::eColor,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};
	commandBuffer->handle.pipelineBarrier(
		vk::PipelineStageFlagBits::eAllCommands,
		vk::PipelineStageFlagBits::eAllGraphics,
		{},
		{},
		{},
		{ imageBarrier }
	);

	this->layout = newLayout;
}

void Image::clear(std::shared_ptr<CommandBuffer> commandBuffer) {
	this->clearBarrier(commandBuffer);

	vk::ClearColorValue clearColor{
		.float32 = {{0., 0., 0., 1.}}
	};
	vk::ImageSubresourceRange range{
		.aspectMask = vk::ImageAspectFlagBits::eColor,
		.baseMipLevel = 0,
		.levelCount = 1,
		.baseArrayLayer = 0,
		.layerCount = 1
	};
	commandBuffer->handle.clearColorImage(this->handle, this->layout, clearColor, { range });
}

Image::~Image() {
	if (this->memory != nullptr) {
		this->setup->device.freeMemory(this->memory);
	}
	this->setup->device.destroyImageView(this->view);
}