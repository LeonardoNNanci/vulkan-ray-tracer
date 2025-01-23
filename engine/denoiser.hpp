#pragma once
#include <memory>
#include <vector>

#include <optix.h>
#include <cuda_runtime_api.h>

#include <glm/glm.hpp>


#include "builder.hpp"

typedef unsigned int uint;

class Denoiser {
public:
	Denoiser(OptixDeviceContext context, CUstream stream, OptixDenoiser handle, uint width, uint heigth, CUdeviceptr denoiserBuffer, CUdeviceptr scratchBuffer, OptixDenoiserSizes sizes, CUdeviceptr hdrIntensity);

	void setSync(cudaExternalSemaphore_t semaphore, uint64_t waitSignal, uint64_t signalSignal);

	void run(float blendFactor, CUdeviceptr inputBuffer, CUdeviceptr albedoBuffer, CUdeviceptr normalBuffer, CUdeviceptr outputBuffer, std::vector<std::pair<glm::ivec2, glm::ivec2>> tileDescriptions);

	void run(float blendFactor, CUdeviceptr inputBuffer, CUdeviceptr albedoBuffer, CUdeviceptr outputBuffer, std::vector<std::pair<glm::ivec2, glm::ivec2>> tileDescriptions);

	void hardSynchronize();

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

	cudaExternalSemaphore_t semaphore;
	uint64_t waitSignal;
	uint64_t signalSignal;

	CUdeviceptr hdrIntensity;
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
	CUdeviceptr hdrIntensity = NULL;
	OptixDenoiserSizes sizes = { 0, 0, 0, 0, 0, 0, 0 };
	bool guideAlbedo = false;
	bool guideNormal = false;

	OptixDeviceContext createContext();

	CUstream createStream();

	OptixDenoiser createDenoiser();

	void setupDenoiser();
};