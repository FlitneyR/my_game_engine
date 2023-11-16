#version 450

layout(location = 0) out vec4 f_emissive;

layout(set = 0, binding = 0, input_attachment_index = 0) uniform subpassInput i_emissive;

void main() {
    vec3 colour = subpassLoad(i_emissive).rgb;
    colour = colour / (colour + vec3(1.0));
    colour = pow(colour, vec3(1.0 / 1.0));
    f_emissive = vec4(colour, 1.0);
}
