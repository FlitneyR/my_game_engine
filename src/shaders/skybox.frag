#version 450

layout(location = 0) out vec4 f_colour;

layout(location = 1) in vec2 v_texcoord;

layout(set = 1, binding = 0) uniform sampler2D tex;

void main() {
    f_colour = texture(tex, v_texcoord);
}
