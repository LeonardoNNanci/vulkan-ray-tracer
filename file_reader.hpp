#pragma once

#include<string>
#include<vector>

#include "scene.hpp"

class FileReader {
public:
	std::vector<char> readSPV(const std::string filename);

	Model3D readPLY(const std::string filename, bool clockwise);

	std::shared_ptr<Scene> readGLTF(const std::string filename, SceneBuilder sceneBuilder);
};

