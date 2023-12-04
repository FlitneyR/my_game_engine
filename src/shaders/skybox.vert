#version 450

#include "camera.glsl"

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec3 bitangent;
layout(location = 4) in vec2 texcoord;
layout(location = 5) in mat4 modelTransform;

layout(location = 0) out vec2 v_texcoord;

layout(set = 0, binding = 0) uniform Uniform_0_0 { Camera camera; };

void main() {
    gl_Position = camera.perspective * camera.view * modelTransform * position;
    gl_Position.xy += camera.jitter * gl_Position.w;

    v_texcoord = texcoord;
}
