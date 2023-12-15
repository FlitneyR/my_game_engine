#version 450

layout(location = 0) out vec4 f_emissive;

layout(set = 0, binding = 0, input_attachment_index = 0) uniform subpassInput i_emissive;

vec3 aces(vec3 x) {
  const float a = 2.51;
  const float b = 0.03;
  const float c = 2.43;
  const float d = 0.59;
  const float e = 0.14;
  return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    vec3 colour = subpassLoad(i_emissive).rgb;
    colour = colour / (colour + vec3(1.0));
    // colour = aces(colour);
    // colour = pow(colour, vec3(1.0 / 1.1));
    f_emissive = vec4(colour, 1.0);
}
