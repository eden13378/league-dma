#include "pch.hpp"

#include "c_renderer.hpp"

#include <regex>

#if enable_new_lua
#include "../lua-v2/lua_def.hpp"
#include "../lua-v2/state.hpp"
#endif
#include "../utils/web_client.hpp"
#if enable_lua
#include "../include/sol/sol.hpp"
#endif

#include "../config/c_config.hpp"

namespace renderer {
    auto Renderer::update_draw_list( ImDrawList* list ) -> void{ m_draw_list = list; }

    auto Renderer::on_draw( ) const -> void{
        for ( auto& callback : m_callbacks ) callback( );
        for ( auto& callback : m_post_callbacks ) callback( );
    }

    auto Renderer::add_draw_callback( const std::function< void( ) >& callback ) -> void{
        m_callbacks.push_back( callback );
    }

    auto Renderer::add_post_draw_callback( const std::function< void( ) >& callback ) -> void{
        m_post_callbacks.push_back( callback );
    }

    auto Renderer::line( Vec2 start, Vec2 end, const Color& color, float thicknesses ) const noexcept -> void{
        const ImVec2 line_begin{ start.x, start.y };
        const ImVec2 line_end{ end.x, end.y };

        // fixes small undrawn pixels in circle_3d
        if ( color.a >= 250 ) {
            const ImVec2 fill_end = {
                start.extend( end, start.dist_to( end ) + 1 ).x,
                start.extend( end, start.dist_to( end ) + 1 ).y
            };

            m_draw_list->AddLine( line_end, fill_end, get_u32( color ), thicknesses > 1.f ? thicknesses - 1.f : 1.f );
        }

        m_draw_list->AddLine(
            line_begin,
            line_end,
            get_u32( color ),
            thicknesses
        );
    }

    auto Renderer::blur_line(Vec2 start, Vec2 end, const Color &color, float thicknesses, int blur_layers, float layer_size) const noexcept -> void
    {
        for (int o = blur_layers; o >= 0; o--) {

            int opacity_step = static_cast<int>(std::floor(60.f / static_cast<float>(blur_layers)));
            int opacity      = o == 0 ? 255 : 60 - opacity_step * o;

            float max_blur = layer_size * o;
            float stroke   = o == 0 ? thicknesses : thicknesses + max_blur;

            auto current_color = Color(color.r, color.g, color.b, opacity);
            g_render->line(start, end, current_color, stroke);
            //if (o == 0) g_render->filled_circle(end, current_color, thicknesses, 16);
        }
    }


#if enable_lua
    auto Renderer::line_lua(
        // ReSharper disable CppPassValueParameterByConstReference
        const sol::object start,
        const sol::object end,
        const sol::object clr,
        const sol::object thicknesses
        // ReSharper restore CppPassValueParameterByConstReference
    ) const -> void{
        lua_arg_check_v( start, Vec2 )
        lua_arg_check_v( end, Vec2 )
        lua_arg_check_v( clr, Color )
        lua_arg_check_ct_v( thicknesses, float, "number" )

        line( start.as< Vec2 >( ), end.as< Vec2 >( ), clr.as< Color >( ), thicknesses.as< float >( ) );
    }
#endif

    auto Renderer::gradient(
        Vec2         position,
        const Vec2   size,
        const Color& color_top_left,
        const Color& color_top_right,
        const Color& color_bottom_right,
        const Color& color_bottom_left
    ) const noexcept -> void{
        m_draw_list->AddRectFilledMultiColor(
            {
                ( position.x ),
                ( position.y )
            },
            {
                position.x + size.x,
                position.y + size.y
            },
            get_u32( color_top_left ),
            get_u32( color_top_right ),
            get_u32( color_bottom_right ),
            get_u32( color_bottom_left )
        );
    }

    auto Renderer::box(
        Vec2         position,
        Vec2         size,
        const Color& color,
        float        rounding,
        float        thickness
    ) const noexcept -> void{
        m_draw_list->AddRect(
            {
                ( position.x ),
                ( position.y )
            },
            {
                position.x + size.x,
                position.y + size.y
            },
            get_u32( color ),
            std::clamp( rounding, -1.f, std::min( std::abs( size.x ), std::abs( size.y ) ) / 4.f ),
            rounding <= 0.f ? ImDrawFlags_RoundCornersNone : 0,
            thickness
        );
    }
#if enable_lua
    auto Renderer::box_lua(
        // ReSharper disable CppPassValueParameterByConstReference
        const sol::object position,
        const sol::object size,
        const sol::object clr,
        const sol::object rounding,
        const sol::object thickness
        // ReSharper restore CppPassValueParameterByConstReference
    ) const -> void{
        lua_arg_check_v( position, Vec2 )
        lua_arg_check_v( size, Vec2 )
        lua_arg_check_v( clr, Color )
        if ( rounding.get_type( ) != sol::type::nil )
            lua_arg_check_ct_v( rounding, float, "number" )
        if ( thickness.get_type( ) != sol::type::nil )
            lua_arg_check_ct_v( thickness, float, "number" )


        box(
            position.as< Vec2 >( ),
            size.as< Vec2 >( ),
            clr.as< Color >( ),
            rounding.get_type( ) == sol::type::nil ? -1.f : rounding.as< float >( ),
            thickness.get_type( ) == sol::type::nil ? 1.f : thickness.as< float >( )
        );
    }
#endif

