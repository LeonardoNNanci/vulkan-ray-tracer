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
	if (size <= 0)
		throw std::runtime_error("Buffer size must be greater than zero.\n");
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
	buffer->hostVisible = (this->properties & vk::MemoryPropertyFlagBits::eHostVisible) == vk::MemoryPropertyFlagBits::eHostVisible;
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
		.memoryTypeIndex = this->setup->findMemoryType(memRequirements.memoryTypeBits, this->properties),
	};
	return this->setup->device.allocateMemory(allocInfo);
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

template void Buffer::fill<Range>(std::vector<Range> data);
template void Buffer::fill<Light>(std::vector<Light> data);
template void Buffer::fill<Vertex>(std::vector<Vertex> data);
template void Buffer::fill<Material>(std::vector<Material> data);
template void Buffer::fill<uint32_t>(std::vector<uint32_t> data);
template void Buffer::fill<vk::Image>(std::vector<vk::Image> data);
template void Buffer::fill<vk::Sampler>(std::vector<vk::Sampler> data);
template void Buffer::fill<unsigned char>(std::vector<unsigned char> data);
template void Buffer::fill<ModelDescription>(std::vector<ModelDescription> data);
template void Buffer::fill<std::pair<int, int>>(std::vector<std::pair<int, int>> data);
template void Buffer::fill<vk::AccelerationStructureInstanceKHR>(std::vector<vk::AccelerationStructureInstanceKHR> data);
template void Buffer::fill<vk::AccelerationStructureInstanceKHR>(std::vector<vk::AccelerationStructureInstanceKHR> data);

template <typename T>
void Buffer::fill(std::vector<T> data) {
	if (this->hostVisible) {
		void* pointer = this->setup->device.mapMemory(this->memory, this->offset, this->size, {});
		memcpy(pointer, data.data(), (size_t)this->size);
		this->setup->device.unmapMemory(this->memory);
	}
	else {
		auto stagingBuffer = BufferBuilder(this->setup)
			.setSize(this->size)
			.setUsage(vk::BufferUsageFlagBits::eTransferSrc)
			.setMemoryProperties(vk::MemoryPropertyFlagBits::eDeviceLocal)
			.setMemoryProperties(vk::MemoryPropertyFlagBits::eHostCoherent)
			.build();

		void* pointer = this->setup->device.mapMemory(stagingBuffer->memory, stagingBuffer->offset, stagingBuffer->size, {});
		memcpy(pointer, data.data(), (size_t)this->size);
		this->setup->device.unmapMemory(stagingBuffer->memory);

		this->copyBuffer(stagingBuffer);
	}
};