#pragma once
#include "denoiser.hpp"
#include <iostream>
#include<algorithm>
#include <stdexcept>
#include <optix.h>
#include <optix_function_table_definition.h>
#include <optix_denoiser_tiling.h>
#include <optix_stubs.h>
#include <cuda_runtime_api.h>

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

	cudaMalloc(reinterpret_cast<void**>(&this->hdrIntensity), sizeof(float));

	auto denoiser = std::make_shared<Denoiser>(
		this->context,
		this->stream,
		this->handle,
		this->width,
		this->height,
		this->denoiserBuffer,
		this->scratchBuffer,
		this->sizes,
		this->hdrIntensity
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
	auto kind = OPTIX_DENOISER_MODEL_KIND_TEMPORAL;
	
	OptixDenoiserOptions options{
		.guideAlbedo = this->guideAlbedo,
		.guideNormal = this->guideNormal,
		.denoiseAlpha = OptixDenoiserAlphaMode::OPTIX_DENOISER_ALPHA_MODE_COPY
	};
	OptixDenoiser denoiser = nullptr;
	
	if (optixDenoiserCreate(this->context, kind, &options, &denoiser) != OPTIX_SUCCESS)
		throw std::runtime_error("Failed to create OptiX Denoiser!");
	return denoiser;
}

void DenoiserBuilder::setupDenoiser() {

	optixDenoiserComputeMemoryResources(this->handle, (uint)this->width, (uint)this->height, &this->sizes);

	auto scratchSize = max(this->sizes.computeIntensitySizeInBytes,
		max(
			this->sizes.withoutOverlapScratchSizeInBytes, this->sizes.withOverlapScratchSizeInBytes
		)
	);
	cudaMalloc(reinterpret_cast<void**>(&this->scratchBuffer), scratchSize);
	cudaMalloc(reinterpret_cast<void**>(&this->denoiserBuffer), this->sizes.stateSizeInBytes);

	cudaDeviceSynchronize();
	optixDenoiserSetup(this->handle, this->stream, this->width, this->height, this->denoiserBuffer, this->sizes.stateSizeInBytes, this->scratchBuffer, this->sizes.withOverlapScratchSizeInBytes);
}

Denoiser::Denoiser(OptixDeviceContext context, CUstream stream, OptixDenoiser handle, uint width, uint heigth, CUdeviceptr denoiserBuffer, CUdeviceptr scratchBuffer, OptixDenoiserSizes sizes, CUdeviceptr hdrIntensity) :
	context(context),
	stream(stream),
	handle(handle),
	width(width),
	height(heigth),
	denoiserBuffer(denoiserBuffer),
	scratchBuffer(scratchBuffer),
	sizes(sizes),
	hdrIntensity(hdrIntensity)
{
}

void Denoiser::setSync(cudaExternalSemaphore_t& semaphore, uint64_t waitSignal, uint64_t signalSignal)
{
	this->semaphore = semaphore;
	this->waitSignal = waitSignal;
	this->signalSignal = signalSignal;
}


std::vector<OptixUtilDenoiserImageTile> calcTiles(std::vector<std::pair<glm::ivec2, glm::ivec2>> extremes, int overlap, size_t WIDTH, size_t HEIGHT, OptixPixelFormat pixelFormat)
{
	uint32_t sizeof_pixel;
	switch (pixelFormat) {
	case OptixPixelFormat::OPTIX_PIXEL_FORMAT_FLOAT2:
		sizeof_pixel = static_cast<uint32_t>(2 * sizeof(float));
		break;
	case OptixPixelFormat::OPTIX_PIXEL_FORMAT_FLOAT3:
		sizeof_pixel = static_cast<uint32_t>(3 * sizeof(float));
		break;
	default:
		throw std::runtime_error("Unsupported pixel format for OptiX Denoiser");
	};

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
		tile.input.format = pixelFormat;

		tile.output.data = (size_t)(outputBottomLeft.y * rowStrideInBytes) + (size_t)(outputBottomLeft.x * sizeof_pixel);
		tile.output.width = outputTopRight.x - outputBottomLeft.x;
		tile.output.height = outputTopRight.y - outputBottomLeft.y;
		tile.output.pixelStrideInBytes = sizeof_pixel;
		tile.output.rowStrideInBytes = rowStrideInBytes;
		tile.output.format = pixelFormat;

		tile.inputOffsetX = outputBottomLeft.x - inputBottomLeft.x;
		tile.inputOffsetY = outputBottomLeft.y - inputBottomLeft.y;

		tiles.push_back(tile);
	}

	return tiles;
}