    auto Renderer::filled_box(
        const Vec2   position,
        const Vec2   size,
        const Color& color,
        const float  rounding
    ) const noexcept -> void{
        m_draw_list->AddRectFilled(
            {
                static_cast< float >( position.x ),
                static_cast< float >( position.y )
            },
            {
                position.x + size.x,
                position.y + size.y
            },
            get_u32( color ),
            rounding,
            rounding <= 0.f ? ImDrawFlags_RoundCornersNone : 0
        );
    }

#if enable_lua
    auto Renderer::filled_box_lua(
        // ReSharper disable CppPassValueParameterByConstReference
        const sol::object position,
        const sol::object size,
        const sol::object clr,
        const sol::object rounding
        // ReSharper restore CppPassValueParameterByConstReference
    ) const -> void{
        lua_arg_check_v( position, Vec2 )
        lua_arg_check_v( size, Vec2 )
        lua_arg_check_v( clr, Color )
        if ( rounding.get_type( ) != sol::type::nil )
            lua_arg_check_ct_v( rounding, float, "number" )


        filled_box(
            position.as< Vec2 >( ),
            size.as< Vec2 >( ),
            clr.as< Color >( ),
            rounding.get_type( ) == sol::type::nil ? 0.f : rounding.as< float >( )
        );
    }
#endif

    auto Renderer::set_clip_box( const Vec2 start, const Vec2 size ) const noexcept -> void{
        m_draw_list->PushClipRect( { start.x, start.y }, { start.x + size.x, start.y + size.y } );
    }

    auto Renderer::reset_clip_box( ) const noexcept -> void{ m_draw_list->PopClipRect( ); }

    auto Renderer::circle(
        Vec2         position,
        const Color& color,
        float        radius,
        int          segments,
        float        thickness
    ) const noexcept -> void{
        m_draw_list->AddCircle(
            {
                position.x,
                position.y
            },
            radius,
            get_u32( color ),
            segments,
            thickness
        );
    }
#if enable_lua
    auto Renderer::circle_lua(
        // ReSharper disable CppPassValueParameterByConstReference
        const sol::object position,
        const sol::object clr,
        const sol::object radius,
        const sol::object segments,
        const sol::object thickness
        // ReSharper restore CppPassValueParameterByConstReference

    ) const -> void{
        lua_arg_check_v( position, Vec2 )
        lua_arg_check_v( clr, Color )
        lua_arg_check_ct_v( radius, float, "number" )
        if ( segments.get_type( ) != sol::type::nil )
            lua_arg_check_ct_v( segments, int, "number" )
        if ( thickness.get_type( ) != sol::type::nil )
            lua_arg_check_ct_v( thickness, int, "number" )

        circle(
            position.as< Vec2 >( ),
            clr.as< Color >( ),
            radius.as< float >( ),
            segments.get_type( ) == sol::type::nil ? 15 : segments.as< int >( ),
            thickness.get_type( ) == sol::type::nil ? 1.f : thickness.as< float >( )
        );
    }
#endif
    auto Renderer::triangle(
        Vec2         left,
        Vec2         right,
        Vec2         bottom,
        const Color& color,
        float        thickness
    ) const noexcept -> void{
        m_draw_list->AddTriangle(
            { left.x, left.y },
            { right.x, right.y },
            { bottom.x, bottom.y },
            get_u32( color ),
            thickness
        );
    }

    auto Renderer::sector(
        const sdk::math::Sector& sector,
        const Color&             color,
        float                    segments,
        float                    thickness
    ) const noexcept -> void{
        auto dx = sector.direction.x - sector.center.x;
        auto dz = sector.direction.z - sector.center.z;

        debug_log( "offset [{}, {}]", dx, dz );

        // (624.9607 / 650) * 57.2957795131
        // 1.29232946 * 57.2957795131

        debug_log(
            "FABZ(DZ) = {}, RADIUS = {}, DZ/RADIUS = {}, ASIN = {}",
            fabs(dz),
            sector.radius,
            (fabs(dz) / sector.radius),
            asin(fabs(dz) / sector.radius)
        );
        auto angle_deg = asin( fabs( dz ) / sector.radius ) * ( 180.f / m_pi );

        debug_log( "DEFAULT ANGLE_DEG: {}", angle_deg );
        debug_log( "PATCHED ANGLE DEG: {}", angle_deg + 180.f );

        if ( dx >= 0.f ) {
            if ( dz < 0.f ) {
                // +, -
                angle_deg = 90.f + angle_deg;
            } else {
                // +, +
                angle_deg = 90.f - angle_deg;
            }
        } else {
            if ( dz >= 0.f ) {
                // -, +
                angle_deg = 270.f + angle_deg;
            } else {
                // -, -
                angle_deg = 270.f - angle_deg;
            }
        }

        debug_log( "HACKED ANGLE_DEG: {}", angle_deg );
        debug_log( "HACKED + PATCHED ANGLE_DEG: {}", angle_deg );

        debug_log( "Angle is: {}", angle_deg );

        auto min_angle_deg = angle_deg - ( sector.angle / 2.f );

        debug_log( "Min angle is: {}, which is angle_deg - half of angle {}", min_angle_deg, sector.angle );

        Vec2 origin;
        sdk::math::world_to_screen( sector.center, origin );

        Vec2 target;
        sdk::math::world_to_screen( sector.center.extend( sector.direction, 600.f ), target );

        const Vec2 square_corner{ target.x, origin.y };

        triangle( origin, target, square_corner, Color::red( ), 1.f );
        //line(origin, target, color::red(), 1.f);

        auto angle_step = sector.angle / segments;

        debug_log( "Angle step is {} which is angle ({}) / segments ({})", angle_step, sector.angle, segments );
        auto last_screen_pos = origin;
        for ( float i = 0; i <= segments; i++ ) {
            const auto angle_rad = ( min_angle_deg + ( angle_step * i ) ) * ( m_pi / 180.f );
            debug_log( "Angle of step {} is {}", i, min_angle_deg + angle_step * i );
            Vec2       segment_position;
            const Vec3 segment_position_game = {
                sector.center.x + sin( angle_rad ) * sector.radius,
                sector.center.y,
                sector.center.z + cos( angle_rad ) * sector.radius
            };

            sdk::math::world_to_screen( segment_position_game, segment_position );
            line( last_screen_pos, segment_position, color, thickness );
            last_screen_pos = segment_position;
        }

        line( last_screen_pos, origin, color, thickness );
    }

