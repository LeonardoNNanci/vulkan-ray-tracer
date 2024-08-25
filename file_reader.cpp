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
            vertices[i].pos = glm::vec4(x / maxVal, y / maxVal, z / maxVal, 1.);
        else
            vertices[i].pos = glm::vec4(z / maxVal, y / maxVal, x / maxVal, 1.);
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

std::shared_ptr<Scene> FileReader::readGLTF(const std::string filename, SceneBuilder sceneBuilder)
{
    tinygltf::Model tmodel;
    tinygltf::TinyGLTF tcontext;
    std::string warn, error;

    std::cout << "Loading GlTF file..." << std::endl;
    if (!tcontext.LoadASCIIFromFile(&tmodel, &error, &warn, filename))
        throw std::runtime_error("Failed loading scene file! " + error);
    
    std::map<int, std::vector<int>> meshPrimitives;
    int meshCount = 0, primitiveCount = 0;

    int materialIndex = 0;

    for (auto mesh : tmodel.meshes) {
        std::cout << "Mesh: " << mesh.name << std::endl;
        for (auto primitive : mesh.primitives) {
            if (primitive.mode != TINYGLTF_MODE_TRIANGLES)
                continue;

            Model3D model;

            {   // Vertex Buffer
                {   // Positions
                    auto positionAccessor = tmodel.accessors[primitive.attributes["POSITION"]];
                    auto positionBufferView = tmodel.bufferViews[positionAccessor.bufferView];
                    auto positionBuffer = tmodel.buffers[positionBufferView.buffer];
                    if (positionAccessor.type != TINYGLTF_TYPE_VEC3)
                        continue;

                    model.vertices = std::vector<Vertex>(positionAccessor.count);
                    int numComponents = tinygltf::GetNumComponentsInType(positionAccessor.type);
                    int componentSize = tinygltf::GetComponentSizeInBytes(positionAccessor.componentType);
                    auto positionData = &positionBuffer.data[positionBufferView.byteOffset + positionAccessor.byteOffset];


                    std::vector<float> pos(numComponents);
                    auto stride = std::max((int)positionBufferView.byteStride, componentSize * numComponents);
                    for (auto [i, p] = std::tuple{ 0, positionData }; i < positionAccessor.count; i++, p += stride) {
                        std::memcpy(pos.data(), p, numComponents * componentSize);
                        model.vertices[i].pos = {pos[0], pos[2], -pos[1], 1.};
                    }
                }
                {   // Normals
                    auto normalAccessor = tmodel.accessors[primitive.attributes["NORMAL"]];
                    auto normalBufferView = tmodel.bufferViews[normalAccessor.bufferView];
                    auto normalBuffer = tmodel.buffers[normalBufferView.buffer];
                    if (normalAccessor.type != TINYGLTF_TYPE_VEC3)
                        continue;
                
                    int numComponents = tinygltf::GetNumComponentsInType(normalAccessor.type);
                    int componentSize = tinygltf::GetComponentSizeInBytes(normalAccessor.componentType);
                    auto positionData = &normalBuffer.data[normalBufferView.byteOffset + normalAccessor.byteOffset];
                
                    std::vector<float> normal(numComponents);
                    auto stride = std::max((int)normalBufferView.byteStride, componentSize * numComponents);
                    for (auto [i, p] = std::tuple{ 0, positionData }; i < normalAccessor.count; i++, p += stride) {
                        std::memcpy(normal.data(), p, numComponents * componentSize);
                        model.vertices[i].normal = {normal[0], normal[2], -normal[1]};
                    }
                }
            }

            {   // Index Buffer
                auto indexAccessor = tmodel.accessors[primitive.indices];
                auto indexBufferView = tmodel.bufferViews[indexAccessor.bufferView];
                auto indexBuffer = tmodel.buffers[indexBufferView.buffer];
                if (indexAccessor.type != TINYGLTF_TYPE_SCALAR)
                    continue;
                
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
                    switch(indexAccessor.componentType) {
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
                        std::vector<USHORT> indices(indexAccessor.count);
                        std::memcpy(indices.data(), indexData, indexAccessor.count * numComponents * componentSize);
                        model.indices = std::vector<UINT>(indices.begin(), indices.end());
                        break;
                    }
                    /*case TINYGLTF_COMPONENT_TYPE_SHORT:{
                        std::vector<SHORT> indices(indexAccessor.count);
                        std::memcpy(indices.data(), indexData, indexAccessor.count * numComponents * componentSize);
                        model.indices = std::vector<UINT>(indices.begin(), indices.end());
                        break;
                    }*/
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
            sceneBuilder.addModel(model);
            meshPrimitives[meshCount].push_back(primitiveCount);
            primitiveCount++;
        }

        meshCount++;
    }
    
    auto gltfScene = tmodel.scenes[tmodel.defaultScene];
    std::stack <std::pair<tinygltf::Node, glm::mat4>> nodeStack;
    for (auto nodeIndex : gltfScene.nodes) {
        auto node = tmodel.nodes[nodeIndex];
        nodeStack.push({ node, glm::mat4(1.) });
    }

    while (!nodeStack.empty()) {
        auto [node, parentTransform] = nodeStack.top();
        nodeStack.pop();

        std::cout << "Node: " << node.name << std::endl;

        auto translation    = !node.translation.empty() ? glm::vec3(node.translation[0], node.translation[2], -node.translation[1])       : glm::vec3(0.);
        auto rotation       = !node.rotation.empty()    ? glm::quat(node.rotation[3], node.rotation[0], node.rotation[2], -node.rotation[1]) : glm::quat(1., 0., 0., 0.);
        auto scale          = !node.scale.empty()       ? glm::vec3(node.scale[0], node.scale[2], -node.scale[1])                            : glm::vec3(1.);
        auto T = glm::translate(glm::mat4(1.), translation);
        auto R = glm::mat4_cast(rotation);
        auto S = glm::scale(glm::mat4(1.), scale);

        auto transform = parentTransform * T * R * S;

        if (node.mesh >= 0)
            for (auto primitiveIndex : meshPrimitives[node.mesh]) {
                Instance instance(primitiveIndex, transform, materialIndex);
                sceneBuilder.addInstance(instance);
                materialIndex ^= 1;
            }

        for (auto nodeIndex : node.children) {
            auto child = tmodel.nodes[nodeIndex];
            nodeStack.push({ child, transform });
        }
    }
    return sceneBuilder.build();
}
