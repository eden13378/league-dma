#pragma once
#include "../math/vec2.hpp"

namespace sdk::game {
    class TacticalMap {
    public:
        char       pad_0000[ 40 ];            // 0x0000
        math::Vec2 world_scale;               // 0x0028
        char       pad_0030[ 40 ];            // 0x0030
        math::Vec2 top_left_clipping_inverse; // 0x0058
        math::Vec2 top_left_clipping;         // 0x0060
        math::Vec2 map_area;                  // 0x0068
    };
}