    auto Renderer::poly_line(
        const ImVec2* points,
        size_t        size,
        const Color&  color,
        float         thickness
    ) const noexcept -> void{
        m_draw_list->AddPolyline(
            points,
            size,
            get_u32( color ),
            ImDrawFlags_None,
            thickness
        );
    }

    auto Renderer::polygon(
        const sdk::math::Polygon& polygon,
        const Color&              color,
        float                     thickness
    ) const noexcept -> void{
        for ( unsigned int i = 0; i < polygon.points.size( ); i++ ) {
            Vec2 p1{ };
            Vec2 p2{ };

            sdk::math::world_to_screen( polygon.points.at( i ), p1 );

            if ( i + 1 != polygon.points.size( ) ) sdk::math::world_to_screen( polygon.points.at( i + 1 ), p2 );
            else sdk::math::world_to_screen( polygon.points.at( 0 ), p2 );
            line( p1, p2, color, thickness );
        }
    }

    auto Renderer::filled_triangle( Vec2 left, Vec2 right, Vec2 bottom, const Color& color ) const noexcept -> void{
        m_draw_list->AddTriangleFilled(
            { left.x, left.y },
            { right.x, right.y },
            { bottom.x, bottom.y },
            get_u32( color )
        );
    }

    auto Renderer::filled_circle(
        Vec2         position,
        const Color& color,
        float        radius,
        int          segments
    ) const noexcept -> void{
        m_draw_list->AddCircleFilled(
            {
                position.x,
                position.y
            },
            radius,
            get_u32( color ),
            segments
        );
    }

    auto Renderer::get_3d_circle_points(
        const Vec3& position,
        float       radius,
        int         segments,
        float       draw_angle,
        Vec3        circle_direction
    ) const noexcept -> std::vector< Vec3 >{
        const auto angle = draw_angle / static_cast< float >( segments );
        if ( circle_direction.length( ) == 0.f ) circle_direction = Vec3( 0, 0, 1 );

        const auto start_direction  = position + circle_direction;
        const auto circle_start_pos = position.extend( start_direction, radius );

        auto last_point = circle_start_pos;

        auto rotate_vec = []( Vec3 point, Vec3 center, float angle )-> Vec3{
            const auto pi      = 2 * std::acos( 0.f );
            const auto c_angle = angle * ( pi / 180.f );

            const auto rotated_x = std::cos( c_angle ) * ( point.x - center.x ) - std::sin( c_angle ) * ( point.z -
                center.z ) + center.x;
            const auto rotated_z = std::sin( c_angle ) * ( point.x - center.x ) + std::cos( c_angle ) * ( point.z -
                center.z ) + center.z;

            return Vec3{ rotated_x, center.y, rotated_z };
        };

        std::vector< Vec3 > circle_points{ };

        for ( auto i = 0; i <= segments; i++ ) {
            const auto new_point = rotate_vec( last_point, position, angle );
            last_point           = new_point;
            circle_points.push_back( new_point );
        }

        return circle_points;
    }

    auto Renderer::line_3d( const Vec3& start, const Vec3& end, Color color, float thickness ) const noexcept -> void{
        Vec2       sp_start, sp_end;
        const auto on_screen_start = world_to_screen( start, sp_start );
        const auto on_screen_end   = world_to_screen( end, sp_end );

        if ( on_screen_start && on_screen_end ) line( sp_start, sp_end, color, thickness );
        else {
            if ( !on_screen_start && !on_screen_end ) {
                /*
                    const auto delta = end - start;
                    const auto middle = start + ( delta / 2.f );

                    vec2 middle_on_screen;
                    if ( !world_to_screen( middle, middle_on_screen ) ) return;
                    */
                return;
            }

            if ( on_screen_start ) {
                const auto delta      = end - start;
                const auto length     = delta.length( );
                const auto normalized = delta / length;

                if ( world_to_screen( start + normalized * 10.f, sp_end ) ) {
                    line(
                        sp_start,
                        sp_start + ( ( sp_end - sp_start ) * ( length / 7.f ) ),
                        color,
                        thickness
                    );
                }
            } else {
                const auto delta  = start - end;
                const auto length = delta.length( );

                const auto normalized = delta / length;
                if ( world_to_screen( end + normalized * 10.f, sp_start ) ) {
                    line(
                        sp_end,
                        sp_end + ( ( sp_start - sp_end ) * ( length / 7.f ) ),
                        color,
                        thickness
                    );
                }
            }
        }
    }

