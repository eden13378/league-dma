#pragma once
#include <complex>
#include <optional>
#include <ostream>

#include "../number_types.hpp"

#include <sol/sol.hpp>
// #include "../../lua-v2/lua_def.hpp"

constexpr auto m_pi = 3.14159265359f;

namespace sdk::math {
    struct projection_info;

    class Vec2;

    class Vec3 {
    public:
        Vec3( ) = default;

        Vec3( const float x, const float y, const float z )
            : x( x ),
            y( y ),
            z( z ){
        }

        [[nodiscard]] auto length( ) const -> float{ return std::sqrtf( x * x + y * y + z * z ); }

        [[nodiscard]] auto length_squared( ) const -> float{ return x * x + y * y + z * z; }

        [[nodiscard]] auto length2d( ) const -> float{ return std::sqrtf( x * x + y * y ); }

        auto operator-( const Vec3& o ) const -> Vec3{ return Vec3( x - o.x, y - o.y, z - o.z ); }

        auto operator+( const Vec3& o ) const -> Vec3{ return Vec3( x + o.x, y + o.y, z + o.z ); }

        auto operator*( const Vec3& o ) const -> Vec3{ return Vec3( x * o.x, y * o.y, z * o.z ); }

        auto operator/( const Vec3& o ) const -> Vec3{ return Vec3( x / o.x, y / o.y, z / o.z ); }

        auto operator-( const float o ) const -> Vec3{ return Vec3( x - o, y - o, z - o ); }

        auto operator+( const float o ) const -> Vec3{ return Vec3( x + o, y + o, z + o ); }

        auto operator*( const float o ) const -> Vec3{ return Vec3( x * o, y * o, z * o ); }

        auto operator/( const float o ) const -> Vec3{ return Vec3( x / o, y / o, z / o ); }

        auto operator-( const i32 o ) const -> Vec3{ return Vec3( x - o, y - o, z - o ); }

        auto operator+( const i32 o ) const -> Vec3{ return Vec3( x + o, y + o, z + o ); }

        auto operator*( const i32 o ) const -> Vec3{ return Vec3( x * o, y * o, z * o ); }

        auto operator/( const i32 o ) const -> Vec3{ return Vec3( x / o, y / o, z / o ); }

        // friend auto operator==( const Vec3& lhs, const Vec3& rhs ) -> bool{
        //     return lhs.x == rhs.x
        //         && lhs.y == rhs.y
        //         && lhs.z == rhs.z;
        // }

        // auto operator==( const Vec3& rhs ) const -> bool{
        //     return x == rhs.x
        //         && y == rhs.y
        //         && z == rhs.z;
        // }

        friend auto operator==( const Vec3& lhs, const Vec3& rhs ) -> bool{
            return lhs.x == rhs.x
                && lhs.y == rhs.y
                && lhs.z == rhs.z;
        }

        friend auto operator!=( const Vec3& lhs, const Vec3& rhs ) -> bool{
            return lhs.x != rhs.x
                || lhs.y != rhs.y
                || lhs.z != rhs.z;
        }

        // friend auto operator!=( const Vec3& lhs, const Vec3& rhs ) -> bool{
        //     return !( lhs == rhs );
        // }

        auto operator+=( const Vec3& o ) -> Vec3{
            x += o.x;
            y += o.y;
            z += o.z;
            return *this;
        }

        auto operator-=( const Vec3& o ) -> Vec3{
            x -= o.x;
            y -= o.y;
            z -= o.z;
            return *this;
        }

        auto operator*=( const Vec3& o ) -> Vec3{
            x *= o.x;
            y *= o.y;
            z *= o.z;
            return *this;
        }

        auto operator/=( const Vec3& o ) -> Vec3{
            x /= o.x;
            y /= o.y;
            z /= o.z;
            return *this;
        }

        auto operator+=( const float o ) -> Vec3{
            x += o;
            y += o;
            z += o;
            return *this;
        }

        auto operator-=( const float o ) -> Vec3{
            x -= o;
            y -= o;
            z -= o;
            return *this;
        }

        auto operator*=( const float o ) -> Vec3{
            x *= o;
            y *= o;
            z *= o;
            return *this;
        }

        auto operator/=( const float o ) -> Vec3{
            x /= o;
            y /= o;
            z /= o;
            return *this;
        }

        friend auto operator<<( std::ostream& os, const Vec3& obj ) -> std::ostream&{
            return os
                << "Vec3( x: " << obj.x
                << ", y: " << obj.y
                << ", z: " << obj.z << " )";
        }

        [[nodiscard]] auto dist_to( const Vec3& o ) const -> float{ return ( ( *this ) - o ).length( ); }

        auto dist_to_squared( const Vec3& o ) const -> float{ return ( *this - 0 ).length_squared( ); }

        // with_lua(
        //     auto dist_to_lua(sol::object other) -> float;
        // )

