#pragma once
#include <glm/glm.hpp>
#include <vector>

struct Vertex {
	alignas(32) glm::vec4 pos;
};

class Model3D {
	std::vector<Vertex> vertexBuffer;
	std::vector<uint32_t> indexBuffer;
};

class Instance {
	Model3D model;
	glm::mat3x4 transform;

};