    auto Renderer::circle_3d(
        const Vec3& position,
        Color       color,
        const float radius,
        int32_t     flags,
        int32_t     segments,
        const float thickness,
        float       angle,
        Vec3        direction
    ) const noexcept -> void{
        if ( segments == -1 ) segments = static_cast< int32_t >( radius / 5.f );

        segments = std::clamp( segments, 10, 40 );

        const auto circle_points = get_3d_circle_points( position, radius, segments, angle, direction );

        std::vector< std::vector< ImVec2 > > points;
        points.emplace_back( );

        if ( flags & filled ) {
            Vec2 sp1;
            for ( auto i = 0; i <= segments; ++i ) {
                if ( !world_to_screen( circle_points[ i ], sp1 ) ) {
                    points.emplace_back( );
                    continue;
                }

                points[ points.size( ) - 1u ].emplace_back( sp1.x, sp1.y );
            }
        }

        if ( flags & outline ) {
            std::optional< Vec3 > last_position;
            for ( auto i = 0; i <= segments; ++i ) {
                if ( !last_position ) {
                    last_position = circle_points[ i ];
                    continue;
                }

                line_3d( *last_position, circle_points[ i ], color, thickness );
                last_position = circle_points[ i ];
            }
        }

        if ( flags & filled ) {
            for ( auto p : points ) {
                m_draw_list->AddConvexPolyFilled(
                    p.data( ),
                    p.size( ),
                    get_u32( color )
                );
            }
        }

        if ( flags & outline ) {
            for ( auto p : points ) {
                m_draw_list->AddPolyline(
                    p.data( ),
                    p.size( ),
                    ( flags & filled ) ? get_u32( color.alpha( 255 ) ) : get_u32( color ),
                    ImDrawFlags_None,
                    thickness
                );
            }
        }

        // draw_ellipse( position, 550, 4, 2.f );
    }

    auto Renderer::draw_ellipse(
        const Vec3& position,
        float       radius,
        int         segments,
        float       draw_angle
    ) const noexcept -> void{
        auto s = get_3d_circle_points( position, radius, 4, 360.f, Vec3( 0, 0, 1 ) );

        auto first  = s[ 0 ].to_screen( );
        auto second = s[ 1 ].to_screen( );
        auto third  = s[ 2 ].to_screen( );
        auto fourth = s[ 3 ].to_screen( );
        //
        // for ( auto& i : s ) {
        //     auto screen = i.to_screen( );<
        //     if ( !screen ) continue;
        //     circle( *screen, Color::white( ), 5.f );
        // }

        if ( !first || !second || !third || !fourth ) return;

        auto x_radius = first->dist_to( *third ) / 2.f;
        auto y_radius = second->dist_to( *fourth ) / 2.f;

        auto center = position.to_screen( );
        if ( !center ) return;

        std::pair< int, int > lastPoint = { center->x + x_radius, center->y };
        const float           PI        = 3.14159265f;
        for ( double theta = 0; theta <= 2 * PI; theta += 0.01 ) {
            int                   x     = center->x + x_radius * cos( theta );
            int                   y     = center->y + y_radius * sin( theta );
            std::pair< int, int > point = { x, y };
            line(
                { static_cast< float >( lastPoint.first ), static_cast< float >( lastPoint.second ) },
                {
                    static_cast
                    < float >( point.first ),
                    static_cast< float >( point.second )
                },
                Color::white( ),
                1.f
            );
            lastPoint = point;
        }

        line(
            { center->x - ( x_radius / 2.f ), center->y },
            { center->x + ( x_radius / 2.f ), center->y },
            Color::white( ),
            1.f
        );

        line(
            { center->x, center->y - ( y_radius / 2.f ) },
            { center->x, center->y + ( y_radius / 2.f ) },
            Color::white( ),
            2.f
        );
    }
#if enable_lua
    auto Renderer::triangle_lua(
        // ReSharper disable CppPassValueParameterByConstReference
        const sol::object left,
        const sol::object right,
        const sol::object bottom,
        const sol::object clr,
        const sol::object thickness
        // ReSharper restore CppPassValueParameterByConstReference

    ) const -> void{
        lua_arg_check_v( left, Vec2 )
        lua_arg_check_v( right, Vec2 )
        lua_arg_check_v( bottom, Vec2 )
        lua_arg_check_v( clr, Color )
        if ( thickness.get_type( ) != sol::type::nil )
            lua_arg_check_ct_v( thickness, float, "number" )

        triangle(
            left.as< Vec2 >( ),
            right.as< Vec2 >( ),
            bottom.as< Vec2 >( ),
            clr.as< Color >( ),
            thickness.get_type( ) == sol::type::nil ? 1.f : thickness.as< float >( )
        );
    }

