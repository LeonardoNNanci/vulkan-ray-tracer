#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_debug_printf : enable

#include "raycommon.glsl"

layout(location=0) rayPayloadInEXT vec3 hitValue;
layout(binding=0, set=0) uniform accelerationStructureEXT topLevelAS;
layout(set=1, binding=0) readonly buffer VertexBuffer { Vertex v[]; } vertexBuffer;
layout(set=1, binding=1) readonly buffer IndexBuffer { int i[]; } indexBuffer;
layout(set=1, binding=2) readonly buffer ModelDescription_ { ModelDescription o[]; } modelDescription;

hitAttributeEXT vec3 attribs;

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

    ModelDescription desc = modelDescription.o[gl_InstanceCustomIndexEXT];
 
	int i1 = indexBuffer.i[desc.indexStride + 3 * gl_PrimitiveID];
	int i2 = indexBuffer.i[desc.indexStride + 3 * gl_PrimitiveID + 1];
	int i3 = indexBuffer.i[desc.indexStride + 3 * gl_PrimitiveID + 2];
	vec3 p1 = gl_WorldToObjectEXT * vertexBuffer.v[desc.vertexStride + i1].pos;
	vec3 p2 = gl_WorldToObjectEXT * vertexBuffer.v[desc.vertexStride + i2].pos;
	vec3 p3 = gl_WorldToObjectEXT * vertexBuffer.v[desc.vertexStride + i3].pos;
	vec3 surfaceNormal = normalize(cross((p3-p2), (p1-p2)));
    if(gl_InstanceID == 2){
        debugPrintfEXT("%d %d %d %v3f %v3f %v3f\n", i1, i2, i3, p1, p2, p3);
    }

	float intensity = dot(surfaceNormal, normalize(hit2Light));
    // float attenuation = 1;
    // isShadowed = true;

	// if(intensity > 0) {
	//     float tMin   = 0.001;
    //     float tMax   = length(hit2Light);
    //     vec3  origin = hitPoint;
    //     vec3  rayDir = hit2Light;
    //     uint  flags =
    //         gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;
    //     isShadowed = true;
    //     traceRayEXT(topLevelAS,  // acceleration structure
    //             flags,       // rayFlags
    //             0xFF,        // cullMask
    //             0,           // sbtRecordOffset
    //             0,           // sbtRecordStride
    //             1,           // missIndex
    //             origin,      // ray origin
    //             tMin,        // ray min range
    //             rayDir,      // ray direction
    //             tMax,        // ray max range
    //             1            // payload (location = 1)
    //     );

    //     if(isShadowed)
    //     {
    //       attenuation = 0.3;
    //     }
	// }


	hitValue = vec3(intensity);
}
