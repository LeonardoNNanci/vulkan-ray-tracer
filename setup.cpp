#include "setup.hpp"

VkInstance globalInstance;
VkDevice globalDevice;

IHasSetup::IHasSetup(std::shared_ptr<Setup> setup) : setup(setup) {};

SetupBuilder SetupBuilder::addExtensions(Requirements extensions)
{
	for (auto& extension : extensions.instanceExtensions)
		this->extensions.instanceExtensions.push_back(extension);

	for (auto& extension : extensions.deviceExtensions)
		this->extensions.deviceExtensions.push_back(extension);

	return *this;
}

std::shared_ptr<Setup> SetupBuilder::build()
{
    this->setup = std::make_shared<Setup>();
    this->setup->instance = this->createInstance();
    this->setup->physicalDevice = this->getPhysicalDevice();
    this->setup->graphicsQueue.familyIndex = this->getQueueFamilyIndex(vk::QueueFlagBits::eGraphics);
    this->setup->device = this->createDevice();
    this->setup->graphicsQueue.handle = this->getQueue(this->setup->graphicsQueue.familyIndex);

    globalInstance = static_cast<VkInstance>(this->setup->instance);
    globalDevice = static_cast<VkDevice>(this->setup->device);

    return this->setup;
}

vk::Instance SetupBuilder::createInstance()
{
    vk::ApplicationInfo appInfo{
        .pApplicationName = "Ray Tracing Renderer",
        .applicationVersion = VK_MAKE_VERSION(0,1,0),
        .pEngineName = "No engine",
        .engineVersion = VK_MAKE_VERSION(0,1,0),
        .apiVersion = VK_API_VERSION_1_3
    };
    vk::InstanceCreateInfo instanceInfo{
        .pApplicationInfo = &appInfo,
    };
    instanceInfo.setPEnabledExtensionNames(this->extensions.instanceExtensions);

    return vk::createInstance(instanceInfo);
}

vk::PhysicalDevice SetupBuilder::getPhysicalDevice()
{
    std::vector<vk::PhysicalDevice> physicalDevices = this->setup->instance.enumeratePhysicalDevices();
    return physicalDevices[0];
}

vk::Device SetupBuilder::createDevice()
{
    std::vector<vk::ExtensionProperties> availableExtensions = this->setup->physicalDevice.enumerateDeviceExtensionProperties();

    vk::PhysicalDeviceProperties deviceProperties = this->setup->physicalDevice.getProperties();
    vk::PhysicalDeviceFeatures deviceFeatures = this->setup->physicalDevice.getFeatures();

    size_t count = 0;
    for (auto& required : this->extensions.deviceExtensions)
        for (auto& available : availableExtensions)
            if (strcmp(required, available.extensionName) == 0) {
                count++;
                break;
            }

    if (count < this->extensions.deviceExtensions.size())
        throw std::runtime_error("One or more device extensions are not available!");

    float priorities[] = { 0.0f };
    vk::DeviceQueueCreateInfo queueInfo{
        .queueFamilyIndex = this->setup->graphicsQueue.familyIndex,
        .queueCount = 1,
        .pQueuePriorities = priorities
    };

    vk::PhysicalDeviceTimelineSemaphoreFeatures timelineSemaphoreFeatures{
        .timelineSemaphore = vk::True
    };
    vk::PhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeatures{
        .pNext = &timelineSemaphoreFeatures,
        .rayTracingPipeline = vk::True
    };
    vk::PhysicalDeviceBufferDeviceAddressFeatures bufferAddressFeatures{
        .pNext = &rtPipelineFeatures,
        .bufferDeviceAddress = vk::True
    };
    vk::PhysicalDeviceAccelerationStructureFeaturesKHR accelerationFeatures{
        .pNext = &bufferAddressFeatures,
        .accelerationStructure = vk::True
    };

    vk::DeviceCreateInfo deviceInfo{
    .pNext = &accelerationFeatures,
    .queueCreateInfoCount = 1,
    .pQueueCreateInfos = &queueInfo,
    };
    deviceInfo.setPEnabledExtensionNames(this->extensions.deviceExtensions);

    return this->setup->physicalDevice.createDevice(deviceInfo);
	return vk::Device();
}

uint32_t SetupBuilder::getQueueFamilyIndex(vk::QueueFlags queueFamily)
{
    auto queueFamilyProperties = this->setup->physicalDevice.getQueueFamilyProperties();

    for (int i = 0; i < queueFamilyProperties.size(); i++)
        if (queueFamilyProperties[i].queueFlags & queueFamily) {
            return i;
        }
}

vk::Queue SetupBuilder::getQueue(uint32_t familyIndex)
{
    return this->setup->device.getQueue(familyIndex, 0);
}

Setup::~Setup() {
    this->device.destroy();
    this->instance.destroy();
}

VkResult vkCreateAccelerationStructureKHR(
    VkDevice                                    device,
    const VkAccelerationStructureCreateInfoKHR* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkAccelerationStructureKHR* pAccelerationStructure) {
    auto func = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR"));
    if (func == nullptr)
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    func(device, pCreateInfo, pAllocator, pAccelerationStructure);
}

void vkDestroyAccelerationStructureKHR(
    VkDevice                                    device,
    VkAccelerationStructureKHR                  accelerationStructure,
    const VkAllocationCallbacks* pAllocator) {
    auto func = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR"));
    func(device, accelerationStructure, pAllocator);
}