    auto Renderer::filled_triangle_lua(
        // ReSharper disable CppPassValueParameterByConstReference
        const sol::object left,
        const sol::object right,
        const sol::object bottom,
        const sol::object clr
        // ReSharper restore CppPassValueParameterByConstReference

    ) const -> void{
        lua_arg_check_v( left, Vec2 );
        lua_arg_check_v( right, Vec2 );
        lua_arg_check_v( bottom, Vec2 );
        lua_arg_check_v( clr, Color );

        filled_triangle( left.as< Vec2 >( ), right.as< Vec2 >( ), bottom.as< Vec2 >( ), clr.as< Color >( ) );
    }

    auto Renderer::filled_circle_lua(
        // ReSharper disable CppPassValueParameterByConstReference
        const sol::object position,
        const sol::object clr,
        const sol::object radius,
        const sol::object segments
        // ReSharper restore CppPassValueParameterByConstReference

    ) const -> void{
        lua_arg_check_v( position, Vec2 );
        lua_arg_check_v( clr, Color );
        lua_arg_check_v( radius, float );
        if ( segments.get_type( ) != sol::type::nil ) { lua_arg_check_v( segments, int ); }

        filled_circle(
            position.as< Vec2 >( ),
            clr.as< Color >( ),
            radius.as< float >( ),
            segments.get_type( ) == sol::type::nil ? 15 : segments.as< int >( )
        );
    }

    auto Renderer::lua_line_3d(
        // ReSharper disable CppPassValueParameterByConstReference
        const sol::object start,
        const sol::object end,
        const sol::object clr,
        const sol::object thickness
        // ReSharper restore CppPassValueParameterByConstReference

    ) const -> void{
        lua_arg_check_v( start, Vec3 );
        lua_arg_check_v( end, Vec3 );
        lua_arg_check_v( clr, Color );
        lua_arg_check_v( thickness, float );

        line_3d( start.as< Vec3 >( ), end.as< Vec3 >( ), clr.as< Color >( ), thickness.as< float >( ) );
    }

    auto Renderer::circle_3d_lua(
        // ReSharper disable CppPassValueParameterByConstReference
        const sol::object position,
        const sol::object clr,
        const sol::object radius,
        const sol::object flags,
        const sol::object segments,
        const sol::object thickness
        // ReSharper restore CppPassValueParameterByConstReference

    ) const -> void{
        lua_arg_check_v( position, Vec3 );
        lua_arg_check_v( clr, Color );
        lua_arg_check_ct_v( radius, float, "number" );
        if ( flags.get_type( ) != sol::type::nil ) { lua_arg_check_ct_v( flags, int32_t, "number" ); }
        if ( segments.get_type( ) != sol::type::nil ) { lua_arg_check_ct_v( segments, int32_t, "number" ); }
        if ( thickness.get_type( ) != sol::type::nil ) { lua_arg_check_ct_v( thickness, float, "number" ); }

        circle_3d(
            position.as< Vec3 >( ),
            clr.as< Color >( ),
            radius.as< float >( ),
            flags.get_type( ) == sol::type::nil ? outline : flags.as< int32_t >( ),
            segments.get_type( ) == sol::type::nil ? -1 : segments.as< int32_t >( ),
            thickness.
            get_type( ) == sol::type::nil
                ? 1.f
                : thickness.as< float >( )
        );
    }
#endif
    auto Renderer::circle_minimap(
        const Vec3& position,
        Color       color,
        const float radius,
        int32_t     segments,
        const float thickness
    ) const noexcept -> void{
        if ( segments == -1 ) segments = static_cast< int32_t >( radius / 5.f );

        const auto circle_points = get_3d_circle_points( position, radius, segments );

        for ( auto i = 0; i <= segments; i++ ) {
            const auto line_start = world_to_minimap( circle_points[ i == 0 ? 0 : i - 1 ] );
            const auto line_end   = world_to_minimap( circle_points[ i ] );

            line( line_start, line_end, color, thickness );
        }
    }

    auto Renderer::text(
        const Vec2    position,
        const Color&  color,
        const ImFont* font,
        const char*   text,
        float         size
    ) const noexcept -> void{
        if ( !font ) return;

        if ( !font->ContainerAtlas ) return;

        if ( size == -1.f ) size = font->FontSize;
        size *= g_config->misc.screen_scaling->get< float >( );

        m_draw_list->PushTextureID( font->ContainerAtlas->TexID );

        m_draw_list->AddText(
            font,
            size,
            { static_cast< float >( position.x ) + 0.8f, static_cast< float >( position.y ) },
            get_u32( color ),
            text
        );

        m_draw_list->PopTextureID( );
    }

    auto Renderer::text_3d(
        const Vec3&   position,
        const Color&  color,
        const ImFont* font,
        const char*   t,
        float         size,
        bool          shadow
    ) const noexcept -> void{
        Vec2 sp{ };
        if ( !world_to_screen( position, sp ) ) return;

        if ( shadow ) {
            text(
                sp + Vec2( 1, 1 ),
                { 0, 0, 0, static_cast< int32_t >( static_cast< float >( color.a ) / 255.f * 200.f ) },
                font,
                t,
                size
            );
        }
        text( sp, color, font, t, size );
    }

