#version 450

#include "utils.glsl"

// fragment shader of the main lighting pass

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec2 in_uv;

layout(binding = 0) uniform sampler2D in_color_texture;
layout(binding = 1) uniform sampler2D in_normal_texture;
layout(binding = 2) uniform sampler2D in_depth_texture;
layout(binding = 3) uniform sampler2D in_shadow_texture;

layout(binding = 0) uniform Data {
    FrameData frame;
};

layout(binding = 1) buffer PointLights {
    PointLight point_lights[];
};

const vec3 ambient = vec3(0.0);

void main() {
    vec3 in_color = texelFetch(in_color_texture, ivec2(gl_FragCoord.xy), 0).rgb;
    vec3 in_normal = texelFetch(in_normal_texture, ivec2(gl_FragCoord.xy), 0).xyz;
    in_normal = normalize(in_normal * 2.0 - 1.0);
    float in_depth = texelFetch(in_depth_texture, ivec2(gl_FragCoord.xy), 0).r;

    vec3 in_position = unproject(in_uv, in_depth, inverse(frame.camera.view_proj));

    vec3 acc = ambient;

    vec4 sun_pos = frame.sun_view_proj * vec4(in_position, 1.0);
    vec2 shadow_uv = (sun_pos.xy + 1.0) / 2.0;
    float shadow_depth = texture(in_shadow_texture, shadow_uv).r;
    if(shadow_depth <= sun_pos.z) {
        acc += frame.sun_color * max(0.0, dot(frame.sun_dir, in_normal));
    }

    for(uint i = 0; i != frame.point_light_count; ++i) {
        PointLight light = point_lights[i];
        const vec3 to_light = (light.position - in_position);
        const float dist = length(to_light);
        const vec3 light_vec = to_light / dist;

        const float NoL = dot(light_vec, in_normal);
        const float att = attenuation(dist, light.radius);
        if(NoL <= 0.0 || att <= 0.0f) {
            continue;
        }

        acc += light.color * (NoL * att);
    }

    out_color = vec4(in_color * acc, 1.0);

#ifdef DEBUG_COLOR
    out_color = vec4(in_color, 1.0);
#elif DEBUG_NORMAL
    out_color = vec4(in_normal * 0.5 + 0.5, 1.0);
#elif DEBUG_LIGHT
    out_color = vec4(acc, 1.0);
#elif DEBUG_DEPTH
    out_color = vec4(vec3(in_depth * 1e5), 1.0);
#endif
}

