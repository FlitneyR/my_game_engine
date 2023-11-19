#version 450

layout(location = 0) out vec4 f_output;

layout(set = 0, binding = 0) uniform sampler2D t_source;

void main() {
    vec2 texelSize = 1.0 / textureSize(t_source, 0);
    vec2 uv = gl_FragCoord.xy * texelSize;

    vec3 originalColour = texture(t_source, uv).rgb;

    vec3 sharpened = 9 * originalColour;

    vec2 offset;
    for (offset.x = -1; offset.x <= 1; offset.x++)
    for (offset.y = -1; offset.y <= 1; offset.y++)
    if (offset != vec2(0, 0))
        sharpened -= texture(t_source, uv + offset * texelSize).rgb;
    
    sharpened = clamp(sharpened, originalColour * 0.5, originalColour * 1.5);
    
    f_output = vec4(mix(originalColour, sharpened, 0.5), 1.0);
}
