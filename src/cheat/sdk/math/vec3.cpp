#include "pch.hpp"

#include "vec3.hpp"

#include "math.hpp"
#include "vec2.hpp"
#if enable_new_lua
#include "../../lua-v2/state.hpp"

#include "../../lua-v2/lua_def.hpp"
#endif
#if enable_lua
#include <sol/sol.hpp>
#endif

namespace sdk::math {
    auto Vec3::to_screen( ) const -> std::optional< Vec2 >{
        auto screen = Vec2( );
        if ( math::world_to_screen( *this, screen ) ) return screen;
        return std::nullopt;
    }

    auto Vec3::to_minimap( ) const -> std::optional< Vec2 >{
        auto screen = world_to_minimap( *this );
        if ( screen.length( ) <= 0.f ) return std::nullopt;

        return screen;
    }

#if enable_lua
    auto Vec3::to_minimap_lua( ) const -> sol::object{
        const auto screen = to_minimap( );
        if ( !screen ) return sol::nil;

        return sol::make_object( g_lua_state2, *screen );
    }
#endif
#if enable_new_lua
    with_lua(
        auto Vec3::to_screen_lua( ) const -> sol::object {
        // return sol::nil;
        const auto result = to_screen();
        if( !result ) return sol::nil;
 
        return sol::make_object( g_lua_state2, *result );
        }
    )
#endif

    auto Vec3::project_on( const Vec3& segment_start, const Vec3& segment_end ) const -> projection_info{
        float      rs;
        const auto cx = x;
        const auto cy = z;
        const auto ax = segment_start.x;
        const auto ay = segment_start.z;
        const auto bx = segment_end.x;
        const auto by = segment_end.z;

        const auto rl = static_cast< float >( ( cx - ax ) * ( bx - ax ) + ( cy - ay ) * ( by - ay ) ) / static_cast<
            float >( pow( bx - ax, 2 ) + pow( by - ay, 2 ) );
        const auto point_line = Vec3( ax + rl * ( bx - ax ), 0, ay + rl * ( by - ay ) );

        if ( rl < 0 ) rs = 0;
        else if ( rl > 1 ) rs = 1;
        else rs               = rl;

        const auto is_on_segment = rs == rl;
        const auto point_segment = is_on_segment ?
                                       point_line :
                                       Vec3( ax + rs * ( bx - ax ), 0, ay + rs * ( by - ay ) );

        return { is_on_segment, point_segment, point_line };
    }

    projection_info::projection_info( const bool is_on_segment, const Vec3& segment_point, const Vec3& line_point ) :
        is_on_segment( is_on_segment ), line_point( line_point ), segment_point( segment_point ){
    }
}
