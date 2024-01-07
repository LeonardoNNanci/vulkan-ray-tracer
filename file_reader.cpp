#include <fstream>

#include "file_reader.hpp"
#include "happly.h"

std::vector<char> FileReader::readSPV(const std::string filename)
{
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

Model3D FileReader::readPLY(const std::string filename)
{
    Model3D model;

    happly::PLYData data(filename);
    auto rawVertices = data.getVertexPositions();
    auto rawIndices = data.getFaceIndices();

    std::vector<Vertex> vertices(rawVertices.size());
    for (int i = 0; i < vertices.size(); i++) {
        auto& [x, y, z] = rawVertices[i];
        vertices[i].pos = glm::vec4(x, y, z, 1.);
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
