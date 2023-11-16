DEPENDENCIES = glm glfw3 vulkan
INCLUDES = -Isrc/headers -Isrc/headers/ECS -Isrc/headers/graphics `pkg-config --cflags $(DEPENDENCIES)`

LFLAGS = -g `pkg-config --libs $(DEPENDENCIES)`
CFLAGS = $(INCLUDES) -std=c++2a -g -O2 -c

VULKAN_SDK_LIB_PATH = /Users/flitneyr/VulkanSDK/1.3.261.1/macOS/lib

MAIN_DEPENDENCIES = build/engine.o build/objloader.o build/instance.o build/light.o build/postProcessing.o

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

# bin/main: export DYLD_LIBRARY_PATH := $(VULKAN_SDK_LIB_PATH):$(DYLD_LIBRARY_PATH)
main: bin/main shaders

bin/main: build/main.o $(MAIN_DEPENDENCIES)
	clang++ $(LFLAGS) -o $@ $^

build/main.o: src/main.cpp
	clang++ $(CFLAGS) -o $@ $<

build/%.o: src/%.cpp src/headers/%.hpp
	clang++ $(CFLAGS) -o $@ $<

build/%.o: src/%.cpp
	clang++ $(CFLAGS) -o $@ $<

build/%.spv: src/shaders/%
	glslc -o $@ $<

shaders: $(COMPILED_SHADERS)

run: export DYLD_LIBRARY_PATH := $(VULKAN_SDK_LIB_PATH):$(DYLD_LIBRARY_PATH)
run: main
	bin/main

clean:
	rm -f build/* bin/*
