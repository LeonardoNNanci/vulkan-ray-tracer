//#define VK_USE_PLATFORM_WIN32_KHR
//#define GLFW_INCLUDE_VULKAN
//#include <GLFW/glfw3.h>
//#define GLFW_EXPOSE_NATIVE_WIN32
//#include <GLFW/glfw3native.h>
//
//#define GLM_FORCE_RADIANS
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
//#include <glm/vec4.hpp>
//#include <glm/mat4x4.hpp>
//#include <glm/gtc/matrix_transform.hpp>
//
//
//#include<fstream>
//#include <iostream>
//#include<vector>
//#include <chrono>
//
//#include<vulkan/vulkan.hpp>
//
//#include "happly.h"
//
//
//// --------------------------------------------------------------------------------------
//// --------------------------------------------------------------------------------------
//// Structs ------------------------------------------------------------------------------
//// --------------------------------------------------------------------------------------
//// --------------------------------------------------------------------------------------
//enum StageIndices {
//    eRaygen,
//    eMiss,
//    eShadowMiss,
//    eClosestHit,
//    eShaderGroupCount
//};
//
//struct Vertex {
//    glm::vec4 pos;
//
//    static vk::VertexInputBindingDescription getBindingDescription();
//
//    static std::array<vk::VertexInputAttributeDescription, 1> getAttributeDescriptions();
//};
//
//struct Ray {
//    glm::vec4 clearColor;
//    glm::vec4 lightPosition;
//    float lightIntensity;
//    int lightType;
//    glm::mat4 projInv;
//    glm::mat4 viewInv;
//};
//
//struct QueueFamilyIndex {
//    uint32_t graphics;
//    uint32_t transfer;
//    uint32_t compute;
//};
//
//// --------------------------------------------------------------------------------------
//// --------------------------------------------------------------------------------------
//// Classes ------------------------------------------------------------------------------
//// --------------------------------------------------------------------------------------
//// --------------------------------------------------------------------------------------
//class AllocatedBuffer {
//public:
//    vk::Buffer buffer;
//    vk::DeviceMemory memory;
//    vk::DeviceSize size;
//
//    AllocatedBuffer() {
//        this->buffer = nullptr;
//        this->memory = nullptr;
//    };
//
//    AllocatedBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags props);
//
//    template<typename T>
//    AllocatedBuffer(std::vector<T> data, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags props);
//
//    template<typename T>
//    void fill(std::vector<T> data);
//
//    vk::DeviceAddress getBufferDeviceAdress();
//
//    ~AllocatedBuffer();
//private:
//    vk::Buffer createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage);
//
//    vk::DeviceMemory createMemory(vk::DeviceSize size, vk::MemoryPropertyFlags props);
//
//    void createBufferAndMem(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags props);
//
//    void copyBuffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size);
//
//    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
//
//};
//
//class Swapchain {
//public:
//    vk::SwapchainKHR swapchain;
//    std::vector<vk::ImageView> imageViews;
//    vk::Extent2D extent;
//    vk::Format format;
//
//    Swapchain();
//
//    ~Swapchain();
//
//private:
//    void createImageViews();
//};
//
//class AccelerationStructure {
//public:
//    vk::AccelerationStructureKHR topLevel;
//    std::vector<vk::AccelerationStructureKHR> bottomLevel;
//    AllocatedBuffer* topLevelBuffer;
//    AllocatedBuffer* bottomLevelBuffer;
//
//    AccelerationStructure() = default;
//
//    /*AccelerationStructure(const std::string model) {
//        Util::vertexBuffer = std::make_unique<AllocatedBuffer>(vertices,
//            vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
//            vk::MemoryPropertyFlagBits::eDeviceLocal);
//        Util::indexBuffer = std::make_unique<AllocatedBuffer>(indices,
//            vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
//            vk::MemoryPropertyFlagBits::eDeviceLocal);
//    }*/
//
//    AccelerationStructure(AllocatedBuffer& vertexBuffer, AllocatedBuffer& indexBuffer, int nVertices, int nIndices) {
//        createBottomLevel(vertexBuffer.getBufferDeviceAdress(), indexBuffer.getBufferDeviceAdress(), nVertices, nIndices);
//        createTopLevel();
//    };
//
//    void createTopLevel();
//
//    void createBottomLevel(vk::DeviceAddress verticesAddress, vk::DeviceAddress indicesAddress, int nVertices, int nIndices);
//};
//
//struct ShaderBindingTable {
//    AllocatedBuffer* buffer;
//    vk::StridedDeviceAddressRegionKHR rgenRegion;
//    vk::StridedDeviceAddressRegionKHR missRegion;
//    vk::StridedDeviceAddressRegionKHR hitRegion;
//    vk::StridedDeviceAddressRegionKHR callRegion;
//};
//
//struct Model3D {
//    uint32_t index;
//    AllocatedBuffer vertexBuffer;
//    AllocatedBuffer indexBuffer;
//};
//
//struct ModelInstance {
//    glm::mat3x4 transform;
//    uint32_t hitShaderIndex;
//    Model3D model;
//};
//
//// --------------------------------------------------------------------------------------
//// --------------------------------------------------------------------------------------
//// Helper handles -----------------------------------------------------------------------
//// --------------------------------------------------------------------------------------
//// --------------------------------------------------------------------------------------
//namespace Util {
//    vk::Instance instance;
//    vk::Device device;
//    vk::PhysicalDevice physicalDevice;
//
//    vk::SurfaceKHR surface;
//    Swapchain* swapchain;
//
//    vk::CommandPool commandPool;
//    QueueFamilyIndex queueFamilyIndex;
//
//    vk::Queue graphicsQueue;
//    vk::Queue transferQueue;
//    vk::Queue computeQueue;
//
//    vk::Pipeline rayTracingPipeline;
//    vk::PipelineLayout pipelineLayout;
//    ShaderBindingTable SBT;
//    AccelerationStructure* accelerationStructure;
//
//    std::unique_ptr<AllocatedBuffer> vertexBuffer;
//    std::unique_ptr<AllocatedBuffer> indexBuffer;
//
//    vk::DescriptorPool descriptorPool;
//    std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
//    std::vector<vk::DescriptorSet> descriptorSets;
//    uint32_t currentImageIndex;
//
//    vk::Fence inFlightFence;
//    vk::Semaphore imageAvailableSemaphore;
//    vk::Semaphore renderFinishedSemaphore;
//};
//
//// --------------------------------------------------------------------------------------
//// --------------------------------------------------------------------------------------
//// Extension Functions ------------------------------------------------------------------
//// --------------------------------------------------------------------------------------
//// --------------------------------------------------------------------------------------
//VkResult vkCreateAccelerationStructureKHR(
//    VkDevice                                    device,
//    const VkAccelerationStructureCreateInfoKHR* pCreateInfo,
//    const VkAllocationCallbacks* pAllocator,
//    VkAccelerationStructureKHR* pAccelerationStructure) {
//    auto func = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR"));
//    if (func == nullptr)
//        return VK_ERROR_EXTENSION_NOT_PRESENT;
//    func(device, pCreateInfo, pAllocator, pAccelerationStructure);
//}
//
//void vkDestroyAccelerationStructureKHR(
//    VkDevice                                    device,
//    VkAccelerationStructureKHR                  accelerationStructure,
//    const VkAllocationCallbacks* pAllocator) {
//    auto func = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR"));
//    func(device, accelerationStructure, pAllocator);
//}
//
//void vkGetAccelerationStructureBuildSizesKHR(
//    VkDevice                                    device,
//    VkAccelerationStructureBuildTypeKHR         buildType,
//    const VkAccelerationStructureBuildGeometryInfoKHR* pBuildInfo,
//    const uint32_t* pMaxPrimitiveCounts,
//    VkAccelerationStructureBuildSizesInfoKHR* pSizeInfo) {
//    auto func = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR"));
//    func(device, buildType, pBuildInfo, pMaxPrimitiveCounts, pSizeInfo);
//}
//
//VkResult vkBuildAccelerationStructuresKHR(
//    VkDevice                                    device,
//    VkDeferredOperationKHR                      deferredOperation,
//    uint32_t                                    infoCount,
//    const VkAccelerationStructureBuildGeometryInfoKHR* pInfos,
//    const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos) {
//    auto func = reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(device, "vkBuildAccelerationStructuresKHR"));
//    if (func == nullptr)
//        return VK_ERROR_EXTENSION_NOT_PRESENT;
//    func(device, deferredOperation, infoCount, pInfos, ppBuildRangeInfos);
//}
//
//void vkCmdBuildAccelerationStructuresKHR(
//    VkCommandBuffer                             commandBuffer,
//    uint32_t                                    infoCount,
//    const VkAccelerationStructureBuildGeometryInfoKHR* pInfos,
//    const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos) {
//    auto func = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(Util::device.getProcAddr("vkCmdBuildAccelerationStructuresKHR"));
//    if (func == nullptr)
//        throw std::runtime_error("Could not load procedure 'vkCmdBuildAccelerationStructuresKHR'!");
//    func(commandBuffer, infoCount, pInfos, ppBuildRangeInfos);
//
//}
//
//void vkCmdWriteAccelerationStructuresPropertiesKHR(
//    VkCommandBuffer                             commandBuffer,
//    uint32_t                                    accelerationStructureCount,
//    const VkAccelerationStructureKHR* pAccelerationStructures,
//    VkQueryType                                 queryType,
//    VkQueryPool                                 queryPool,
//    uint32_t                                    firstQuery) {
//    auto func = reinterpret_cast<PFN_vkCmdWriteAccelerationStructuresPropertiesKHR>(Util::device.getProcAddr("vkCmdWriteAccelerationStructuresPropertiesKHR"));
//    if (func == nullptr)
//        throw std::runtime_error("Could not load procedure 'vkCmdBuildAccelerationStructuresKHR'!");
//    func(commandBuffer, accelerationStructureCount, pAccelerationStructures, queryType, queryPool, firstQuery);
//}
//
//VkResult vkCopyAccelerationStructureKHR(
//    VkDevice                                    device,
//    VkDeferredOperationKHR                      deferredOperation,
//    const VkCopyAccelerationStructureInfoKHR* pInfo) {
//    auto func = reinterpret_cast<PFN_vkCopyAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkCopyAccelerationStructureKHR"));
//    if (func == nullptr)
//        return VK_ERROR_EXTENSION_NOT_PRESENT;
//    func(device, deferredOperation, pInfo);
//}
//
//VkResult vkCreateRayTracingPipelinesKHR(
//    VkDevice                                    device,
//    VkDeferredOperationKHR                      deferredOperation,
//    VkPipelineCache                             pipelineCache,
//    uint32_t                                    createInfoCount,
//    const VkRayTracingPipelineCreateInfoKHR* pCreateInfos,
//    const VkAllocationCallbacks* pAllocator,
//    VkPipeline* pPipelines) {
//    auto func = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR"));
//    if (func == nullptr)
//        return VK_ERROR_EXTENSION_NOT_PRESENT;
//    func(device, deferredOperation, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
//}
//
//
//void vkCmdCopyAccelerationStructureKHR(
//    VkCommandBuffer                             commandBuffer,
//    const VkCopyAccelerationStructureInfoKHR* pInfo) {
//    auto func = reinterpret_cast<PFN_vkCmdCopyAccelerationStructureKHR>(Util::device.getProcAddr("vkCmdCopyAccelerationStructureKHR"));
//    func(commandBuffer, pInfo);
//}
//
//VkResult vkGetRayTracingShaderGroupHandlesKHR(
//    VkDevice                                    device,
//    VkPipeline                                  pipeline,
//    uint32_t                                    firstGroup,
//    uint32_t                                    groupCount,
//    size_t                                      dataSize,
//    void*                                       pData) {
//    auto func = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(Util::device.getProcAddr("vkGetRayTracingShaderGroupHandlesKHR"));
//    return func(device, pipeline, firstGroup, groupCount, dataSize, pData);
//}
//
//void vkCmdTraceRaysKHR(
//    VkCommandBuffer                             commandBuffer,
//    const VkStridedDeviceAddressRegionKHR*      pRaygenShaderBindingTable,
//    const VkStridedDeviceAddressRegionKHR*      pMissShaderBindingTable,
//    const VkStridedDeviceAddressRegionKHR*      pHitShaderBindingTable,
//    const VkStridedDeviceAddressRegionKHR*      pCallableShaderBindingTable,
//    uint32_t                                    width,
//    uint32_t                                    height,
//    uint32_t                                    depth) {
//    auto func = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(Util::device.getProcAddr("vkCmdTraceRaysKHR"));
//    func(commandBuffer, pRaygenShaderBindingTable, pMissShaderBindingTable, pHitShaderBindingTable, pCallableShaderBindingTable, width, height, depth);
//}
//
//class CommandBuffer {
//public:
//    vk::CommandBuffer cmd;
//    
//    CommandBuffer() {
//        vk::CommandBufferAllocateInfo commandBufferInfo{
//        .commandPool = Util::commandPool,
//        .level = vk::CommandBufferLevel::ePrimary,
//        .commandBufferCount = 1
//        };
//        cmd = Util::device.allocateCommandBuffers(commandBufferInfo)[0];
//        vk::CommandBufferBeginInfo beginInfo{};
//        cmd.begin(beginInfo);
//    }
//
//    void submit() {
//        cmd.end();
//        vk::SubmitInfo submitInfo{
//            .commandBufferCount = 1,
//            .pCommandBuffers = &cmd
//        };
//        Util::graphicsQueue.submit(submitInfo);
//        Util::graphicsQueue.waitIdle();
//    }
//
//    ~CommandBuffer() {
//        Util::device.freeCommandBuffers(Util::commandPool, { cmd });
//    }
//};
//
//VkDeviceAddress vkGetBufferDeviceAddress(VkDevice device, const VkBufferDeviceAddressInfo* pInfo) {
//    auto func = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(Util::device.getProcAddr("vkGetBufferDeviceAddressKHR"));
//    return func(device, pInfo);
//}
//
//VkDeviceAddress vkGetAccelerationStructureDeviceAddressKHR(
//    VkDevice                                    device,
//    const VkAccelerationStructureDeviceAddressInfoKHR* pInfo) {
//    auto func = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(Util::device.getProcAddr("vkGetAccelerationStructureDeviceAddressKHR"));
//    return func(device, pInfo);
//}
//
//// --------------------------------------------------------------------------------------
//// --------------------------------------------------------------------------------------
//// Vertex Implementation ----------------------------------------------------------------
//// --------------------------------------------------------------------------------------
//// --------------------------------------------------------------------------------------
//
//vk::VertexInputBindingDescription Vertex::getBindingDescription() {
//    return {
//    .binding = 0,
//    .stride = sizeof(Vertex),
//    .inputRate = vk::VertexInputRate::eVertex
//    };
//}
//
//std::array<vk::VertexInputAttributeDescription, 1> Vertex::getAttributeDescriptions() {
//    vk::VertexInputAttributeDescription posDescription{
//        .location = 0,
//        .binding = 0,
//        .format = vk::Format::eR32G32B32Sfloat,
//        .offset = offsetof(Vertex, pos)
//    };
//    return { posDescription };
//}
//
//// --------------------------------------------------------------------------------------
//// --------------------------------------------------------------------------------------
//// Helper Functions ---------------------------------------------------------------------
//// --------------------------------------------------------------------------------------
//// --------------------------------------------------------------------------------------
//vk::Instance createInstance(const std::vector<const char*>& validationLayers, std::vector<const char*>& instanceExtensions) {
//    vk::ApplicationInfo appInfo{
//        .pApplicationName = "Ray Tracing Renderer",
//        .applicationVersion = VK_MAKE_VERSION(0,1,0),
//        .pEngineName = "No engine",
//        .engineVersion = VK_MAKE_VERSION(1,0,0),
//        .apiVersion = VK_API_VERSION_1_3
//    };
//    vk::InstanceCreateInfo instanceInfo{
//        .pApplicationInfo = &appInfo,
//        .enabledLayerCount = static_cast<uint32_t>(validationLayers.size()),
//        .ppEnabledLayerNames = validationLayers.data(),
//        .enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size()),
//        .ppEnabledExtensionNames = instanceExtensions.data(),
//    };
//    return vk::createInstance(instanceInfo);
//}
//
//vk::Device createDevice(std::vector<const char*>& deviceExtensions) {
//    std::vector<vk::ExtensionProperties> availableExtensions = Util::physicalDevice.enumerateDeviceExtensionProperties();
//
//    vk::PhysicalDeviceProperties deviceProperties = Util::physicalDevice.getProperties();
//    vk::PhysicalDeviceFeatures deviceFeatures = Util::physicalDevice.getFeatures();
//
//    size_t count = 0;
//    for (auto& required : deviceExtensions)
//        for (auto& available : availableExtensions)
//            if (strcmp(required, available.extensionName) == 0) {
//                count++;
//                break;
//            }
//
//    if (count < deviceExtensions.size())
//        throw std::runtime_error("One or more device extensions are not available!");
//
//    float priorities[] = { 0.0f };
//    vk::DeviceQueueCreateInfo queueInfo{
//        .queueFamilyIndex = Util::queueFamilyIndex.graphics,
//        .queueCount = 1,
//        .pQueuePriorities = priorities
//    };
//
//    vk::PhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeatures{
//        .rayTracingPipeline = vk::True
//    };
//    vk::PhysicalDeviceBufferDeviceAddressFeatures bufferAddressFeatures{
//        .pNext = &rtPipelineFeatures,
//        .bufferDeviceAddress = vk::True
//    };
//    vk::PhysicalDeviceAccelerationStructureFeaturesKHR accelerationFeatures{
//        .pNext = &bufferAddressFeatures,
//        .accelerationStructure = vk::True
//    };
//    vk::DeviceCreateInfo deviceInfo{
//        .pNext = &accelerationFeatures,
//        .queueCreateInfoCount = 1,
//        .pQueueCreateInfos = &queueInfo,
//        .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
//        .ppEnabledExtensionNames = deviceExtensions.data(),
//    };
//
//    return Util::physicalDevice.createDevice(deviceInfo);
//}
//
//vk::SurfaceKHR createSurface(GLFWwindow* window) {
//    auto graphicsQueue = Util::device.getQueue(Util::queueFamilyIndex.graphics, 0);
//    vk::Win32SurfaceCreateInfoKHR surfaceInfo{
//        .hinstance = GetModuleHandle(nullptr),
//        .hwnd = glfwGetWin32Window(window)
//    };
//    return Util::instance.createWin32SurfaceKHR(surfaceInfo);
//}
//
//vk::CommandPool createCommandPool(uint32_t queueIndex)
//{
//    vk::CommandPoolCreateInfo commandPoolInfo{
//        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
//        .queueFamilyIndex = queueIndex
//    };
//
//    return Util::device.createCommandPool(commandPoolInfo);
//}
//
//void endCommandBuffer(vk::CommandBuffer commandBuffer) {
//    commandBuffer.end();
//}
//
//void recordRenderPass(vk::CommandBuffer commandBuffer, vk::RenderPass renderPass, vk::Framebuffer framebuffer) {
//    vk::ClearValue clearColor{
//        .color = {
//            .float32 = {{0.0f, 0.0f, 0.0f, 1.0f}}
//        }
//    };
//    vk::RenderPassBeginInfo renderPassInfo{
//        .renderPass = renderPass,
//        .framebuffer = framebuffer,
//        .renderArea = {
//            .offset = {0,0},
//            .extent = Util::swapchain->extent
//        },
//        .clearValueCount = 1,
//        .pClearValues = &clearColor
//    };
//    commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
//
//    commandBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, Util::rayTracingPipeline);
//
//    vk::Viewport viewport{
//        .x = 0.0f,
//        .y = 0.0f,
//        .width = static_cast<float>(Util::swapchain->extent.width),
//        .height = static_cast<float>(Util::swapchain->extent.height),
//        .minDepth = 0.0f,
//        .maxDepth = 1.0f
//    };
//    commandBuffer.setViewport(0, { viewport });
//
//    vk::Rect2D scissor{
//        .offset = {0, 0},
//        .extent = Util::swapchain->extent
//    };
//    commandBuffer.setScissor(0, { scissor });
//
//    int VERTEX_COUNT = 1; // TODO get real vertex count
//    commandBuffer.draw(VERTEX_COUNT, 1, 0, 0);
//
//    commandBuffer.endRenderPass();
//}
//
//vk::RenderPass createRenderPass() {
//    vk::AttachmentDescription colorAttachmentDesc{
//        .format = Util::swapchain->format,
//        .samples = vk::SampleCountFlagBits::e1,
//        .loadOp = vk::AttachmentLoadOp::eClear,
//        .storeOp = vk::AttachmentStoreOp::eStore,
//        .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
//        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
//        .initialLayout = vk::ImageLayout::eUndefined,
//        .finalLayout = vk::ImageLayout::ePresentSrcKHR,
//    };
//
//    vk::AttachmentReference colorAttachmentRef{
//        .attachment = 0,
//        .layout = vk::ImageLayout::eColorAttachmentOptimal
//    };
//
//    vk::SubpassDescription subpassDesc{
//        .pipelineBindPoint = vk::PipelineBindPoint::eRayTracingKHR,
//        .colorAttachmentCount = 1,
//        .pColorAttachments = &colorAttachmentRef
//    };
//
//    vk::RenderPassCreateInfo renderPassInfo{
//        .attachmentCount = 1,
//        .pAttachments = &colorAttachmentDesc,
//        .subpassCount = 1,
//        .pSubpasses = &subpassDesc
//    };
//
//    return Util::device.createRenderPass(renderPassInfo);
//}
//
//uint32_t getQueueFamilyIndex(vk::QueueFlagBits queueFamily)
//{
//    auto queueFamilyProperties = Util::physicalDevice.getQueueFamilyProperties();
//
//    for (int i = 0; i < queueFamilyProperties.size(); i++)
//        if (queueFamilyProperties[i].queueFlags & queueFamily) {
//            return i;
//        }
//}
//
//vk::DescriptorSetLayout createRTDescriptorSetLayout() {
//    std::vector<vk::DescriptorSetLayoutBinding> bindings({
//        {
//        .binding = 0,
//        .descriptorType = vk::DescriptorType::eAccelerationStructureKHR,
//        .descriptorCount = 1,
//        .stageFlags = vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR
//        },
//        {
//        .binding = 1,
//        .descriptorType = vk::DescriptorType::eStorageImage,
//        .descriptorCount = 1,
//        .stageFlags = vk::ShaderStageFlagBits::eRaygenKHR
//        }
//    });
//
//    vk::DescriptorSetLayoutCreateInfo layoutInfo{};
//    layoutInfo.setBindings(bindings);
//
//    return Util::device.createDescriptorSetLayout(layoutInfo);
//}
//
//vk::DescriptorSetLayout createSceneDescriptorSetLayout() {
//    std::vector<vk::DescriptorSetLayoutBinding> bindings({
//        {
//        .binding = 0,
//        .descriptorType = vk::DescriptorType::eStorageBuffer,
//        .descriptorCount = 1,
//        .stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR
//        },
//        {
//        .binding = 1,
//        .descriptorType = vk::DescriptorType::eStorageBuffer,
//        .descriptorCount = 1,
//        .stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR
//        }
//    });
//
//    vk::DescriptorSetLayoutCreateInfo layoutInfo{};
//    layoutInfo.setBindings(bindings);
//
//    return Util::device.createDescriptorSetLayout(layoutInfo);
//}
//
//vk::ShaderModule createShaderModule(std::vector<char> code) {
//    vk::ShaderModuleCreateInfo shaderInfo{
//        .codeSize = static_cast<uint32_t>(code.size()),
//        .pCode = reinterpret_cast<uint32_t*>(code.data())
//    };
//    return Util::device.createShaderModule(shaderInfo);
//}
//
//std::vector<char> readSPV(const std::string& filename) {
//    std::ifstream file(filename, std::ios::ate | std::ios::binary);
//
//    if (!file.is_open()) {
//        throw std::runtime_error("failed to open file!");
//    }
//
//    size_t fileSize = (size_t)file.tellg();
//    std::vector<char> buffer(fileSize);
//
//    file.seekg(0);
//    file.read(buffer.data(), fileSize);
//
//    file.close();
//
//    return buffer;
//}
//
//std::tuple<std::vector<Vertex>, std::vector<uint32_t>> readPLY(const std::string& filename) {
//    happly::PLYData data(filename);
//    auto rawVertices = data.getVertexPositions();
//    auto rawIndices = data.getFaceIndices();
//
//    std::vector<Vertex> vertices(rawVertices.size());
//    for (int i = 0; i < vertices.size(); i++) {
//        auto& [x, y, z] = rawVertices[i];
//        vertices[i].pos = glm::vec4(x, y, z, 1.);
//    }
//
//    std::vector<uint32_t> indices(3 * rawIndices.size());
//    for (int i = 0; i < rawIndices.size(); i++) {
//        indices[i * 3] = rawIndices[i][0];
//        indices[i * 3 + 1] = rawIndices[i][1];
//        indices[i * 3 + 2] = rawIndices[i][2];
//    }
//
//    return{ vertices, indices };
//}
//
//vk::Pipeline createRayTracingPipeline() {
//    auto rayGenShader = createShaderModule(readSPV("./shaders/raygen.spv"));
//    auto missShader = createShaderModule(readSPV("./shaders/miss.spv"));
//    auto shadowShader = createShaderModule(readSPV("./shaders/shadow.spv"));
//    auto closestHitShader = createShaderModule(readSPV("./shaders/closesthit.spv"));
//    std::vector<vk::PipelineShaderStageCreateInfo> stages(StageIndices::eShaderGroupCount);
//    stages[StageIndices::eRaygen] = {
//            .stage = vk::ShaderStageFlagBits::eRaygenKHR,
//            .module = rayGenShader,
//            .pName = "main",
//        };
//    stages[StageIndices::eMiss] = {
//        .stage = vk::ShaderStageFlagBits::eMissKHR,
//        .module = missShader,
//        .pName = "main",
//        };
//    stages[StageIndices::eShadowMiss] = {
//        .stage = vk::ShaderStageFlagBits::eMissKHR,
//        .module = shadowShader,
//        .pName = "main",
//    };
//    stages[StageIndices::eClosestHit] = {
//        .stage = vk::ShaderStageFlagBits::eClosestHitKHR,
//        .module = closestHitShader,
//        .pName = "main",
//        };
//
//    std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shaderGroups({
//        {
//            .type = vk::RayTracingShaderGroupTypeKHR::eGeneral,
//            .generalShader = StageIndices::eRaygen,
//            .closestHitShader = vk::ShaderUnusedKHR,
//            .anyHitShader = vk::ShaderUnusedKHR,
//            .intersectionShader = vk::ShaderUnusedKHR
//        },
//        {
//            .type = vk::RayTracingShaderGroupTypeKHR::eGeneral,
//            .generalShader = StageIndices::eMiss,
//            .closestHitShader = vk::ShaderUnusedKHR,
//            .anyHitShader = vk::ShaderUnusedKHR,
//            .intersectionShader = vk::ShaderUnusedKHR
//        },
//        {
//            .type = vk::RayTracingShaderGroupTypeKHR::eGeneral,
//            .generalShader = StageIndices::eShadowMiss,
//            .closestHitShader = vk::ShaderUnusedKHR,
//            .anyHitShader = vk::ShaderUnusedKHR,
//            .intersectionShader = vk::ShaderUnusedKHR
//        },
//        {
//            .type = vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup,
//            .generalShader = StageIndices::eClosestHit,
//            .closestHitShader = vk::ShaderUnusedKHR,
//            .anyHitShader = vk::ShaderUnusedKHR,
//            .intersectionShader = vk::ShaderUnusedKHR
//        },
//        });
//
//    vk::PushConstantRange pcRange{
//        .stageFlags = vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR,
//        .offset = 0,
//        .size = sizeof(Ray),
//    };
//
//    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{
//        .pushConstantRangeCount = 1,
//        .pPushConstantRanges = &pcRange
//    };
//    pipelineLayoutCreateInfo.setSetLayouts(Util::descriptorSetLayouts);
//
//    Util::pipelineLayout = Util::device.createPipelineLayout(pipelineLayoutCreateInfo);
//
//    vk::RayTracingPipelineCreateInfoKHR pipelineInfo{
//        .maxPipelineRayRecursionDepth = 2,
//        .layout = Util::pipelineLayout,
//    };
//    pipelineInfo.setStages(stages);
//    pipelineInfo.setGroups(shaderGroups);
//
//    auto pipeline = Util::device.createRayTracingPipelineKHR(nullptr, nullptr, pipelineInfo).value;
//
//    Util::device.destroyShaderModule(rayGenShader);
//    Util::device.destroyShaderModule(missShader);
//    Util::device.destroyShaderModule(shadowShader);
//    Util::device.destroyShaderModule(closestHitShader);
//
//    return pipeline;
//}
//
//void cmdCompactBLAS(vk::AccelerationStructureKHR BLAS, vk::CommandBuffer commandBuffer, vk::AccelerationStructureKHR& compactBLAS, vk::AccelerationStructureBuildSizesInfoKHR buildSizeInfo) {
//
//    vk::QueryPoolCreateInfo queryPoolInfo{
//       .queryType = vk::QueryType::eAccelerationStructureCompactedSizeKHR,
//       .queryCount = 1,
//    };
//    vk::QueryPool queryPool = Util::device.createQueryPool(queryPoolInfo);
//
//    commandBuffer.writeAccelerationStructuresPropertiesKHR(BLAS, vk::QueryType::eAccelerationStructureCompactedSizeKHR, queryPool, 0);
//    auto compactSize = Util::device.getQueryPoolResult<vk::DeviceSize>(queryPool, static_cast<uint32_t>(0), static_cast<uint32_t>(1), sizeof(vk::DeviceSize));
//
//    vk::AccelerationStructureBuildSizesInfoKHR compactSizeInfo = buildSizeInfo;
//    compactSizeInfo.setAccelerationStructureSize(compactSize.value);
//
//    auto compactBuffer = AllocatedBuffer(compactSizeInfo.accelerationStructureSize, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress, {});
//
//    vk::AccelerationStructureCreateInfoKHR compactInfo{
//        .buffer = compactBuffer.buffer,
//        .size = compactSizeInfo.accelerationStructureSize,
//        .type = vk::AccelerationStructureTypeKHR::eBottomLevel,
//    };
//
//    compactBLAS = Util::device.createAccelerationStructureKHR(compactInfo);
//
//    vk::CopyAccelerationStructureInfoKHR copyInfo{
//        .src = BLAS,
//        .dst = compactBLAS,
//        .mode = vk::CopyAccelerationStructureModeKHR::eCompact
//    };
//
//    commandBuffer.copyAccelerationStructureKHR(copyInfo);
//}
//
//void submitCommandBuffer(vk::CommandBuffer commandBuffer) {
//    commandBuffer.end();
//    vk::SubmitInfo submitInfo{
//        .commandBufferCount = 1,
//        .pCommandBuffers = &commandBuffer
//    };
//    Util::graphicsQueue.submit(submitInfo);
//    Util::graphicsQueue.waitIdle();
//}
//
//vk::DescriptorPool createDescriptorPool() {
//    std::vector<vk::DescriptorPoolSize> poolSizes({
//        {
//            .type = vk::DescriptorType::eAccelerationStructureKHR,
//            .descriptorCount = 10
//        },
//        {
//            .type = vk::DescriptorType::eStorageImage,
//            .descriptorCount = 10
//        },
//        {
//            .type = vk::DescriptorType::eStorageBuffer,
//            .descriptorCount = 10
//        }
//        });
//    vk::DescriptorPoolCreateInfo poolInfo{
//        .maxSets = 10,
//        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
//        .pPoolSizes = poolSizes.data()
//    };
//    return Util::device.createDescriptorPool(poolInfo);
//}
//
//std::vector<vk::DescriptorSet> allocateDescriptors(std::vector<vk::DescriptorSetLayout> descriptorSetLayouts) {
//    vk::DescriptorSetAllocateInfo setInfo{
//        .descriptorPool = Util::descriptorPool
//    };
//    setInfo.setSetLayouts(descriptorSetLayouts);
//    return Util::device.allocateDescriptorSets(setInfo);
//}
//
//void updateDescriptorSets() {
//    
//    vk::WriteDescriptorSetAccelerationStructureKHR accelerationData{
//        .accelerationStructureCount = 1,
//        .pAccelerationStructures = &Util::accelerationStructure->topLevel,
//    };
//    vk::DescriptorBufferInfo accelerationBufferInfo{
//        .buffer = Util::accelerationStructure->topLevelBuffer->buffer,
//        .offset = 0
//    };
//    vk::WriteDescriptorSet writeAccelerationStructure{
//        .pNext = &accelerationData,
//        .dstSet = Util::descriptorSets[0],
//        .dstBinding = 0,
//        .descriptorCount = 1,
//        .descriptorType = vk::DescriptorType::eAccelerationStructureKHR,
//        .pBufferInfo = &accelerationBufferInfo
//    };
//
//    vk::DescriptorImageInfo storageImageInfo{
//        .imageView = Util::swapchain->imageViews[Util::currentImageIndex],
//        .imageLayout = vk::ImageLayout::eGeneral,
//    };
//    vk::WriteDescriptorSet writeStorageImage{
//        .dstSet = Util::descriptorSets[0],
//        .dstBinding = 1,
//        .descriptorCount = 1,
//        .descriptorType = vk::DescriptorType::eStorageImage,
//        .pImageInfo = &storageImageInfo
//    };
//
//    vk::DescriptorBufferInfo vertexBufferInfo{
//        .buffer = Util::vertexBuffer->buffer,
//        .offset = 0,
//        .range = Util::vertexBuffer->size
//    };
//    vk::WriteDescriptorSet writeVertexBuffer{
//        .dstSet = Util::descriptorSets[1],
//        .dstBinding = 0,
//        .descriptorCount = 1,
//        .descriptorType = vk::DescriptorType::eStorageBuffer,
//        .pBufferInfo = &vertexBufferInfo
//    };
//
//    vk::DescriptorBufferInfo indexBufferInfo{
//        .buffer = Util::indexBuffer->buffer,
//        .offset = 0,
//        .range = Util::indexBuffer->size
//    };
//    vk::WriteDescriptorSet writeIndexBuffer{
//        .dstSet = Util::descriptorSets[1],
//        .dstBinding = 1,
//        .descriptorCount = 1,
//        .descriptorType = vk::DescriptorType::eStorageBuffer,
//        .pBufferInfo = &indexBufferInfo
//    };
//
//    Util::device.updateDescriptorSets({ writeAccelerationStructure, writeStorageImage, writeVertexBuffer, writeIndexBuffer }, {});
//}
//
//uint32_t align_up(uint32_t size, uint32_t alignment) {
//    return (size + alignment - 1) & ~(alignment - 1);
//}
//
//void createRtShaderBindingTable()
//{
//    vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rtProps;
//    vk::PhysicalDeviceProperties2 props{ .pNext = &rtProps };
//    Util::physicalDevice.getProperties2(&props);
//
//    uint32_t missCount{ 2 };
//    uint32_t hitCount{ 1 };
//    auto     handleCount = 1 + missCount + hitCount;
//    uint32_t handleSize = rtProps.shaderGroupHandleSize;
//
//    // The SBT (buffer) need to have starting groups to be aligned and handles in the group to be aligned.
//    uint32_t handleSizeAligned = align_up(handleSize, rtProps.shaderGroupHandleAlignment);
//
//    Util::SBT.rgenRegion.stride = align_up(handleSizeAligned, rtProps.shaderGroupBaseAlignment);
//    Util::SBT.rgenRegion.size = Util::SBT.rgenRegion.stride;  // The size member of pRayGenShaderBindingTable must be equal to its stride member
//    Util::SBT.missRegion.stride = handleSizeAligned;
//    Util::SBT.missRegion.size = align_up(missCount * handleSizeAligned, rtProps.shaderGroupBaseAlignment);
//    Util::SBT.hitRegion.stride = handleSizeAligned;
//    Util::SBT.hitRegion.size = align_up(hitCount * handleSizeAligned, rtProps.shaderGroupBaseAlignment);
//
//    // Get the shader group handles
//    uint32_t             dataSize = handleCount * handleSize;
//    std::vector<uint8_t> handles(dataSize);
//    auto result = vkGetRayTracingShaderGroupHandlesKHR(Util::device, Util::rayTracingPipeline, 0, handleCount, dataSize, handles.data());
//    assert(result == VK_SUCCESS);
//
//    // Allocate a buffer for storing the SBT.
//    VkDeviceSize sbtSize = Util::SBT.rgenRegion.size + Util::SBT.missRegion.size + Util::SBT.hitRegion.size + Util::SBT.callRegion.size;
//    Util::SBT.buffer = new AllocatedBuffer(sbtSize,
//        vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eShaderDeviceAddress| vk::BufferUsageFlagBits::eShaderBindingTableKHR,
//        vk::MemoryPropertyFlagBits::eHostVisible| vk::MemoryPropertyFlagBits::eHostCoherent);
//
//    // Find the SBT addresses of each group
//    VkDeviceAddress           sbtAddress = Util::SBT.buffer->getBufferDeviceAdress();
//    Util::SBT.rgenRegion.deviceAddress = sbtAddress;
//    Util::SBT.missRegion.deviceAddress = sbtAddress + Util::SBT.rgenRegion.size;
//    Util::SBT.hitRegion.deviceAddress = sbtAddress + Util::SBT.rgenRegion.size + Util::SBT.missRegion.size;
//
//    // Helper to retrieve the handle data
//    auto getHandle = [&](int i) { return handles.data() + i * handleSize; };
//
//    // Map the SBT buffer and write in the handles.
//    auto* pSBTBuffer = reinterpret_cast<uint8_t*>(Util::device.mapMemory(Util::SBT.buffer->memory, 0, sbtSize, {}));
//    
//    uint8_t* pData{ nullptr };
//    uint32_t handleIdx{ 0 };
//    // Raygen
//    pData = pSBTBuffer;
//    memcpy(pData, getHandle(handleIdx++), handleSize);
//    // Miss
//    pData = pSBTBuffer + Util::SBT.rgenRegion.size;
//    for (uint32_t c = 0; c < missCount; c++)
//    {
//        memcpy(pData, getHandle(handleIdx++), handleSize);
//        pData += Util::SBT.missRegion.stride;
//    }
//    // Hit
//    pData = pSBTBuffer + Util::SBT.rgenRegion.size + Util::SBT.missRegion.size;
//    for (uint32_t c = 0; c < hitCount; c++)
//    {
//        memcpy(pData, getHandle(handleIdx++), handleSize);
//        pData += Util::SBT.hitRegion.stride;
//    }
//
//    Util::device.unmapMemory(Util::SBT.buffer->memory);
//}
//
//void raytrace(vk::CommandBuffer commandBuffer)
//{
//    // Initializing push constant values
//    Ray pc.data;
//    pc.data.clearColor = glm::vec4(0.);
//    pc.data.lightPosition = glm::vec4(2., 2., 2., 1.);
//    pc.data.lightIntensity = .8;
//    pc.data.lightType = 1;
//    static auto startTime = std::chrono::high_resolution_clock::now();
//
//    auto currentTime = std::chrono::high_resolution_clock::now();
//    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
//    auto cameraPosition = glm::rotate(glm::mat4(1.0f), time * glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::vec4(2.0f, 2.0f, 2.0f, 1.0f);
//    pc.data.projInv = glm::inverse(glm::perspective(glm::radians(45.0f), Util::swapchain->extent.width / (float)Util::swapchain->extent.height, 0.1f, 10.0f));
//    pc.data.viewInv = glm::inverse(glm::lookAt(glm::vec3(cameraPosition), glm::vec3(0.f, 0.0f, 0.5f), glm::vec3(0.0f, 0.0f, -1.0f)));
//
//
//    commandBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, Util::rayTracingPipeline);
//    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, Util::pipelineLayout, 0, Util::descriptorSets, {});
//    commandBuffer.pushConstants(Util::pipelineLayout,
//        vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR,
//        0, sizeof(Ray), &pc.data);
//    commandBuffer.traceRaysKHR(Util::SBT.rgenRegion, Util::SBT.missRegion, Util::SBT.hitRegion, Util::SBT.callRegion, Util::swapchain->extent.width, Util::swapchain->extent.height, 1);
//}
//
//void createSyncObjects() {
//    Util::imageAvailableSemaphore = Util::device.createSemaphore({});
//    Util::renderFinishedSemaphore = Util::device.createSemaphore({});
//    Util::inFlightFence = Util::device.createFence({ .flags = vk::FenceCreateFlagBits::eSignaled });
//}
//
//void drawFrame() {
//        Util::device.waitForFences({ Util::inFlightFence }, vk::True, UINT64_MAX);
//        Util::device.resetFences({ Util::inFlightFence });
//
//        uint32_t imageIndex;
//        Util::currentImageIndex = Util::device.acquireNextImageKHR(Util::swapchain->swapchain, UINT64_MAX, Util::imageAvailableSemaphore, nullptr).value;
//        updateDescriptorSets(); // shoud be redesigned to update only current image and not the buffers
//
//        //vkResetCommandBuffer(commandBuffer, /*VkCommandBufferResetFlagBits*/ 0);
//        //recordCommandBuffer(commandBuffer, imageIndex);
//        CommandBuffer commandBuffer;
//        raytrace(commandBuffer.cmd);
//
//        std::vector<vk::Semaphore> waitSemaphores = { Util::imageAvailableSemaphore };
//        std::vector<vk::PipelineStageFlags> waitStages = { vk::PipelineStageFlagBits::eRayTracingShaderKHR };
//        std::vector<vk::CommandBuffer> commandBuffers = { commandBuffer.cmd };
//        std::vector<vk::Semaphore> signalSemaphores = { Util::renderFinishedSemaphore };
//
//        std::vector<vk::SubmitInfo> submitInfo(1);
//        submitInfo[0].setCommandBuffers(commandBuffers);
//        submitInfo[0].setWaitDstStageMask(waitStages);
//        submitInfo[0].setWaitSemaphores(waitSemaphores);
//        submitInfo[0].setSignalSemaphores(signalSemaphores);
//
//        commandBuffer.cmd.end();
//        Util::graphicsQueue.submit(submitInfo, Util::inFlightFence);
//
//        std::vector<vk::SwapchainKHR> swapchains = { Util::swapchain->swapchain };
//        std::vector<uint32_t> imageIndices = { Util::currentImageIndex };
//
//        vk::PresentInfoKHR presentInfo{};
//        presentInfo.setWaitSemaphores(signalSemaphores);
//        presentInfo.setSwapchains(swapchains);
//        presentInfo.setImageIndices(imageIndices);
//
//        Util::graphicsQueue.presentKHR(presentInfo);
//    }
//
//// --------------------------------------------------------------------------------------
//// --------------------------------------------------------------------------------------
//// AllocatedBuffer Implementation -------------------------------------------------------
//// --------------------------------------------------------------------------------------
//// --------------------------------------------------------------------------------------
//
//AllocatedBuffer::AllocatedBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags props) {
//    this->size = size;
//    this->buffer = createBuffer(size, usage);
//    this->memory = createMemory(size, props);
//    Util::device.bindBufferMemory(this->buffer, this->memory, 0);
//}
//
//template<typename T>
//AllocatedBuffer::AllocatedBuffer(std::vector<T> data, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags props) {
//    uint32_t size = data.size() * sizeof(T);
//    this->size = size;
//    this->buffer = createBuffer(size, usage);
//    this->memory = createMemory(size, props);
//    Util::device.bindBufferMemory(this->buffer, this->memory, 0);
//    fill(data);
//}
//
//template<typename T>
//void AllocatedBuffer::fill(std::vector<T> data) {
//    auto stagingBuffer = createBuffer(size, vk::BufferUsageFlagBits::eTransferSrc);
//    auto stagingMemory = createMemory(size, vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostCoherent);
//    Util::device.bindBufferMemory(stagingBuffer, stagingMemory, 0);
//
//    void* pointer = Util::device.mapMemory(stagingMemory, 0, size, {});
//    memcpy(pointer, data.data(), (size_t)size);
//    Util::device.unmapMemory(stagingMemory);
//
//    copyBuffer(stagingBuffer, this->buffer, size);
//};
//
//vk::DeviceAddress AllocatedBuffer::getBufferDeviceAdress() {
//    vk::BufferDeviceAddressInfo addressInfo{
//        .buffer = this->buffer
//    };
//    return Util::device.getBufferAddress(addressInfo);
//}
//
//AllocatedBuffer::~AllocatedBuffer() {
//    std::cout << "Destroy" << std::endl;
//    if (this->buffer != nullptr) {
//        Util::device.destroyBuffer(this->buffer);
//        this->buffer = nullptr;
//    }
//    if (this->memory != nullptr) {
//        Util::device.freeMemory(this->memory);
//        this->memory = nullptr;
//    }
//}
//
//vk::Buffer AllocatedBuffer::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage) {
//    vk::BufferCreateInfo bufferInfo{
//    .size = size,
//    .usage = usage,
//    .sharingMode = vk::SharingMode::eExclusive
//    };
//    return Util::device.createBuffer(bufferInfo);
//}
//
//vk::DeviceMemory AllocatedBuffer::createMemory(vk::DeviceSize size, vk::MemoryPropertyFlags props) {
//    vk::MemoryRequirements memRequirements = Util::device.getBufferMemoryRequirements(buffer);
//    vk::MemoryAllocateFlagsInfo memFlagsInfo{
//        .flags = vk::MemoryAllocateFlagBits::eDeviceAddress
//    };
//    vk::MemoryAllocateInfo allocInfo{
//        .pNext = &memFlagsInfo,
//        .allocationSize = memRequirements.size,
//        .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, props),
//    };
//    return Util::device.allocateMemory(allocInfo);
//}
//
//void AllocatedBuffer::copyBuffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size) {
//    vk::CommandBufferAllocateInfo allocInfo{
//        .commandPool = Util::commandPool,
//        .level = vk::CommandBufferLevel::ePrimary,
//        .commandBufferCount = 1
//    };
//    vk::CommandBuffer commandBuffer = Util::device.allocateCommandBuffers(allocInfo)[0];
//
//    vk::CommandBufferBeginInfo beginInfo{
//        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
//    };
//    commandBuffer.begin(beginInfo);
//    vk::BufferCopy copyInfo{
//        .srcOffset = 0,
//        .dstOffset = 0,
//        .size = size
//    };
//    commandBuffer.copyBuffer(src, dst, { copyInfo });
//    commandBuffer.end();
//
//    vk::SubmitInfo submitInfo{
//        .commandBufferCount = 1,
//        .pCommandBuffers = &commandBuffer
//    };
//    Util::graphicsQueue.submit({ submitInfo });
//    Util::graphicsQueue.waitIdle();
//
//    Util::device.freeCommandBuffers(Util::commandPool, { commandBuffer });
//}
//
//uint32_t AllocatedBuffer::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
//    vk::PhysicalDeviceMemoryProperties memProperties = Util::physicalDevice.getMemoryProperties();
//    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
//        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
//            return i;
//    throw std::runtime_error("Could not find suitable memory type!");
//}
//
//// --------------------------------------------------------------------------------------
//// --------------------------------------------------------------------------------------
//// Swapchain Implementation -------------------------------------------------------------
//// --------------------------------------------------------------------------------------
//// --------------------------------------------------------------------------------------
//Swapchain::Swapchain() {
//
//    std::vector<vk::SurfaceFormatKHR> surfaceFormats = Util::physicalDevice.getSurfaceFormatsKHR(Util::surface);
//    std::vector<vk::PresentModeKHR> surfacePresentModes = Util::physicalDevice.getSurfacePresentModesKHR(Util::surface);
//    vk::SurfaceCapabilitiesKHR surfaceCapabilities = Util::physicalDevice.getSurfaceCapabilitiesKHR(Util::surface);
//
//    vk::SurfaceFormatKHR format_;
//    for (vk::SurfaceFormatKHR availableFormat : surfaceFormats)
//        if (availableFormat.format == vk::Format::eR8G8B8A8Unorm && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
//            format_ = availableFormat;
//            break;
//        }
//    extent = surfaceCapabilities.currentExtent;
//    format = format_.format;
//
//    vk::SwapchainCreateInfoKHR swapchainInfo{
//        .surface = Util::surface,
//        .minImageCount = surfaceCapabilities.minImageCount,
//        .imageFormat = format,
//        .imageColorSpace = format_.colorSpace,
//        .imageExtent = extent,
//        .imageArrayLayers = 1,
//        .imageUsage = vk::ImageUsageFlagBits::eStorage,
//        .imageSharingMode = vk::SharingMode::eExclusive,
//        .queueFamilyIndexCount = 1,
//        .pQueueFamilyIndices = &Util::queueFamilyIndex.graphics,
//        .presentMode = vk::PresentModeKHR::eFifo,
//        .clipped = vk::True
//    };
//    swapchain = Util::device.createSwapchainKHR(swapchainInfo);
//
//    createImageViews();
//}
//
//Swapchain::~Swapchain() {
//    for (vk::ImageView& imageView : imageViews)
//        Util::device.destroyImageView(imageView);
//    if (swapchain != nullptr)
//        Util::device.destroySwapchainKHR(swapchain);
//}
//
//void Swapchain::createImageViews() {
//    std::vector<vk::Image> scImages = Util::device.getSwapchainImagesKHR(swapchain);
//    int nImages = scImages.size();
//
//    imageViews.resize(nImages);
//    vk::ImageViewCreateInfo imageViewInfo{
//            .viewType = vk::ImageViewType::e2D,
//            .format = format,
//            .components = {
//                .r = vk::ComponentSwizzle::eIdentity,
//                .g = vk::ComponentSwizzle::eIdentity,
//                .b = vk::ComponentSwizzle::eIdentity,
//                .a = vk::ComponentSwizzle::eIdentity
//            },
//            .subresourceRange = {
//                .aspectMask = vk::ImageAspectFlagBits::eColor,
//                .baseMipLevel = 0,
//                .levelCount = 1,
//                .baseArrayLayer = 0,
//                .layerCount = 1
//            }
//    };
//    for (int i = 0; i < nImages; i++) {
//        imageViewInfo.setImage(scImages[i]);
//        imageViews[i] = Util::device.createImageView(imageViewInfo);
//    }
//}
//
//// --------------------------------------------------------------------------------------
//// --------------------------------------------------------------------------------------
//// AccelerationStructure Implementation ---------------------------------------------------------------------------
//// --------------------------------------------------------------------------------------
//// --------------------------------------------------------------------------------------
//void AccelerationStructure::createTopLevel() {
//    std::vector<VkAccelerationStructureInstanceKHR> instances;
//    instances.reserve(this->bottomLevel.size());
//    for (auto BLAS : this->bottomLevel) {
//        vk::AccelerationStructureDeviceAddressInfoKHR addressInfo{
//            .accelerationStructure = BLAS
//        };
//
//        auto BLASAddress = Util::device.getAccelerationStructureAddressKHR(addressInfo);
//
//        vk::AccelerationStructureInstanceKHR rayInst{
//            .transform = {{{
//                    10., 0., 0., 0.,
//                    0., 0., 10., 0.,
//                    0., 10., 0., -0.5
//            }}},
//            .instanceCustomIndex = 0,
//            .mask = 0xFF,
//            .instanceShaderBindingTableRecordOffset = 0,
//            .accelerationStructureReference = BLASAddress
//        };
//        rayInst.setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable);
//        instances.emplace_back(rayInst);
//    }
//
//    uint32_t nInstances = static_cast<uint32_t>(this->bottomLevel.size());
//    CommandBuffer commandBuffer;
//
//    AllocatedBuffer instancesBuffer(instances,
//        vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
//        vk::MemoryPropertyFlagBits::eDeviceLocal);
//
//    vk::MemoryBarrier barrier{
//        .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
//        .dstAccessMask = vk::AccessFlagBits::eAccelerationStructureWriteKHR
//    };
//
//    commandBuffer.cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR, {}, { barrier }, nullptr, nullptr);
//    
//    vk::AccelerationStructureGeometryInstancesDataKHR instancesData{
//        .data = {.deviceAddress = instancesBuffer.getBufferDeviceAdress()}
//    };
//
//    vk::AccelerationStructureGeometryKHR TLASGeometry{
//        .geometryType = vk::GeometryTypeKHR::eInstances,
//        .geometry = {
//            .instances = instancesData
//}
//    };
//
//    vk::AccelerationStructureBuildGeometryInfoKHR buildInfo{
//        .type = vk::AccelerationStructureTypeKHR::eTopLevel,
//        .flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
//        .mode = vk::BuildAccelerationStructureModeKHR::eBuild,
//        .geometryCount = 1,
//        .pGeometries = &TLASGeometry
//    };
//
//    auto sizeInfo = Util::device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, buildInfo, nInstances);
//    this->topLevelBuffer = new AllocatedBuffer(sizeInfo.accelerationStructureSize,
//        vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR,
//        vk::MemoryPropertyFlagBits::eDeviceLocal);
//    AllocatedBuffer scratchBuffer(sizeInfo.buildScratchSize,
//        vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer,
//        vk::MemoryPropertyFlagBits::eDeviceLocal);
//    auto scratchAdress = scratchBuffer.getBufferDeviceAdress();
//    buildInfo.setScratchData({ .deviceAddress = scratchAdress });
//    vk::AccelerationStructureCreateInfoKHR structureInfo{
//        .buffer = this->topLevelBuffer->buffer,
//        .offset = 0,
//        .size = sizeInfo.accelerationStructureSize,
//        .type = vk::AccelerationStructureTypeKHR::eTopLevel,
//    };
//    this->topLevel = Util::device.createAccelerationStructureKHR(structureInfo);
//    buildInfo.setDstAccelerationStructure(this->topLevel);
//
//    vk::AccelerationStructureBuildRangeInfoKHR buildOffsetInfo{
//        .primitiveCount = nInstances,
//        .primitiveOffset = 0,
//        .firstVertex = 0
//    };
//
//
//    commandBuffer.cmd.buildAccelerationStructuresKHR({ buildInfo }, { &buildOffsetInfo });
//    commandBuffer.submit();
//    Util::graphicsQueue.waitIdle();
//
//
//}
//
//void AccelerationStructure::createBottomLevel(vk::DeviceAddress verticesAddress, vk::DeviceAddress indicesAddress, int nVertices, int nIndices) {
//    vk::AccelerationStructureBuildRangeInfoKHR rangeInfo{
//        .primitiveCount = static_cast<uint32_t>(nIndices / 3), //faces
//        .primitiveOffset = 0,
//        .firstVertex = 0,
//        .transformOffset = 0
//    };
//
//    auto descriptions = Vertex::getAttributeDescriptions();
//
//    vk::AccelerationStructureGeometryTrianglesDataKHR trianglesData{
//        .vertexFormat = descriptions[0].format,
//        .vertexData = {.deviceAddress = verticesAddress}, // vertex buffer address
//        .vertexStride = sizeof(Vertex),
//        .maxVertex = static_cast<uint32_t>(nVertices - 1),
//        .indexType = vk::IndexType::eUint32,
//        .indexData = {.deviceAddress = indicesAddress} // index buffer address
//    };
//
//    vk::AccelerationStructureGeometryDataKHR geometryData{
//        .triangles = trianglesData,
//    };
//
//    std::vector<vk::AccelerationStructureGeometryKHR> geometryInfos({
//        {
//            .geometryType = vk::GeometryTypeKHR::eTriangles,
//            .geometry = geometryData,
//            .flags = vk::GeometryFlagBitsKHR::eOpaque,
//        }
//        });
//
//    vk::AccelerationStructureBuildGeometryInfoKHR buildGeometryInfo{
//        .type = vk::AccelerationStructureTypeKHR::eBottomLevel,
//        .flags = vk::BuildAccelerationStructureFlagBitsKHR::eAllowCompaction,
//        .mode = vk::BuildAccelerationStructureModeKHR::eBuild,
//        .geometryCount = static_cast<uint32_t>(geometryInfos.size()),
//        .pGeometries = geometryInfos.data(),
//    };
//
//
//    vk::AccelerationStructureBuildSizesInfoKHR buildSizeInfo = Util::device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, buildGeometryInfo, rangeInfo.primitiveCount);
//
//
//    this->bottomLevelBuffer = new AllocatedBuffer(buildSizeInfo.accelerationStructureSize, vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR, vk::MemoryPropertyFlagBits::eDeviceLocal);
//
//    vk::AccelerationStructureCreateInfoKHR BLASInfo{
//        .buffer = this->bottomLevelBuffer->buffer,
//        .size = buildSizeInfo.accelerationStructureSize,
//        .type = vk::AccelerationStructureTypeKHR::eBottomLevel
//    };
//
//    auto sizeInfo = Util::device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, buildGeometryInfo, { rangeInfo.primitiveCount });
//    AllocatedBuffer scratchBuffer(sizeInfo.buildScratchSize, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress, {});
//    auto scratchAdress = scratchBuffer.getBufferDeviceAdress();
//    buildGeometryInfo.setScratchData({ .deviceAddress = scratchAdress });
//
//    this->bottomLevel.push_back(Util::device.createAccelerationStructureKHR(BLASInfo));
//    buildGeometryInfo.setDstAccelerationStructure(this->bottomLevel.back());
//    //vk::AccelerationStructureKHR compactBLAS(VkAccelerationStructureKHR{});
//
//
//    CommandBuffer cmdBuffer;
//    cmdBuffer.cmd.buildAccelerationStructuresKHR({ buildGeometryInfo }, { &rangeInfo });
//    cmdBuffer.submit();
//
//    //beginCommandBuffer(cmdBuffer);
//    //cmdCompactBLAS(BLAS, cmdBuffer, compactBLAS, buildSizeInfo);
//    //endCommandBuffer(cmdBuffer);
//
//    //Util::graphicsQueue.submit({ submitInfo });
//    //Util::graphicsQueue.waitIdle();
//
//
//    //Util::device.destroyAccelerationStructureKHR(BLAS);
//
//    //return compactBLAS;
//}
//
//void cleanup(){
//}
//
//// --------------------------------------------------------------------------------------
//// --------------------------------------------------------------------------------------
//// Input Data ---------------------------------------------------------------------------
//// --------------------------------------------------------------------------------------
//// --------------------------------------------------------------------------------------
//std::vector<Vertex> groundVertices = {
//    {{ -.5,  .5, -.5, 1. }},
//    {{ .5,  .5, -.5, 1. }},
//    {{ -.5, -.5, -.5, 1. }},
//    {{ .5, -.5, -.5, 1. }},
//    {{ -.5,  .5,  .5, 1. }},
//    {{ .5,  .5,  .5, 1. }},
//    {{ -.5, -.5,  .5, 1. }},
//    {{ .5, -.5,  .5, 1. }},
//
//    {{ -10., 10.,  -.5, 1. }},
//    {{ 10., 10.,  -.5, 1. }},
//    {{ -10., -10.,  -.5, 1. }},
//    {{ 10., -10.,  -.5, 1. }},
//};
//
//std::vector<uint32_t> groundIndices = {
//            //0, 1, 2, // Side 0
//            //2, 1, 3,
//            //4, 0, 6, // Side 1
//            //6, 0, 2,
//            //7, 5, 6, // Side 2
//            //6, 5, 4,
//            //3, 1, 7, // Side 3 
//            //7, 1, 5,
//            //4, 5, 0, // Side 4 
//            //0, 5, 1,
//            //3, 7, 2, // Side 5 
//            //2, 7, 6,
//            9, 8, 10, //floor
//            9, 10, 11
//};
//
//int main() {
//    try {
//        auto&& [vertices, indices] = readPLY("./models/dragon_vrip.ply");
//        glfwInit();
//
//        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
//        GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);
//
//        uint32_t glfwExtensionCount = 0;
//        const char** glfwExtensions;
//
//        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
//        std::vector<const char*> instanceExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
//        instanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
//        instanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
//
//        std::vector<const char*> deviceExtensions = {
//        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
//        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
//        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
//        VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
//        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
//        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
//        VK_KHR_MAINTENANCE3_EXTENSION_NAME,
//        VK_KHR_SPIRV_1_4_EXTENSION_NAME,
//        VK_KHR_SWAPCHAIN_EXTENSION_NAME
//        };
//        const std::vector<const char*> validationLayers = {
//        "VK_LAYER_KHRONOS_validation"
//        };
//
//#ifdef NDEBUG
//        const bool enableValidationLayers = false;
//#else
//        const bool enableValidationLayers = true;
//#endif
//        Util::instance = createInstance(validationLayers, instanceExtensions);
//
//        std::vector<vk::PhysicalDevice> physicalDevices = Util::instance.enumeratePhysicalDevices();
//        Util::physicalDevice = physicalDevices[0];
//        Util::device = createDevice(deviceExtensions);
//
//        Util::queueFamilyIndex.graphics = getQueueFamilyIndex(vk::QueueFlagBits::eGraphics);
//        Util::queueFamilyIndex.transfer = getQueueFamilyIndex(vk::QueueFlagBits::eTransfer);
//        Util::queueFamilyIndex.compute = getQueueFamilyIndex(vk::QueueFlagBits::eCompute);
//
//        Util::graphicsQueue = Util::device.getQueue(Util::queueFamilyIndex.graphics, 0);
//
//        createSyncObjects();
//
//        Util::surface = createSurface(window);
//        Util::swapchain = new Swapchain();
//
//        Util::commandPool = createCommandPool(Util::queueFamilyIndex.transfer);
//
//        Util::vertexBuffer = std::make_unique<AllocatedBuffer>(vertices,
//            vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
//            vk::MemoryPropertyFlagBits::eDeviceLocal);
//        Util::indexBuffer = std::make_unique<AllocatedBuffer>(indices,
//            vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
//            vk::MemoryPropertyFlagBits::eDeviceLocal);
//
//        Util::accelerationStructure = new AccelerationStructure(*Util::vertexBuffer, *Util::indexBuffer, vertices.size(), indices.size());
//        Util::descriptorPool = createDescriptorPool();
//        Util::descriptorSetLayouts = {
//            createRTDescriptorSetLayout(),
//            createSceneDescriptorSetLayout()
//        };
//        Util::descriptorSets = allocateDescriptors(Util::descriptorSetLayouts);
//
//        Util::rayTracingPipeline = createRayTracingPipeline();
//        createRtShaderBindingTable();
//
//        while (!glfwWindowShouldClose(window)) {
//            drawFrame();
//            glfwPollEvents();
//        }
//
//        delete Util::swapchain;
//        Util::instance.destroySurfaceKHR(Util::surface);
//        Util::device.destroy();
//        Util::instance.destroy();
//
//        glfwDestroyWindow(window);
//
//        glfwTerminate();
//    }
//    catch (const std::runtime_error& e) {
//        std::cerr << e.what() << std::endl;
//    }
//    catch (const std::exception& e) {
//        std::cerr << e.what() << std::endl;
//    }
//
//    return 0;
//}

