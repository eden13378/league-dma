#pragma once
#include "../math/vec3.hpp"

namespace sdk::game {
    class HudManager {
    public:
        char            pad_0000[ 32 ];             // 0x0000
        sdk::math::Vec3 cursor_position_clipped;   // 0x0020
        sdk::math::Vec3 cursor_position_unclipped; //0x002C
        sdk::math::Vec3 last_waypoint; //0x0038
        char            pad_0034[ 8 ]; //0x0044
        int16_t        hovered_object_handle; //0x004C
        char            pad_004E[2];
        uint32_t        hovered_object_handle_alt; //0x0050
        char            pad_0044[ 4 ]; //0x0054
        int16_t         last_target_handle; //0x0058
    };
}
