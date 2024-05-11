#pragma once
#include "feature.hpp"
#include "names.hpp"

namespace features {
    class Visuals final : public IFeature {
    public:
        ~Visuals( ) override = default;

        auto on_draw( ) -> void override{
            if ( !g_local ) return;

            // draw_local_attack_range( );
            //draw_enemy_attack_range( );
#if __DEBUG && profiling_overlay
            draw_debug_overlay( );
#endif
#if _DEBUG && enable_lua && lua_callback_debug
            draw_lua_calls( );
#endif
        }

        auto get_name( ) noexcept -> hash_t override{ return names::visuals; }

#if _DEBUG
        auto get_full_name( ) -> std::string override{ return "c_visuals"; }
#endif

#if __DEBUG && profiling_overlay
        struct debug_profiling_t {
            int32_t time;
            std::string name;
            int32_t max;
            std::chrono::time_point< std::chrono::high_resolution_clock > last_max;

            auto update( const int32_t time ) -> void {
                if ( time > max ) {
                    max = time;
                    last_max = std::chrono::high_resolution_clock::now( );
                }
                this->time = time;
                if ( std::chrono::duration_cast< std::chrono::seconds >( std::chrono::high_resolution_clock::now( ).time_since_epoch( ) - last_max.time_since_epoch( ) ).count( ) > 5 ) {
                    max = 0;
                }
            }
        };

        std::map< hash_t, debug_profiling_t > debug_profiling_times{ };
#endif

    private:
#if _DEBUG && enable_lua && lua_callback_debug
        auto draw_lua_calls( ) -> void;
#endif

        static auto draw_enemy_attack_range( ) -> void;

#if __DEBUG && profiling_overlay
        auto draw_debug_overlay( ) const -> void;
#endif
    };
}
