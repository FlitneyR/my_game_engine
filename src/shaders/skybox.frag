#version 450

layout(location = 3) out vec4 f_emissive;

layout(location = 0) in vec2 v_texcoord;

layout(set = 1, binding = 0) uniform sampler2D tex;

void main() {
    f_emissive = texture(tex, v_texcoord);
}
