#ifndef CAMERA_GLSL
#define CAMERA_GLSL

struct Camera {
    mat4 view;
    mat4 perspective;
    vec2 jitter;
    
    mat4 previousView;
    mat4 previousPerspective;
    vec2 previousJitter;
};

#endif