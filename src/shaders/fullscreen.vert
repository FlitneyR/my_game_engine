#version 450

layout(location = 0) in vec3 screenPosition;

layout(location = 0) out vec2 v_screenCoord;

void main() {
    gl_Position = vec4(screenPosition, 1.0);
    v_screenCoord = screenPosition.xy;
}
