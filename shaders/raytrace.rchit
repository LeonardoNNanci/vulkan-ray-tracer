#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_debug_printf : enable

#include "raycommon.glsl"

layout(location=0) rayPayloadInEXT hitPayload prd;
layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(set=1, binding=0) readonly buffer VertexBuffer { Vertex v[]; } vertexBuffer;
layout(set=1, binding=1) readonly buffer IndexBuffer { int i[]; } indexBuffer;
layout(set=1, binding=2) readonly buffer ModelDescription_ { ModelDescription o[]; } modelDescription;
hitAttributeEXT vec3 attribs;

layout(push_constant) uniform constants {
    mat4 proj;
    mat4 projInv;
    mat4 view;
    mat4 viewInv;
};

void main()
{
    ModelDescription desc = modelDescription.o[gl_InstanceCustomIndexEXT];
	int i1 = indexBuffer.i[desc.indexStride + 3 * gl_PrimitiveID];
	int i2 = indexBuffer.i[desc.indexStride + 3 * gl_PrimitiveID + 1];
	int i3 = indexBuffer.i[desc.indexStride + 3 * gl_PrimitiveID + 2];
	Vertex v1 = vertexBuffer.v[desc.vertexStride + i1];
	Vertex v2 = vertexBuffer.v[desc.vertexStride + i2];
	Vertex v3 = vertexBuffer.v[desc.vertexStride + i3];
	vec3 objectNormal = normalize((v1.normal + v2.normal + v3.normal) / 3);

    // pre-pass
    if(prd.fillGuideLayers){
        vec3 worldNormal = normalize(mat3(gl_ObjectToWorldEXT) * objectNormal);
        vec3 cameraNormal = normalize(mat3(view) * worldNormal);
            cameraNormal.g = -cameraNormal.g;

            uint linear = gl_LaunchIDEXT.y * gl_LaunchSizeEXT.x * 3 + gl_LaunchIDEXT.x * 3;

            prd.albedo = vec3(9.);
            prd.normal = cameraNormal;
            return;
    }

    // ray trace
    if(prd.depth >= 31){
        prd.hitValue = vec3(0.);
        return;
    }

    vec2 seed = vec2(gl_HitTEXT, gl_HitTEXT * gl_HitTEXT);
    vec3 dir = normalize(rand3(seed));
    dir = objectNormal + (dir * 0.999);
    vec3 origin = (gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT).xyz;
    vec3 direction = (gl_ObjectToWorldEXT * vec4(dir, 0.)).xyz;

    prd.depth++;
    traceRayEXT(topLevelAS,         // acceleration structure
        gl_RayFlagsNoneEXT,  // rayFlags
        0xFF,               // cullMask
        0,                  // sbtRecordOffset
        0,                  // sbtRecordStride
        0,                  // missIndex
        origin,             // ray origin
        0.000001,                // ray min range
        direction,             // ray direction
        100000.0,           // ray max range
        0                   // payload (location = 0)
    );
    prd.depth--;

    prd.hitValue = .8 * vec3(1.) * prd.hitValue;
}
