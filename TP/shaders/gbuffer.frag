#version 450

#include "utils.glsl"

layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_color;
layout(location = 3) in vec3 in_position;
layout(location = 4) in vec3 in_tangent;
layout(location = 5) in vec3 in_bitangent;

layout(location = 0) out vec4 g_color;
layout(location = 1) out vec4 g_normal;

layout(binding = 0) uniform sampler2D u_texture;
layout(binding = 1) uniform sampler2D u_normalMap;

void main() {
#ifdef NORMAL_MAPPED
    const vec3 normalMap = unpack_normal_map(texture(u_normalMap, in_uv).rg);
    const vec3 normal = normalMap.x * in_tangent + normalMap.y * in_bitangent + normalMap.z * in_normal;
#else
    const vec3 normal = in_normal;
#endif

    // Store normal in gbuffer (rgba format)
    g_normal = vec4((normalize(normal) + 1.0) / 2.0, 1.0);
    // Store color in gbuffer (rgba format)
    g_color = vec4(in_color, 1.0);

#ifdef TEXTURED
    g_color *= texture(u_texture, in_uv);
#endif
}