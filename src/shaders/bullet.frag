#version 450

layout(location = 0) out vec4 f_colour;

layout(location = 0) in vec4 v_colour;

void main() {
    f_colour = v_colour;
}
