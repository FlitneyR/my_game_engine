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

layout(set = 1, binding = 0, input_attachment_index = 0) uniform subpassInput i_depth;
layout(set = 1, binding = 1, input_attachment_index = 1) uniform subpassInput i_albedo;
layout(set = 1, binding = 2, input_attachment_index = 2) uniform subpassInput i_normal;
layout(set = 1, binding = 3, input_attachment_index = 3) uniform subpassInput i_arm;

layout(set = 0, binding = 0) uniform Uniform_0_0 { Camera camera; };

const float MINIMUM_RADIANCE = 0.01;

void main() {
    float depth = subpassLoad(i_depth).r;
    vec3 albedo = subpassLoad(i_albedo).rgb;
    vec3 normal = subpassLoad(i_normal).rgb;
    vec3 arm = subpassLoad(i_arm).rgb;

    if (v_type == AMBIENT) {
        vec3 diffuse = v_colour * albedo * arm.r;
        f_emissive = vec4(diffuse, 1.0);
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
   
    vec3 colour = calculateLighting(gd, lightDirection, radiance);

    f_emissive = vec4(colour, 1.0);
}
