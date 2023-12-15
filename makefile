# === Definitions ===

DEPENDENCIES = glm glfw3 vulkan
INCLUDES = -Isrc/headers -Isrc/headers/ECS -Isrc/headers/graphics `pkg-config --cflags $(DEPENDENCIES)`

LFLAGS = -g `pkg-config --libs $(DEPENDENCIES)`
CFLAGS = $(INCLUDES) -std=c++2a -g -O2 -c

VULKAN_SDK_LIB_PATH = /Users/flitneyr/VulkanSDK/1.3.261.1/macOS/lib

MAIN_DEPENDENCIES = build/engine.o \
					build/objloader.o \
					build/instance.o \
					build/light.o \
					build/postProcessing.o \
					build/taa.o \
					build/bloom.o \
					build/collision.o \

COMPILED_SHADERS = build/mvp.vert.spv \
				   build/pbr.frag.spv \
				   build/skybox.vert.spv \
				   build/skybox.frag.spv \
				   build/bullet.vert.spv \
				   build/bullet.frag.spv \
				   build/fullscreenLight.vert.spv \
				   build/light.frag.spv \
				   build/shadowMapLight.frag.spv \
				   build/fullscreen.vert.spv \
				   build/hdrColourCorrection.frag.spv \
				   build/taa.frag.spv \
				   build/sharpen.frag.spv \
				   build/bloom.frag.spv \

# === Asteroids Game ===
asteroids: export DYLD_LIBRARY_PATH := $(VULKAN_SDK_LIB_PATH):$(DYLD_LIBRARY_PATH)
asteroids: bin/asteroids shaders
	bin/asteroids

bin/asteroids: build/asteroids.o $(MAIN_DEPENDENCIES)
	clang++ $(LFLAGS) -o $@ $^

build/asteroids.o: src/demos/asteroids/asteroids.cpp src/demos/asteroids/logic.hpp
	clang++ $(CFLAGS) -o $@ $<


# === Sponza Demo ===
sponza: export DYLD_LIBRARY_PATH := $(VULKAN_SDK_LIB_PATH):$(DYLD_LIBRARY_PATH)
sponza: bin/sponza shaders
	bin/sponza

bin/sponza: build/sponza.o $(MAIN_DEPENDENCIES)
	clang++ $(LFLAGS) -o $@ $^

build/sponza.o: src/demos/sponza/sponza.cpp
	clang++ $(CFLAGS) -o $@ $<

# === Generics ===

build/%.o: src/%.cpp src/headers/%.hpp
	clang++ $(CFLAGS) -o $@ $<

build/%.o: src/%.cpp
	clang++ $(CFLAGS) -o $@ $<

build/%.spv: src/shaders/%
	glslc -o $@ $<

shaders: $(COMPILED_SHADERS)

clean:
	rm -f build/* bin/*
