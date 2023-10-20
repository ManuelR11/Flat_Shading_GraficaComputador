#pragma once
#include "color.h"
#include "glm/glm.hpp"

struct Vertex {
    glm::vec3 position;
    Color color;
};

struct Fragment {
    glm::vec3 position;
    Color color;
};