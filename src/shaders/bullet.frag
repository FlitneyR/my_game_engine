#version 450

layout(location = 0) out vec4 f_albedo;
layout(location = 1) out vec4 f_normal;
layout(location = 2) out vec4 f_arm;
layout(location = 3) out vec4 f_emissive;

layout(location = 0) in vec4 v_colour;

void main() {
    f_emissive = v_colour;
    f_albedo = vec4(0, 0, 0, 1);
    f_normal = vec4(0, 0, 0, 1);
    f_arm = vec4(1, 1, 0, 1);
}
