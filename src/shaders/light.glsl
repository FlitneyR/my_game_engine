#ifndef LIGHT_GLSL
#define LIGHT_GLSL

#include "camera.glsl"

const float PI = 3.141592654;

#define AMBIENT 0
#define DIRECTIONAL 1
#define POINT 2
#define SPOT 3

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

struct GeometryData {
    vec3 worldPos;
    vec3 viewDir;
    vec3 albedo;
    vec3 normal;
    float ao;
    float roughness;
    float metallness;
};

GeometryData makeGeometryData(Camera camera, vec2 screenCoord, float depth, vec3 albedo, vec3 normal, vec3 arm) {
    GeometryData result;

    result.albedo = albedo;
    result.normal = normal;
    result.ao = arm.r;
    result.roughness = arm.g;
    result.metallness = arm.b;

    mat4 inverseView = inverse(camera.view);
    mat4 inversePerspective = inverse(camera.perspective);

    vec4 viewSpace = vec4(screenCoord, depth, 1);
    vec4 worldPos = inverseView * inversePerspective * viewSpace;
    worldPos /= worldPos.w;

    vec4 camPos = inverseView * vec4(0, 0, 0, 1);

    result.worldPos = worldPos.xyz;
    result.viewDir = normalize(vec3(worldPos - camPos));

    return result;
}

vec3 calculateLighting(GeometryData gd, vec3 lightDirection, vec3 radiance) {
    // lightDirection = normalize(lightDirection);
    // gd.normal = normalize(gd.normal);

    // vec3 diffuse = gd.albedo * radiance * max(dot(lightDirection, gd.normal), 0);

    // vec3 halfWay = normalize(gd.viewDir + lightDirection);

    // vec3 specular = gd.albedo * radiance * pow(max(dot(halfWay, gd.normal), 0), 5);

    // return diffuse + specular;

    vec3 F0 = mix(vec3(0.04), gd.albedo, gd.metallness);

    vec3 halfway = normalize(gd.viewDir + lightDirection);

    float NDF = DistributionGGX(gd.normal, halfway, gd.roughness);
    float G = GeometrySmith(gd.normal, gd.viewDir, lightDirection, gd.roughness);
    vec3 F = fresnelSchlick(max(dot(halfway, gd.viewDir), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - gd.metallness;

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(gd.normal, gd.viewDir), 0.0) * max(dot(gd.normal, lightDirection), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
    
    float NdotL = max(dot(gd.normal, lightDirection), 0.0);
    return (kD * gd.ao * gd.albedo / PI + specular) * radiance * NdotL;
}

#endif