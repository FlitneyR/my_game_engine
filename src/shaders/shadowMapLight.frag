#version 450

#include "light.glsl"
#include "camera.glsl"

layout(location = 0) in vec2 v_screenCoord;
layout(location = 1) in vec3 v_colour;
layout(location = 2) in vec3 v_position;
layout(location = 3) in vec3 v_direction;
layout(location = 4) in float v_angle;
layout(location = 5) flat in uint v_type;

layout(location = 0) out vec4 f_emissive;

layout(set = 0, binding = 0) uniform Uniform_0_0 { Camera camera; };

layout(set = 1, binding = 0) uniform sampler2D depthTex;
layout(set = 1, binding = 1) uniform Uniform_1_1 { Camera lightView; };

layout(set = 2, binding = 0, input_attachment_index = 0) uniform subpassInput i_depth;
layout(set = 2, binding = 1, input_attachment_index = 1) uniform subpassInput i_albedo;
layout(set = 2, binding = 2, input_attachment_index = 2) uniform subpassInput i_normal;
layout(set = 2, binding = 3, input_attachment_index = 3) uniform subpassInput i_arm;

float pcfFilter(sampler2D depthTex, vec3 worldPos, float filterSize, int filterSteps);

const float FILTER_SIZE = 3;
const int FILTER_SAMPLES = 3;
const float MINIMUM_RADIANCE = 0.01;

void main() {
    float depth = subpassLoad(i_depth).r;
    vec3 albedo = subpassLoad(i_albedo).rgb;
    vec3 normal = subpassLoad(i_normal).rgb;
    vec3 arm = subpassLoad(i_arm).rgb;

    if (v_type == AMBIENT) {
        f_emissive = vec4(v_colour * albedo * arm.r, 1.0);
        return;
    }

    GeometryData gd = makeGeometryData(camera, v_screenCoord, depth, albedo, normal, arm);

    vec3 lightPositionDelta = v_position - gd.worldPos.xyz;
    vec3 lightDirection;
    switch (v_type) {
    case DIRECTIONAL: lightDirection = normalize(v_direction); break;
    case POINT: case SPOT: lightDirection = -normalize(lightPositionDelta); break;
    }

    vec3 radiance;
    float dist;
    switch (v_type) {
    case DIRECTIONAL: radiance = v_colour; break;
    case POINT:
        dist = length(lightPositionDelta);
        radiance = v_colour / (dist * dist);
        break;
    case SPOT:
        dist = length(lightPositionDelta);
        float angleFalloff = dot(lightDirection, v_direction);
        angleFalloff = clamp((angleFalloff - 1.0) / (1.0 - cos(v_angle)) + 1.0, 0.0, 1.0);
        radiance = v_colour * angleFalloff * angleFalloff / (dist * dist);
        break;
    }

    if (length(radiance) < MINIMUM_RADIANCE) discard;

    radiance *= pcfFilter(depthTex, gd.worldPos, FILTER_SIZE, FILTER_SAMPLES);

    if (length(radiance) < MINIMUM_RADIANCE) discard;

    vec3 colour = calculateLighting(gd, lightDirection, radiance);
   
    f_emissive = vec4(colour, 1.0);
}

float pcfFilter(sampler2D depthTex, vec3 worldPos, float filterSize, int filterSteps) {
    vec4 lightClipSpace = lightView.perspective * lightView.view * vec4(worldPos, 1.0);
    lightClipSpace.xyz /= lightClipSpace.w;
    lightClipSpace.xy = lightClipSpace.xy * 0.5 + 0.5;

    float scaledFilterSize = filterSize * lightClipSpace.w;

    if (scaledFilterSize <= 0) {
        float shadowMapDepth = texture(depthTex, lightClipSpace.xy).r;
        return float(lightClipSpace.z < shadowMapDepth);
    }

    vec2 texel = 1.0 / textureSize(depthTex, 0);
    float totalCount = 0, closerCount = 0;

    vec2 offset;
    float halfFilterSize = scaledFilterSize / 2;

    for (offset.x = -halfFilterSize; offset.x <= halfFilterSize; offset.x += halfFilterSize / filterSteps)
    for (offset.y = -halfFilterSize; offset.y <= halfFilterSize; offset.y += halfFilterSize / filterSteps) {
        // float sampleDistance = max(abs(offset.x), abs(offset.y));
        // vec2 _offset = normalize(offset) * sampleDistance;

        float shadowMapDepth = texture(depthTex, lightClipSpace.xy + offset * texel).r;
        float depthDelta = shadowMapDepth - lightClipSpace.z;

        closerCount += float(depthDelta <= 0.0);
        totalCount++;
    }

    return 1.0 - (closerCount / totalCount);
}