    auto Renderer::get_text_size( const std::string_view& text, ImFont* font, float text_size ) noexcept -> Vec2{
        if ( !font ) return { -1.f, -1.f };
        if ( text_size == -1.f ) text_size = font->FontSize;
        if ( !text.data( ) ) return { -1.f, -1.f };

        if ( g_config ) text_size *= g_config->misc.screen_scaling->get< float >( );

        const auto size = font->CalcTextSizeA( text_size, 1024, 0.f, text.data( ), text.data( ) + text.size( ) );

        return { size.x, size.y };
    }

    auto Renderer::text_shadow(
        Vec2         position,
        const Color& color,
        ImFont*      font,
        const char*  t,
        float        size
    ) const noexcept -> void{
        text(
            position + Vec2( 1, 1 ),
            { 0, 0, 0, static_cast< int32_t >( static_cast< float >( color.a ) / 255.f * 200.f ) },
            font,
            t,
            size
        );
        text( position, color, font, t, size );
    }

    auto Renderer::load_texture_from_memory(
        const std::vector< unsigned char >& data
    ) noexcept -> std::shared_ptr< Texture >{
        std::unique_lock lock( m_texture_mutex );

        auto texture = std::make_shared< Texture >( );
        if ( !overlay::load_texture_from_memory(
            data.data( ),
            data.size( ),
            &texture->texture,
            &texture->width,
            &texture->height
        ) )
            return nullptr;

        texture->name = _( "memory" );

        m_textures.push_back( texture );

        return texture;
    }

    auto Renderer::load_texture_from_memory( const unsigned* bytes, int size ) noexcept -> std::shared_ptr< Texture >{
        std::unique_lock lock( m_texture_mutex );

        auto texture = std::make_shared< Texture >( );
        if ( !overlay::load_texture_from_memory(
            ( unsigned char* )( bytes ),
            size,
            &texture->texture,
            &texture->width,
            &texture->height
        ) )
            return nullptr;

        texture->name = _( "memory" );

        m_textures.push_back( texture );

        return texture;
    }

    auto Renderer::load_texture_from_memory(
        const unsigned* bytes,
        int             size,
        hash_t          name
    ) noexcept -> std::shared_ptr< Texture >{
        std::unique_lock lock( m_texture_mutex );

        auto texture = std::make_shared< Texture >( );
        if ( !overlay::load_texture_from_memory(
            ( unsigned char* )( bytes ),
            size,
            &texture->texture,
            &texture->width,
            &texture->height
        ) )
            return nullptr;

        texture->name = _( "memory" );

        m_textures.push_back( texture );
        m_texture_cache[ name ] = texture;

        return texture;
    }

    auto Renderer::load_texture_from_file( const std::string_view file ) noexcept -> std::shared_ptr< Texture >{
        if ( file.empty( ) ) return nullptr;
        const auto hashed = rt_hash( file.data( ) );

        std::unique_lock lock( m_texture_mutex );

        const auto found = m_texture_cache.find( hashed );
        if ( found != m_texture_cache.end( ) ) return found->second;

        auto texture = std::make_shared< Texture >( );
        if ( !overlay::load_texture_from_file(
            file.data( ),
            &texture->texture,
            &texture->width,
            &texture->height
        ) )
            return nullptr;

        texture->name = file;

        m_textures.push_back( texture );
        m_texture_cache[ hashed ] = texture;

        return texture;
    }

    auto Renderer::image(
        const Vec2                        position,
        const Vec2                        size,
        const std::shared_ptr< Texture >& texture
    ) const noexcept -> std::expected< void, const char* >{ return image( position, size, texture.get( ) ); }

    auto Renderer::image(
        const Vec2     position,
        const Vec2     size,
        const Texture* texture
    ) const noexcept -> std::expected
        < void, const char* >{
        if ( !texture ) return std::unexpected( "texture is null" );
        m_draw_list->AddImage(
            texture->texture,
            ImVec2( position.x, position.y ),
            ImVec2( position.x + size.x, position.y + size.y )
        );

        return { };
    }

    auto Renderer::get_screensize( ) const noexcept -> sdk::math::Vec2{
        const auto  monitor = MonitorFromWindow( nullptr, MONITOR_DEFAULTTONEAREST );
        MONITORINFO info;
        info.cbSize = sizeof( MONITORINFO );
        GetMonitorInfo( monitor, &info );

        return {
            static_cast< float >( info.rcMonitor.right - info.rcMonitor.left ),
            static_cast< float >( info.rcMonitor.bottom - info.rcMonitor.top )
        };
    }

