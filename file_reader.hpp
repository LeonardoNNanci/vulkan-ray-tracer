#pragma once

#include<string>
#include<vector>

#include "scene.hpp"

class FileReader {
public:
	std::vector<char> readSPV(const std::string filename);

	Model3D readPLY(const std::string filename);
};

