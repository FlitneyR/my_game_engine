cmake_minimum_required(VERSION 3.12)
project(MyGameEngine)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

find_package(Vulkan REQUIRED)
find_package(glfw3 3.3 REQUIRED)

include_directories(${Vulkan_INCLUDE_DIR} src/headers src/headers/ecs src/headers/graphics)

link_libraries(${Vulkan_LIBRARY} ${GLFW3_LIBRARY})

add_library(mge
    src/bloom.cpp
    src/collision.cpp
    src/engine.cpp
    src/instance.cpp
    src/light.cpp
    src/objloader.cpp
    src/postProcessing.cpp
    src/taa.cpp
)

link_libraries(mge)

add_executable(asteroids src/demos/asteroids/asteroids.cpp)
add_executable(sponza src/demos/sponza/sponza.cpp)

file(GLOB SHADERS src/shaders/*.vert src/shaders/*.frag)

foreach(SHADER IN LISTS SHADERS)
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(COMPILED_SHADER shaders/${SHADER_NAME}.spv)
    add_custom_command(
        OUTPUT ${COMPILED_SHADER}
        COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${SHADER} -o ${COMPILED_SHADER}
        DEPENDS ${SHADER}
        COMMENT "Compiling ${SHADER}")
    list(APPEND SPV_SHADERS ${COMPILED_SHADER})
endforeach()

add_custom_target(shaders ALL DEPENDS ${SPV_SHADERS})

add_dependencies(asteroids shaders)
add_dependencies(sponza shaders)

file(COPY assets DESTINATION .)

install(TARGETS asteroids sponza DESTINATION bin)
install(DIRECTORY assets DESTINATION bin)
install(DIRECTORY ${CMAKE_BINARY_DIR}/shaders DESTINATION bin)

set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/package")
set(CPACK_GENERATOR "ZIP")

include(CPack)
