#pragma once
#include <ostream>

namespace renderer {
    class Color {
    public:
        Color( ) = default;

        Color( const int32_t r, const int32_t g, const int32_t b ): r( r ), g( g ), b( b ){
        }

        Color( const int32_t r, const int32_t g, const int32_t b, const int32_t a ): r( r ), g( g ), b( b ), a( a ){
        }

        Color( const float r, const float g, const float b ): r( static_cast< int32_t >( r ) ),
            g(
                static_cast<
                    int32_t >( g )
            ),
            b( static_cast< int32_t >( b ) ){
        }

        Color( const float r, const float g, const float b, const float a ):
            r( static_cast< int32_t >( r ) ),
            g( static_cast< int32_t >( g ) ),
            b( static_cast< int32_t >( b ) ),
            a( static_cast< int32_t >( a ) ){
        }

        explicit Color( const std::string& hex_color ){
            if ( !hex_color.starts_with( "#" ) ) return;
            const auto parsed = std::stoi( hex_color.substr( 1 ), 0, 16 );

            r = ( parsed >> 16 ) & 0xFF;
            g = ( parsed >> 8 ) & 0xFF;
            b = parsed & 0xFF;
        }

        auto to_hex( ) const -> std::string{ return std::to_string( ( r << 16 ) + ( g << 8 ) + b ); }


        friend auto operator==( const Color& lhs, const Color& rhs ) -> bool{
            return lhs.r == rhs.r
                && lhs.g == rhs.g
                && lhs.b == rhs.b
                && lhs.a == rhs.a;
        }

        friend auto operator!=( const Color& lhs, const Color& rhs ) -> bool{ return !( lhs == rhs ); }

        friend auto operator<<( std::ostream& os, const Color& obj ) -> std::ostream&{
            return os
                << "color( r: " << obj.r
                << ", g: " << obj.g
                << ", b: " << obj.b
                << ", a: " << obj.a << " )";
        }

        // color( const float r, const float g, const float b ): r( static_cast< int32_t >( r ) ),
        //     g( static_cast< int32_t >( g ) ), b( static_cast< int32_t >( b ) ){
        // }
        //
        // color( const float r, const float g, const float b, const float a ): r( static_cast< int32_t >( r ) ),
        //     g( static_cast< int32_t >( g ) ),
        //     b( static_cast< int32_t >( b ) ), a( static_cast< int32_t >( a ) ){
        // }

        static auto white( ) -> Color{ return Color( 255, 255, 255 ); }

        static auto black( ) -> Color{ return Color( 0, 0, 0 ); }

        static auto red( ) -> Color{ return Color( 255, 0, 0 ); }

        static auto green( ) -> Color{ return Color( 0, 255, 0 ); }

        static auto blue( ) -> Color{ return Color( 0, 0, 255 ); }

        int32_t r{ 255 };
        int32_t g{ 255 };
        int32_t b{ 255 };
        int32_t a{ 255 };

        auto alpha( int32_t new_value ) -> Color{
            a = new_value;
            return *this;
        }

        auto red( int32_t new_value ) -> Color{
            r = new_value;
            return *this;
        }

        auto green( int32_t new_value ) -> Color{
            g = new_value;
            return *this;
        }

        auto blue( int32_t new_value ) -> Color{
            b = new_value;
            return *this;
        }
    };
}

using namespace renderer;
