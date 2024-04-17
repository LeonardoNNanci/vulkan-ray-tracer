#include "denoiser.hpp"
#include <iostream>

#pragma once
#include <stdexcept>
#include <optix.h>
#include <optix_function_table_definition.h>
#include <optix_denoiser_tiling.h>
#include <optix_stubs.h>
#include <cuda_runtime_api.h>

uint32_t sizeof_pixel = static_cast<uint32_t>(3 * sizeof(float));

void optixLogCallback(unsigned int level, const char* tag, const char* message, void* cbdata)
{
	printf("OptiX [%s]: %s\n", tag, message);
}

DenoiserBuilder::DenoiserBuilder(uint width, uint height) : width(width), height(height) {}

DenoiserBuilder DenoiserBuilder::setGuideAlbedo()
{
	this->guideAlbedo = true;
	return *this;
}

DenoiserBuilder DenoiserBuilder::setGuideNormal()
{
	this->guideNormal = true;
	return *this;
}

std::shared_ptr<Denoiser> DenoiserBuilder::build()
{
	this->context = this->createContext();
	this->stream = this->createStream();
	this->handle = this->createDenoiser();
	this->setupDenoiser();

	auto denoiser = std::make_shared<Denoiser>(
		this->context,
		this->stream,
		this->handle,
		this->width,
		this->height,
		this->denoiserBuffer,
		this->scratchBuffer,
		this->sizes
	);

	return denoiser;
}

OptixDeviceContext DenoiserBuilder::createContext() {
	cudaFree(0);
	CUcontext cuCtx = 0;

	if (optixInit() != OPTIX_SUCCESS)
		throw std::runtime_error("Failed to initialize OptiX!\n");

	OptixDeviceContextOptions options = {
		.logCallbackFunction = &optixLogCallback,
		.logCallbackLevel = 4
	};

	OptixDeviceContext context = nullptr;
	if (optixDeviceContextCreate(cuCtx, &options, &context) != OPTIX_SUCCESS)
		throw std::runtime_error("Failed to create OptiX Context!\n");
	return context;
}

CUstream DenoiserBuilder::createStream()
{
	CUstream stream;
	if (cuStreamCreate(&stream, CU_STREAM_DEFAULT) != CUDA_SUCCESS) {
		throw std::runtime_error("Failed to create stream!\n");
	}
	return stream;
}

OptixDenoiser DenoiserBuilder::createDenoiser() {
	auto kind = OPTIX_DENOISER_MODEL_KIND_LDR;
	
	OptixDenoiserOptions options{
		.guideAlbedo = this->guideAlbedo,
		.guideNormal = this->guideNormal,
		.denoiseAlpha = OptixDenoiserAlphaMode::OPTIX_DENOISER_ALPHA_MODE_DENOISE
	};
	OptixDenoiser denoiser = nullptr;
	
	if (optixDenoiserCreate(this->context, kind, &options, &denoiser) != OPTIX_SUCCESS)
		throw std::runtime_error("Failed to create OptiX Denoiser!");
	return denoiser;
}

void DenoiserBuilder::setupDenoiser() {

	optixDenoiserComputeMemoryResources(this->handle, (uint)this->width, (uint)this->height, &this->sizes);

	cudaMalloc(reinterpret_cast<void**>(&this->denoiserBuffer), this->sizes.stateSizeInBytes);
	cudaMalloc(reinterpret_cast<void**>(&this->scratchBuffer), this->sizes.withOverlapScratchSizeInBytes);

	cudaDeviceSynchronize();
	optixDenoiserSetup(this->handle, this->stream, this->width, this->height, this->denoiserBuffer, this->sizes.stateSizeInBytes, this->scratchBuffer, this->sizes.withOverlapScratchSizeInBytes);
}

Denoiser::Denoiser(OptixDeviceContext context, CUstream stream, OptixDenoiser handle, uint width, uint heigth, CUdeviceptr denoiserBuffer, CUdeviceptr scratchBuffer, OptixDenoiserSizes sizes) :
	context(context),
	stream(stream),
	handle(handle),
	width(width),
	height(heigth),
	denoiserBuffer(denoiserBuffer),
	scratchBuffer(scratchBuffer),
	sizes(sizes)
{}

void Denoiser::setSync(cudaExternalSemaphore_t semaphore, uint64_t waitSignal, uint64_t signalSignal)
{
	this->semaphore = semaphore;
	this->waitSignal = waitSignal;
	this->signalSignal = signalSignal;
}


