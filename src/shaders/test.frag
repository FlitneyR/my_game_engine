#version 450

layout(location = 0) out vec4 f_colour;

layout(location = 0) in vec3 v_normal;
layout(location = 1) in vec2 v_texcoord;
layout(location = 2) in vec3 viewdir;

layout(set = 1, binding = 0) uniform sampler2D tex;

const vec3 lightDirection = normalize(vec3(1, -1, 1));

const vec3 specularLight = vec3(1);
const vec3 diffuseLight = vec3(0.6);
const vec3 ambientLight = vec3(0.05);

void main() {
    float diffuse = max(0.0, dot(v_normal, -lightDirection));

    vec3 halfway = -normalize(viewdir + lightDirection);
    float specular = max(0.0, pow(dot(halfway, v_normal), 15));

    vec3 light;

    if (specular > 0.5) light = specularLight;
    else if (diffuse > 0.5) light = diffuseLight;
    else light = ambientLight;

    light = specularLight * specular + diffuseLight * diffuse + ambientLight;

    f_colour = vec4(texture(tex, v_texcoord).xyz * light, 1.0);

    // f_colour.xyz = f_colour.xyz * floor(length(f_colour.xyz) * 3.0) / 3.0;

    // f_colour = vec4(vec3(1) * specular, 1);

    // f_colour = vec4(v_normal, 1);
}
