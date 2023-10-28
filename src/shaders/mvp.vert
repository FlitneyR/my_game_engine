#version 450

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord;
layout(location = 3) in mat4 modelTransform;

layout(location = 0) out vec3 v_worldpos;
layout(location = 1) out vec3 v_normal;
layout(location = 2) out vec2 v_texcoord;
layout(location = 3) out vec3 camPos;

layout(set = 0, binding = 0) uniform Camera {
    mat4 view;
    mat4 perspective;
} camera;

void main() {
    vec4 worldpos = modelTransform * position;
    v_worldpos = worldpos.xyz / worldpos.w;

    v_normal = normalize((modelTransform * vec4(normal, 0.0)).xyz);

    v_texcoord = texcoord;
    camPos = (inverse(camera.view) * vec4(0, 0, 0, 1)).xyz;

    gl_Position = camera.perspective * camera.view * worldpos;
}
