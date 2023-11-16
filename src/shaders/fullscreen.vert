#version 450

vec2 points[3] = vec2[3](
    vec2(-1.0, -1.0),
    vec2( 3.0, -1.0),
    vec2(-1.0,  3.0)
);

void main() {
    gl_Position = vec4(points[gl_VertexIndex], 0.0, 1.0);
}
