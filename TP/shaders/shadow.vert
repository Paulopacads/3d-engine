#version 450

#include "utils.glsl"

layout(location = 0) in vec3 in_position;

layout(binding = 0) uniform Data {
    FrameData frame;
};

layout(binding = 2) buffer Models {
    Model models[];
};

void main() {
    gl_Position = frame.sun_view_proj * (models[gl_InstanceID].transform * vec4(in_position, 1.0));
}