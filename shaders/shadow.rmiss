#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_debug_printf : enable

layout(location = 1) rayPayloadInEXT bool isShadowed;

void main()
{
  debugPrintfEXT("Shadow shader\n");
  isShadowed = false;
}
