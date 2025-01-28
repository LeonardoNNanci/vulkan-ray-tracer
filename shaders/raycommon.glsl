struct hitPayload
{
	vec3 hitValue;
	uint depth;
	vec3 albedo;
	vec3 normal;
	vec3 hitPoint;
	bool fillGuideLayers;
	uint seed;
};

struct DenoiserInfo{
	vec3 albedo;
	vec3 normal;
};

struct Vertex{
	vec4 position;
	vec3 normal;
	vec4 tangent;
	vec2 textureCoordinates;
};

struct ModelDescription {
	uint vertexStride;
	uint indexStride;
	uint materialIndex;
};

struct Range {
	float ratio;
	float innerRadius;
	float outerRadius;
};

float rand(vec2 co){
    return (fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453) - 0.5) * 2;
}

vec3 rand3(vec2 seed) {
    float x = rand(seed);
    float y = rand(seed * x);
    float z = rand(seed * y);
    return vec3(x, y, z);
}

struct Texture{
	uint imageIndex;
	uint samplerIndex;
};

struct Light{
	vec4 position;
	vec3 direction;
	vec3 color;
	float intensity;
	uint type;
};

struct Material {
	float metalicFactor;
	float roughnessFactor;
	vec4 baseColor;

	uint colorTexture;
	uint metalicRoughnessTexture;
	uint normalTexture;
	uint occlusionTexture;
	uint emissiveTexture;

	bool doubleSided;
};
