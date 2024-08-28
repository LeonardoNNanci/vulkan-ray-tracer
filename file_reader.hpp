#pragma once

#include<string>
#include<vector>

#include "scene.hpp"

class FileReader {
public:
	std::vector<char> readSPV(const std::string filename);

	Model3D readPLY(const std::string filename, bool clockwise);

	SceneBuilder readGLTF(const std::string folder, const std::string filename, SceneBuilder sceneBuilder);

	std::tuple<std::vector<unsigned char>, uint32_t, uint32_t> readImage(std::string filename);
};

