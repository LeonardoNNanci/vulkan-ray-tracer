#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_debug_printf : enable

#include "raycommon.glsl"

layout(location=0) rayPayloadInEXT hitPayload prd;
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
    return (fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453) - 0.5) * 2;
}

vec3 rand3() {
    float x = rand(gl_LaunchIDEXT.xy * gl_HitTEXT);
    float y = rand(gl_LaunchIDEXT.xy + vec2(x));
    float z = rand(gl_LaunchIDEXT.xy + vec2(y));
    return vec3(x, y, z);
}

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
    
    if(dot(objectNormal, gl_ObjectRayDirectionEXT) > 0){
        prd.done = true;
        prd.hitValue = vec3(1., 1., 0.);
        // debugPrintfEXT("%d\n", prd.depth);
    }
    else{
        vec3 dir = normalize(rand3());
        dir = objectNormal + (dir * 0.999);
        prd.done = false;
        prd.rayOrigin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT * 0.999;
        prd.rayDirection = gl_ObjectToWorldEXT * vec4(dir, 1.);
    }
}
