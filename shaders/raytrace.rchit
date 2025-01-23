#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_debug_printf : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "raycommon.glsl"
#include "random.glsl"
#include "sampling/cosine_hemisphere.glsl"

layout(location=0) rayPayloadInEXT hitPayload prd;
layout(location=1) rayPayloadEXT bool isLit;

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(set=1, binding=0) readonly buffer VertexBuffer { Vertex v[]; } vertexBuffer;
layout(set=1, binding=1) readonly buffer IndexBuffer { int i[]; } indexBuffer;
layout(set=1, binding=2) readonly buffer ModelDescriptionBuffer { ModelDescription o[]; } modelDescription;
layout(set=1, binding=3) readonly buffer MaterialBuffer { Material m[]; } materialBuffer;
layout(set=1, binding=4) readonly buffer LightBuffer {Light l[]; } lightBuffer;
layout(set=1, binding=5) uniform sampler2D textures[];
hitAttributeEXT vec3 attribs;

layout(push_constant) uniform constants {
    mat4 proj;
    mat4 projInv;
    mat4 view;
    mat4 viewInv;
};

void main()
{
    // russian roulette
    float rr_prob = 1.;
    // if(prd.depth > 1){
    //     rr_prob = 0.7;
    //     if(rand(prd.seed) >= rr_prob)
    //         return;
    // }

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
    const vec2 textureCoordinates = v0.textureCoordinates * barycentrics.x + v1.textureCoordinates * barycentrics.y + v2.textureCoordinates * barycentrics.z;
    if(material.colorTexture < iNULL){
        vec4 textureColor = texture(textures[nonuniformEXT(material.colorTexture)], textureCoordinates);
        albedo *= textureColor;
    }

	vec3 localNormal = normalize(v0.normal * barycentrics.x + v1.normal * barycentrics.y + v2.normal * barycentrics.z);
    vec4 localTangent = v0.tangent * barycentrics.x + v1.tangent * barycentrics.y + v2.tangent * barycentrics.z;
    localTangent.xyz = normalize(localTangent.xyz);
    vec3 localBitangent = cross(localNormal, localTangent.xyz) * localTangent.w;
    mat3 TBN = mat3(gl_ObjectToWorldEXT) * mat3(localTangent.xyz, localBitangent, localNormal);
    
    vec3 texNormal = texture(textures[nonuniformEXT(material.normalTexture)], textureCoordinates).rgb;
    
    vec3 worldNormal = normalize(TBN * texNormal);

    // pre-pass
    if(prd.fillGuideLayers){
        vec3 cameraNormal = normalize(mat3(view) * worldNormal);
        cameraNormal.g *= -1;

        uint linear = gl_LaunchIDEXT.y * gl_LaunchSizeEXT.x * 3 + gl_LaunchIDEXT.x * 3;

        prd.albedo = albedo.rgb;
        prd.normal = cameraNormal;
        return;
    }

    if(prd.depth>=1) return;

    // Direct light ------------------------
    vec3 origin = (gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT).xyz;
    Light light = lightBuffer.l[0];

    isLit = false;
    prd.hitValue = vec3(0.);

    // may be lit
    if(dot(worldNormal, normalize(light.direction.xyz)) > 0.){
        traceRayEXT(
            topLevelAS,         // acceleration structure
            gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsSkipClosestHitShaderEXT,  // rayFlags
            0xFF,               // cullMask
            1,                  // sbtRecordOffset
            0,                  // sbtRecordStride
            1,                  // missIndex
            origin,             // ray origin
            0.000001,                // ray min range
            light.direction.xyz,             // ray direction
            100000.0,           // ray max range
            1                   // payload (location = 0)
        );
        if(isLit)
            prd.hitValue = light.color * 2;
        else if (prd.depth > 1)
            return;
    }

    float directCoef = dot(light.direction.xyz, worldNormal);
    vec3 directLight = prd.hitValue * directCoef;

    // Indirect light ----------------------------------

    // probably works due to isotropy
    vec3 a = vec3(1., 0., 0.);
    if(abs(dot(a, worldNormal)) < 0.1)
        a = vec3(0., 1., 0.);
    vec3 T = normalize(cross(worldNormal, a));
    vec3 S = cross(worldNormal, T);
    mat3 shadingToWorld = mat3(gl_ObjectToWorldEXT) * mat3(S, T, worldNormal);

    int MCSamples = 2;
    vec3 indirectLight = vec3(0.);
    for(int i = 0; i < MCSamples; i++){

        vec2 xi = vec2(rnd(prd.seed), rnd(prd.seed));
        vec3 s = sampleCosineHemisphere(xi);
        vec3 direction = normalize(shadingToWorld * s);

        prd.depth++;
        traceRayEXT(
            topLevelAS,         // acceleration structure
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

        float prob = cosineHemispherePDF(dot(s, worldNormal));
        float indirectCoef = dot(direction, worldNormal) / prob;
        indirectLight += indirectCoef * prd.hitValue;
    }
    indirectLight = indirectLight / MCSamples;

    vec3 brdf = albedo.rgb / PI;
    prd.hitValue = brdf * (directLight + indirectLight) / 2;
}
