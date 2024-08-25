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
    prd.hitValue = vec3(1.);
}