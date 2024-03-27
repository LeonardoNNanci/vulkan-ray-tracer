#include "acceleration_structure.hpp"

AccelerationStructureBuilder::AccelerationStructureBuilder(std::shared_ptr<Setup> setup, std::shared_ptr<CommandBuffer> commandBuffer)
    : IHasSetup(setup), commandBuffer(commandBuffer) {}

std::shared_ptr<AccelerationStructure> AccelerationStructureBuilder::build()
{   
    this->commandBuffer->setFence();



    this->bottomLevelStructures.reserve(this->scene->models.size());
    for (auto model : this->scene->models) {
        this->bottomLevelStructures.emplace_back(createBottomLevel(model));
    }
    auto topLevel = createTopLevel();

    auto accelerationStructure = std::make_shared<AccelerationStructure>();
    accelerationStructure->bottomLevel = this->bottomLevelStructures;
    accelerationStructure->topLevel = topLevel;

    return accelerationStructure;
}

//AccelerationStructureBuilder AccelerationStructureBuilder::addModel3D(Model3D model) {
//	this->models.push_back(model);
//	return *this;
//}
//
//AccelerationStructureBuilder AccelerationStructureBuilder::addInstance(Instance instance, uint32_t modelIndex) {
//	this->instances.push_back({instance, modelIndex});
//	return *this;
//}

AccelerationStructureBuilder AccelerationStructureBuilder::setScene(std::shared_ptr<Scene> scene)
{
    this->scene = scene;
    return *this;
}

Requirements AccelerationStructureBuilder::getRequirements()
{
    Requirements extensions;
    extensions.deviceExtensions = {
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME
    };
    return extensions;
}

AccelerationStructureBuilder::~AccelerationStructureBuilder() {

}

std::shared_ptr<BottomLevelStructure> AccelerationStructureBuilder::createBottomLevel(Model3D model)
{
    vk::AccelerationStructureBuildRangeInfoKHR rangeInfo{
        .primitiveCount = static_cast<uint32_t>(model.indices.size() / 3), //faces
        .primitiveOffset = model.indexOffset * sizeof(uint32_t),
        .firstVertex = model.vertexOffset,
        .transformOffset = 0
    };

    auto descriptions = Vertex::getAttributeDescriptions();

    vk::AccelerationStructureGeometryTrianglesDataKHR trianglesData{
        .vertexFormat = descriptions[0].format,
        .vertexData = {.deviceAddress = this->scene->vertexBuffer->getDeviceAddress()}, // vertex buffer address
        .vertexStride = sizeof(Vertex),
        .maxVertex = static_cast<uint32_t>(model.vertexOffset + model.vertices.size() - 1),
        .indexType = vk::IndexType::eUint32,
        .indexData = {.deviceAddress = this->scene->indexBuffer->getDeviceAddress()} // index buffer address
    };

    vk::AccelerationStructureGeometryDataKHR geometryData{
        .triangles = trianglesData,
    };

    std::vector<vk::AccelerationStructureGeometryKHR> geometryInfos({
        {
            .geometryType = vk::GeometryTypeKHR::eTriangles,
            .geometry = geometryData,
            .flags = vk::GeometryFlagBitsKHR::eOpaque,
        }
        });

    vk::AccelerationStructureBuildGeometryInfoKHR buildGeometryInfo{
        .type = vk::AccelerationStructureTypeKHR::eBottomLevel,
        .flags = vk::BuildAccelerationStructureFlagBitsKHR::eAllowCompaction,
        .mode = vk::BuildAccelerationStructureModeKHR::eBuild,
        .geometryCount = static_cast<uint32_t>(geometryInfos.size()),
        .pGeometries = geometryInfos.data(),
    };


    vk::AccelerationStructureBuildSizesInfoKHR buildSizeInfo = this->setup->device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, buildGeometryInfo, rangeInfo.primitiveCount);

    auto bottomLevelStructureBuffer = BufferBuilder(this->setup)
        .setCommandBuffer(this->commandBuffer)
        .setSize(buildSizeInfo.accelerationStructureSize)
        .setUsage(vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR)
        .setMemoryProperties(vk::MemoryPropertyFlagBits::eDeviceLocal)
        .build();

    vk::AccelerationStructureCreateInfoKHR BLASInfo{
        .buffer = bottomLevelStructureBuffer->handle,
        .size = buildSizeInfo.accelerationStructureSize,
        .type = vk::AccelerationStructureTypeKHR::eBottomLevel
    };

    auto sizeInfo = this->setup->device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, buildGeometryInfo, { rangeInfo.primitiveCount });
    auto scratchBuffer = BufferBuilder(this->setup)
        .setCommandBuffer(this->commandBuffer)
        .setSize(sizeInfo.buildScratchSize)
        .setUsage(vk::BufferUsageFlagBits::eStorageBuffer)
        .setUsage(vk::BufferUsageFlagBits::eShaderDeviceAddress)
        .build();

    buildGeometryInfo.setScratchData({ .deviceAddress = scratchBuffer->getDeviceAddress() });

    auto handle = this->setup->device.createAccelerationStructureKHR(BLASInfo);
    buildGeometryInfo.setDstAccelerationStructure(handle);

    this->commandBuffer->setFence();
    this->commandBuffer->begin();
    this->commandBuffer->handle.buildAccelerationStructuresKHR({ buildGeometryInfo }, { &rangeInfo });
    this->commandBuffer->submit();
    this->commandBuffer->waitFinished();
    this->commandBuffer->resetFence();

    auto bottomLevelStructure = std::make_shared<BottomLevelStructure>(this->setup);
    bottomLevelStructure->handle = handle;
    bottomLevelStructure->buffer = bottomLevelStructureBuffer;
    bottomLevelStructure->vertexBuffer = this->scene->vertexBuffer;
    bottomLevelStructure->indexBuffer = this->scene->indexBuffer;

    return bottomLevelStructure;
}

