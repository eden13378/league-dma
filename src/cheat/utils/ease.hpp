#pragma once

namespace utils::ease {
    inline auto factorize( const float input, const int32_t factor ) -> float{
        auto n = input;
        for ( auto i = 0; i < factor; ++i ) n *= input;

        return n;
    }

    inline auto ease_out( const float t, const int32_t f = 3 ) -> float{ return std::clamp( factorize( t, 1 + f * 2 ), 0.f, 1.f ); }

    inline auto ease_in( const float t, const int32_t f = 3 ) -> float{
        return std::clamp( 1.f - factorize( 1.f - t, 1 + f * 2 ), 0.f, 1.f );
    }

    inline auto ease_in_sine( const float x ) -> float{ return 1.f - std::cos( ( x * 3.14159f ) / 2.f ); }

    inline auto ease_out_in( float t ) -> float{
        if ( t <= 0.5f ) return ease_out( t );
        t -= 0.5f;
        return ease_in( t );
    }

    inline auto ease_in_out( float t ) -> float{
        if ( t <= 0.5f ) return ease_in( t );
        t -= 0.5f;
        return ease_out( t );
    }

    // --- sin

    inline auto ease_in_out_sine( const float x ) -> float{ return -( std::cos( 3.14159f * x ) - 1.f ) / 2.f; }
    inline auto ease_out_sine( const float x ) -> float{ return std::sin( ( x * 3.14159f ) / 2.f ); }

    // --- quad
    inline auto ease_in_quad( const float x ) -> float{ return x * x; }
    inline auto ease_out_quad( const float x ) -> float{ return 1.f - ( 1.f - x ) * ( 1.f - x ); }

    inline auto ease_in_out_quad( const float x ) -> float{
        return x < 0.5f ? 2.f * x * x : 1.f - std::pow( -2.f * x + 2.f, 2.f ) / 2.f;
    }

    // --- cubic
    inline auto ease_in_out_cubic( const float x ) -> float{
        return x < 0.5f ? 4.f * x * x * x : 1.f - std::pow( -2.f * x + 2.f, 3.f ) / 2.f;
    }

    inline auto ease_in_cubic( const float x ) -> float{ return x * x * x; }
    inline auto ease_out_cubic( const float x ) -> float{ return 1.f - std::pow( 1.f - x, 3.f ); }

    // --- quart
    inline auto ease_in_quart( const float x ) -> float{ return x * x * x * x; }
    inline auto ease_out_quart( const float x ) -> float{ return 1.f - std::pow( 1.f - x, 4.f ); }

    inline auto ease_in_out_quart( const float x ) -> float{
        return x < 0.5f ? 8.f * x * x * x * x : 1.f - std::pow( -2.f * x + 2.f, 4.f ) / 2.f;
    }

    // --- quint
    inline auto ease_in_quint( const float x ) -> float{ return x * x * x * x * x; }
    inline auto ease_out_quint( const float x ) -> float{ return 1.f - std::pow( 1.f - x, 5.f ); }

    inline auto ease_in_out_quint( const float x ) -> float{
        return x < 0.5f ? 16.f * x * x * x * x * x : 1.f - std::pow( -2.f * x + 2.f, 5.f ) / 2.f;
    }

    // -- expo
    inline auto ease_in_expo( const float x ) -> float{ return x == 0.f ? 0.f : std::pow( 2.f, 10.f * x - 10.f ); }
    inline auto ease_out_expo( const float x ) -> float{ return x == 1.f ? 1.f : 1.f - std::pow( 2.f, -10.f * x ); }

    inline auto ease_in_out_expo( const float x ) -> float{
        return x == 0.f ?
                   0.f :
                   x == 1.f ?
                       1.f :
                       x < 0.5f ?
                           std::pow( 2.f, 20.f * x - 10.f ) / 2.f :
                           ( 2.f - std::pow( 2.f, -20.f * x + 10.f ) ) / 2.f;
    }

    // --- circ
    inline auto ease_in_circ( const float x ) -> float{ return 1.f - std::sqrt( 1.f - std::pow( x, 2.f ) ); }
    inline auto ease_out_circ( const float x ) -> float{ return std::sqrt( 1.f - std::pow( x - 1.f, 2.f ) ); }

    inline auto ease_in_out_circ( const float x ) -> float{
        return x < 0.5f ?
                   ( 1.f - std::sqrt( 1.f - std::pow( 2.f * x, 2.f ) ) ) / 2.f :
                   ( std::sqrt( 1.f - std::pow( -2.f * x + 2.f, 2.f ) ) + 1.f ) / 2.f;
    }
}
