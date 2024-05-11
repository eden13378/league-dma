#include "pch.hpp"

#include "visuals.hpp"

#include <ranges>

#include "entity_list.hpp"
#include "../config/c_config.hpp"
#include "../renderer/c_fonts.hpp"


namespace features {
#if __DEBUG && profiling_overlay
    auto c_visuals::draw_debug_overlay( ) const -> void {
        float y_offset = 0.f;
        const auto add_text = [&]( const std::string& text ) -> void{
            const auto text_size = g_render->get_text_size( text, g_fonts->get_default( ), 16 );
            g_render->text( vec2( 1500.f, 100.f + y_offset ), color::white( ), g_fonts->get_default( ), text.data( ), 16 );

            y_offset += text_size.y + 6.f;
        };

        for ( const auto& val : debug_profiling_times | std::views::values ) {
            add_text( std::format( "{}: took {}mis    max: {}mis", val.name, val.time, val.max ) );
        }
    }
#endif
#if _DEBUG && enable_lua && lua_callback_debug
    auto c_visuals::draw_lua_calls( ) -> void {
        float y_offset = 0.f;
        const auto add_text = [&]( const std::string& text ) -> void{
            const auto text_size = g_render->get_text_size( text, g_fonts->get_default( ), 16 );
            g_render->text( vec2( 1000.f, 100.f + y_offset ), color::white( ), g_fonts->get_default( ), text.data( ), 16 );

            y_offset += text_size.y + 6.f;
        };

        add_text( std::format( "feature_calls: {}", g_lua->get_feature_calls( ) ) );
        add_text( std::format( "pre_feature_calls: {}", g_lua->get_pre_feature_calls( ) ) );
        add_text( std::format( "render_calls: {}", g_lua->get_render_calls( ) ) );
    }
#endif
    auto Visuals::draw_enemy_attack_range( ) -> void{
        return;
        if ( !g_config->visuals.enemy_attack_range->get< bool >( ) ) return;

        const auto& enemies = g_entity_list.get_enemies( );

        for ( const auto enemy : enemies ) {
            if ( !enemy || enemy->is_invisible( ) || enemy->is_dead( ) ) continue;

            const auto distance = enemy->dist_to_local( );

            g_render->circle_3d(
                enemy->position,
                distance <= enemy->attack_range ? Color::red( ).alpha( 50 ) : Color::white( ).alpha( 50 ),
                enemy->attack_range + 65.f,
                Renderer::E3dCircleFlags::filled | Renderer::E3dCircleFlags::outline
            );
        }
    }
}
