#pragma once

#include<string>
#include<vector>

#include "tiny_gltf.h"

#include "scene.hpp"

class FileReader {
public:
	std::vector<char> readSPV(const std::string filename);

	Model3D readPLY(const std::string filename, bool clockwise);

	tinygltf::Model readGLTF(const std::string folder, const std::string filename);

	std::tuple<std::vector<unsigned char>, uint32_t, uint32_t> readImage(std::string filename);
};

