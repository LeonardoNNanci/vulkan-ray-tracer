#version 460

#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_debug_printf : enable

#include "raycommon.glsl"

layout(location = 0) rayPayloadInEXT hitPayload prd;

void main()
{   
    prd.done = true;
    prd.hitValue = vec3(0., 0., 0.);
    // debugPrintfEXT("%d\t|\t", prd.depth);
}