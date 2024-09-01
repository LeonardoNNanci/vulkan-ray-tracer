#include <fstream>
#include<limits>
#include "file_reader.hpp"
#include "happly.h"

#include "tinygltf/tiny_gltf.cc"

#define GLM_ENABLE_EXPERIMENTAL
#include<glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include<stack>

std::vector<char> FileReader::readSPV(const std::string filename)
{
    std::cout << "Reading file " << filename << "..." << std::endl;
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

Model3D FileReader::readPLY(const std::string filename, bool clockwise = true)
{
    Model3D model;

    happly::PLYData data(filename);
    auto rawVertices = data.getVertexPositions();
    auto rawIndices = data.getFaceIndices();

    float maxVal = 0;
    float minY = std::numeric_limits<float>::infinity();
    for (auto& [x, y, z] : rawVertices)
    {
        minY = std::min<double>(minY, y);
    }
    for (auto& v : rawVertices)
    {
        v[1] -= minY;
        maxVal = std::max<double>({ maxVal, abs(v[0]), abs(v[1]), abs(v[2]) });
    }
    std::cout << minY << " " << maxVal << std::endl;
    std::vector<Vertex> vertices(rawVertices.size());
    for (int i = 0; i < vertices.size(); i++) {
        auto& [x, y, z] = rawVertices[i];
        if (clockwise)
            vertices[i].position = glm::vec4(x / maxVal, y / maxVal, z / maxVal, 1.);
        else
            vertices[i].position = glm::vec4(z / maxVal, y / maxVal, x / maxVal, 1.);
    }

    std::vector<uint32_t> indices(3 * rawIndices.size());
    for (int i = 0; i < rawIndices.size(); i++) {
        indices[i * 3] = rawIndices[i][0];
        indices[i * 3 + 1] = rawIndices[i][1];
        indices[i * 3 + 2] = rawIndices[i][2];
    }

    model.vertices = vertices;
    model.indices = indices;
    return model;
}

class GlTFParser {
public:
    GlTFParser(const std::string folder, const std::string filename, SceneBuilder sceneBuilder) : folder(folder), sceneBuilder(sceneBuilder) {
        std::string warn, error;

        std::cout << "Loading GlTF file..." << std::endl;
        if (!tcontext.LoadASCIIFromFile(&this->tmodel, &error, &warn, folder + filename))
            throw std::runtime_error("Failed loading scene file! " + error);
    }

    SceneBuilder parse() {
        loadImages();
        loadSamplers();
        loadTextures();
        loadMaterials();
        loadMeshes();
        loadSceneGraph();
        return sceneBuilder;
    }

private:
    SceneBuilder sceneBuilder;
    tinygltf::Model tmodel;
    tinygltf::TinyGLTF tcontext;
    std::string folder;
    std::map<int, std::vector<int>> meshPrimitives;

    void loadImages() {
        for (auto imageData : tmodel.images) {
            std::cout << "Image: " << imageData.name << std::endl;
            if (!imageData.uri.empty()) {
                std::string img = imageData.uri;
                sceneBuilder.addImage(folder + img);
                continue;
            }

            throw std::runtime_error("Raw image loading not implemented.");
        }
    }

    void loadSamplers() {
        std::map<int, vk::Filter> filterMap= {
            {TINYGLTF_TEXTURE_FILTER_LINEAR, vk::Filter::eLinear},
            {TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR, vk::Filter::eLinear}
        };

        std::map<int, vk::SamplerAddressMode> wrapMap = {
            {TINYGLTF_TEXTURE_WRAP_REPEAT, vk::SamplerAddressMode::eRepeat}
        };

        for (auto samplerData : tmodel.samplers) {
            std::cout << "Sampler: " << samplerData.name << std::endl;

            SamplerInfo sampler{
                .magFilter = filterMap[samplerData.magFilter],
                .minFilter = filterMap[samplerData.minFilter]
            };

            sceneBuilder.addSampler(sampler);
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
            if(!baseColor.empty())
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

            if (node.mesh >= 0)
                for (auto primitiveIndex : meshPrimitives[node.mesh]) {
                    Instance instance(primitiveIndex, transform, 0);
                    sceneBuilder.addInstance(instance);
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
                for (UINT i = 0; i < model.indices.size(); i++)
                    model.indices[i] = i;
            }
            else
                switch (indexAccessor.componentType) {
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
                    std::vector<USHORT> indices(indexAccessor.count);
                    std::memcpy(indices.data(), indexData, indexAccessor.count * numComponents * componentSize);
                    model.indices = std::vector<UINT>(indices.begin(), indices.end());
                    break;
                }
                case TINYGLTF_COMPONENT_TYPE_SHORT: {
                    std::vector<USHORT> indices(indexAccessor.count);
                    std::memcpy(indices.data(), indexData, indexAccessor.count * numComponents * componentSize);
                    model.indices = std::vector<UINT>(indices.begin(), indices.end());
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

SceneBuilder FileReader::readGLTF(const std::string folder, const std::string filename, SceneBuilder sceneBuilder)
{
    return GlTFParser(folder, filename, sceneBuilder).parse();
}

std::tuple<std::vector<unsigned char>, uint32_t, uint32_t> FileReader::readImage(std::string filename)
{
    int imgChannels, imgWidth, imgHeight;
    stbi_uc* pixelData = stbi_load(filename.c_str(), &imgWidth, &imgHeight, &imgChannels, STBI_rgb_alpha);

    std::cout << "Loading " << filename << " data..." << std::endl;

    if (!pixelData)
        throw std::runtime_error("Failed loading image from file!\n");

    int imgDataSize = imgWidth * imgHeight * 4;

    std::vector<unsigned char> returnData(pixelData, pixelData + imgDataSize);
    pixelData[0] = 111;
    stbi_image_free(pixelData);

    return {
        returnData,
        static_cast<uint32_t>(imgWidth),
        static_cast <uint32_t>(imgHeight)
    };
}


