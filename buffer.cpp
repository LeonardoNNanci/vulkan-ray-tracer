#include "buffer.hpp"
#include "scene.hpp"

BufferBuilder::BufferBuilder(std::shared_ptr<Setup> setup) : IHasSetup(setup) {}

BufferBuilder BufferBuilder::setUsage(vk::BufferUsageFlags usage)
{
	this->usage |= usage;
	return *this;
}

BufferBuilder BufferBuilder::setMemoryProperties(vk::MemoryPropertyFlags properties)
{
	this->properties |= properties;
	return *this;
}

BufferBuilder BufferBuilder::setSize(vk::DeviceSize size)
{
	this->size = size;
	return *this;
}

BufferBuilder BufferBuilder::setOffset(vk::DeviceSize offset)
{
	this->offset = offset;
	return *this;
}

BufferBuilder BufferBuilder::setCommandBuffer(std::shared_ptr<CommandBuffer> commandBuffer) {
	this->commandBuffer = commandBuffer;
	return *this;
}

std::shared_ptr<Buffer> BufferBuilder::build() {
	this->buffer = this->createBuffer();
	this->memory = this->createMemory();
	this->setup->device.bindBufferMemory(this->buffer, this->memory, 0);

	auto buffer = std::make_shared<Buffer>(this->setup);
	buffer->handle = this->buffer;
	buffer->memory = this->memory;
	buffer->offset = this->offset;
	buffer->size = this->size;
	buffer->commandBuffer = this->commandBuffer;

	return buffer;
}

vk::Buffer BufferBuilder::createBuffer() {
	vk::BufferCreateInfo bufferInfo{
		.size = this->size,
		.usage = this->usage,
		.sharingMode = vk::SharingMode::eExclusive
	};
	return this->setup->device.createBuffer(bufferInfo);
}

vk::DeviceMemory BufferBuilder::createMemory() {
	vk::MemoryRequirements memRequirements = this->setup->device.getBufferMemoryRequirements(this->buffer);
	vk::MemoryAllocateFlagsInfo memFlagsInfo{
		.flags = vk::MemoryAllocateFlagBits::eDeviceAddress
	};
	vk::MemoryAllocateInfo allocInfo{
		.pNext = &memFlagsInfo,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits),
	};
	return this->setup->device.allocateMemory(allocInfo);
}

uint32_t BufferBuilder::findMemoryType(uint32_t typeFilter) {
	vk::PhysicalDeviceMemoryProperties memProperties = this->setup->physicalDevice.getMemoryProperties();
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			return i;
	throw std::runtime_error("Could not find suitable memory type!");
}

Buffer::Buffer(std::shared_ptr<Setup> setup) : IHasSetup(setup) {}

vk::DeviceAddress Buffer::getDeviceAddress()
{
	if (this->address != NULL)
		return this->address;

	vk::BufferDeviceAddressInfo addressInfo{
		.buffer = this->handle
	};
	this->address = this->setup->device.getBufferAddress(addressInfo);

	return this->address;
}

Buffer::~Buffer() {
	this->setup->device.destroyBuffer(this->handle);
	this->setup->device.freeMemory(this->memory);
}

void Buffer::copyBuffer(std::shared_ptr<Buffer> source) {
	this->commandBuffer->begin();
	vk::BufferCopy copyInfo{
		.srcOffset = source->offset,
		.dstOffset = this->offset,
		.size = this->size
	};
	commandBuffer->handle.copyBuffer(source->handle, this->handle, { copyInfo });
	commandBuffer->submit();
	commandBuffer->queue.handle.waitIdle();
}

template void Buffer::fill<uint32_t>(std::vector<uint32_t> data);
template void Buffer::fill<Vertex>(std::vector<Vertex> data);
template void Buffer::fill<vk::AccelerationStructureInstanceKHR>(std::vector<vk::AccelerationStructureInstanceKHR> data);

template <typename T>
void Buffer::fill(std::vector<T> data) {
	auto stagingBuffer = BufferBuilder(this->setup)
		.setSize(this->size)
		.setUsage(vk::BufferUsageFlagBits::eTransferSrc)
		.setMemoryProperties(vk::MemoryPropertyFlagBits::eDeviceLocal)
		.setMemoryProperties(vk::MemoryPropertyFlagBits::eHostCoherent)
		.build();

	void* pointer = this->setup->device.mapMemory(stagingBuffer->memory, stagingBuffer->offset, stagingBuffer->size, {});
	memcpy(pointer, data.data(), (size_t)size);
	this->setup->device.unmapMemory(stagingBuffer->memory);

	this->copyBuffer(stagingBuffer);
};