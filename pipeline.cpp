#include "pipeline.hpp"

Requirements PipelineBuilder::getRequirements()
{
    Requirements extensions;
    extensions.deviceExtensions = {
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME
    };
    return extensions;
}

std::shared_ptr<Pipeline> PipelineBuilder::build()
{
    std::vector<vk::DescriptorSetLayout> descriptorSetLayouts(this->descriptorSets.size());
    std::transform(this->descriptorSets.begin(), this->descriptorSets.end(), descriptorSetLayouts.begin(), [](std::shared_ptr<DescriptorSet> x) { return x->layout; });

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.setPushConstantRanges(this->pushConstantRanges);
    pipelineLayoutInfo.setSetLayouts(descriptorSetLayouts);
    auto pipelineLayout = this->setup->device.createPipelineLayout(pipelineLayoutInfo);

    vk::RayTracingPipelineCreateInfoKHR pipelineInfo{
        .maxPipelineRayRecursionDepth = this->maxRecursionDepth,
        .layout = pipelineLayout,
    };
    pipelineInfo.setStages(this->stages);
    pipelineInfo.setGroups(this->shaderGroups);
    auto handle = this->setup->device.createRayTracingPipelineKHR(nullptr, nullptr, pipelineInfo).value;

    auto shaderBindingTable = this->createShaderBindingTable(handle);

    auto pipeline = std::make_shared<Pipeline>(this->setup);
    pipeline->handle = handle;
    pipeline->layout = pipelineLayout;
    pipeline->SBT = shaderBindingTable;
    pipeline->descriptorSets = this->descriptorSets;

    for (auto& shaderModule : this->shaderModules) {
        this->setup->device.destroyShaderModule(shaderModule);
    }

    return pipeline;
}

PipelineBuilder::PipelineBuilder(std::shared_ptr<Setup> setup) : IHasSetup(setup) {}

vk::ShaderModule PipelineBuilder::createShaderModule(std::vector<char>& code)
{
    vk::ShaderModuleCreateInfo shaderInfo{
        .codeSize = static_cast<uint32_t>(code.size()),
        .pCode = reinterpret_cast<uint32_t*>(code.data())
    };
    return this->setup->device.createShaderModule(shaderInfo);
}

vk::PipelineShaderStageCreateInfo PipelineBuilder::createStage(vk::ShaderModule shaderModule, vk::ShaderStageFlagBits stage)
{
    vk::PipelineShaderStageCreateInfo stageInfo{
        .stage = stage,
        .module = shaderModule,
        .pName = "main",
    };
    return stageInfo;
}

PipelineBuilder PipelineBuilder::addShader(const std::string& shaderFileName, vk::ShaderStageFlagBits stage)
{
    auto code = FileReader().readSPV(shaderFileName);
    auto shaderModule = this->createShaderModule(code);
    auto stageInfo = this->createStage(shaderModule, stage);

    vk::RayTracingShaderGroupTypeKHR shaderGroupType;
    switch (stage)
    {
    case vk::ShaderStageFlagBits::eClosestHitKHR:
        shaderGroupType = vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup;
        this->hitCount++;
        break;

    case vk::ShaderStageFlagBits::eMissKHR:
        shaderGroupType = vk::RayTracingShaderGroupTypeKHR::eGeneral;
        this->missCount++;
        break;
        
    case vk::ShaderStageFlagBits::eCallableKHR:
        shaderGroupType = vk::RayTracingShaderGroupTypeKHR::eGeneral;
        callCount++;
        break;
        
    case vk::ShaderStageFlagBits::eAnyHitKHR:
        shaderGroupType = vk::RayTracingShaderGroupTypeKHR::eGeneral;
        anyCount++;
        break;

    default:
        break;
    }
    vk::RayTracingShaderGroupCreateInfoKHR shaderGroup{
            .type = shaderGroupType,
            .generalShader = static_cast<uint32_t>(this->shaderModules.size()),
            .closestHitShader = vk::ShaderUnusedKHR,
            .anyHitShader = vk::ShaderUnusedKHR,
            .intersectionShader = vk::ShaderUnusedKHR
    };

    this->shaderModules.push_back(shaderModule);
    this->stages.push_back(stageInfo);
    this->shaderGroups.push_back(shaderGroup);

    return *this;
}

