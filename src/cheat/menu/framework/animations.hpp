#pragma once
#include <chrono>
#include <deque>

namespace menu::framework {
    inline auto __get_curtime( ) -> float{
        return static_cast< float >( std::chrono::duration_cast< std::chrono::milliseconds >( std::chrono::steady_clock::now( ).time_since_epoch( ) ).count( ) ) / 1000.f;
    }

    template <typename t>
    struct animation_t {
        float start{ };
        float duration{ };
        t start_value{ };
        t current_value{ };
        t end_value{ };
    };


    template <typename t>
    struct animation_queue_t {
        auto add( t end_value, float duration ) -> void{
            auto c = current( );
            queue.emplace_back( animation_t< t >{ -1.f, duration, c, c, end_value } );
        }

        auto current( ) -> t{
            if ( queue.empty( ) )
                return { };
            auto& first = queue.front( );
            auto current_time = __get_curtime( );

            if ( first.start + first.duration < current_time ) {
                if ( queue.size( ) > 1 ) {
                    queue.pop_front( );
                    queue.begin( )->start = current_time;
                    return current( );
                }
                return first.end_value;
            }

            const auto delta = current_time - first.start;
            const auto progress = delta / first.duration;

            first.current_value = first.start_value + ( first.end_value - first.start_value ) * progress;
            return first.current_value;
        }

    private:
        std::deque< animation_t< t > > queue;
    };
}
