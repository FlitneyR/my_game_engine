#version 450

layout(location = 0) out vec4 f_albedo;
layout(location = 1) out vec4 f_normal;
layout(location = 2) out vec4 f_arm;
layout(location = 3) out vec4 f_emissive;

layout(location = 1) in vec2 v_texcoord;

layout(set = 1, binding = 0) uniform sampler2D tex;

void main() {
    f_emissive = texture(tex, v_texcoord);
}
