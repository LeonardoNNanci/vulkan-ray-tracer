#include "buffer_external.hpp"

std::shared_ptr<BufferExternal> BufferExternalBuilder::buildExternal()
{	
	this->properties |= vk::MemoryPropertyFlagBits::eDeviceLocal;
	this->buffer= this->createBuffer();
	this->memory = this->createMemory();
	this->setup->device.bindBufferMemory(this->buffer, this->memory, 0);
	this->optixBuffer = this->createOptixBuffer();

	auto buffer = std::make_shared<BufferExternal>(this->setup);
	buffer->handle = this->buffer;
	buffer->memory = this->memory;
	buffer->offset = this->offset;
	buffer->size = this->size;
	buffer->commandBuffer = this->commandBuffer;
	buffer->optixBuffer = this->optixBuffer;

	return buffer;
}

vk::Buffer BufferExternalBuilder::createBuffer()
{
	vk::ExternalMemoryBufferCreateInfo externalInfo{
		.handleTypes = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32
	};
	vk::BufferCreateInfo bufferInfo{
		.pNext = &externalInfo,
		.size = this->size,
		.usage = this->usage,
		.sharingMode = vk::SharingMode::eExclusive
	};
	return this->setup->device.createBuffer(bufferInfo);
}

vk::DeviceMemory BufferExternalBuilder::createMemory()
{
	vk::MemoryRequirements memRequirements = this->setup->device.getBufferMemoryRequirements(this->buffer);
	vk::ExportMemoryAllocateInfo memoryHandleExternal{
		.handleTypes = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32
	};
	vk::MemoryAllocateFlagsInfo memFlagsInfo{
		.pNext = &memoryHandleExternal,
		.flags = vk::MemoryAllocateFlagBits::eDeviceAddress
	};
	vk::MemoryAllocateInfo allocInfo{
		.pNext = &memFlagsInfo,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits),
	};
	return this->setup->device.allocateMemory(allocInfo);
}

CUdeviceptr BufferExternalBuilder::createOptixBuffer() {
	VkMemoryGetWin32HandleInfoKHR info{ VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR };
	info.memory = static_cast<VkDeviceMemory>(this->memory);
	info.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
	HANDLE handle;
	auto vkres = vkGetMemoryWin32HandleKHR(static_cast<VkDevice>(setup->device), &info, &handle);

	cudaExternalMemoryHandleDesc cuda_ext_mem_handle_desc{};
	cuda_ext_mem_handle_desc.size = this->size;
	cuda_ext_mem_handle_desc.type = cudaExternalMemoryHandleTypeOpaqueWin32;
	cuda_ext_mem_handle_desc.handle.win32.handle = handle;

	cudaExternalMemory_t cuda_ext_mem_vertex_buffer{};
	auto res = cudaImportExternalMemory(&cuda_ext_mem_vertex_buffer, &cuda_ext_mem_handle_desc);
	if (res != CUDA_SUCCESS) {
		printf("Erro!\n");
	}

	cuda_ext_mem_handle_desc.handle.fd = -1;

	cudaExternalMemoryBufferDesc cuda_ext_buffer_desc{};
	cuda_ext_buffer_desc.offset = 0;
	cuda_ext_buffer_desc.size = this->size;
	cuda_ext_buffer_desc.flags = 0;
	CUdeviceptr optixBuffer_;
	res = cudaExternalMemoryGetMappedBuffer((void**)&optixBuffer_, cuda_ext_mem_vertex_buffer, &cuda_ext_buffer_desc);
	if (res != CUDA_SUCCESS) {
		printf("Erro!\n");
	}

	return optixBuffer;
}

Requirements BufferExternalBuilder::getRequirements()
{
	Requirements reqs{
		.deviceExtensions = {
			VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
			VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME
		},
		.instanceExtensions = {
			VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME
		}
	};
	return reqs;
}
