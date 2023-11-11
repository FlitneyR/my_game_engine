#version 450

layout(location = 0) in vec3 v_worldpos;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec3 v_tangent;
layout(location = 3) in vec3 v_bitangent;
layout(location = 4) in vec2 v_texcoord;

layout(location = 0) out vec4 f_albedo;
layout(location = 1) out vec4 f_normal;
layout(location = 2) out vec4 f_arm;
layout(location = 3) out vec4 f_emissive;

layout(set = 1, binding = 0) uniform sampler2D albedoTex;
layout(set = 1, binding = 1) uniform sampler2D armTex;
layout(set = 1, binding = 2) uniform sampler2D normalTex;

void main() {
    vec4 albedo = texture(albedoTex, v_texcoord);
    if (albedo.a < 0.5) discard;

    vec3 arm = texture(armTex, v_texcoord).rgb;
    vec3 normal = texture(normalTex, v_texcoord).rgb;
    normal = 2.0 * normal - 1.0;

    f_albedo = vec4(albedo.rgb, 1.0);
    f_arm = vec4(1.0, arm.gb, 1.0);
    f_normal = vec4(normalize(v_tangent * normal.x + v_bitangent * normal.y + v_normal * normal.z), 1.0);
    f_emissive = vec4(0.0, 0.0, 0.0, 1.0);
}
