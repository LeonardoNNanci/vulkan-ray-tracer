#include "image.hpp"
#include "buffer.hpp"
#include "file_reader.hpp"

#include <iostream>

Image::Image(std::shared_ptr<Setup> setup, vk::Image handle, vk::Format format, uint32_t width, uint32_t height, vk::DeviceMemory memory)
	: IHasSetup(setup), handle(handle), width(width), height(height), memory(memory), selfDestroy(false), format(format) {
	this->createImageView();
	this->layout = vk::ImageLayout::eUndefined;
}

Image::Image(std::shared_ptr<Setup> setup, std::vector<unsigned char> bytes, uint32_t width, uint32_t height, std::shared_ptr<CommandBuffer> commandBuffer) : 
	IHasSetup(setup), width(width), height(height), format(vk::Format::eR8G8B8A8Srgb), selfDestroy(true)
{
	vk::DeviceSize imgDataSize = this->width * this->height * 4;

	auto stagingBuffer = BufferBuilder(setup)
		.setSize(imgDataSize)
		.setUsage(vk::BufferUsageFlagBits::eTransferSrc)
		.setMemoryProperties(vk::MemoryPropertyFlagBits::eHostVisible)
		.setMemoryProperties(vk::MemoryPropertyFlagBits::eHostCoherent)
		.setCommandBuffer(commandBuffer)
		.build();

	stagingBuffer->fill(bytes);

	vk::ImageCreateInfo imageInfo{
		.imageType = vk::ImageType::e2D,
		.format = this->format,
		.extent = {
			.width = this->width,
			.height = this->height,
			.depth = 1
		},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = vk::SampleCountFlagBits::e1,
		.tiling = vk::ImageTiling::eOptimal,
		.usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
		.sharingMode = vk::SharingMode::eExclusive,
		.initialLayout = vk::ImageLayout::eUndefined
	};
	this->layout = vk::ImageLayout::eUndefined;

	this->handle = setup->device.createImage(imageInfo);

	auto memRequirements = setup->device.getImageMemoryRequirements(this->handle);
	vk::MemoryAllocateInfo allocInfo{
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = setup->findMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal)
	};

	this->memory = this->setup->device.allocateMemory(allocInfo);
	this->setup->device.bindImageMemory(this->handle, this->memory, 0);

	commandBuffer->begin();
	this->copyBuffer(commandBuffer, stagingBuffer);
	this->layoutChangeBarrier(commandBuffer, vk::ImageLayout::eShaderReadOnlyOptimal);
	commandBuffer->submit();
	commandBuffer->waitFinished();

	this->createImageView();
}

void Image::createImageView() {
	vk::ImageViewCreateInfo imageViewInfo{
		.image = this->handle,
		.viewType = vk::ImageViewType::e2D,
		.format = this->format,
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

void Image::layoutChangeBarrier(std::shared_ptr<CommandBuffer> commandBuffer, vk::ImageLayout newLayout) {
	vk::ImageMemoryBarrier imageBarrier{
		.srcAccessMask = vk::AccessFlagBits::eMemoryRead,
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

void Image::copyBuffer(std::shared_ptr<CommandBuffer> commandBuffer, std::shared_ptr<Buffer> source)
{
	vk::BufferImageCopy region{
		.bufferOffset = source->offset,
		.imageSubresource = {
			.aspectMask = vk::ImageAspectFlagBits::eColor,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = 1
		},
		.imageOffset = {0, 0, 0},
		.imageExtent = {
			.width = this->width,
			.height = this->height,
			.depth = 1
		}
	};
	this->layoutChangeBarrier(commandBuffer, vk::ImageLayout::eTransferDstOptimal);
	commandBuffer->handle.copyBufferToImage(source->handle, this->handle, this->layout, {region});
}

Image::~Image() {
	if (this->memory != nullptr) {
		this->setup->device.freeMemory(this->memory);
	}
	if (this->selfDestroy)
		this->setup->device.destroyImage(this->handle);
	this->setup->device.destroyImageView(this->view);
}