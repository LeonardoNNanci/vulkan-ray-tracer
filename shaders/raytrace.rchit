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

vec3 albedo[] = {
    vec3(1.),
    vec3(1.),
    vec3(1.),
    vec3(1., .5, .5),
    vec3(1.),
    vec3(1.),
    vec3(0.9)
};

void main()
{
    ModelDescription desc = modelDescription.o[gl_InstanceCustomIndexEXT];
	int i1 = indexBuffer.i[desc.indexStride + 3 * gl_PrimitiveID];
	int i2 = indexBuffer.i[desc.indexStride + 3 * gl_PrimitiveID + 1];
	int i3 = indexBuffer.i[desc.indexStride + 3 * gl_PrimitiveID + 2];
	vec3 p1 = vertexBuffer.v[desc.vertexStride + i1].pos.xyz;
	vec3 p2 = vertexBuffer.v[desc.vertexStride + i2].pos.xyz;
	vec3 p3 = vertexBuffer.v[desc.vertexStride + i3].pos.xyz;
	vec3 objectNormal = normalize(cross((p3 - p2), (p1 - p2)));

    // pre-pass
    if(prd.fillGuideLayers){
        vec3 worldNormal = normalize(gl_ObjectToWorldEXT * vec4(objectNormal, 0.));
        vec3 cameraNormal = (view * vec4(worldNormal, 0.)).xyz;
            cameraNormal = normalize(cameraNormal.xyz);
            cameraNormal.g = -cameraNormal.g;

            uint linear = gl_LaunchIDEXT.y * gl_LaunchSizeEXT.x * 3 + gl_LaunchIDEXT.x * 3;

            prd.albedo = albedo[gl_InstanceID % 7];
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

    prd.hitValue = .8 * albedo[gl_InstanceID] * prd.hitValue;
}
