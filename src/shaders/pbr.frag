#version 450

layout(location = 0) in vec3 v_worldpos;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_texcoord;
layout(location = 3) in vec3 camPos;

layout(location = 0) out vec4 f_colour;

layout(set = 1, binding = 0) uniform sampler2D albedoTex;
layout(set = 1, binding = 1) uniform sampler2D armTex;

const vec3 lightDirection = -normalize(vec3(1, 1, -1));
const vec3 lightColour = vec3(1, 1, 1);
const vec3 ambientLight = vec3(0.01);
const float PI = 3.141592654;

float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 fresnelSchlick(float cosTheta, vec3 F0);

void main() {
    vec3 albedo = texture(albedoTex, v_texcoord).rgb;
    vec3 arm = texture(armTex, v_texcoord).rgb;

    float ao = arm.r;
    float roughness = arm.g;
    float metallness = arm.b;

    vec3 F0 = mix(vec3(0.04), albedo, metallness);

    vec3 viewdir = normalize(camPos - v_worldpos);
    vec3 halfway = normalize(viewdir + lightDirection);

    float NDF = DistributionGGX(v_normal, halfway, roughness);
    float G = GeometrySmith(v_normal, viewdir, lightDirection, roughness);
    vec3 F = fresnelSchlick(max(dot(halfway, viewdir), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallness;

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(v_normal, viewdir), 0.0) * max(dot(v_normal, lightDirection), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
        
    // add to outgoing radiance Lo
    float NdotL = max(dot(v_normal, lightDirection), 0.0);
    vec3 illumination = (kD * albedo / PI + specular) * lightColour * NdotL;

    vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 colour = ambient + illumination;

    colour = colour / (colour + vec3(1.0));
    colour = pow(colour, vec3(1.0 / 1.0));
   
    f_colour = vec4(colour, 1.0);
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
