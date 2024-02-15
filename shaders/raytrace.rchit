#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_debug_printf : enable

#include "raycommon.glsl"

layout(location=0) rayPayloadInEXT hitPayload prd;
layout(set=1, binding=0) readonly buffer VertexBuffer { Vertex v[]; } vertexBuffer;
layout(set=1, binding=1) readonly buffer IndexBuffer { int i[]; } indexBuffer;
layout(set=1, binding=2) readonly buffer ModelDescription_ { ModelDescription o[]; } modelDescription;
layout(set=0, binding=2, rgba32f) uniform image2D albedoImage;
layout(set=0, binding=3, rgba32f) uniform image2D normalImage;
hitAttributeEXT vec3 attribs;

layout(push_constant) uniform constants {
    mat4 proj;
    mat4 projInv;
    mat4 view;
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
    
    vec3 worldNormal = normalize(gl_ObjectToWorldEXT * vec4(objectNormal, 0.));

    // backface hit
    if(dot(objectNormal, gl_ObjectRayDirectionEXT) > 0){
        prd.done = true;
        prd.hitValue = vec3(0., 0., 0.);
    }
    // hit
    else{
        vec3 dir = normalize(rand3());
        dir = objectNormal + (dir * 0.999);
        prd.done = false;
        prd.rayOrigin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT; // * 0.999;
        prd.rayDirection = gl_ObjectToWorldEXT * vec4(dir, 1.);
        
        // denoiser: albedo & normal
        if(prd.depth == 0) {
            vec3 cameraNormal = (view * vec4(worldNormal, 0.)).xyz;
            cameraNormal = normalize(cameraNormal.xyz);
            cameraNormal.g = -cameraNormal.g;
            cameraNormal = cameraNormal / 2. + vec3(0.5);
            cameraNormal = normalize(cameraNormal);
            imageStore(albedoImage, ivec2(gl_LaunchIDEXT.xy), vec4(1.));
            imageStore(normalImage, ivec2(gl_LaunchIDEXT.xy), vec4(cameraNormal.rgb, 1.));

        }
    }
}
