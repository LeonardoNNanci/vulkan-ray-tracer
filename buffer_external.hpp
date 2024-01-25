#pragma once
#include "buffer.hpp"

#include <cuda.h>
#include <cuda_runtime_api.h>
#include <optix.h>

class BufferExternal : public Buffer {
public:
	CUdeviceptr optixBuffer;

	using Buffer::Buffer;
};

class BufferExternalBuilder : private BufferBuilder {
public:
	static Requirements getRequirements();

	using BufferBuilder::BufferBuilder;

	std::shared_ptr<BufferExternal> buildExternal();

	BufferExternalBuilder setUsage(vk::BufferUsageFlags usage) {
		BufferBuilder::setUsage(usage);
		return *this;
	};

	BufferExternalBuilder setMemoryProperties(vk::MemoryPropertyFlags properties) {
		BufferBuilder::setMemoryProperties(properties);
		return *this;
	};

	BufferExternalBuilder setSize(vk::DeviceSize size) {
		BufferBuilder::setSize(size);
		return *this;
	};

	BufferExternalBuilder setOffset(vk::DeviceSize offset) {
		BufferBuilder::setOffset(offset);
		return *this;
	};

	BufferExternalBuilder setCommandBuffer(std::shared_ptr<CommandBuffer> commandBuffer) {
		BufferBuilder::setCommandBuffer(commandBuffer);
		return *this;
	};

protected:
	CUdeviceptr optixBuffer = NULL;

	vk::Buffer createBuffer() override;

	vk::DeviceMemory createMemory() override;

	CUdeviceptr createOptixBuffer();
};