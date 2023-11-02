#ifndef COMPONENT_HPP
#define COMPONEN_HPP

#include <entity.hpp>

namespace mge::ecs {

struct Component {
    Entity m_parent;
};

}

#endif