#include "setup.hpp"
#include "presentation.hpp"
#include "command_pool.hpp"
#include "buffer.hpp"
#include "acceleration_structure.hpp"
#include "file_reader.hpp"
#include "descriptor_sets.hpp"
#include "pipeline.hpp"
#include<glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

Model3D groundModel {
	.vertices = {
		{{ -10., 10.,  -.5, 1. }},
		{{ 10., 10.,  -.5, 1. }},
		{{ -10., -10.,  -.5, 1. }},
		{{ 10., -10.,  -.5, 1. }}},
	.indices = {
		1, 0, 2, //floor
		1, 2, 3}
};

#include<iostream>
int main() {
	auto setup = SetupBuilder()
		.addExtensions(PresentationBuilder::getRequirements())
		.addExtensions(AccelerationStructureBuilder::getRequirements())
		.addExtensions(DescriptorSetsBuilder::getRequirements())
		.addExtensions(PipelineBuilder::getRequirements())
		.build();
	auto presentation = PresentationBuilder(setup).build();
	auto commandPool = CommandPoolBuilder(setup).build();

	auto dragonModel = FileReader().readPLY("./models/dragon_vrip.ply");
	auto bunnyModel = FileReader().readPLY("./models/bunny.ply");
	Instance dragon(glm::rotate(glm::scale(glm::translate(glm::identity<glm::mat4>(), glm::vec3(0., 1., 0.)), glm::vec3(10.)), glm::pi<glm::float32>() / 2, glm::vec3(1., 0., 0.)), 0);
	Instance bunny(glm::rotate(glm::scale(glm::translate(glm::identity<glm::mat4>(), glm::vec3(1., 0., 0.)), glm::vec3(10.)), glm::pi<glm::float32>() / 2, glm::vec3(1., 0., 0.)), 0);
	Instance ground(glm::identity<glm::mat4>(), 0);

	auto scene = SceneBuilder(setup, commandPool->createCommandBuffer())
		.addModel(groundModel)
		.addModel(dragonModel)
		.addModel(bunnyModel)
		.build();

	// TODO: change argument to command pool, instead of commandBuffer
	auto BVH = AccelerationStructureBuilder(setup, commandPool->createCommandBuffer())
		.addModel3D(groundModel)
		.addModel3D(dragonModel)
		.addModel3D(bunnyModel)
		.addInstance(ground, 0)
		.addInstance(dragon, 1)
		.addInstance(bunny, 2)
		.build();

	Descriptor bvhDescriptor{
		.set = 0,
		.binding = 0,
		.type = vk::DescriptorType::eAccelerationStructureKHR,
		.stagesUsed = vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR
	};
	Descriptor imageDescriptor{
		.set = 0,
		.binding = 1,
		.type = vk::DescriptorType::eStorageImage,
		.stagesUsed = vk::ShaderStageFlagBits::eRaygenKHR
	};
	Descriptor vertexBufferDescriptor{
		.set = 1,
		.binding = 0,
		.type = vk::DescriptorType::eStorageBuffer,
		.stagesUsed = vk::ShaderStageFlagBits::eClosestHitKHR
	};
	Descriptor indexBufferDescriptor{
		.set = 1,
		.binding = 1,
		.type = vk::DescriptorType::eStorageBuffer,
		.stagesUsed = vk::ShaderStageFlagBits::eClosestHitKHR
	};
	Descriptor objectDescDescriptor{
		.set = 1,
		.binding = 2,
		.type = vk::DescriptorType::eStorageBuffer,
		.stagesUsed = vk::ShaderStageFlagBits::eClosestHitKHR,
	};

	auto sets = DescriptorSetsBuilder(setup)
		.addDescriptor(bvhDescriptor)
		.addDescriptor(imageDescriptor)
		.addDescriptor(vertexBufferDescriptor)
		.addDescriptor(indexBufferDescriptor)
		.addDescriptor(objectDescDescriptor)
		.build();

	auto rayTracingSet = sets[0];
	auto sceneSet = sets[1];
	rayTracingSet->updateDescriptor(bvhDescriptor, BVH);
	sceneSet->updateDescriptor(vertexBufferDescriptor, scene->vertexBuffer);
	sceneSet->updateDescriptor(indexBufferDescriptor, scene->indexBuffer);
	sceneSet->updateDescriptor(objectDescDescriptor, scene->objectDescriptionBuffer);

	auto semaphore = setup->device.createSemaphore({});
	int imageIndex = setup->device.acquireNextImageKHR(presentation->swapchain.handle, UINT64_MAX, {semaphore}, {}).value;
	rayTracingSet->updateDescriptor(imageDescriptor, presentation->swapchain.imageViews[imageIndex]);

	setup->device.destroySemaphore(semaphore);

	PushConstant pc;
	pc.stagesUsed = vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitNV;

	auto pipeline = PipelineBuilder(setup)
		.addShader("./shaders/raygen.spv", vk::ShaderStageFlagBits::eRaygenKHR)
		.addShader("./shaders/miss.spv", vk::ShaderStageFlagBits::eMissKHR)
		//.addShader("./shaders/shadow.spv", vk::ShaderStageFlagBits::eMissKHR)
		.addShader("./shaders/closesthit.spv", vk::ShaderStageFlagBits::eClosestHitKHR)
		.addDescriptorSet(rayTracingSet)
		.addDescriptorSet(sceneSet)
		.addPushconstant(pc)
		.build();

	pc.data.clearColor = glm::vec4(0.);
	pc.data.lightPosition = glm::vec4(2., 2., 2., 1.);
	pc.data.lightIntensity = .8;
	pc.data.lightType = 1;
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
	auto cameraPosition = glm::rotate(glm::mat4(1.0f), time * glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::vec4(2.0f, 2.0f, 2.0f, 1.0f);
	pc.data.projInv = glm::inverse(glm::perspective(glm::radians(45.0f), presentation->swapchain.extent.width / (float)presentation->swapchain.extent.height, 0.1f, 10.0f));
	pc.data.viewInv = glm::inverse(glm::lookAt(glm::vec3(cameraPosition), glm::vec3(0.f, 0.0f, 0.5f), glm::vec3(0.0f, 0.0f, -1.0f)));

	pc.data = pc.data;
	auto commandBuffer = commandPool->createCommandBuffer();
	commandBuffer->begin();
	pipeline->run(commandBuffer, presentation->swapchain, {pc});
	commandBuffer->submit();

	setup->graphicsQueue.handle.waitIdle();

	std::vector<vk::SwapchainKHR> swapchains = { presentation->swapchain.handle};
	std::vector<uint32_t> imageIndices = { static_cast<uint32_t>(imageIndex) };
		
	vk::PresentInfoKHR presentInfo{};
	presentInfo.setSwapchains(swapchains);
	presentInfo.setImageIndices(imageIndices);
		
	setup->graphicsQueue.handle.presentKHR(presentInfo);

	std::cin >> pc.data.lightType;
}
