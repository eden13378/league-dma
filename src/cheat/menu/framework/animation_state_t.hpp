#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>

#include "../../utils/ease.hpp"

namespace menu::framework {
    struct AnimationState {
        enum EType {
            ease_in,
            ease_out,
            ease_in_out,
            ease_out_in
        };

        AnimationState( ) = default;

        explicit AnimationState( const float time )
            : animation_time( time ){
        }

        explicit AnimationState( const float time, const EType type )
            : animation_time( time ),
              type( type ){
        }

        float animation_time{ .5f };
        float animation_start{ };
        EType type{ ease_in };

        auto start( ) -> void{
            animation_start = get_time( );
        }

        auto reset( ) -> void{
            animation_start = 0;
        }

        auto get_time_left( ) const -> float{
            const auto left = ( get_time_ms( ) - animation_start ) / ( animation_time * 1000.f );
            if ( left > animation_time ) return 0.f;
            return left;
        }

        auto get_progress( ) const -> float{
            if ( animation_start == 0 ) return 1.f;

            switch ( type ) {
            case ease_in:
                return utils::ease::ease_in( std::clamp( ( get_time( ) - animation_start ) / animation_time, 0.f, 1.f ) );
            case ease_in_out:
                return utils::ease::ease_in_out( std::clamp( ( get_time( ) - animation_start ) / animation_time, 0.f, 1.f ) );
            case ease_out_in:
                return utils::ease::ease_in_out( std::clamp( ( get_time( ) - animation_start ) / animation_time, 0.f, 1.f ) );
            case ease_out:
                return utils::ease::ease_out( std::clamp( ( get_time( ) - animation_start ) / animation_time, 0.f, 1.f ) );
            default: ;
            }

            return 1.f;
        }

    private:
        auto get_time_ms( ) const -> int32_t{
            using namespace std::chrono;
            return static_cast< int32_t >( std::chrono::duration_cast< milliseconds >( high_resolution_clock::now( ).time_since_epoch( ) ).count( ) );
        }

        auto get_time( ) const -> float{
            return get_time_ms( ) / 1000.f;
        }
    };
}
