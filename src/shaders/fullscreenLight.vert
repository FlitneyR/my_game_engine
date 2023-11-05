#version 450

layout(location = 0) in vec3 screenPosition;
layout(location = 1) in vec3 colour;
layout(location = 2) in vec3 position;
layout(location = 3) in vec3 direction;
layout(location = 4) in float angle;
layout(location = 5) in uint type;

layout(location = 0) out vec2 v_screenCoord;
layout(location = 1) out vec3 v_colour;
layout(location = 2) out vec3 v_position;
layout(location = 3) out vec3 v_direction;
layout(location = 4) out float v_angle;
layout(location = 5) out uint v_type;

void main() {
    v_screenCoord = screenPosition.xy;
    v_colour = colour;
    v_position = position;
    v_direction = direction;
    v_angle = angle;
    v_type = type;

    gl_Position = vec4(screenPosition, 1);
}
