#pragma once
#include "../math/vec3.hpp"
#include "..\..\utils\memory_holder.hpp"

// TODO: update this class
namespace sdk::game {
    enum class ECollisionFlag : uintptr_t {
        none          = 0,
        grass         = 1,
        wall          = 2,
        unknown_1     = 4,
        unknown_2     = 8,
        unknown_3     = 16,
        unknown_4     = 32,
        building      = 64,
        river         = 128,
        global_vision = 256,
        unknown_5     = 512,
        unknown_6     = 1024, 
        unknown_7     = 2048,
        unknown_8     = 4096,
    };

    class Navgrid {
    public:
        [[nodiscard]] auto get_collision( const sdk::math::Vec3& position ) const -> ECollisionFlag{
            // return
            return ECollisionFlag::none;
            if ( x_cell_count == 0 || y_cell_count == 0 ) return ECollisionFlag::none;
            const auto x_cell = std::clamp(
                static_cast< int >( ( position.x - min_grid_pos.x ) * cell_size ),
                0,
                x_cell_count - 1
            ); 
            const auto y_cell = std::clamp(
                static_cast< int >( ( position.z - min_grid_pos.z ) * cell_size ),
                0,
                y_cell_count - 1
            );

            auto collision = app->memory->read< ECollisionFlag >(
                collision_map + 16 * ( x_cell + y_cell * x_cell_count ) + 8
            ); 

            if ( !collision.has_value( ) ) return ECollisionFlag::none;

            return *collision;
        }

#if enable_lua
        auto lua_is_wall( sol::object position ) const -> sol::object;
        auto lua_is_bush( sol::object position ) const -> sol::object;
        auto lua_is_river( sol::object position ) const -> sol::object;
        auto lua_is_building( sol::object position ) const -> sol::object;
#endif

        [[nodiscard]] auto is_wall( const sdk::math::Vec3& position ) const -> bool{
            return ( static_cast< uintptr_t >( get_collision( position ) ) & static_cast< uintptr_t >(
                ECollisionFlag::wall ) ) != 0;
        }

        [[nodiscard]] auto is_bush( const sdk::math::Vec3& position ) const -> bool{
            return ( static_cast< uintptr_t >( get_collision( position ) ) & static_cast< uintptr_t >(
                ECollisionFlag::grass ) ) != 0;
        }

        [[nodiscard]] auto is_river( const sdk::math::Vec3& position ) const -> bool{
            return ( static_cast< uintptr_t >( get_collision( position ) ) & static_cast< uintptr_t >(
                ECollisionFlag::river ) ) != 0;
        }

        [[nodiscard]] auto is_building( const sdk::math::Vec3& position ) const -> bool{
            return ( static_cast< uintptr_t >( get_collision( position ) ) & static_cast< uintptr_t >(
                ECollisionFlag::building ) ) != 0;
        }

        [[nodiscard]] auto get_height( const math::Vec3& position ) const -> float{
            return 0.f;
            // if ( max_grid_pos.x < position.x || min_grid_pos.x > position.x ||
            //     max_grid_pos.z < position.z || min_grid_pos.z > position.z )
            //     return 0.f;
            //
            // int   v7; // ebp
            // int   v8; // esi
            // int   v9; // edx
            // int   v10; // edi
            // int   v11; // eax
            // float v12; // xmm2_4
            // bool  v13; // cc
            // int   v14; // ebx
            // int   v15; // edx
            // float v16; // xmm3_4
            // int   v17; // eax
            // int   v18; // esi
            // int   v19; // eax
            // int   v20; // ebp
            // int   v21; // eax
            // int   v22; // edx
            // int   v23; // edi
            // int   v24; // ebx
            // int   v26; // [esp+0h] [ebp-4h]
            // int   v27; // [esp+8h] [ebp+4h]
            // int   v28;
            //
            // const auto y_sampled_height = *reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( this ) + 0x114 +
            //     24 );
            // const auto x_sampled_height = *reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( this ) + 0x110 +
            //     24 );
            //
            // const auto x_sampled_height_count = *reinterpret_cast< int* >( reinterpret_cast< uintptr_t >( this ) +
            //     0x108 + 24 );
            // const auto y_sampled_height_count = *reinterpret_cast< int* >( reinterpret_cast< uintptr_t >( this ) +
            //     0x10C + 24 );
            //
            // const float v5 = ( position.z - min_grid_pos.z ) / y_sampled_height;
            // const float v6 = ( position.x - min_grid_pos.x ) / x_sampled_height;
            // v7             = ( int )v6;
            // v8             = ( int )v5;
            // v28            = ( int )v5;
            // v9             = x_sampled_height_count - 1;
            // v27            = ( int )v5;
            // v10            = ( int )v6;
            // v11            = ( int )v6;
            //
            // if ( ( int )v6 >= v9 ) {
            //     --v10;
            //     v12 = 1.0f;
            // } else v12 = v6 - ( float )v10;
            //
            // v13 = v11 < v9;
            // v14 = v11 + 1;
            // v15 = y_sampled_height_count;
            //
            // if ( !v13 ) v14 = v7;
            //
            // if ( v8 >= v15 - 1 ) {
            //     --v8;
            //     v16 = 1.0f;
            // } else v16 = v5 - ( float )v8;
            //
            // v17 = v8 * x_sampled_height_count;
            // v26 = v17 + v10;
            // v18 = v17 + v14;
            // v19 = v27 + 1;
            // v20 = ( DWORD )x_sampled_height_count;
            //
            // if ( v27 >= v15 - 1 ) v19 = v28;
            //
            // v21 = v20 * v19;
            // v22 = v20 * v15;
            // v23 = v21 + v10;
            // v24 = v21 + v14;
            //
            //
            // if ( v26 >= v22 || v18 >= v22 || v23 >= v22 || v24 >= v22 ) return 0.f;
            //
            // return ( ( ( ( ( 1.0f - v12 ) * app->memory->read< float >( sampled_heights + 4 * v26 ) ) +
            //         ( v12 * app->memory->read< float >( sampled_heights + 4 * v26 ) ) ) * ( 1.0f - v16 ) )
            //     + ( ( ( ( 1.0f - v12 ) * app->memory->read< float >( sampled_heights + 4 * v26 ) )
            //         + ( v12 * app->memory->read< float >( sampled_heights + 4 * v26 ) ) ) * v16 ) );
        }


        char pad_0000[220]; //0x0000
        math::Vec3 min_grid_pos; //0x00DC
        math::Vec3 max_grid_pos; //0x00E8
        char pad_00F4[12]; //0x00F4
        uint64_t collision_map; //0x0100
        char pad_0108[16]; //0x0108
        uint64_t sampled_heights; //0x0118
        char pad_0120[1480]; //0x0120
        int32_t x_cell_count;
        int32_t y_cell_count; //0x06AC
        char pad_06B0[4]; //0x06B0
        float cell_size; //0x06B4
        char pad_06B8[64]; //0x06B8



    };
}