    auto Renderer::rectangle_3d(
        const Vec3& start,
        const Vec3& end,
        float       radius,
        Color       color,
        int32_t     flags,
        float       thickness,
        bool        outline_front
    ) const noexcept -> void{
        Vec2 sp_start{ }, sp_end{ };

        const auto rect    = sdk::math::Rectangle( start, end, radius );
        const auto polygon = rect.to_polygon( );


        if ( flags & filled ) {
            std::vector< std::vector< ImVec2 > > points;
            points.emplace_back( );

            for ( size_t i = 0u; i < polygon.points.size( ); i++ ) {
                const auto start_point = polygon.points[ i ];
                const auto end_point = i == polygon.points.size( ) - 1 ? polygon.points[ 0 ] : polygon.points[ i + 1 ];

                if ( !world_to_screen( start_point, sp_start ) || !world_to_screen( end_point, sp_end ) ) continue;


                points[ points.size( ) - 1u ].emplace_back( sp_start.x, sp_start.y );
                points[ points.size( ) - 1u ].emplace_back( sp_end.x, sp_end.y );
            }


            for ( auto p : points ) m_draw_list->AddConvexPolyFilled( p.data( ), p.size( ), get_u32( color ) );
        }

        if ( flags & outline ) {
            for ( size_t i = 0u; i < polygon.points.size( ); i++ ) {
                const auto start_point = polygon.points[ i ];
                const auto end_point = i == polygon.points.size( ) - 1 ? polygon.points[ 0 ] : polygon.points[ i + 1 ];

                if ( !outline_front && i == polygon.points.size( ) - 2 ) continue;

                line_3d(
                    start_point,
                    end_point,
                    flags & filled ? color.alpha( 255 ) : color,
                    thickness
                );
            }
        }
    }

    auto Renderer::polygon_3d(
        const class Polygon& polygon,
        Color                color,
        int32_t              flags,
        float                thickness
    ) const noexcept -> void{
        Vec2 sp_start{ }, sp_end{ };


        if ( flags & filled ) {
            std::vector< std::vector< ImVec2 > > points;
            points.emplace_back( );

            for ( size_t i = 0u; i < polygon.points.size( ); i++ ) {
                const auto start_point = polygon.points[ i ];
                const auto end_point = i == polygon.points.size( ) - 1 ? polygon.points[ 0 ] : polygon.points[ i + 1 ];

                if ( !world_to_screen( start_point, sp_start ) || !world_to_screen( end_point, sp_end ) ) continue;
                // @tore pls fix
                points[ points.size( ) - 1u ].emplace_back( sp_start.x, sp_start.y );
                points[ points.size( ) - 1u ].emplace_back( sp_end.x, sp_end.y );
            }


            for ( auto p : points ) m_draw_list->AddConvexPolyFilled( p.data( ), p.size( ), get_u32( color ) );
        }

        if ( flags & outline ) {
            for ( size_t i = 0u; i < polygon.points.size( ); i++ ) {
                const auto start_point = polygon.points[ i ];
                const auto end_point = i == polygon.points.size( ) - 1 ? polygon.points[ 0 ] : polygon.points[ i + 1 ];

                line_3d(
                    start_point,
                    end_point,
                    flags & filled ? color.alpha( 255 ) : color,
                    thickness
                );
            }
        }
    }

    auto Renderer::get_u32( const Color& color ) -> ImU32{
        return
            ( ( color.a & 0xff ) << 24 ) + ( ( color.b & 0xff ) << 16 ) + ( ( color.g & 0xff ) << 8 ) + ( color.r &
                0xff );
    }
#if enable_lua
    auto Renderer::circle_minimap_lua(
        // ReSharper disable CppPassValueParameterByConstReference
        const sol::object position,
        const sol::object clr,
        const sol::object radius,
        const sol::object segments,
        const sol::object thickness
        // ReSharper restore CppPassValueParameterByConstReference

    ) const -> void{
        lua_arg_check_v( position, Vec3 );
        lua_arg_check_v( clr, Color );
        lua_arg_check_ct_v( radius, float, "number" );
        if ( segments.get_type( ) != sol::type::nil )
            lua_arg_check_ct_v( segments, int32_t, "number" );
        if ( thickness.get_type( ) != sol::type::nil )
            lua_arg_check_ct_v( thickness, float, "number" );

        circle_minimap(
            position.as< Vec3 >( ),
            clr.as< Color >( ),
            radius.as< float >( ),
            segments.get_type( ) == sol::type::nil ? -1 : segments.as< int32_t >( ),
            thickness.get_type( ) == sol::type::nil ? 1.f : thickness.as< float >( )
        );
    }

    auto Renderer::text_lua(
        // ReSharper disable CppPassValueParameterByConstReference
        const sol::object position,
        const sol::object clr,
        const sol::object text,
        const sol::object font,
        const sol::object size
        // ReSharper restore CppPassValueParameterByConstReference

    ) -> void{
        lua_arg_check_v( position, Vec2 );
        lua_arg_check_v( clr, Color );
        lua_arg_check_ct_v( text, std::string, "string" );
        if ( font.get_type( ) != sol::type::nil )
            lua_arg_check_ct_v( font, std::string, "string" );
        lua_arg_check_ct_v( size, float, "number" );


        const ImFont* f;
        if ( font.get_type( ) != sol::type::nil ) {
            const auto found_font = g_fonts->get_lua_font( font.as< std::string >( ) );
            if ( !found_font ) {
                static auto last_font_error = std::chrono::steady_clock::now( );
                if ( std::chrono::duration_cast< std::chrono::seconds >(
                    std::chrono::steady_clock::now( ) - last_font_error
                ).count( ) > 5 ) {
                    lua_logger->error( "Invalid font name: {}", font.as< std::string >( ) );
                    last_font_error = std::chrono::steady_clock::now( );;
                }
                return;
            }
            f = found_font->get( size.as< int32_t >( ) );
            if ( !f ) f = g_fonts->get_default( );
        } else f = g_fonts->get_default( );

        this->text(
            position.as< Vec2 >( ),
            clr.as< Color >( ),
            f,
            text.as< std::string >( ).data( ),
            size.as< float >( )
        );
    }

