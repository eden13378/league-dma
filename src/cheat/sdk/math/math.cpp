#include "pch.hpp"

#include "math.hpp"

#include "geometry.hpp"
#include "../globals.hpp"
#include "../../lua-v2/lua_def.hpp"
#include "../game/render_manager.hpp"
#include "../game/tactical_map.hpp"
#if enable_lua
#include <sol/sol.hpp>
#endif
#pragma comment(lib, "d3dx9.lib")


auto sdk::math::world_to_screen( const Vec3& origin, Vec2& screen ) -> bool{
    if ( !g_render_manager.is_valid( ) ) return false;

    const auto screen_size = g_render->get_screensize( );

    static D3DXMATRIX matrix    = { };
    static auto       last_time = std::chrono::steady_clock::now( );
    // const auto matrix = g_render_manager->get_valid_view_projection_matrix( );

    const auto delta = std::chrono::duration_cast< std::chrono::microseconds >(
        std::chrono::steady_clock::now( ).time_since_epoch( ) - last_time.time_since_epoch( )
    );

    if ( delta >= std::chrono::microseconds( 750 ) ) {
        matrix    = g_render_manager->get_valid_view_projection_matrix( );
        last_time = std::chrono::steady_clock::now( );
    }

    const auto z = matrix.m[ 0 ][ 3 ] * origin.x +
        matrix.m[ 1 ][ 3 ] * origin.y +
        matrix.m[ 2 ][ 3 ] * origin.z +
        matrix.m[ 3 ][ 3 ];

    if ( z < 0.1f ) return false;

    const auto cw = static_cast< float >( screen_size.x ) * .5f;
    const auto x  = matrix.m[ 0 ][ 0 ] * origin.x +
        matrix.m[ 1 ][ 0 ] * origin.y +
        matrix.m[ 2 ][ 0 ] * origin.z +
        matrix.m[ 3 ][ 0 ];

    screen.x = cw + cw * x / z;
    if ( screen.x < 0.f || screen.x > screen_size.x ) return false;

    const auto ch = static_cast< float >( screen_size.y ) * .5f;
    const auto y  = matrix.m[ 0 ][ 1 ] * origin.x +
        matrix.m[ 1 ][ 1 ] * origin.y +
        matrix.m[ 2 ][ 1 ] * origin.z +
        matrix.m[ 3 ][ 1 ];

    screen.y = ch - ch * y / z;
    if ( screen.y < 0.f || screen.y > screen_size.y ) return false;

    return true;
}

auto sdk::math::world_to_minimap( const Vec3 position ) -> Vec2{
    if ( !g_minimap ) return { };

    const auto world_scale = g_minimap->world_scale;
    const auto map_area    = g_minimap->map_area;

    const Vec2 origin{ g_minimap->top_left_clipping.x, g_minimap->top_left_clipping.y + map_area.y };
    const Vec2 multiplier = { position.x / world_scale.x, position.z / world_scale.y };

    auto result = Vec2{
        origin.x + g_minimap->map_area.x * multiplier.x,
        origin.y - g_minimap->map_area.y * multiplier.y
    };

    if ( result.x < g_minimap->top_left_clipping.x ) result.x = g_minimap->top_left_clipping.x;
    else if ( result.x > g_minimap->top_left_clipping.x + map_area.x ) {
        result.x = g_minimap->top_left_clipping.x +
            map_area.x;
    }

    if ( result.y < g_minimap->top_left_clipping.y ) result.y = g_minimap->top_left_clipping.y;
    else if ( result.y > g_minimap->top_left_clipping.y + map_area.y ) {
        result.y = g_minimap->top_left_clipping.y +
            map_area.y;
    }

    return result;
}

namespace sdk::math {
#if enable_lua
    // ReSharper disable once CppPassValueParameterByConstReference
    auto Polygon::add_lua( const sol::object point ) -> void{
        lua_arg_check_v( point, Vec3 )

        add( point.as< Vec3 >( ) );
    }

    // ReSharper disable once CppPassValueParameterByConstReference
    auto Polygon::point_in_polygon_lua( const sol::object point ) -> void{
        lua_arg_check_v( point, Vec3 )

        point_in_polygon( point.as< Vec3 >( ) );
    }
#endif
}
