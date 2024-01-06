#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_debug_printf : enable

struct Vertex{
	vec4 pos;
};

layout(binding=0, set=0) uniform accelerationStructureEXT topLevelAS;
layout(set=1, binding=0) readonly buffer VertexBuffer{ Vertex v[]; } vertexBuffer;
layout(set=1, binding=1) readonly buffer IndexBuffer { int i[]; } indexBuffer;

hitAttributeEXT vec3 attribs;
layout(location=0) rayPayloadInEXT vec3 hitValue;
layout(location=1) rayPayloadEXT bool isShadowed;

layout(push_constant) uniform constants {
    vec4 clearColor;
    vec4 lightPosition;
    float lightIntensity;
    int lightType;
    mat4 projInv;
    mat4 viewInv;
};

float rand(vec2 co){
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

void main()
{
	vec3 lightPos = lightPosition.xyz;
	vec3 hitPoint = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
	vec3 hit2Light = lightPos - hitPoint;

	int i1 = indexBuffer.i[3 * gl_PrimitiveID];
	int i2 = indexBuffer.i[3 * gl_PrimitiveID + 1];
	int i3 = indexBuffer.i[3 * gl_PrimitiveID + 2];
	vec3 p1 = vertexBuffer.v[i1].pos.xyz;
	vec3 p2 = vertexBuffer.v[i2].pos.xyz;
	vec3 p3 = vertexBuffer.v[i3].pos.xyz;
	vec3 surfaceNormal = normalize(cross((p2-p1), (p3-p1)));

	float intensity = dot(surfaceNormal, normalize(hit2Light));
    float attenuation = 1;
    isShadowed = true;

	if(intensity > 0) {
	    float tMin   = 0.001;
        float tMax   = length(hit2Light);
        vec3  origin = hitPoint;
        vec3  rayDir = hit2Light;
        uint  flags =
            gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;
        isShadowed = true;
        traceRayEXT(topLevelAS,  // acceleration structure
                flags,       // rayFlags
                0xFF,        // cullMask
                0,           // sbtRecordOffset
                0,           // sbtRecordStride
                1,           // missIndex
                origin,      // ray origin
                tMin,        // ray min range
                rayDir,      // ray direction
                tMax,        // ray max range
                1            // payload (location = 1)
        );

        if(isShadowed)
        {
          attenuation = 0.3;
        }
	}


	hitValue = vec3(intensity * attenuation);
}
