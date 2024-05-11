#pragma once

#include "../number_types.hpp"
#if enable_new_lua
#include <sol/forward.hpp>
#endif

namespace sdk::math {
    class Vec2 {
    public:
        Vec2( ) = default;

        Vec2( const float x, const float y )
            : x( x ),
            y( y ){
        }
        
        auto length( ) const -> float{ return std::sqrtf( x * x + y * y ); }

        [[nodiscard]] auto length_squared( ) const -> float{ return x * x + y * y; }

        auto dist_to( const Vec2& o ) const -> float{ return ( ( *this ) - o ).length( ); }

        [[nodiscard]] auto dist_to_squared( const Vec2& other ) const -> float{
            return ( *this - other ).length_squared( );
        }

        [[nodiscard]] auto dot( const Vec2& other ) const -> float{ return x * other.x + y * other.y; }

        [[nodiscard]] auto extend( const Vec2& to, const float distance ) const -> Vec2{
            const auto from   = *this;
            const auto result = from + ( to - from ).normalize( ) * distance;
            return result;
        }

        [[nodiscard]] auto normalize( ) const -> Vec2{
            const auto l = this->length( );
            if ( l != 0.0f ) {
                const auto inv = 1.0f / l;
                return { x * inv, y * inv };
            }

            return *this;
        }

        auto normalize_in_place( ) -> Vec2{
            const auto l = length( );
            if ( l != 0.f ) *this /= length( );

            return *this;
        }

#if enable_lua
        auto dist_to_lua( const sol::object other ) const -> float;
#endif

        // Operators

        auto operator+( const Vec2& other ) const -> Vec2{ return { x + other.x, y + other.y }; }

        auto operator-( const Vec2& other ) const -> Vec2{ return { x - other.x, y - other.y }; }

        auto operator*( const Vec2& other ) const -> Vec2{ return { x * other.x, y * other.y }; }

        auto operator/( const Vec2& other ) const -> Vec2{ return { x / other.x, y / other.y }; }

        auto operator+( const float other ) const -> Vec2{ return { x + other, y + other }; }

        auto operator-( const float other ) const -> Vec2{ return { x - other, y - other }; }

        auto operator*( const float other ) const -> Vec2{ return { x * other, y * other }; }

        auto operator/( const float other ) const -> Vec2{ return { x / other, y / other }; }

        auto operator-( const i32 o ) const -> Vec2{ return Vec2( x - o, y - o ); }

        auto operator+( const i32 o ) const -> Vec2{ return Vec2( x + o, y + o ); }

        auto operator*( const i32 o ) const -> Vec2{ return Vec2( x * o, y * o ); }

        auto operator/( const i32 o ) const -> Vec2{ return Vec2( x / o, y / o ); }

        auto operator+=( const Vec2& other ) -> Vec2{
            x += other.x;
            y += other.y;

            return *this;
        }

        auto operator+=( const float other ) -> Vec2{
            x += other;
            y += other;

            return *this;
        }

        auto operator-=( const Vec2& other ) -> Vec2{
            x -= other.x;
            y -= other.y;

            return *this;
        }

        auto operator-=( const float other ) -> Vec2{
            x -= other;
            y -= other;

            return *this;
        }

        auto operator*=( const Vec2& other ) -> Vec2{
            x *= other.x;
            y *= other.y;

            return *this;
        }

        auto operator*=( const float other ) -> Vec2{
            x *= other;
            y *= other;

            return *this;
        }

        auto operator/=( const Vec2& other ) -> Vec2{
            x /= other.x;
            y /= other.y;

            return *this;
        }

        auto operator/=( const float other ) -> Vec2{
            x /= other;
            y /= other;

            return *this;
        }

        friend auto operator==( const Vec2& lhs, const Vec2 rhs ) -> bool{
            return lhs.x == rhs.x
                && lhs.y == rhs.y;
        }

        friend auto operator!=( const Vec2 lhs, const Vec2 rhs ) -> bool{ return lhs != rhs; }

        friend std::ostream& operator<<( std::ostream& os, const Vec2& vec_2 ){
            os << "Vec2( x: " << vec_2.x << ", y: " << vec_2.y << " )";
            return os;
        }

        f32 x{ };
        f32 y{ };
    };
}
