#version 450

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec3 bitangent;
layout(location = 4) in vec2 texcoord;
layout(location = 5) in mat4 modelTransform;

layout(location = 0) out vec3 v_worldpos;
layout(location = 1) out vec3 v_normal;
layout(location = 2) out vec3 v_tangent;
layout(location = 3) out vec3 v_bitangent;
layout(location = 4) out vec2 v_texcoord;

layout(set = 0, binding = 0) uniform Camera {
    mat4 view;
    mat4 perspective;
} camera;

void main() {
    vec4 worldpos = modelTransform * position;
    v_worldpos = worldpos.xyz / worldpos.w;

    v_normal = normalize(-(modelTransform * vec4(normal, 0.0)).xyz);
    v_tangent = normalize(-(modelTransform * vec4(tangent, 0.0)).xyz);
    v_bitangent = normalize(-(modelTransform * vec4(bitangent, 0.0)).xyz);

    v_texcoord = texcoord;

    gl_Position = camera.perspective * camera.view * worldpos;
}
