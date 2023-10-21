# My Game Engine

This is my attempt at a simple Vulkan-based game engine.

## Goal

To create a simple game engine with the following features:
- GPU accelerated rendering in Vulkan
- A user friendly input system
- A simple physics system

## Design

- The main program will be an `mge::Engine`
- Every game will be a class extending `mge::Engine`
- `mge::Engine` will by default handle
    - Initialising vulkan
        - Creating an instance
        - Activating validation layers
        - Selecting a GPU
        - Making the required render passes
- Left for implementation by the user will be
    - `mge::Engine::draw(vk::CommandBuffer cmd)` to record a command buffer
    - `mge::Engine::onTick(float deltaTime)` to update the game state per frame
    - `mge::Engine::onFixedTick(float deltaTime)` to update the game state per simulation tick

### Graphics

- All rendering will be handled using `mge::Model`s
- An `mge::Model` will have one template parameter: `Vertex`
- An `mge::Model<Vertex>` will have two members
    - `mge::Mesh<Vertex>::Instance& mesh`
    - `mge::Material<Vertex>::Instance& material`
- You will be able to acquire a `mge::Mesh<Vertex>::Instance&` and `mge::Material<Vertex>::Instance&` for an `mge::Mesh<Vertex>` and an `mge::Material<Vertex>` respectively
- `mge::Mesh`s will store the vertices, indices, and instances of a mesh in `std::vector`s, along with their buffers and device memory
- `mge::Material`s will store the shaders and pipelines

### Input

***TBD***

### Physics

***TBD***
