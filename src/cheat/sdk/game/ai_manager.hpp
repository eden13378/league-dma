#pragma once
#include <cstdint>
#include "..\..\utils\memory_holder.hpp"
#include <vector>

#include "../math/vec3.hpp"

namespace sdk::game {
    class AiManager {
    public:
        auto get_path( ) const -> std::vector< math::Vec3 >{
            const auto ptr = *reinterpret_cast< uintptr_t* >( reinterpret_cast< uintptr_t >( this ) + 0x2E8 + 32 );
            if ( !ptr ) return { };

            std::vector< math::Vec3 > nodes{ };

            for ( auto i = 0u; i < path_node_count; i++ ) {
                auto temp = app->memory->read< math::Vec3 >( ptr + static_cast< uintptr_t >( 0xC ) * i );
                if(!temp.has_value(  )) break;

                if ( temp->length( ) <= 0.f ) break;

                nodes.push_back( *temp );

                //if ( i >= path_node_count ) break;
            }

            return nodes;
        }

#if enable_new_lua
        auto get_path_table( ) const -> sol::as_table_t< std::vector< math::Vec3 > >{
            return sol::as_table( get_path( ) );
        }
#endif

        [[nodiscard]] auto get_path_end_position( ) const -> const Vec3&{
            return path_end;
            //return *reinterpret_cast< math::Vec3 * >( reinterpret_cast< uintptr_t >( this ) + 0x14 );
        }

        [[nodiscard]] auto get_velocity( ) const -> const math::Vec3& {
            return *reinterpret_cast<Vec3 *>(reinterpret_cast<intptr_t>(this) + 0x418 + 32);
        }

        auto get_server_position() const -> const math::Vec3 {
            return *reinterpret_cast<Vec3 *>(reinterpret_cast<intptr_t>(this) + 0x434);
        }

        char       pad_0000[ 704 ];    // 0x0000
        uint64_t   nav_grid_ptr;       // 0x02A0
        char       pad_02A8[ 20 ];     // 0x02A8
        bool       is_moving;          // 0x02BC
        char       pad_02BD[ 3 ];      // 0x02BD
        int32_t    next_path_node;     // 0x02C0
        char       pad_02C4[ 12 ];     // 0x02C4
        math::Vec3 path_start;         // 0x02D0
        math::Vec3 path_end;           // 0x02DC
        char       pad_02E8[ 8 ];      // 0x02E8
        uint32_t   path_node_count;    // 0x02F0
        char       pad_02F4[ 12 ];     // 0x02F4
        float      dash_speed;         // 0x0300
        char       pad_0304[ 32 ];     // 0x0304
        bool       is_dashing;         // 0x0324
        char       pad_0321[ 251 ];    // 0x0321
        math::Vec3 velocity;           // 0x0418
        char       pad_0424[ 236 ];    // 0x0424
        bool       has_speed_increase; // 0x0510
    };
}
