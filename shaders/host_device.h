#ifndef COMMON_HOST_DEVICE
#define COMMON_HOST_DEVICE
struct Vertex{
	vec4 position;
};

struct ModelDescription {
	uint vertexStride;
	uint indexStride;
}
#endif