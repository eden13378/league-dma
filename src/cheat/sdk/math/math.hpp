#pragma once
#include "vec2.hpp"
#include "vec3.hpp"

namespace sdk::math {
    auto world_to_screen( const Vec3& origin, Vec2& screen ) -> bool;
    auto world_to_minimap( Vec3 position ) -> Vec2;
}
