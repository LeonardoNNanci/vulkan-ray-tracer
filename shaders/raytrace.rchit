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
        vec3 dir = normalize(rand3(gl_HitTEXT));
        dir = objectNormal + (dir * 0.999);
        prd.done = false;
        prd.rayOrigin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
        prd.rayDirection = gl_ObjectToWorldEXT * vec4(dir, 0.);
        
        // denoiser: albedo & normal
        if(prd.fillGuideLayers) {
            vec3 cameraNormal = (view * vec4(worldNormal, 0.)).xyz;
            cameraNormal = normalize(cameraNormal.xyz);
            cameraNormal.g = -cameraNormal.g;

            uint linear = gl_LaunchIDEXT.y * gl_LaunchSizeEXT.x * 3 + gl_LaunchIDEXT.x * 3;

            prd.albedo = vec3(1.);
            prd.normal = cameraNormal;
        }
    }
}