std::vector<OptixUtilDenoiserImageTile> calcTiles(std::vector<std::pair<glm::ivec2, glm::ivec2>> extremes, int overlap, size_t WIDTH, size_t HEIGHT)
{
	std::vector<OptixUtilDenoiserImageTile> tiles;
	size_t rowStrideInBytes = WIDTH * sizeof_pixel;

	for (auto [p1, p2] : extremes) {
		glm::ivec2 outputBottomLeft = { min(p1.x, p2.x), min(p1.y, p2.y) };
		glm::ivec2 outputTopRight = { max(p1.x, p2.x), max(p1.y, p2.y) };

		glm::ivec2 inputBottomLeft = outputBottomLeft - glm::ivec2(overlap);
		inputBottomLeft.x = max(inputBottomLeft.x, 0);
		inputBottomLeft.y = max(inputBottomLeft.y, 0);

		glm::ivec2 inputTopRight = outputTopRight + glm::ivec2(overlap);
		inputTopRight.x = min(inputTopRight.x, WIDTH);
		inputTopRight.y = min(inputTopRight.y, HEIGHT);

		OptixUtilDenoiserImageTile tile;
		tile.input.data = (size_t)(inputBottomLeft.y * rowStrideInBytes) + (size_t)(inputBottomLeft.x * sizeof_pixel);
		tile.input.width = inputTopRight.x - inputBottomLeft.x;
		tile.input.height = inputTopRight.y - inputBottomLeft.y;
		tile.input.pixelStrideInBytes = sizeof_pixel;
		tile.input.rowStrideInBytes = rowStrideInBytes;
		tile.input.format = OptixPixelFormat::OPTIX_PIXEL_FORMAT_FLOAT3;

		tile.output.data = (size_t)(outputBottomLeft.y * rowStrideInBytes) + (size_t)(outputBottomLeft.x * sizeof_pixel);
		tile.output.width = outputTopRight.x - outputBottomLeft.x;
		tile.output.height = outputTopRight.y - outputBottomLeft.y;
		tile.output.pixelStrideInBytes = sizeof_pixel;
		tile.output.rowStrideInBytes = rowStrideInBytes;
		tile.output.format = OptixPixelFormat::OPTIX_PIXEL_FORMAT_FLOAT3;

		tile.inputOffsetX = outputBottomLeft.x - inputBottomLeft.x;
		tile.inputOffsetY = outputBottomLeft.y - inputBottomLeft.y;

		tiles.push_back(tile);
	}

	return tiles;
}

void Denoiser::run(float blendFactor, CUdeviceptr inputBuffer, CUdeviceptr albedoBuffer, CUdeviceptr normalBuffer, CUdeviceptr outputBuffer, std::vector<std::pair<glm::ivec2, glm::ivec2>> tileDescriptions)
{
	cudaExternalSemaphoreWaitParams waitParams{	};
	waitParams.params.fence.value = this->waitSignal;
	if (cudaWaitExternalSemaphoresAsync(&this->semaphore, &waitParams, 1, this->stream) != CUDA_SUCCESS)
		throw std::runtime_error("Failed to sync cuda-vulkan\n");

	try {
		OptixDenoiserParams params = {
			.blendFactor = blendFactor
		};

		auto tiles = calcTiles(tileDescriptions, 200, this->width, this->height);

		for (int i = 0; i < tiles.size(); i++) {
			OptixDenoiserGuideLayer guideLayer{
				.albedo = tiles[i].input,
				.normal = tiles[i].input
			};

			OptixDenoiserLayer layers{
				.input = tiles[i].input,
				.output = tiles[i].output
			};

			guideLayer.albedo.data += albedoBuffer;
			guideLayer.normal.data += normalBuffer;
			layers.input.data += inputBuffer;
			layers.output.data += outputBuffer;

			optixDenoiserInvoke(this->handle, this->stream, &params, this->denoiserBuffer, this->sizes.stateSizeInBytes, &guideLayer, &layers, 1, tiles[i].inputOffsetX, tiles[i].inputOffsetY, this->scratchBuffer, this->sizes.withOverlapScratchSizeInBytes);
		}
		
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}

	cudaExternalSemaphoreSignalParams signalParams{};
	signalParams.params.fence.value = this->signalSignal;
	if(cudaSignalExternalSemaphoresAsync(&this->semaphore, &signalParams, 1, this->stream) != CUDA_SUCCESS)
		throw std::runtime_error("Failed to sync cuda-vulkan\n");
}


void Denoiser::run(float blendFactor, CUdeviceptr inputBuffer, CUdeviceptr albedoBuffer, CUdeviceptr outputBuffer, std::vector<std::pair<glm::ivec2, glm::ivec2>> tileDescriptions)
{
	this->run(blendFactor, inputBuffer, albedoBuffer, NULL, outputBuffer, tileDescriptions);
}

void Denoiser::hardSynchronize()
{
	cudaStreamSynchronize(this->stream);
}

Denoiser::~Denoiser()
{	
	cudaFree((void*)this->scratchBuffer);
	cudaFree((void*)this->denoiserBuffer);
	optixDenoiserDestroy(this->handle);
	optixDeviceContextDestroy(this->context);
}
