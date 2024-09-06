#include <vulkan/vulkan.hpp>

#include <stack>
#include <iostream>

#define GLM_ENABLE_EXPERIMENTAL
#include<glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include "scene.hpp"


class GlTFParser {
public:
    GlTFParser(tinygltf::Model tmodel, SceneBuilder& sceneBuilder, std::string srcFolder) : tmodel(tmodel), sceneBuilder(sceneBuilder), srcFolder(srcFolder) {}

    SceneBuilder parse() {
        loadImages();
        loadSamplers();
        loadTextures();
        loadMaterials();
        loadMeshes();

        loadLights();

        loadSceneGraph();
        return sceneBuilder;
    }

private:
    tinygltf::Model tmodel;
    SceneBuilder& sceneBuilder;

    std::string srcFolder;
    std::map<int, std::vector<int>> meshPrimitives;

    void loadImages() {
        for (auto imageData : tmodel.images) {
            std::cout << "Image: " << imageData.name << std::endl;
            if (!imageData.uri.empty()) {
                sceneBuilder.addImage(imageData.image, imageData.width, imageData.height);
                continue;
            }

            throw std::runtime_error("Raw image loading not implemented.");
        }
    }

    void loadSamplers() {
        std::map<int, vk::Filter> filterMap = {
            {TINYGLTF_TEXTURE_FILTER_LINEAR, vk::Filter::eLinear},
            {TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR, vk::Filter::eLinear}
        };

        std::map<int, vk::SamplerAddressMode> wrapMap = {
            {TINYGLTF_TEXTURE_WRAP_REPEAT, vk::SamplerAddressMode::eRepeat}
        };

        for (auto samplerData : tmodel.samplers) {
            std::cout << "Sampler: " << samplerData.name << std::endl;

            sceneBuilder.addSampler(
                filterMap[samplerData.magFilter],
                filterMap[samplerData.minFilter]
            );
        }
    }

    void loadTextures() {
        for (auto textureData : tmodel.textures) {
            std::cout << "Texture: " << textureData.name << std::endl;
            TextureIndices texture{
                .imageIndex = static_cast<uint32_t>(textureData.source),
                .samplerIndex = static_cast<uint32_t>(textureData.sampler)
            };
            sceneBuilder.addTexture(texture);
        }
    }

    void loadMaterials() {
        for (auto materialData : tmodel.materials) {
            Material material;

            auto baseColor = materialData.pbrMetallicRoughness.baseColorFactor;
            if (!baseColor.empty())
                material.baseColor = { baseColor[0], baseColor[1], baseColor[2], baseColor[3] };

            auto textureIndex = materialData.pbrMetallicRoughness.baseColorTexture.index;
            material.colorTexture = static_cast<uint32_t>(textureIndex);
            //material.colorTexture.imageIndex = static_cast<uint32_t>(texture.source);
            //material.colorTexture.samplerIndex = static_cast<uint32_t>(texture.sampler);

            material.metalicFactor = materialData.pbrMetallicRoughness.metallicFactor;
            material.roughnessFactor = materialData.pbrMetallicRoughness.roughnessFactor;

            sceneBuilder.addMaterial(material);
        }
    }

    void loadMeshes() {
        int meshCount = 0, primitiveCount = 0;

        for (auto mesh : tmodel.meshes) {
            std::cout << "Mesh: " << mesh.name << std::endl;
            for (auto primitive : mesh.primitives) {
                if (primitive.mode != TINYGLTF_MODE_TRIANGLES)
                    continue;

                Model3D model;

                loadPrimitivePositions(model, primitive);
                loadPrimitiveNormals(model, primitive);
                loadPrimitiveTextureCoordinates(model, primitive);
                loadPrimitiveIndices(model, primitive, meshCount, primitiveCount);
                model.materialIndex = static_cast<uint32_t>(primitive.material);

                sceneBuilder.addModel(model);
                meshPrimitives[meshCount].push_back(primitiveCount);
                primitiveCount++;
            }

            meshCount++;
        }
    }

    void loadLights() {
        for (auto lightData : tmodel.lights) {
            Light light;

            auto color = lightData.color;
            light.color = { color[0], color[1], color[2] };
            light.intensity = lightData.intensity;

            if (lightData.type == "directional")
                light.type = 0;
            else
                throw std::runtime_error("Light type not supported: " + lightData.type + "\n");

            this->sceneBuilder.addLight(light);
        }
    }

