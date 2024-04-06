#pragma once
#include <memory>

#include <optix.h>
#include <glm/glm.hpp>

#include "builder.hpp"

typedef unsigned int uint;

class Denoiser {
public:
	Denoiser(OptixDeviceContext context, CUstream stream, OptixDenoiser handle, uint width, uint heigth, CUdeviceptr denoiserBuffer, CUdeviceptr scratchBuffer, OptixDenoiserSizes sizes);

	void run(CUdeviceptr inputBuffer, CUdeviceptr albedoBuffer, CUdeviceptr normalBuffer, CUdeviceptr outputBuffer, glm::ivec2 bottomLeft, int overlap);

	void run(CUdeviceptr inputBuffer, CUdeviceptr albedoBuffer, CUdeviceptr outputBuffer, glm::ivec2 bottomLeft, int overlap);

	~Denoiser();

private:
	OptixDeviceContext context;
	CUstream stream;

	OptixDenoiser handle;

	uint width;
	uint height;

	OptixDenoiserSizes sizes = {};

	CUdeviceptr denoiserBuffer;
	CUdeviceptr scratchBuffer;
};

class DenoiserBuilder : public Builder <std::shared_ptr< Denoiser >> {
public:
	DenoiserBuilder(uint width, uint height);

	DenoiserBuilder setGuideAlbedo();

	DenoiserBuilder setGuideNormal();

	std::shared_ptr<Denoiser> build();

private:
	OptixDeviceContext context = NULL;
	CUstream stream = NULL;
	uint width;
	uint height;
	OptixDenoiser handle = NULL;
	CUdeviceptr denoiserBuffer = NULL;
	CUdeviceptr scratchBuffer = NULL;
	OptixDenoiserSizes sizes = { 0, 0, 0, 0, 0, 0, 0 };
	bool guideAlbedo = false;
	bool guideNormal = false;

	OptixDeviceContext createContext();

	CUstream createStream();

	OptixDenoiser createDenoiser();

	void setupDenoiser();
};