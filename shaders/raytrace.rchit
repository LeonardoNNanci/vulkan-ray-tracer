#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_debug_printf : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "raycommon.glsl"

layout(location=0) rayPayloadInEXT hitPayload prd;
layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(set=1, binding=0) readonly buffer VertexBuffer { Vertex v[]; } vertexBuffer;
layout(set=1, binding=1) readonly buffer IndexBuffer { int i[]; } indexBuffer;
layout(set=1, binding=2) readonly buffer ModelDescriptionBuffer { ModelDescription o[]; } modelDescription;
layout(set=1, binding=3) readonly buffer MaterialBuffer { Material m[]; } materialBuffer;
layout(set=1, binding=4) uniform sampler2D textures[];
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

	int i0 = indexBuffer.i[desc.indexStride + 3 * gl_PrimitiveID];
	int i1 = indexBuffer.i[desc.indexStride + 3 * gl_PrimitiveID + 1];
	int i2 = indexBuffer.i[desc.indexStride + 3 * gl_PrimitiveID + 2];

	Vertex v0 = vertexBuffer.v[desc.vertexStride + i0];
	Vertex v1 = vertexBuffer.v[desc.vertexStride + i1];
	Vertex v2 = vertexBuffer.v[desc.vertexStride + i2];

    Material material = materialBuffer.m[desc.materialIndex];

    const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

    vec4 albedo = material.baseColor;
    if(material.colorTexture >= 0){
        const vec2 textureCoordinates = v0.textureCoordinates * barycentrics.x + v1.textureCoordinates * barycentrics.y + v2.textureCoordinates * barycentrics.z;
        vec4 textureColor = texture(textures[nonuniformEXT(material.colorTexture)], textureCoordinates);
        albedo *= textureColor;
    }
	vec3 objectNormal = normalize(v0.normal * barycentrics.x + v1.normal * barycentrics.y + v2.normal * barycentrics.z);
    
    // pre-pass
    if(prd.fillGuideLayers){
        vec3 worldNormal = normalize(mat3(gl_ObjectToWorldEXT) * objectNormal);

        vec3 cameraNormal = normalize(mat3(view) * worldNormal);
        cameraNormal.g = -cameraNormal.g;

        uint linear = gl_LaunchIDEXT.y * gl_LaunchSizeEXT.x * 3 + gl_LaunchIDEXT.x * 3;

        prd.albedo = albedo.rgb;
        prd.normal = cameraNormal;// * -dot(worldNormal, gl_WorldRayDirectionEXT);//vec3(.5) + cameraNormal / 2;
        return;
    }

    // ray trace
    if(prd.depth >= 2){
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

    prd.hitValue = albedo.rgb * prd.hitValue; // + emission
}
