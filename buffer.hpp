#pragma once

#include "setup.hpp"
#include "builder.hpp"
#include "command_pool.hpp"

class Buffer : private IHasSetup {
public:
	vk::Buffer handle;
	vk::DeviceMemory memory;
	vk::DeviceSize offset = 0;
	vk::DeviceSize size = 0;
	std::shared_ptr<CommandBuffer> commandBuffer;

	Buffer(std::shared_ptr<Setup> setup);

	template<typename T>
	void fill(std::vector<T> data);

	vk::DeviceAddress getDeviceAddress();

	~Buffer();

private:
	vk::DeviceAddress address = NULL;
	void copyBuffer(std::shared_ptr<Buffer> other);
};

class BufferBuilder : public Builder <std::shared_ptr<Buffer>> , protected IHasSetup{
public:
	BufferBuilder(std::shared_ptr<Setup> setup);

	BufferBuilder setUsage(vk::BufferUsageFlags usage);

	BufferBuilder setMemoryProperties(vk::MemoryPropertyFlags properties);

	BufferBuilder setSize(vk::DeviceSize size);

	BufferBuilder setOffset(vk::DeviceSize offset);

	BufferBuilder setCommandBuffer(std::shared_ptr<CommandBuffer> commandBuffer);

	std::shared_ptr<Buffer> build();

protected:
	vk::Buffer buffer;
	vk::DeviceMemory memory;
	vk::MemoryPropertyFlags properties;
	vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eTransferDst;
	vk::DeviceSize size = 0;
	vk::DeviceSize offset = 0;
	std::shared_ptr<CommandBuffer> commandBuffer;

	virtual vk::Buffer createBuffer();

	virtual vk::DeviceMemory createMemory();

	uint32_t findMemoryType(uint32_t typeFilter);
};
