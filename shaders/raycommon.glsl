struct hitPayload
{
	bool done;
	vec3 rayDirection;
	vec3 rayOrigin;
	vec3 hitValue;
	uint depth;
	vec3 albedo;
	vec3 normal;
	bool fillGuideLayers;
};

struct DenoiserInfo{
	vec3 albedo;
	vec3 normal;
};

struct Vertex{
	vec4 pos;
};

struct ModelDescription {
	uint vertexStride;
	uint indexStride;
};

struct Range {
	float ratio;
	float innerRadius;
	float outerRadius;
};


float rand(vec2 co){
    return (fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453) - 0.5) * 2;
}

vec3 rand3(float seed) {
    float x = rand(vec2(seed));
    float y = rand(vec2(x));
    float z = rand(vec2(y));
    return vec3(x, y, z);
}
