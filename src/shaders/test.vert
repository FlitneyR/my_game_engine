#version 450

layout(location = 0) in vec4 position;
layout(location = 1) in vec4 colour;
layout(location = 2) in mat4 modelTransform;

layout(location = 0) out vec4 v_colour;

layout(set = 0, binding = 0) uniform Camera {
    mat4 view;
    mat4 perspective;
} camera;

void main() {
    gl_Position = camera.perspective * camera.view * modelTransform * position;
    v_colour = colour;
}
