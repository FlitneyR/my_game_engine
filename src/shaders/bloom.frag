#version 450

layout(location = 0) out vec4 f_colour;

layout(set = 0, binding = 0) uniform sampler2D s_source;

const int STAGE_FILTER_HIGHLIGHTS = 0;
const int STAGE_BLUR = 1;
const int STAGE_COMBINE = 2;
const int STAGE_OVERLAY = 3;

const int DIRECTION_HORIZONTAL = 0;
const int DIRECTION_VERTICAL = 1;

const vec2[2] g_directions = vec2[2](
    vec2(1.0, 0.0),
    vec2(0.0, 1.0)
);

layout(push_constant) uniform PC {
    int stage;
    int direction;
    vec2 viewportSize;
    float threshold;
    float combineFactor;
    float overlayFactor;
} pc;

void filterHighlights();
void blur();
void combine();
void overlay();

vec2 g_uv;

void main() {
    g_uv = gl_FragCoord.xy / pc.viewportSize;

    switch (pc.stage) {
    case STAGE_FILTER_HIGHLIGHTS:
        filterHighlights();
        return;
    case STAGE_BLUR:
        blur();
        return;
    case STAGE_COMBINE:
        combine();
        return;
    case STAGE_OVERLAY:
        overlay();
        return;
    }
}

void filterHighlights() {
    vec3 col = texture(s_source, g_uv).rgb;
    float factor = max(col.r + col.g + col.b - pc.threshold, 0.0);
    f_colour = vec4(col * factor, 1.0);
}

float gaussian(float f) {
    return exp(-pow(f, 2));
}

void blur() {
    vec3 col = vec3(0);
    float weightSum = 0;

    for (float i = -1.0; i <= 1.0; i += 1.0) {
        vec2 offset = g_directions[pc.direction];

        vec2 uv = g_uv + offset * i / pc.viewportSize;

        float factor = gaussian(i);

        col += texture(s_source, uv).rgb * factor;
        weightSum += factor;
    }

    f_colour = vec4(col / weightSum, 1.0);
}

void combine() {
    vec3 colour = texture(s_source, g_uv).rgb;
    f_colour = vec4(colour, pc.combineFactor);
}

void overlay() {
    vec3 bloom = texture(s_source, g_uv).rgb;
    f_colour = vec4(bloom / (bloom + 1.0), pc.overlayFactor);
}