    void loadSceneGraph() {
        auto gltfScene = tmodel.scenes[tmodel.defaultScene];
        std::stack <std::pair<tinygltf::Node, glm::mat4>> nodeStack;
        for (auto nodeIndex : gltfScene.nodes) {
            auto node = tmodel.nodes[nodeIndex];
            nodeStack.push({ node, glm::mat4(1.) });
        }

        int i = 0;
        while (!nodeStack.empty()) {
            auto [node, parentTransform] = nodeStack.top();
            nodeStack.pop();

            std::cout << "Node: " << node.name << std::endl;

            auto translation = !node.translation.empty() ? glm::vec3(node.translation[0], node.translation[2], -node.translation[1]) : glm::vec3(0.);
            auto rotation = !node.rotation.empty() ? glm::quat(node.rotation[3], node.rotation[0], node.rotation[2], -node.rotation[1]) : glm::quat(1., 0., 0., 0.);
            auto scale = !node.scale.empty() ? glm::vec3(node.scale[0], node.scale[2], -node.scale[1]) : glm::vec3(1.);
            auto T = glm::translate(glm::mat4(1.), translation);
            auto R = glm::mat4_cast(rotation);
            auto S = glm::scale(glm::mat4(1.), scale);

            auto transform = parentTransform * T * R * S;

            // surface node
            if (node.mesh >= 0)
                for (auto primitiveIndex : meshPrimitives[node.mesh]) {
                    Instance instance(primitiveIndex, transform, 0);
                    sceneBuilder.addInstance(instance);
                }

            // light node
            if (node.light >= 0) {
                auto positionTransform = parentTransform * T;
                auto directionTransform = parentTransform * R;

                Light& light = this->sceneBuilder.lights[node.light];
                light.position = positionTransform * light.position;
                light.direction = directionTransform * light.direction;
                int x = 0;
            }

            for (auto nodeIndex : node.children) {
                auto child = tmodel.nodes[nodeIndex];
                nodeStack.push({ child, transform });
            }
        }
    }

    void loadPrimitivePositions(Model3D& model, tinygltf::Primitive primitive) {
        auto p = primitive.attributes.find("POSITION");
        if (p == primitive.attributes.end())
            return;

        auto index = p->second;
        auto positionAccessor = tmodel.accessors[index];
        auto positionBufferView = tmodel.bufferViews[positionAccessor.bufferView];
        auto positionBuffer = tmodel.buffers[positionBufferView.buffer];

        if (positionAccessor.type != TINYGLTF_TYPE_VEC3)
            return;

        model.vertices = std::vector<Vertex>(positionAccessor.count);
        int numComponents = tinygltf::GetNumComponentsInType(positionAccessor.type);
        int componentSize = tinygltf::GetComponentSizeInBytes(positionAccessor.componentType);
        auto positionData = &positionBuffer.data[positionBufferView.byteOffset + positionAccessor.byteOffset];

        std::vector<float> pos(numComponents);
        auto stride = std::max((int)positionBufferView.byteStride, componentSize * numComponents);
        for (auto [i, p] = std::tuple{ 0, positionData }; i < positionAccessor.count; i++, p += stride) {
            std::memcpy(pos.data(), p, numComponents * componentSize);
            model.vertices[i].position = { pos[0], pos[2], -pos[1], 1. };
        }
    }

    void loadPrimitiveNormals(Model3D& model, tinygltf::Primitive primitive) {
        auto p = primitive.attributes.find("NORMAL");
        if (p == primitive.attributes.end())
            return;

        auto index = p->second;
        auto normalAccessor = tmodel.accessors[index];
        auto normalBufferView = tmodel.bufferViews[normalAccessor.bufferView];
        auto normalBuffer = tmodel.buffers[normalBufferView.buffer];
        if (normalAccessor.type != TINYGLTF_TYPE_VEC3)
            return;

        int numComponents = tinygltf::GetNumComponentsInType(normalAccessor.type);
        int componentSize = tinygltf::GetComponentSizeInBytes(normalAccessor.componentType);
        auto positionData = &normalBuffer.data[normalBufferView.byteOffset + normalAccessor.byteOffset];

        std::vector<float> normal(numComponents);
        auto stride = std::max((int)normalBufferView.byteStride, componentSize * numComponents);
        for (auto [i, p] = std::tuple{ 0, positionData }; i < normalAccessor.count; i++, p += stride) {
            std::memcpy(normal.data(), p, numComponents * componentSize);
            model.vertices[i].normal = { normal[0], normal[2], -normal[1] };
        }
    }