std::shared_ptr<TopLevelStructure> AccelerationStructureBuilder::createTopLevel()
{
    std::vector<vk::AccelerationStructureInstanceKHR> bottomLevelInstances;
    bottomLevelInstances.reserve(this->scene->instances.size());

    uint32_t i = 0;
    for (auto& instance : this->scene->instances) {
        vk::AccelerationStructureDeviceAddressInfoKHR addressInfo{
            .accelerationStructure = this->bottomLevelStructures[instance.modelId]->handle
        };
        auto BLASAddress = this->setup->device.getAccelerationStructureAddressKHR(addressInfo);

        vk::AccelerationStructureInstanceKHR rayInst{
            .transform = {{{
                    instance.transform[0][0], instance.transform[1][0], instance.transform[2][0], instance.transform[3][0],
                    instance.transform[0][1], instance.transform[1][1], instance.transform[2][1], instance.transform[3][1],
                    instance.transform[0][2], instance.transform[1][2], instance.transform[2][2], instance.transform[3][2]
            }}},
            .instanceCustomIndex = instance.modelId,
            .mask = 0xFF,
            .instanceShaderBindingTableRecordOffset = instance.hitShaderOffset,
            .accelerationStructureReference = BLASAddress
        };
        rayInst.setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable);
        bottomLevelInstances.emplace_back(rayInst);
    }

    auto instancesBuffer = BufferBuilder(this->setup)
        .setCommandBuffer(this->commandBuffer)
        .setSize(bottomLevelInstances.size() * sizeof(vk::AccelerationStructureInstanceKHR))
        .setUsage(vk::BufferUsageFlagBits::eShaderDeviceAddress)
        .setUsage(vk::BufferUsageFlagBits::eTransferDst)
        .setUsage(vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR)
        .setMemoryProperties(vk::MemoryPropertyFlagBits::eDeviceLocal)
        .build();
    instancesBuffer->fill(bottomLevelInstances);

    vk::MemoryBarrier barrier{
        .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
        .dstAccessMask = vk::AccessFlagBits::eAccelerationStructureWriteKHR
    };

    vk::AccelerationStructureGeometryInstancesDataKHR instancesData{
        .data = {.deviceAddress = instancesBuffer->getDeviceAddress()}
    };

    vk::AccelerationStructureGeometryKHR TLASGeometry{
        .geometryType = vk::GeometryTypeKHR::eInstances,
        .geometry = {
            .instances = instancesData
}
    };

    vk::AccelerationStructureBuildGeometryInfoKHR buildInfo{
        .type = vk::AccelerationStructureTypeKHR::eTopLevel,
        .flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
        .mode = vk::BuildAccelerationStructureModeKHR::eBuild,
        .geometryCount = 1,
        .pGeometries = &TLASGeometry
    };

    auto sizeInfo = this->setup->device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, buildInfo, this->scene->instances.size());
    auto topLevelBuffer = BufferBuilder(this->setup)
        .setSize(sizeInfo.accelerationStructureSize)
        .setUsage(vk::BufferUsageFlagBits::eTransferDst)
        .setUsage(vk::BufferUsageFlagBits::eShaderDeviceAddress)
        .setUsage(vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR)
        .setMemoryProperties(vk::MemoryPropertyFlagBits::eDeviceLocal)
        .build();
    auto scratchBuffer = BufferBuilder(this->setup)
        .setSize(sizeInfo.buildScratchSize)
        .setUsage(vk::BufferUsageFlagBits::eTransferDst)
        .setUsage(vk::BufferUsageFlagBits::eShaderDeviceAddress)
        .setUsage(vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR)
        .setUsage(vk::BufferUsageFlagBits::eStorageBuffer)
        .setMemoryProperties(vk::MemoryPropertyFlagBits::eDeviceLocal)
        .build();

    buildInfo.setScratchData({ .deviceAddress = scratchBuffer->getDeviceAddress() });
    vk::AccelerationStructureCreateInfoKHR structureInfo{
        .buffer = topLevelBuffer->handle,
        .offset = topLevelBuffer->offset,
        .size = topLevelBuffer->size,
        .type = vk::AccelerationStructureTypeKHR::eTopLevel,
    };

    auto handle = this->setup->device.createAccelerationStructureKHR(structureInfo);
    buildInfo.setDstAccelerationStructure(handle);

    vk::AccelerationStructureBuildRangeInfoKHR buildOffsetInfo{
        .primitiveCount = static_cast<uint32_t>(this->scene->instances.size()),
        .primitiveOffset = 0,
        .firstVertex = 0
    };

    this->commandBuffer->resetFence();
    this->commandBuffer->begin();
    this->commandBuffer->handle.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR, {}, { barrier }, nullptr, nullptr);
    this->commandBuffer->handle.buildAccelerationStructuresKHR({ buildInfo }, { &buildOffsetInfo });
    this->commandBuffer->submit();
    this->commandBuffer->waitFinished();
    this->commandBuffer->resetFence();

    auto topLevelStructure = std::make_shared<TopLevelStructure>(this->setup);
    topLevelStructure->handle = handle;
    topLevelStructure->buffer = topLevelBuffer;
    topLevelStructure->instanceBuffer = instancesBuffer;

    return topLevelStructure;
}

BottomLevelStructure::BottomLevelStructure(std::shared_ptr<Setup> setup) : AccelerationStructureLevel(setup) {}

TopLevelStructure::TopLevelStructure(std::shared_ptr<Setup> setup) : AccelerationStructureLevel(setup) {}

AccelerationStructureLevel::AccelerationStructureLevel(std::shared_ptr<Setup> setup) : IHasSetup(setup) {}

AccelerationStructureLevel::~AccelerationStructureLevel() {
    this->setup->device.destroyAccelerationStructureKHR(this->handle);
}

AccelerationStructure::AccelerationStructure() {}