void vkGetAccelerationStructureBuildSizesKHR(
    VkDevice                                    device,
    VkAccelerationStructureBuildTypeKHR         buildType,
    const VkAccelerationStructureBuildGeometryInfoKHR* pBuildInfo,
    const uint32_t* pMaxPrimitiveCounts,
    VkAccelerationStructureBuildSizesInfoKHR* pSizeInfo) {
    auto func = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR"));
    func(device, buildType, pBuildInfo, pMaxPrimitiveCounts, pSizeInfo);
}

VkResult vkBuildAccelerationStructuresKHR(
    VkDevice                                    device,
    VkDeferredOperationKHR                      deferredOperation,
    uint32_t                                    infoCount,
    const VkAccelerationStructureBuildGeometryInfoKHR* pInfos,
    const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos) {
    auto func = reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(device, "vkBuildAccelerationStructuresKHR"));
    if (func == nullptr)
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    func(device, deferredOperation, infoCount, pInfos, ppBuildRangeInfos);
}

void vkCmdBuildAccelerationStructuresKHR(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    infoCount,
    const VkAccelerationStructureBuildGeometryInfoKHR* pInfos,
    const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos) {
    auto func = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(globalDevice, "vkCmdBuildAccelerationStructuresKHR"));
    if (func == nullptr)
        throw std::runtime_error("Could not load procedure 'vkCmdBuildAccelerationStructuresKHR'!");
    func(commandBuffer, infoCount, pInfos, ppBuildRangeInfos);

}

void vkCmdWriteAccelerationStructuresPropertiesKHR(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    accelerationStructureCount,
    const VkAccelerationStructureKHR* pAccelerationStructures,
    VkQueryType                                 queryType,
    VkQueryPool                                 queryPool,
    uint32_t                                    firstQuery) {
    auto func = reinterpret_cast<PFN_vkCmdWriteAccelerationStructuresPropertiesKHR>(vkGetDeviceProcAddr(globalDevice, "vkCmdWriteAccelerationStructuresPropertiesKHR"));
    if (func == nullptr)
        throw std::runtime_error("Could not load procedure 'vkCmdBuildAccelerationStructuresKHR'!");
    func(commandBuffer, accelerationStructureCount, pAccelerationStructures, queryType, queryPool, firstQuery);
}

VkResult vkCopyAccelerationStructureKHR(
    VkDevice                                    device,
    VkDeferredOperationKHR                      deferredOperation,
    const VkCopyAccelerationStructureInfoKHR* pInfo) {
    auto func = reinterpret_cast<PFN_vkCopyAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkCopyAccelerationStructureKHR"));
    if (func == nullptr)
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    func(device, deferredOperation, pInfo);
}

VkResult vkCreateRayTracingPipelinesKHR(
    VkDevice                                    device,
    VkDeferredOperationKHR                      deferredOperation,
    VkPipelineCache                             pipelineCache,
    uint32_t                                    createInfoCount,
    const VkRayTracingPipelineCreateInfoKHR* pCreateInfos,
    const VkAllocationCallbacks* pAllocator,
    VkPipeline* pPipelines) {
    auto func = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR"));
    if (func == nullptr)
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    func(device, deferredOperation, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
}


void vkCmdCopyAccelerationStructureKHR(
    VkCommandBuffer                             commandBuffer,
    const VkCopyAccelerationStructureInfoKHR* pInfo) {
    auto func = reinterpret_cast<PFN_vkCmdCopyAccelerationStructureKHR>(vkGetDeviceProcAddr(globalDevice, "vkCmdCopyAccelerationStructureKHR"));
    func(commandBuffer, pInfo);
}

VkResult vkGetRayTracingShaderGroupHandlesKHR(
    VkDevice                                    device,
    VkPipeline                                  pipeline,
    uint32_t                                    firstGroup,
    uint32_t                                    groupCount,
    size_t                                      dataSize,
    void*                                       pData) {
    auto func = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(globalDevice, "vkGetRayTracingShaderGroupHandlesKHR"));
    return func(device, pipeline, firstGroup, groupCount, dataSize, pData);
}

void vkCmdTraceRaysKHR(
    VkCommandBuffer                             commandBuffer,
    const VkStridedDeviceAddressRegionKHR*      pRaygenShaderBindingTable,
    const VkStridedDeviceAddressRegionKHR*      pMissShaderBindingTable,
    const VkStridedDeviceAddressRegionKHR*      pHitShaderBindingTable,
    const VkStridedDeviceAddressRegionKHR*      pCallableShaderBindingTable,
    uint32_t                                    width,
    uint32_t                                    height,
    uint32_t                                    depth) {
    auto func = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(globalDevice, "vkCmdTraceRaysKHR"));
    func(commandBuffer, pRaygenShaderBindingTable, pMissShaderBindingTable, pHitShaderBindingTable, pCallableShaderBindingTable, width, height, depth);
}

VkDeviceAddress vkGetAccelerationStructureDeviceAddressKHR(
    VkDevice                                    device,
    const VkAccelerationStructureDeviceAddressInfoKHR* pInfo) {
    auto func = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(globalDevice, "vkGetAccelerationStructureDeviceAddressKHR"));
    return func(device, pInfo);
}

VkResult vkGetMemoryWin32HandleKHR(
    VkDevice                                    device,
    const VkMemoryGetWin32HandleInfoKHR* pGetWin32HandleInfo,
    HANDLE* pHandle) {
    auto func = reinterpret_cast<PFN_vkGetMemoryWin32HandleKHR>(vkGetDeviceProcAddr(globalDevice, "vkGetMemoryWin32HandleKHR"));
    return func(device, pGetWin32HandleInfo, pHandle);
}