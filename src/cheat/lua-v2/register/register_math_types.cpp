#include "pch.hpp"
#include "../state.hpp"
#include "../../sdk/math/geometry.hpp"
#include "../../sdk/math/vec2.hpp"
#include "../../sdk/math/vec3.hpp"

namespace lua {
        auto LuaState::register_math_types( sol::state& state ) -> void{
        using Vec2 = sdk::math::Vec2;

        state.new_usertype< Vec2 >(
            "vec2",
            sol::constructors< Vec2( ), Vec2( float, float ) >( ),
            "x",
            ( &Vec2::x ),
            "y",
            ( &Vec2::y ),
            sol::meta_function::addition,
            sol::overload(
                sol::resolve< Vec2( const Vec2& ) const >( &Vec2::operator+ ),
                sol::resolve< Vec2( const float ) const >( &Vec2::operator+ )
            ),
            sol::meta_function::subtraction,
            sol::overload(
                sol::resolve< Vec2( const Vec2& ) const >( &Vec2::operator- ),
                sol::resolve< Vec2( const float ) const >( &Vec2::operator- )
            ),
            sol::meta_function::multiplication,
            sol::overload(
                sol::resolve< Vec2( const Vec2& ) const >( &Vec2::operator* ),
                sol::resolve< Vec2( const float ) const >( &Vec2::operator* )
            ),
            sol::meta_function::division,
            sol::overload(
                sol::resolve< Vec2( const Vec2& ) const >( &Vec2::operator/ ),
                sol::resolve< Vec2( const float ) const >( &Vec2::operator/ )
            ),
            sol::meta_function::equal_to,
            []( const Vec2& lhs, const Vec2& rhs ) -> bool{ return lhs == rhs; },
            "add",
            sol::overload(
                sol::resolve< Vec2( const Vec2& ) >( &Vec2::operator+= ),
                sol::resolve< Vec2( const float ) >( &Vec2::operator+= )
            ),
            "subtract",
            sol::overload(
                sol::resolve< Vec2( const Vec2& ) >( &Vec2::operator-= ),
                sol::resolve< Vec2( const float ) >( &Vec2::operator-= )
            ),
            "multiply",
            sol::overload(
                sol::resolve< Vec2( const Vec2& ) >( &Vec2::operator*= ),
                sol::resolve< Vec2( const float ) >( &Vec2::operator*= )
            ),
            "divide",
            sol::overload(
                sol::resolve< Vec2( const Vec2& ) >( &Vec2::operator/= ),
                sol::resolve< Vec2( const float ) >( &Vec2::operator/= )
            ),
            "dist_to",
            &Vec2::dist_to,
            "dist_to_squared",
            &Vec2::dist_to_squared,
            "dot",
            &Vec2::dot,
            "length",
            &Vec2::length,
            "length_squared",
            &Vec2::length_squared,
            "normalize",
            &Vec2::normalize,
            "normalize_in_place",
            &Vec2::normalize_in_place,
            "extend",
            &Vec2::extend
        );

        using Vec3 = sdk::math::Vec3;


        state.new_usertype< Vec3 >(
            "vec3",
            sol::constructors< Vec3( ), Vec3( float, float, float ) >( ),
            "x",
            ( &Vec3::x ),
            "y",
            ( &Vec3::y ),
            "z",
            ( &Vec3::z ),
            sol::meta_function::addition,
            sol::overload(
                sol::resolve< Vec3( const Vec3& ) const >( &Vec3::operator+ ),
                sol::resolve< Vec3( const float ) const >( &Vec3::operator+ )
            ),
            sol::meta_function::subtraction,
            sol::overload(
                sol::resolve< Vec3( const Vec3& ) const >( &Vec3::operator- ),
                sol::resolve< Vec3( const float ) const >( &Vec3::operator- )
            ),
            sol::meta_function::multiplication,
            sol::overload(
                sol::resolve< Vec3( const Vec3& ) const >( &Vec3::operator* ),
                sol::resolve< Vec3( const float ) const >( &Vec3::operator* )
            ),
            sol::meta_function::division,
            sol::overload(
                sol::resolve< Vec3( const Vec3& ) const >( &Vec3::operator/ ),
                sol::resolve< Vec3( const float ) const >( &Vec3::operator/ )
            ),
            sol::meta_function::equal_to,
            []( const Vec3& lhs, const Vec3& rhs ) -> bool{ return lhs == rhs; },
            "add",
            sol::overload(
                sol::resolve< Vec3( const Vec3& ) >( &Vec3::operator+= ),
                sol::resolve< Vec3( const float ) >( &Vec3::operator+= )
            ),
            "subtract",
            sol::overload(
                sol::resolve< Vec3( const Vec3& ) >( &Vec3::operator-= ),
                sol::resolve< Vec3( const float ) >( &Vec3::operator-= )
            ),
            "multiply",
            sol::overload(
                sol::resolve< Vec3( const Vec3& ) >( &Vec3::operator*= ),
                sol::resolve< Vec3( const float ) >( &Vec3::operator*= )
            ),
            "divide",
            sol::overload(
                sol::resolve< Vec3( const Vec3& ) >( &Vec3::operator/= ),
                sol::resolve< Vec3( const float ) >( &Vec3::operator/= )
            ),
            "dist_to",
            &Vec3::dist_to,
            "dist_to_squared",
            &Vec3::dist_to_squared,
            "dot",
            &Vec3::dot,
            "length",
            &Vec3::length,
            "length2d",
            &Vec3::length2d,
            "length_squared",
            &Vec3::length_squared,
            "normalized",
            &Vec3::normalize,
            "normalize_in_place",
            &Vec3::normalize_in_place,
            "extend",
            &Vec3::extend,
            "cross",
            sol::resolve( &Vec3::cross ),
            "angle_between",
            sol::resolve( &Vec3::angle_between ),
            "rotated",
            sol::resolve( &Vec3::rotated ),
            "to_screen",
            sol::resolve( &Vec3::to_screen_lua ),
            "to_minimap",
            sol::resolve( &Vec3::to_minimap_lua )
        );

        using Polygon = sdk::math::Polygon;

        state.new_usertype< Polygon >(
            "polygon_t",
            sol::constructors< Polygon( ) >( ),
            "add",
            sol::resolve( &Polygon::add ),
            "is_inside",
            sol::resolve( &Polygon::is_inside ),
            "is_outside",
            sol::resolve( &Polygon::is_outside ),
            "point_in_polygon",
            sol::resolve( &Polygon::point_in_polygon ),
            "points",
            &Polygon::points
        );

        using Arc = sdk::math::Arc;

        state.new_usertype< Arc >(
            "arc_t",
            sol::constructors< Arc( const Vec3&, const Vec3&, int ) >( ),
            "circle_circle_intersection",
            sol::resolve( &Arc::circle_circle_intersection ),
            "to_polygon",
            sol::resolve( &Arc::to_polygon ),
            "end",
            ( &Arc::end ),
            "start",
            ( &Arc::start ),
            "hitbox",
            ( &Arc::hitbox ),
            "distance",
            ( &Arc::distance )
        );

        using Circle = sdk::math::Circle;

        state.new_usertype< Circle >(
            "circle_t",
            sol::constructors< Circle( const Vec3&, float ) >( ),
            "to_polygon",
            sol::resolve( &Circle::to_polygon ),
            "center",
            ( &Circle::center ),
            "radius",
            ( &Circle::radius )
        );

        using Rectangle = sdk::math::Rectangle;

        state.new_usertype< Rectangle >(
            "rectangle_t",
            sol::constructors< Rectangle( const Vec3&, const Vec3&, float ) >( ),
            "to_polygon",
            sol::resolve( &Rectangle::to_polygon ),
            "intersection",
            sol::resolve( &Rectangle::intersection ),
            "direction",
            ( &Rectangle::direction ),
            "perpendicular",
            ( &Rectangle::perpendicular ),
            "r_end",
            ( &Rectangle::r_end ),
            "r_start",
            ( &Rectangle::r_start ),
            "width",
            ( &Rectangle::width )
        );

        using Ring = sdk::math::Ring;

        state.new_usertype< Ring >(
            "ring_t",
            sol::constructors< Ring( const Vec3&, float, float ) >( ),
            "to_polygon",
            sol::resolve( &Ring::to_polygon ),
            "center",
            ( &Ring::center ),
            "radius",
            ( &Ring::radius ),
            "ring_radius",
            ( &Ring::ring_radius )
        );

        using Sector = sdk::math::Sector;

        state.new_usertype< Sector >(
            "sector_t",
            sol::constructors< Sector( const Vec3&, const Vec3&, float, float ) >( ),
            "to_polygon",
            sol::resolve( &Sector::to_polygon ),
            "contains",
            sol::resolve( &Sector::contains ),
            "angle",
            ( &Sector::angle ),
            "center",
            ( &Sector::center ),
            "direction",
            ( &Sector::direction ),
            "radius",
            ( &Sector::radius )
        );

        using Geometry = sdk::math::Geometry;

        state.new_usertype< Geometry >(
            "geometry",
            // "clip_polygons",
            // sol::resolve( Geometry::clip_polygons ),
            "cut_path",
            sol::resolve( Geometry::cut_path ),
            "path_length",
            sol::resolve( Geometry::path_length ),
            // "vector_movement_collision",
            // sol::resolve( Geometry::vector_movement_collision ),
            "position_after",
            sol::resolve( Geometry::position_after )
            // "to_polygon",
            // sol::resolve( Geometry::to_polygon ),
            // "to_polygons",
            // sol::resolve( Geometry::to_polygons )
        );
    }
}
