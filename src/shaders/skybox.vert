#version 450

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord;
layout(location = 3) in mat4 modelTransform;

layout(location = 1) out vec2 v_texcoord;

layout(set = 0, binding = 0) uniform Camera {
    mat4 view;
    mat4 perspective;
} camera;

void main() {
    gl_Position = camera.perspective * camera.view * modelTransform * position;
    v_texcoord = texcoord;
}