        [[nodiscard]] auto dot( const Vec3& o ) const -> float{ return x * o.x + y * o.y + z * o.z; }

        // with_lua(
        //     auto dot_lua( sol::object other ) const -> float;
        // )

        [[nodiscard]] auto cross( const Vec3& o ) const -> Vec3{
            return Vec3( y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x );
        }

        // with_lua( auto cross_lua( sol::object other ) const -> sol::object; )

        [[nodiscard]] auto perpendicular( ) const -> Vec3{ return Vec3( -z, 0.f, x ); }

        [[nodiscard]] auto normalize( ) const -> Vec3{
            const auto l = this->length( );
            if ( l != 0.0f ) {
                const auto inv = 1.0f / l;
                return { x * inv, y * inv, z * inv };
            }

            return *this;
        }

        auto normalize_in_place( ) -> Vec3{
            const auto l = this->length( );
            if ( l != 0.f ) {
                *this /= l;
                return *this;
            }
            return *this;
        }

        // with_lua(
        //     auto normalized_lua( ) const -> sol::object;
        // )

        [[nodiscard]] auto dot_product( const Vec3& other ) const -> float{ return this->x * other.x + this->z * other.z; }

        [[nodiscard]] auto close( const float a, const float b, float eps ) const -> bool{
            if ( abs( eps ) < FLT_EPSILON ) eps = static_cast< float >( 1e-9 );
            return abs( a - b ) <= eps;
        }

        [[nodiscard]] auto polar( ) const -> float{
            if ( this->close( x, 0.f, 0.f ) ) {
                if ( z > 0.f ) return 90.f;
                return z < 0.f ?
                           270.f :
                           0.f;
            }

            auto theta = atan( z / x ) * 180.f / m_pi;
            if ( x < 0.f ) theta = theta + 180.f;
            if ( theta < 0.f ) theta = theta + 360.f;
            return theta;
        }


        [[nodiscard]] auto angle_between( const Vec3& other ) const -> float{
            auto theta = this->polar( ) - other.polar( );
            if ( theta < 0.f ) theta = theta + 360.f;
            if ( theta > 180.f ) theta = 360.f - theta;
            return theta;
        }

        // with_lua( auto angle_between_lua( sol::object other ) const -> sol::object; )

        [[nodiscard]] auto angle_between_degrees( const Vec3& other ) const -> float{
            if ( this->close( x, 0.f, 0.f ) ) {
                if ( z > 0.f ) return 90.f;
                return z < 0.f ?
                           270.f :
                           0.f;
            }

            const auto rise = other.z - z;
            const auto run  = other.x - x;

            auto theta = atan( fabs( rise / run ) ) * ( 180.f / m_pi );

            if ( run > 0.f ) {
                theta = 90.f + ( rise > 0.f ?
                                     -theta :
                                     theta );
            } else {
                theta = 270.f + ( rise > 0.f ?
                                      theta :
                                      -theta );
            }
            return theta;
        }

        [[nodiscard]] auto rotated( const float angle ) const -> Vec3{
            const auto c = cos( angle );
            const auto s = sin( angle );

            return Vec3( ( x * c - z * s ), 0.f, ( z * c + x * s ) );
        }

        [[nodiscard]] auto rotated_raw( float angle ) const -> Vec3{
            angle = angle * ( 3.14159265358979323846264338327950288419716939937510f / 180.f );

            const auto c = cos( angle );
            const auto s = sin( angle );

            return Vec3( ( x * c - z * s ), 0.f, ( z * c + x * s ) );
        }

        // with_lua(
        //     auto rotated_lua( sol::object angle ) const -> sol::object;
        // )

        [[nodiscard]] auto extend( const Vec3& to, const float distance ) const -> Vec3{
            const auto from = *this;

            if ( from.dist_to( to ) < distance ) {
                auto result = from + ( to - from ).normalize( ) * distance;
                result.y    = to.y;

                return result;
            }

            return Vec3{ from + ( to - from ).normalize( ) * distance };
        }

        // with_lua(
        //     auto extend_lua( sol::object to, sol::object distance ) const -> sol::object;
        // )

        [[nodiscard]] auto to_screen( ) const -> std::optional< Vec2 >;
        [[nodiscard]] auto to_minimap( ) const -> std::optional< Vec2 >;

        auto project_on( const Vec3& segment_start, const Vec3& segment_end ) const -> projection_info;

#if enable_lua
        auto to_minimap_lua( ) const -> sol::object;
        auto to_screen_lua( ) const -> sol::object;
#endif

        f32 x{ };
        f32 y{ };
        f32 z{ };
    };

    struct projection_info {
        bool is_on_segment{ };
        Vec3 line_point{ };
        Vec3 segment_point{ };

        projection_info( bool is_on_segment, const Vec3& segment_point, const Vec3& line_point );
    };
}
