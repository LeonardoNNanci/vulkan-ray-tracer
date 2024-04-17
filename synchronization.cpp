#include "synchronization.hpp"
#include <dxgi1_2.h>
#include <AclAPI.h>
#include <cuda_d3d11_interop.h>

Semaphore::Semaphore(std::shared_ptr<Setup> setup, uint64_t initialValue) : IHasSetup(setup) {
	this->type = initialValue < UINT64_MAX ? 
		vk::SemaphoreType::eTimeline : 
		vk::SemaphoreType::eBinary;

	initialValue = initialValue < UINT64_MAX ?
		initialValue :
		0;
	VkSemaphore s;
	this->createExternalSemaphore(s, VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR, initialValue, this->type);
	this->importCudaExternalSemaphore(this->cuda, s, VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT);

	this->handle = vk::Semaphore(s);
}

void* Semaphore::getSemaphoreHandle(
	VkSemaphore semaphore, VkExternalSemaphoreHandleTypeFlagBits handleType) {
#ifdef _WIN64
	HANDLE handle;

	VkSemaphoreGetWin32HandleInfoKHR semaphoreGetWin32HandleInfoKHR = {};
	semaphoreGetWin32HandleInfoKHR.sType =
		VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR;
	semaphoreGetWin32HandleInfoKHR.pNext = NULL;
	semaphoreGetWin32HandleInfoKHR.semaphore = semaphore;
	semaphoreGetWin32HandleInfoKHR.handleType = handleType;

	PFN_vkGetSemaphoreWin32HandleKHR fpGetSemaphoreWin32HandleKHR;
	fpGetSemaphoreWin32HandleKHR =
		(PFN_vkGetSemaphoreWin32HandleKHR)vkGetDeviceProcAddr(
			static_cast<VkDevice>(this->setup->device), "vkGetSemaphoreWin32HandleKHR");
	if (!fpGetSemaphoreWin32HandleKHR) {
		throw std::runtime_error("Failed to retrieve vkGetMemoryWin32HandleKHR!");
	}
	if (fpGetSemaphoreWin32HandleKHR(static_cast<VkDevice>(this->setup->device), &semaphoreGetWin32HandleInfoKHR,
		&handle) != VK_SUCCESS) {
		throw std::runtime_error("Failed to retrieve handle for buffer!");
	}

	return (void*)handle;
#else
	int fd;

	VkSemaphoreGetFdInfoKHR semaphoreGetFdInfoKHR = {};
	semaphoreGetFdInfoKHR.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR;
	semaphoreGetFdInfoKHR.pNext = NULL;
	semaphoreGetFdInfoKHR.semaphore = semaphore;
	semaphoreGetFdInfoKHR.handleType = handleType;

	PFN_vkGetSemaphoreFdKHR fpGetSemaphoreFdKHR;
	fpGetSemaphoreFdKHR = (PFN_vkGetSemaphoreFdKHR)vkGetDeviceProcAddr(
		m_device, "vkGetSemaphoreFdKHR");
	if (!fpGetSemaphoreFdKHR) {
		throw std::runtime_error("Failed to retrieve vkGetMemoryWin32HandleKHR!");
	}
	if (fpGetSemaphoreFdKHR(m_device, &semaphoreGetFdInfoKHR, &fd) !=
		VK_SUCCESS) {
		throw std::runtime_error("Failed to retrieve handle for buffer!");
	}

	return (void*)(uintptr_t)fd;
#endif /* _WIN64 */
}

void Semaphore::importCudaExternalSemaphore(
	cudaExternalSemaphore_t& cudaSem, VkSemaphore& vkSem,
	VkExternalSemaphoreHandleTypeFlagBits handleType) {
	cudaExternalSemaphoreHandleDesc externalSemaphoreHandleDesc = {};

	if(this->type == vk::SemaphoreType::eTimeline)
	{
		if (handleType & VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT) {
			externalSemaphoreHandleDesc.type =
				cudaExternalSemaphoreHandleTypeTimelineSemaphoreWin32;
		}
		else if (handleType &
			VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT) {
			externalSemaphoreHandleDesc.type =
				cudaExternalSemaphoreHandleTypeTimelineSemaphoreWin32;
		}
		else if (handleType & VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT) {
			externalSemaphoreHandleDesc.type =
				cudaExternalSemaphoreHandleTypeTimelineSemaphoreFd;
		}
	}
	else
	{
		if (handleType & VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT) {
			externalSemaphoreHandleDesc.type =
				cudaExternalSemaphoreHandleTypeOpaqueWin32;
		}
		else if (handleType &
			VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT) {
			externalSemaphoreHandleDesc.type =
				cudaExternalSemaphoreHandleTypeOpaqueWin32Kmt;
		}
		else if (handleType & VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT) {
			externalSemaphoreHandleDesc.type =
				cudaExternalSemaphoreHandleTypeOpaqueFd;
		}
	}

#ifdef _WIN64
	externalSemaphoreHandleDesc.handle.win32.handle =
		(HANDLE)getSemaphoreHandle(vkSem, handleType);
#else
	externalSemaphoreHandleDesc.handle.fd =
		(int)(uintptr_t)getSemaphoreHandle(vkSem, handleType);
#endif

	externalSemaphoreHandleDesc.flags = 0;

	auto cudaRes = cudaImportExternalSemaphore(&cudaSem, &externalSemaphoreHandleDesc);
	if (cudaRes != CUDA_SUCCESS)
		throw std::runtime_error("Failed to import vulkan semaphore to cuda.");
}

void Semaphore::createExternalSemaphore(
	VkSemaphore& semaphore, VkExternalSemaphoreHandleTypeFlagBits handleType, uint64_t initialValue, vk::SemaphoreType type_) {
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VkExportSemaphoreCreateInfoKHR exportSemaphoreCreateInfo = {};
	exportSemaphoreCreateInfo.sType =
		VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO_KHR;

	if(type_ == vk::SemaphoreType::eTimeline){
		VkSemaphoreTypeCreateInfo timelineCreateInfo;
		timelineCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
		timelineCreateInfo.pNext = NULL;
		timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
		timelineCreateInfo.initialValue = initialValue;
		exportSemaphoreCreateInfo.pNext = &timelineCreateInfo;
	}
	else
	exportSemaphoreCreateInfo.pNext = NULL;

	exportSemaphoreCreateInfo.handleTypes = handleType;
	semaphoreInfo.pNext = &exportSemaphoreCreateInfo;

	if (vkCreateSemaphore(static_cast<VkDevice>(this->setup->device), &semaphoreInfo, nullptr, &semaphore) !=
		VK_SUCCESS) {
		throw std::runtime_error(
			"failed to create synchronization objects for a CUDA-Vulkan!");
	}
}

void Semaphore::waitSignaled(uint64_t value) {
	vk::SemaphoreWaitInfo waitInfo{
		.semaphoreCount = 1,
		.pSemaphores = &this->handle,
		.pValues = &value
	};
	this->setup->device.waitSemaphores(waitInfo, UINT64_MAX);
}

Semaphore::~Semaphore() {
	this->setup->device.destroySemaphore(this->handle);
}

Requirements Semaphore::getRequirements()
{
	Requirements extensions;
	extensions.deviceExtensions = {
		VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME
	};
	return extensions;
}
