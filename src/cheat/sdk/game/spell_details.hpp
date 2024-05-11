#pragma once
#include <cstdint>
#include "..\..\utils\memory_holder.hpp"
#include "../math/vec3.hpp"

namespace sdk::game {
    class SpellDetails {
    public:
        SpellDetails( ) = default; // TODO: update this!!

        char            pad_0000[ 0x18 ]; // 0x0000
        sdk::math::Vec3 last_start_position; // 0x0018
        sdk::math::Vec3 last_unknown_position; // 0x001C
        sdk::math::Vec3 last_end_position; // 0x0028
        char            pad_0028[ 4 ]; // 0x002C
    };
} // namespace sdk::game
