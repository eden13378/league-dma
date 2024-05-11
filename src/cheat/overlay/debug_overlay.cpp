#include "pch.hpp"

#include "debug_overlay.hpp"

#include "../renderer/c_renderer.hpp"

std::shared_ptr< overlay::DebugOverlay > g_debug_overlay = std::make_shared< overlay::DebugOverlay >( );

namespace overlay {
    auto DebugOverlay::get_timings(
        const Timing& timing
    ) noexcept -> std::expected< std::tuple< float, float, float >, const char* >{
        float min = std::numeric_limits< float >::max( );
        float max = 0.f;
        float avg = 0.f;

        for ( const auto& t : timing.timings ) {
            const auto current = static_cast< float >( t ) / 1000.f;

            avg += current;
            if ( current < min ) min = current;
            if ( current > max ) max = current;
        }

        if ( max == 0.f ) return std::unexpected( "max = 0" );
        if ( avg == 0.f ) return std::unexpected( "avg = 0" );
        if ( min == std::numeric_limits< float >::max( ) ) return std::unexpected( "min = infinite" );

        return std::make_tuple( avg / static_cast< float >( DEBUG_ENTRIES ), min, max );
    }

    auto DebugOverlay::draw_timings( ) noexcept -> void{
        for ( auto timing : m_timings ) {
            auto timings = get_timings( timing.second );
            if ( !timings ) continue;
            auto text = std::format(
                "{}: {:.5f}s | min( {:.5f}s ) max( {:.5f}s )",
                timing.first,
                std::get< 0 >( *timings ),
                std::get< 1 >( *timings ),
                std::get< 2 >( *timings )
            );

            g_render->text(
                m_draw_position,
                Color::white( ).alpha( timing.second.get_alpha( ) ),
                g_fonts->get_default( ),
                text.data( ),
                16.f
            );

            m_draw_position.y += 18.f;
        }
    }

    auto DebugOverlay::draw_sizes( ) noexcept -> void{
        for ( auto s : m_sizes ) {
            const auto size_kb = static_cast< float >( s.second.size ) / 1000.f;

            std::string text;
            if ( size_kb < 1000.f ) {
                text = std::format(
                    "{}: {:.3f}kb",
                    s.first,
                    size_kb
                );
            } else {
                text = std::format(
                    "{}: {:.3f}mb",
                    s.first,
                    size_kb / 1000.f
                );
            }

            g_render->text(
                m_draw_position,
                Color::white( ).alpha( s.second.get_alpha( ) ),
                g_fonts->get_default( ),
                text.data( ),
                16.f
            );

            m_draw_position.y += 18.f;
        }
    }

    auto DebugOverlay::remove_old_timings( ) noexcept -> void{
        std::lock_guard lock( m_mutex );

        try {
            const auto to_remove = std::ranges::find_if(
                m_timings,
                []( auto t ) -> bool{
                    return t.second.last_add_time + std::chrono::seconds( 10 ) < std::chrono::steady_clock::now( );
                }
            );

            if ( to_remove != m_timings.end( ) ) m_timings.erase( to_remove );
        } catch ( ... ) {
        }
    }

    auto DebugOverlay::remove_old_sizes( ) noexcept -> void{
        std::lock_guard lock( m_mutex );
        try {
            const auto to_remove = std::ranges::find_if(
                m_sizes,
                []( auto t ) -> bool{
                    return t.second.last_add_time + std::chrono::seconds( 10 ) < std::chrono::steady_clock::now( );
                }
            );

            if ( to_remove != m_sizes.end( ) ) m_sizes.erase( to_remove );
        } catch ( ... ) {
        }
    }
}
