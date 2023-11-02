#ifndef OBJLOADER_HPP
#define OBJLOADER_HPP

#include <mesh.hpp>
#include <vertex.hpp>

namespace mge {

Mesh<ModelVertex> loadObjMesh(Engine& engine, const char* filename);

}

#endif