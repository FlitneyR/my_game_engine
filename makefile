DEPENDENCIES = glm glfw3 vulkan
INCLUDES = -Isrc/headers `pkg-config --cflags $(DEPENDENCIES)`

LFLAGS = -g `pkg-config --libs $(DEPENDENCIES)`
CFLAGS = $(INCLUDES) -std=c++2a -g -O2 -c

VULKAN_SDK_LIB_PATH = /Users/flitneyr/VulkanSDK/1.3.261.1/macOS/lib

MAIN_DEPENDENCIES = build/engine.o

COMPILED_SHADERS = build/test.vert.spv build/test.frag.spv

# bin/main: export DYLD_LIBRARY_PATH := $(VULKAN_SDK_LIB_PATH):$(DYLD_LIBRARY_PATH)
main: bin/main $(COMPILED_SHADERS)

bin/main: build/main.o $(MAIN_DEPENDENCIES)
	clang++ $(LFLAGS) -o $@ $^

build/%.o: src/%.cpp src/headers/%.hpp
	clang++ $(CFLAGS) -o $@ $<

build/%.o: src/%.cpp
	clang++ $(CFLAGS) -o $@ $<

memtest: bin/main
	leaks -atExit -- bin/main

bench: bin/main
	time bin/main

build/%.spv: src/shaders/%
	glslc -o $@ $<

shaders: $(COMPILED_SHADERS)

run: export DYLD_LIBRARY_PATH := $(VULKAN_SDK_LIB_PATH):$(DYLD_LIBRARY_PATH)
run: bin/main shaders
	bin/main

clean:
	rm -f build/* bin/*
