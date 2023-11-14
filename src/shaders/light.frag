#version 450

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

layout(set = 0, binding = 0) uniform Camera {
    mat4 view;
    mat4 perspective;
} camera;

#define AMBIENT 0
#define DIRECTIONAL 1
#define POINT 2
#define SPOT 3

const float PI = 3.141592654;

float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 fresnelSchlick(float cosTheta, vec3 F0);

void main() {
    float depth = subpassLoad(i_depth).r;
    vec3 albedo = subpassLoad(i_albedo).rgb;
    vec3 normal = subpassLoad(i_normal).rgb;
    vec3 arm = subpassLoad(i_arm).rgb;

    float ao = arm.r;
    float roughness = arm.g;
    float metallness = arm.b;

    if (v_type == AMBIENT) {
        f_emissive = vec4(v_colour * albedo * ao, 1.0);
        return;
    }

    mat4 inversePerspective = inverse(camera.perspective);
    mat4 inverseView = inverse(camera.view);

    vec4 viewSpace = vec4(v_screenCoord, depth, 1);
    vec4 worldPos = inverseView * inversePerspective * viewSpace;
    worldPos /= worldPos.w;

    vec4 camPos = inverseView * vec4(0, 0, 0, 1);

    vec3 lightPositionDelta = v_position - worldPos.xyz;
    vec3 lightDirection;
    switch (v_type) {
    case DIRECTIONAL: lightDirection = -normalize(v_direction); break;
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

    vec3 F0 = mix(vec3(0.04), albedo, metallness);

    vec3 viewdir = normalize(vec3(worldPos - camPos));
    vec3 halfway = normalize(viewdir + lightDirection);

    float NDF = DistributionGGX(normal, halfway, roughness);
    float G = GeometrySmith(normal, viewdir, lightDirection, roughness);
    vec3 F = fresnelSchlick(max(dot(halfway, viewdir), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallness;

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(normal, viewdir), 0.0) * max(dot(normal, lightDirection), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
    
    float NdotL = max(dot(normal, lightDirection), 0.0);
    vec3 illumination = (kD * ao * albedo / PI + specular) * radiance * NdotL;

    vec3 colour = illumination;

    colour = colour / (colour + 1.0);
   
    f_emissive = vec4(colour, 1.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