void Denoiser::run(float blendFactor, CUdeviceptr inputBuffer, CUdeviceptr albedoBuffer, CUdeviceptr normalBuffer, CUdeviceptr opticalFlowBuffer, CUdeviceptr outputBuffer, std::vector<std::pair<glm::ivec2, glm::ivec2>> tileDescriptions)
{
	cudaExternalSemaphoreWaitParams waitParams{	};
	waitParams.params.fence.value = this->waitSignal;
	if (cudaWaitExternalSemaphoresAsync(&this->semaphore, &waitParams, 1, this->stream) != CUDA_SUCCESS)
		throw std::runtime_error("Failed to sync cuda-vulkan\n");

	auto img = calcTiles({ {{0,0}, {this->width, this->height}} }, 0, this->width, this->height, OptixPixelFormat::OPTIX_PIXEL_FORMAT_FLOAT3)[0];
	img.input.data += inputBuffer;
	auto optResult = optixDenoiserComputeIntensity(this->handle, this->stream, &img.input, this->hdrIntensity, this->scratchBuffer, this->sizes.computeIntensitySizeInBytes);
	if (optResult != OPTIX_SUCCESS)
		throw std::runtime_error("Failed to compute HDR Intensity\n");

	OptixDenoiserParams params = {
		.hdrIntensity = hdrIntensity,
		.blendFactor = blendFactor,
	};

	auto tiles = calcTiles(tileDescriptions, 0, this->width, this->height, OptixPixelFormat::OPTIX_PIXEL_FORMAT_FLOAT3);
	auto flows = calcTiles(tileDescriptions, 0, this->width, this->height, OptixPixelFormat::OPTIX_PIXEL_FORMAT_FLOAT2);

	for (int i = 0; i < tiles.size(); i++) {
		OptixDenoiserGuideLayer guideLayer{
			.albedo = tiles[i].input,
			.normal = tiles[i].input,
			.flow = flows[i].input
		};

		OptixDenoiserLayer layers{
			.input = tiles[i].input,
			.previousOutput = tiles[i].input,
			.output = tiles[i].output,
		};

		guideLayer.albedo.data += albedoBuffer;
		guideLayer.normal.data += normalBuffer;
		guideLayer.flow.data += opticalFlowBuffer;
		layers.input.data += inputBuffer;
		layers.output.data += outputBuffer;
		layers.previousOutput.data += this->previousOutput ? this->previousOutput : inputBuffer;

		auto optResult = optixDenoiserInvoke(this->handle, this->stream, &params, this->denoiserBuffer, this->sizes.stateSizeInBytes, &guideLayer, &layers, 1, tiles[i].inputOffsetX, tiles[i].inputOffsetY, this->scratchBuffer, this->sizes.withOverlapScratchSizeInBytes);
		if (optResult != OPTIX_SUCCESS)
			throw std::runtime_error("Failed to invoke denoiser\n");
	}


	cudaExternalSemaphoreSignalParams signalParams{};
	signalParams.params.fence.value = this->signalSignal;
	auto cudaRes = cudaSignalExternalSemaphoresAsync(&this->semaphore, &signalParams, 1, this->stream);
	if (cudaRes != CUDA_SUCCESS)
		throw std::runtime_error("Failed to sync cuda-vulkan\n");

	this->previousOutput = outputBuffer;
}


void Denoiser::run(float blendFactor, CUdeviceptr inputBuffer, CUdeviceptr albedoBuffer, CUdeviceptr opticalFlowBuffer, CUdeviceptr outputBuffer, std::vector<std::pair<glm::ivec2, glm::ivec2>> tileDescriptions)
{
	this->run(blendFactor, inputBuffer, albedoBuffer, NULL, opticalFlowBuffer, outputBuffer, tileDescriptions);
}

void Denoiser::hardSynchronize()
{
	cudaStreamSynchronize(this->stream);
}

Denoiser::~Denoiser()
{	
	cudaFree((void*)this->hdrIntensity);
	cudaFree((void*)this->scratchBuffer);
	cudaFree((void*)this->denoiserBuffer);
	optixDenoiserDestroy(this->handle);
	optixDeviceContextDestroy(this->context);
}
