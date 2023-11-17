#version 450

layout(location = 0) out vec4 f_emissive;

layout(set = 0, binding = 0) uniform sampler2D previousFrame;
layout(set = 0, binding = 1) uniform sampler2D currentFrame;

void main() {
    vec2 texel = 1.0 / textureSize(previousFrame, 0);
    vec2 uv = gl_FragCoord.xy * texel;

    vec3 previousFrameColour = clamp(texture(previousFrame, uv).rgb, 0, 1);
    vec3 currentFrameColour = clamp(texture(currentFrame, uv).rgb, 0, 1);

    // compute aabb
    vec3[2] aabb = vec3[2](currentFrameColour, currentFrameColour);

    for (int i = -1; i <= 1; i++)
    for (int j = -1; j <= 1; j++) {
        vec2 deltaUV = vec2(i, j) * texel;
        vec3 p = clamp(texture(currentFrame, uv + deltaUV).rgb, 0, 1);
        aabb[0] = min(aabb[0], p);
        aabb[1] = max(aabb[1], p);
    }

    // clamp previous frame colour within aabb
    previousFrameColour = clamp(previousFrameColour, aabb[0], aabb[1]);

    f_emissive = vec4(mix(previousFrameColour, currentFrameColour, 0.1), 1.0);
}