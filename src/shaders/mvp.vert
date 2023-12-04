#version 450

#include "camera.glsl"

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec3 bitangent;
layout(location = 4) in vec2 texcoord;
layout(location = 5) in mat4 modelTransform;
layout(location = 9) in mat4 previousModelTransform;

layout(location = 0) out vec3 v_worldPos;
layout(location = 1) out vec3 v_normal;
layout(location = 2) out vec3 v_tangent;
layout(location = 3) out vec3 v_bitangent;
layout(location = 4) out vec2 v_texcoord;
layout(location = 5) out vec4 v_currentScreenPos;
layout(location = 6) out vec4 v_previousScreenPos;

layout(set = 0, binding = 0) uniform Uniform_0_0 { Camera camera; };

void main() {
    vec4 worldPos = modelTransform * position;
    v_worldPos = worldPos.xyz / worldPos.w;

    vec4 previousWorldPos = previousModelTransform * position;
    previousWorldPos /= previousWorldPos.w;

    v_normal = normalize(-(modelTransform * vec4(normal, 0.0)).xyz);
    v_tangent = normalize(-(modelTransform * vec4(tangent, 0.0)).xyz);
    v_bitangent = normalize(-(modelTransform * vec4(bitangent, 0.0)).xyz);

    v_texcoord = texcoord;

    gl_Position = camera.perspective * camera.view * worldPos;
    gl_Position.xy += camera.jitter * gl_Position.w;

    vec4 previousPosition = camera.previousPerspective * camera.previousView * previousWorldPos;
    previousPosition.xy += camera.previousJitter * previousPosition.w;

    v_currentScreenPos = gl_Position;
    v_previousScreenPos = previousPosition;
}
