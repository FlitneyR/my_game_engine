#version 450

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec3 bitangent;
layout(location = 4) in vec2 texcoord;
layout(location = 5) in mat4 modelTransform;

layout(location = 1) out vec2 v_texcoord;

layout(set = 0, binding = 0) uniform Camera {
    mat4 view;
    mat4 perspective;
    vec2 jitter;
    
    mat4 previousView;
    mat4 previousPerspective;
    vec2 previousJitter;
} camera;

void main() {
    gl_Position = camera.perspective * camera.view * modelTransform * position;
    v_texcoord = texcoord;
}
