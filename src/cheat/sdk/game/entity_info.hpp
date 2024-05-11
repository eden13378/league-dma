#pragma once
#include <cstdint>
// #include "../../utils/c_memory_holder.hpp"
#include "../math/vec3.hpp"

namespace sdk::game {
    class EntityInfo {
    public:
        EntityInfo( ) = default; // TODO: update this!!

        auto get_cast_position( ) const -> const math::Vec3&{
            return *reinterpret_cast< math::Vec3* >( reinterpret_cast< uintptr_t >( this ) + 0x100 );
        }

        char pad_0000[ 0x200 ];      // 0x0000
    };
} // namespace sdk::game
