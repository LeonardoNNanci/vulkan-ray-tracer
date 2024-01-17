struct hitPayload
{
	bool done;
	vec3 rayDirection;
	vec3 rayOrigin;
	vec3 hitValue;
	uint depth;
};

struct Vertex{
	vec4 pos;
};

struct ModelDescription {
	uint vertexStride;
	uint indexStride;
};