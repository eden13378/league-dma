#pragma once
#include "../build.hpp"

#include <tuple>

namespace overlay {
    constexpr auto DEBUG_ENTRIES = 256;

    class DebugOverlay {
        struct Timing {
            int32_t                                index{ };
            std::array< long long, DEBUG_ENTRIES > timings{ };
            std::chrono::steady_clock::time_point  last_add_time{ };

            [[nodiscard]] auto get_alpha( ) const noexcept -> int32_t{
                const auto diff = std::chrono::duration_cast< std::chrono::milliseconds >(
                    std::chrono::steady_clock::now( ) - last_add_time
                ).count( );

                if ( diff <= 9000 ) return 255;

                return 255 - static_cast< int32_t >( ( static_cast< float >( ( diff - 9000 ) ) / 1000.f ) * 255.f );
            }
        };

        struct Size {
            std::chrono::steady_clock::time_point last_add_time{ };
            size_t                                size;

            [[nodiscard]] auto get_alpha( ) const noexcept -> int32_t{
                const auto diff = std::chrono::duration_cast< std::chrono::milliseconds >(
                    std::chrono::steady_clock::now( ) - last_add_time
                ).count( );

                if ( diff <= 9000 ) return 255;

                return 255 - static_cast< int32_t >( ( static_cast< float >( ( diff - 9000 ) ) / 1000.f ) * 255.f );
            }
        };

    public:
        ~DebugOverlay( ){ std::lock_guard lock( m_mutex ); }

        auto track_time( const std::string& key, const int64_t ms ) noexcept -> void{
            std::lock_guard lock( m_mutex );

            auto& timing = m_timings[ key ];

            timing.timings[ ( timing.index++ ) % DEBUG_ENTRIES ] = ms;
            timing.last_add_time                                 = std::chrono::steady_clock::now( );
        }

        auto track_size( const std::string& key, const size_t bytes ) noexcept -> void{
            std::lock_guard lock( m_mutex );

            auto& s = m_sizes[ key ];

            s.last_add_time = std::chrono::steady_clock::now( );
            s.size          = bytes;
        }

        auto draw( ) noexcept -> void{
            m_draw_position = Vec2( 100, 80 );

            draw_timings( );
            draw_sizes( );

            remove_old_timings( );
        }

    private:
        auto draw_timings( ) noexcept -> void;
        auto draw_sizes( ) noexcept -> void;

        auto remove_old_timings( ) noexcept -> void;
        auto remove_old_sizes( ) noexcept -> void;

        static auto get_timings(
            const Timing& timing
        ) noexcept -> std::expected< std::tuple< float, float, float >, const char* >;

    private:
        Vec2 m_draw_position{ };

        std::mutex                      m_mutex;
        std::map< std::string, Timing > m_timings{ };
        std::map< std::string, Size >   m_sizes{ };
    };
}

extern std::shared_ptr< overlay::DebugOverlay > g_debug_overlay;
