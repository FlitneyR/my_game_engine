#version 450

layout(location = 0) out vec4 f_colour;

layout(set = 0, binding = 0) uniform sampler2D t_previousFrame;
layout(set = 0, binding = 1) uniform sampler2D t_currentFrame;
layout(set = 0, binding = 2) uniform sampler2D t_velocity;
layout(set = 0, binding = 3) uniform sampler2D t_depth;

void main() {
    vec2 texel = 1.0 / textureSize(t_previousFrame, 0);
    vec2 uv = gl_FragCoord.xy * texel;

    vec3 currentFrameColour = texture(t_currentFrame, uv).rgb;

    float depth = texture(t_depth, uv).r;
    if (depth == 1.0) {
        f_colour = vec4(currentFrameColour, 1.0);
        return;
    }
    
    vec2 closestUV = uv;
    for (int i = -2; i <= 2; i++)
    for (int j = -2; j <= 2; j++) {
        vec2 deltaUV = vec2(i, j) * texel;
        float s_depth = texture(t_depth, uv + deltaUV).r;
        if (s_depth < depth) {
            depth = s_depth;
            closestUV = uv + deltaUV;
        }
    }

    vec3 velocity = texture(t_velocity, closestUV).rgb;

    vec2 previousUV = uv - velocity.xy * 0.5;

    // if the previous screen position was off screen
    if (clamp(previousUV, 0, 1) != previousUV) {
        f_colour = vec4(currentFrameColour, 1.0);
        return;
    }

    vec3 previousFrameColour = clamp(texture(t_previousFrame, previousUV).rgb, 0, 1);

    // compute aabb
    vec3[2] aabb = vec3[2](currentFrameColour, currentFrameColour);

    for (int i = -1; i <= 1; i++)
    for (int j = -1; j <= 1; j++) {
        vec2 deltaUV = vec2(i, j) * texel;
        vec3 p = clamp(texture(t_currentFrame, uv + deltaUV).rgb, 0, 1);
        aabb[0] = min(aabb[0], p);
        aabb[1] = max(aabb[1], p);
    }

    // clamp previous frame colour within aabb
    previousFrameColour = clamp(previousFrameColour, aabb[0], aabb[1]);

    f_colour = vec4(mix(previousFrameColour, currentFrameColour, 0.05), 1.0);
}