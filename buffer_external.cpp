#include "buffer_external.hpp"
#include <dxgi1_2.h>
#include <AclAPI.h>

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
	buffer->hostVisible = (this->properties & vk::MemoryPropertyFlagBits::eHostVisible) == vk::MemoryPropertyFlagBits::eHostVisible;

	return buffer;
}

void BufferExternal::fill(std::shared_ptr<Image> image)
{
	vk::BufferImageCopy region{
		.imageSubresource = {
			.aspectMask = vk::ImageAspectFlagBits::eColor,
			.layerCount = 1
		},
		.imageExtent = {
			.width = image->width,
			.height = image->height,
			.depth = 1
		}
	};
	this->commandBuffer->begin();
	image->pipelineBarrier(this->commandBuffer, vk::ImageLayout::eTransferSrcOptimal);
	this->commandBuffer->handle.copyImageToBuffer(image->handle, image->layout, this->handle, { region });
	image->pipelineBarrier(this->commandBuffer, vk::ImageLayout::eGeneral);
	this->commandBuffer->submit();
}

void BufferExternal::toImage(std::shared_ptr<Image> image)
{
	vk::BufferImageCopy region{
		.bufferRowLength = image->width,
		.bufferImageHeight = image->height,
		.imageSubresource = {
			.aspectMask = vk::ImageAspectFlagBits::eColor,
			.layerCount = 1
		},
		.imageExtent = {
			.width = image->width,
			.height = image->height,
			.depth = 1
		}
	};
	this->commandBuffer->begin();
	auto initial_layout = image->layout;
	image->pipelineBarrier(this->commandBuffer, vk::ImageLayout::eTransferDstOptimal);
	this->commandBuffer->handle.copyBufferToImage(this->handle, image->handle, image->layout, { region });
	image->pipelineBarrier(this->commandBuffer, vk::ImageLayout::ePresentSrcKHR);
	this->commandBuffer->submit();
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

	SECURITY_DESCRIPTOR* securityDescriptor = (SECURITY_DESCRIPTOR *)malloc(SECURITY_DESCRIPTOR_MIN_LENGTH + 2 * sizeof(void**));
	if (InitializeSecurityDescriptor(securityDescriptor, SECURITY_DESCRIPTOR_REVISION) == 0) {
		throw std::runtime_error("Failed to allocate external memory!\n");
	}

	PSID* sid = (PSID*)((PBYTE)securityDescriptor + SECURITY_DESCRIPTOR_MIN_LENGTH);
	SID_IDENTIFIER_AUTHORITY sid_identifier = SECURITY_WORLD_SID_AUTHORITY;
	if (AllocateAndInitializeSid(&sid_identifier, 1, SECURITY_WORLD_RID,
		0, 0, 0, 0, 0, 0, 0, sid) == 0) {
		throw std::runtime_error("Failed to allocate external memory!\n");
	}

	EXPLICIT_ACCESS explicitAccess;
	memset((void*)&explicitAccess, 0, sizeof(explicitAccess));
	explicitAccess.grfAccessPermissions = STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL;
	explicitAccess.grfAccessMode = SET_ACCESS;
	explicitAccess.grfInheritance = INHERIT_ONLY;
	explicitAccess.Trustee.TrusteeForm = TRUSTEE_IS_SID;
	explicitAccess.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	explicitAccess.Trustee.ptstrName = (LPTSTR)*sid;

	PACL* acl = (PACL*)((PBYTE)sid + sizeof(PSID*));
	if (SetEntriesInAcl(1, &explicitAccess, nullptr, acl) != ERROR_SUCCESS) {
		throw std::runtime_error("Failed to allocate external memory!\n");
	}
	if (SetSecurityDescriptorDacl(securityDescriptor, TRUE, *acl, FALSE) == 0) {
		throw std::runtime_error("Failed to allocate external memory!\n");
	}

	SECURITY_ATTRIBUTES securityAttributes;
	securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
	securityAttributes.lpSecurityDescriptor = securityDescriptor;
	securityAttributes.bInheritHandle = TRUE;

	VkExportMemoryWin32HandleInfoKHR handleInfo;
	handleInfo.pNext = NULL;
	handleInfo.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_WIN32_HANDLE_INFO_KHR;
	handleInfo.pAttributes = &securityAttributes;
	handleInfo.dwAccess = DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE;
	handleInfo.name = (LPCWSTR)NULL;

	vk::ExportMemoryAllocateInfo exportInfo{
		.pNext = &handleInfo,
		.handleTypes = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32
	};
	vk::MemoryAllocateInfo allocInfo{
		.pNext = &exportInfo,
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
	if (vkres != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate external memory!\n");
	}

	cudaExternalMemoryHandleDesc cuda_ext_mem_handle_desc{};
	cuda_ext_mem_handle_desc.size = this->size;
	cuda_ext_mem_handle_desc.type = cudaExternalMemoryHandleTypeOpaqueWin32;
	cuda_ext_mem_handle_desc.handle.win32.handle = handle;

	cudaExternalMemory_t cuda_ext_mem{};
	auto res = cudaImportExternalMemory(&cuda_ext_mem, &cuda_ext_mem_handle_desc);
	if (res != CUDA_SUCCESS) {
		throw std::runtime_error("Failed to allocate external memory!\n");
	}

	cuda_ext_mem_handle_desc.handle.fd = -1;

	cudaExternalMemoryBufferDesc cuda_ext_buffer_desc{};
	cuda_ext_buffer_desc.offset = 0;
	cuda_ext_buffer_desc.size = this->size;
	cuda_ext_buffer_desc.flags = 0;
	CUdeviceptr optixBuffer_;
	res = cudaExternalMemoryGetMappedBuffer((void**)&optixBuffer_, cuda_ext_mem, &cuda_ext_buffer_desc);
	if (res != CUDA_SUCCESS) {
		throw std::runtime_error("Failed to allocate external memory!\n");
	}

	return optixBuffer_;
}

Requirements BufferExternalBuilder::getRequirements()
{
	Requirements reqs{
		.deviceExtensions = {
			VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
			VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME
		},
		.instanceExtensions = {
			VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
		}
	};
	return reqs;
}