PipelineBuilder PipelineBuilder::addDescriptorSet(std::shared_ptr<DescriptorSet> descriptorSet)
{
    this->descriptorSets.push_back(descriptorSet);
    return *this;
}

PipelineBuilder PipelineBuilder::addPushconstant(PushConstant pushConstant)
{
    vk::PushConstantRange range{
        .stageFlags = pushConstant.stagesUsed,
        .offset = 0,
        .size = pushConstant.size(),
    };
    this->pushConstantRanges.push_back(range);

    return *this;
}

PipelineBuilder PipelineBuilder::setMaxRecursionDepth(uint32_t depth)
{
    this->maxRecursionDepth = depth;
    return *this;
}

uint32_t alignUp(uint32_t size, uint32_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

ShaderBindingTable PipelineBuilder::createShaderBindingTable(vk::Pipeline pipeline) {
    vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rtProps;
    vk::PhysicalDeviceProperties2 props{ .pNext = &rtProps };
    this->setup->physicalDevice.getProperties2(&props);

    auto     handleCount = 1 + this->missCount + this->hitCount + this->callCount + this->anyCount;
    uint32_t handleSize = rtProps.shaderGroupHandleSize;

    vk::StridedDeviceAddressRegionKHR rayGenRegion;
    vk::StridedDeviceAddressRegionKHR missRegion;
    vk::StridedDeviceAddressRegionKHR hitRegion;
    vk::StridedDeviceAddressRegionKHR callRegion;
    vk::StridedDeviceAddressRegionKHR anyRegion;

    // The SBT (buffer) need to have starting groups to be aligned and handles in the group to be aligned.
    uint32_t handleSizeAligned = alignUp(handleSize, rtProps.shaderGroupHandleAlignment);
    rayGenRegion.stride = alignUp(handleSizeAligned, rtProps.shaderGroupBaseAlignment);
    rayGenRegion.size = rayGenRegion.stride;  // The size member of pRayGenShaderBindingTable must be equal to its stride member
    missRegion.stride = handleSizeAligned;
    missRegion.size = alignUp(missCount * handleSizeAligned, rtProps.shaderGroupBaseAlignment);
    hitRegion.stride = handleSizeAligned;
    hitRegion.size = alignUp(hitCount * handleSizeAligned, rtProps.shaderGroupBaseAlignment);
    callRegion.stride = handleSizeAligned;
    callRegion.size = alignUp(callCount * handleSizeAligned, rtProps.shaderGroupBaseAlignment);
    anyRegion.stride = handleSizeAligned;
    anyRegion.size = alignUp(anyCount * handleSizeAligned, rtProps.shaderGroupBaseAlignment);

    // Get the shader group handles
    uint32_t             dataSize = handleCount * handleSize;
    std::vector<uint8_t> handles(dataSize);
    auto result = vkGetRayTracingShaderGroupHandlesKHR(this->setup->device, pipeline, 0, handleCount, dataSize, handles.data());
    assert(result == VK_SUCCESS);

    // Allocate a buffer for storing the shaderBindingTable->
    VkDeviceSize sbtSize = rayGenRegion.size + missRegion.size + hitRegion.size + callRegion.size + anyRegion.size;
    auto buffer = BufferBuilder(this->setup)
        .setSize(sbtSize)
        .setUsage(vk::BufferUsageFlagBits::eTransferSrc)
        .setUsage(vk::BufferUsageFlagBits::eShaderDeviceAddress)
        .setUsage(vk::BufferUsageFlagBits::eShaderBindingTableKHR)
        .setMemoryProperties(vk::MemoryPropertyFlagBits::eHostVisible)
        .setMemoryProperties(vk::MemoryPropertyFlagBits::eHostCoherent)
        .build();

    // Find the SBT addresses of each group
    VkDeviceAddress           sbtAddress = buffer->getDeviceAddress();
    rayGenRegion.deviceAddress = sbtAddress;
    missRegion.deviceAddress = rayGenRegion.deviceAddress + rayGenRegion.size;
    hitRegion.deviceAddress = missRegion.deviceAddress + missRegion.size;
    callRegion.deviceAddress = hitRegion.deviceAddress + hitRegion.size;
    anyRegion.deviceAddress = callRegion.deviceAddress + callRegion.size;

    // Helper to retrieve the handle data
    auto getHandle = [&](int i) { return handles.data() + i * handleSize; };

    // Map the SBT buffer and write in the handles.
    auto* pSBTBuffer = reinterpret_cast<uint8_t*>(this->setup->device.mapMemory(buffer->memory, 0, sbtSize, {}));

    uint8_t* pData{ nullptr };
    uint32_t handleIdx{ 0 };
    // Raygen
    pData = pSBTBuffer;
    memcpy(pData, getHandle(handleIdx++), handleSize);
    // Miss
    pData = pSBTBuffer + rayGenRegion.size;
    for (uint32_t c = 0; c < missCount; c++)
    {
        memcpy(pData, getHandle(handleIdx++), handleSize);
        pData += missRegion.stride;
    }
    // Hit
    pData = pSBTBuffer + rayGenRegion.size + missRegion.size;
    for (uint32_t c = 0; c < hitCount; c++)
    {
        memcpy(pData, getHandle(handleIdx++), handleSize);
        pData += hitRegion.stride;
    }
    // Call
    pData = pSBTBuffer + rayGenRegion.size + missRegion.size + hitRegion.size;
    for (uint32_t c = 0; c < callCount; c++)
    {
        memcpy(pData, getHandle(handleIdx++), handleSize);
        pData += callRegion.stride;
    }
    // Any
    pData = pSBTBuffer + rayGenRegion.size + missRegion.size + hitRegion.size + callRegion.size;
    for (uint32_t c = 0; c < anyCount; c++)
    {
        memcpy(pData, getHandle(handleIdx++), handleSize);
        pData += anyRegion.stride;
    }

    this->setup->device.unmapMemory(buffer->memory);

    auto shaderBindingTable = ShaderBindingTable();
    shaderBindingTable.buffer = buffer;
    shaderBindingTable.rayGenRegion = rayGenRegion;
    shaderBindingTable.missRegion = missRegion;
    shaderBindingTable.hitRegion = hitRegion;
    shaderBindingTable.callRegion = callRegion;
    shaderBindingTable.anyRegion = anyRegion;

    return shaderBindingTable;
}

Pipeline::Pipeline(std::shared_ptr<Setup> setup) : IHasSetup(setup) {}

Pipeline::~Pipeline() {
    this->setup->device.destroyPipelineLayout(this->layout);
    this->setup->device.destroyPipeline(this->handle);
}

void Pipeline::run(std::shared_ptr<CommandBuffer> commandBuffer, Swapchain swapchain, std::vector<PushConstant> pushConstants) {
    std::vector<vk::DescriptorSet> descriptorSetHandles(this->descriptorSets.size());
    std::transform(this->descriptorSets.begin(), this->descriptorSets.end(), descriptorSetHandles.begin(), [](std::shared_ptr<DescriptorSet> x) { return x->handle; });

    commandBuffer->handle.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, this->handle);
    commandBuffer->handle.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, this->layout, 0, descriptorSetHandles, {});
    for(auto& pushConstant : pushConstants)
    commandBuffer->handle.pushConstants(this->layout,
        pushConstant.stagesUsed,
        0, pushConstant.size(), pushConstant.pointer());
    commandBuffer->handle.traceRaysKHR(this->SBT.rayGenRegion, this->SBT.missRegion, this->SBT.hitRegion, this->SBT.callRegion, swapchain.extent.width, swapchain.extent.height, 1);
}

uint32_t PushConstant::size()
{
    return sizeof(PushConstantData);
}

void* PushConstant::pointer()
{
    return &this->data;
}
