#version 450

/*
glm::vec3 m_position;
glm::vec3 m_normal;
glm::vec2 m_texcoord;
*/

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord;
layout(location = 3) in mat4 modelTransform;

layout(location = 0) out vec3 v_normal;
layout(location = 1) out vec2 v_texcoord;
layout(location = 2) out vec3 viewdir;

layout(set = 0, binding = 0) uniform Camera {
    mat4 view;
    mat4 perspective;
} camera;

void main() {
    gl_Position = camera.perspective * camera.view * modelTransform * position;
    v_normal = normalize((modelTransform * vec4(normal, 0.0)).xyz);
    v_texcoord = texcoord;
    
    viewdir = normalize((inverse(camera.view) * vec4(0, 0, -1, 0)).xyz);
}