    void loadPrimitiveTextureCoordinates(Model3D& model, tinygltf::Primitive primitive) {
        auto p = primitive.attributes.find("TEXCOORD_0");
        if (p == primitive.attributes.end())
            return;

        auto index = p->second;
        auto accessor = tmodel.accessors[index];
        auto bufferView = tmodel.bufferViews[accessor.bufferView];
        auto buffer = tmodel.buffers[bufferView.buffer];
        if (accessor.type != TINYGLTF_TYPE_VEC2)
            return;

        int numComponents = tinygltf::GetNumComponentsInType(accessor.type);
        int componentSize = tinygltf::GetComponentSizeInBytes(accessor.componentType);
        auto data = &buffer.data[bufferView.byteOffset + accessor.byteOffset];

        std::vector<float> normal(numComponents);
        auto stride = std::max((int)bufferView.byteStride, componentSize * numComponents);
        for (auto [i, p] = std::tuple{ 0, data }; i < accessor.count; i++, p += stride) {
            std::memcpy(normal.data(), p, numComponents * componentSize);
            model.vertices[i].textureCoordinates = { normal[0], normal[1] };
        }
    }

    void loadPrimitiveIndices(Model3D& model, tinygltf::Primitive primitive, int meshCount, int primitiveCount) {
        // unindexed
        if (primitive.indices < 0) {
            model.indices.resize(model.vertices.size());
            for (int i = 0; i < model.indices.size(); i++)
                model.indices[i] = i;
            return;
        }

        // indexed
        auto indexAccessor = tmodel.accessors[primitive.indices];
        auto indexBufferView = tmodel.bufferViews[indexAccessor.bufferView];
        auto indexBuffer = tmodel.buffers[indexBufferView.buffer];
        if (indexAccessor.type != TINYGLTF_TYPE_SCALAR)
            return;

        int numComponents = tinygltf::GetNumComponentsInType(indexAccessor.type);
        int componentSize = tinygltf::GetComponentSizeInBytes(indexAccessor.componentType);
        auto indexData = &indexBuffer.data[indexBufferView.byteOffset + indexAccessor.byteOffset];
        if (indexAccessor.count == 0)
        {
            model.indices.resize(model.vertices.size());
            for (unsigned int i = 0; i < model.indices.size(); i++)
                model.indices[i] = i;
        }
        else
            switch (indexAccessor.componentType) {
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
                std::vector<unsigned short> indices(indexAccessor.count);
                std::memcpy(indices.data(), indexData, indexAccessor.count * numComponents * componentSize);
                model.indices = std::vector<unsigned int>(indices.begin(), indices.end());
                break;
            }
            case TINYGLTF_COMPONENT_TYPE_SHORT: {
                std::vector<unsigned short> indices(indexAccessor.count);
                std::memcpy(indices.data(), indexData, indexAccessor.count * numComponents * componentSize);
                model.indices = std::vector<unsigned int>(indices.begin(), indices.end());
                break;
            }
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
            {
                model.indices.resize(indexAccessor.count);
                std::memcpy(model.indices.data(), indexData, indexAccessor.count * numComponents * componentSize);
                break;
            }
            default:
                throw std::runtime_error("Index component type not supported: " + indexAccessor.componentType);
            }
    }
};

vk::VertexInputBindingDescription Vertex::getBindingDescription() {
    return {
    .binding = 0,
    .stride = sizeof(Vertex),
    .inputRate = vk::VertexInputRate::eVertex
    };
}

std::array<vk::VertexInputAttributeDescription, 1> Vertex::getAttributeDescriptions() {
    vk::VertexInputAttributeDescription posDescription{
        .location = 0,
        .binding = 0,
        .format = vk::Format::eR32G32B32Sfloat,
        .offset = offsetof(Vertex, position)
    };
    return { posDescription,  };
}

Instance::Instance(uint32_t modelId, glm::mat4 transform, uint32_t hitShaderOffset) : modelId(modelId), transform(transform), hitShaderOffset(hitShaderOffset) {}

SceneBuilder::SceneBuilder(std::shared_ptr<Setup> setup, std::shared_ptr<CommandBuffer> commandBuffer) : IHasSetup(setup), commandBuffer(commandBuffer) {}

SceneBuilder SceneBuilder::addModel(Model3D model)
{
    this->models.push_back(model);
    return *this;
}

SceneBuilder SceneBuilder::addInstance(Instance instance)
{
    this->instances.push_back(instance);
    return *this;
}

SceneBuilder SceneBuilder::addTexture(TextureIndices texture)
{
    this->textureIndices.push_back(texture);
    return *this;
}

SceneBuilder SceneBuilder::addMaterial(Material material)
{
    this->materials.push_back(material);
    return *this;
}

SceneBuilder SceneBuilder::addSampler(vk::Filter mag, vk::Filter min)
{
    auto sampler = std::make_shared<Sampler>(this->setup, mag, min);
    this->samplers.push_back(sampler);
    return *this;
}

