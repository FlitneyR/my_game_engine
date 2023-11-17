#version 450

/*
glm::vec3 m_position;
glm::vec3 m_normal;
glm::vec2 m_texcoord;
*/

layout(location = 0) in vec4 position;
layout(location = 1) in vec4 colour;
layout(location = 2) in mat4 modelTransform;

layout(location = 0) out vec4 v_colour;

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
    v_colour = colour;
}