    auto Renderer::get_text_size_lua(
        // ReSharper disable CppPassValueParameterByConstReference
        const sol::object text,
        const sol::object font,
        const sol::object font_size
        // ReSharper restore CppPassValueParameterByConstReference

    ) -> sol::object{
        lua_arg_check_ct( text, std::string, "string" );
        lua_arg_check_ct( font, std::string, "string" );
        lua_arg_check_ct( font_size, float, "number" );

        const ImFont* f;
        if ( font.get_type( ) != sol::type::nil ) {
            const auto  found_font      = g_fonts->get_lua_font( font.as< std::string >( ) );
            static auto last_font_error = std::chrono::steady_clock::now( );
            if ( std::chrono::duration_cast< std::chrono::seconds >(
                std::chrono::steady_clock::now( ) - last_font_error
            ).count( ) > 5 ) {
                lua_logger->error( "Invalid font name: {}", font.as< std::string >( ) );
                last_font_error = std::chrono::steady_clock::now( );;
            }
            f = found_font->get( font_size.as< int32_t >( ) );
            if ( !f ) f = g_fonts->get_default( );
        } else f = g_fonts->get_default( );
        const auto t = text.as< std::string >( );

        const auto size = f->CalcTextSizeA( font_size.as< float >( ), 1024.f, 0.f, t.data( ), t.data( ) + t.size( ) );

        return sol::make_object(
            g_lua_state2,
            Vec2( size.x, size.y )
        );
    }

    // ReSharper disable once CppPassValueParameterByConstReference
    auto Renderer::load_texture_from_file_lua( const sol::object file ) -> std::shared_ptr< Texture >{
        lua_arg_check_ct( file, std::string, "string" );

        // const std::regex regex_url_check(
        //     R"(^(([^:\/?#]+):)?(//([^\/?#]*))?([^?#]*)(\?([^#]*))?(#(.*))?)",
        //     std::regex::extended
        // );
        // const auto texture_name = file.as< std::string >( );
        // if ( std::regex_search( texture_name, regex_url_check ) ) {
        //     return load_texture_from_url( file.as< std::string >( ) );
        // }

        return load_texture_from_file( file.as< std::string >( ) );
    }
#endif

    auto Renderer::load_texture_from_url( std::string url ) noexcept -> std::shared_ptr< Texture >{
        auto hashed_url = rt_hash( url.data( ) );

        const auto found = m_texture_cache.find( hashed_url );
        if ( found != m_texture_cache.end( ) ) return found->second;

        static auto base_directory = std::format(
            ( "C:\\{}\\cache" ),
            user::c_hwid_system( ).get_hwid_short_form( )
        );
        if ( !std::filesystem::exists( base_directory ) ) std::filesystem::create_directories( base_directory );

        auto path = std::format( ( "{}\\{}.scch" ), base_directory, hashed_url );

        if ( std::filesystem::exists( path ) ) {
            std::ifstream stream;
            stream.open( path, std::ios::binary );
            if ( stream.is_open( ) ) {
                std::vector< unsigned > data;
                stream.seekg( 0, std::ios::end );
                data.resize( stream.tellg( ) );
                stream.seekg( 0, std::ios::beg );
                stream.read( reinterpret_cast< char* >( data.data( ) ), data.size( ) );
                stream.close( );

                // auto texture = std::make_shared< texture_t >( );
                return load_texture_from_memory( data.data( ), data.size( ) );
            }
        }

        WebClient web_client;
        auto      data = web_client.get( url );

        if ( !data ) return nullptr;

        std::ofstream stream;
        stream.open( path, std::ios::binary );
        if ( stream.is_open( ) ) {
            stream.write( reinterpret_cast< char* >( data->data( ) ), data->size( ) );
            stream.close( );
        }

        return load_texture_from_memory( reinterpret_cast< unsigned* >( data->data( ) ), data->size( ) / 4 );
    }

#if enable_lua
    auto Renderer::image_lua(
        // ReSharper disable CppPassValueParameterByConstReference
        const sol::object position,
        const sol::object size,
        const sol::object texture
        // ReSharper restore CppPassValueParameterByConstReference

    ) -> void{
        lua_arg_check_v( position, Vec2 );
        lua_arg_check_v( size, Vec2 );
        lua_arg_check_ct_v( texture, std::shared_ptr< Texture >, "texture" );

        if ( image( position.as< Vec2 >( ), size.as< Vec2 >( ), texture.as< std::shared_ptr< Texture > >( ) ) ) {
            return;
        }
    }

    auto Renderer::get_screensize_lua( ) const -> sol::object{
        return sol::make_object( g_lua_state2, get_screensize( ) );
    }
#endif
}