SceneBuilder SceneBuilder::addLight(Light light)
{
    this->lights.push_back(light);
    return *this;
}

SceneBuilder SceneBuilder::addImage(std::vector<unsigned char> bytes, uint32_t width, uint32_t height)
{
    auto image = std::make_shared<Image>(this->setup, bytes, width, height, this->commandBuffer);
    this->images.push_back(image);
    return *this;
}

SceneBuilder SceneBuilder::loadGlTF(tinygltf::Model tmodel, std::string srcFolder)
{
    GlTFParser(tmodel, *this, srcFolder).parse();
    return *this;
}

std::shared_ptr<Scene> SceneBuilder::build()
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<ModelDescription> objectDescriptions;

    std::vector <TexturePointers> texturePointers(textureIndices.size());

    if (!this->images.empty()) {
        for (int i = 0; i < textureIndices.size(); i++) {
            texturePointers[i].image = this->images[textureIndices[i].imageIndex];
            texturePointers[i].sampler = samplers[textureIndices[i].samplerIndex];
        }
    }

    for (auto& model : this->models) {
        objectDescriptions.push_back({
            .vertexStride = static_cast<uint32_t>(vertices.size()),
            .indexStride = static_cast<uint32_t>(indices.size()),
            .materialIndex = static_cast<uint32_t>(model.materialIndex)
        });
        model.vertexOffset = vertices.size();
        model.indexOffset = indices.size();
        vertices.insert(vertices.end(), model.vertices.begin(), model.vertices.end());
        indices.insert(indices.end(), model.indices.begin(), model.indices.end());
    }

    auto vertexBuffer = BufferBuilder(this->setup)
        .setSize(vertices.size() * sizeof(vertices[0]))
        .setCommandBuffer(this->commandBuffer)
        .setUsage(vk::BufferUsageFlagBits::eStorageBuffer)
        .setUsage(vk::BufferUsageFlagBits::eShaderDeviceAddress)
        .setUsage(vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR)
        .setMemoryProperties(vk::MemoryPropertyFlagBits::eDeviceLocal)
        .build();
    vertexBuffer->fill(vertices);

    auto indexBuffer = BufferBuilder(this->setup)
        .setSize(indices.size() * sizeof(indices[0]))
        .setCommandBuffer(this->commandBuffer)
        .setUsage(vk::BufferUsageFlagBits::eStorageBuffer)
        .setUsage(vk::BufferUsageFlagBits::eShaderDeviceAddress)
        .setUsage(vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR)
        .setMemoryProperties(vk::MemoryPropertyFlagBits::eDeviceLocal)
        .build();
    indexBuffer->fill(indices);

    auto descriptionBuffer = BufferBuilder(this->setup)
        .setSize(objectDescriptions.size() * sizeof(objectDescriptions[0]))
        .setCommandBuffer(this->commandBuffer)
        .setUsage(vk::BufferUsageFlagBits::eStorageBuffer)
        .setMemoryProperties(vk::MemoryPropertyFlagBits::eDeviceLocal)
        .build();
    descriptionBuffer->fill(objectDescriptions);

    std::shared_ptr<Buffer> materialBuffer = nullptr;
    if (!this->materials.empty()) {
        materialBuffer = BufferBuilder(this->setup)
            .setSize(this->materials.size() * sizeof(this->materials[0]))
            .setCommandBuffer(this->commandBuffer)
            .setUsage(vk::BufferUsageFlagBits::eStorageBuffer)
            .setMemoryProperties(vk::MemoryPropertyFlagBits::eDeviceLocal)
            .build();
        materialBuffer->fill(this->materials);
    }

    auto lightBuffer = BufferBuilder(this->setup)
        .setSize(this->lights.size() * sizeof(this->lights[0]))
        .setCommandBuffer(this->commandBuffer)
        .setUsage(vk::BufferUsageFlagBits::eStorageBuffer)
        .setMemoryProperties(vk::MemoryPropertyFlagBits::eDeviceLocal)
        .build();
    lightBuffer->fill(this->lights);

    auto scene = std::make_shared<Scene>();
    scene->models = this->models;
    scene->instances = this->instances;
    scene->vertexBuffer = vertexBuffer;
    scene->indexBuffer = indexBuffer;
    scene->objectDescriptionBuffer = descriptionBuffer;
    scene->materialBuffer = materialBuffer;
    scene->texturePointers = texturePointers;
    scene->lightCount = static_cast<uint32_t>(this->lights.size());
    scene->lightBuffer = lightBuffer;

    return scene;
}