# My Game Engine

This is my attempt at a simple Vulkan-based game engine.

## Demos

- Asteroids game: https://youtu.be/uIyIgphtwv8

## Screenshots

![Shadow Mapping in Sponza Scene](screenshots/Sponza.png)

## Goal

To create a simple game engine with the following features:
- GPU-accelerated rendering in Vulkan
- An Entity Component System
- A Simple Physics System


### Graphics

The graphics system uses deferred rendering to support limitless lights and more advanced graphics techniques.
The graphics pipeline consists of the following passes
- a depth pass for each shadow mapped light in the scene
- a GBuffer pass, which renders surface information to a geometry buffer consisting of six textures:
    - albedo, the surface's base colour
    - normal, the direction the surface is facing
    - ambient occlusion - roughness - metallness, the physical properties of the surface
    - emissive, how much light the surface is emitting
    - depth, analogous to the distance the surface is from the camera
    - and velocity, used for TAA later on
- a lighting pass, which computes surface illumination from each light in the scene and outputs to the emissive texture
- a post-processing pass to add bloom, colour correction, and apply Temporal Anti Aliasing

#### TODO!
- Add occlusion culling

### Physics

The physics system is implemented within the ECS framework. It uses an SAT algorithm to support collisions between spheres, oriented bounding boxes, and capsules. It uses a KD Tree to reduce the number of pairs of colliders it needs to check collisions between.

The KD Tree split algorithm works as follows:
- Find the mean position of all the colliders center of mass' along one of the X, Y, or Z axis
- Split the space into two sub-trees, one containing all the colliders at least partly on one side, one containing all the colliders at least partly on the other side
- If you have reached the maximum tree depth, or the minimum number of child colliders, stop
- Otherwise, repeat along the next axis for each subtree
