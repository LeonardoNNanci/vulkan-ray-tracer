#version 460

#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_debug_printf : enable

#include "raycommon.glsl"

layout(location = 0) rayPayloadInEXT hitPayload prd;

void main()
{

    vec3 unit_direction = gl_WorldRayDirectionEXT;
    float a = 0.5*(unit_direction.y + 1.0);
    prd.hitValue = vec3(0.);
}