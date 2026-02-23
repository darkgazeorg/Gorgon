#pragma once

#include <Gorgon/Geometry/Point.h>

using Gorgon::Geometry::Point;
using Gorgon::Geometry::Pointf;

inline float RandomFloat(float min, float max) {
    return (max - min) * rand() / RAND_MAX + min;
}
