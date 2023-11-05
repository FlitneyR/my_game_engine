#version 450

layout(location = 0) in vec3 v_worldpos;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_texcoord;
layout(location = 3) in vec3 camPos;

layout(location = 0) out vec4 f_albedo;
layout(location = 1) out vec4 f_normal;
layout(location = 2) out vec4 f_arm;
layout(location = 3) out vec4 f_emissive;

layout(set = 1, binding = 0) uniform sampler2D albedoTex;
layout(set = 1, binding = 1) uniform sampler2D armTex;

void main() {
    vec3 albedo = texture(albedoTex, v_texcoord).rgb;
    vec3 arm = texture(armTex, v_texcoord).rgb;

    f_albedo = vec4(albedo, 1.0);
    f_arm = vec4(arm, 1.0);
    f_normal = vec4(v_normal, 1.0);
    f_emissive = vec4(0.0, 0.0, 0.0, 1.0);
}
