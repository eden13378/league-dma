#include "pch.hpp"

#include "c_component_base.hpp"

#include <ranges>

#include "../c_base_window.hpp"
#include "../../../renderer/c_fonts.hpp"

namespace menu::framework::components {
    auto c_component_base::draw_tooltip( ) -> void{
        constexpr auto max_tooltip_width = 150.f;

        if ( is_hovered( ) && get_animation_state( tooltip_delay )->get_progress( ) == 1.f && m_tooltip ) {
            // const auto space_size = g_render->get_text_size( _( " " ), g_fonts->get_default_small( ), 12.f );

            auto        size           = g_render->get_text_size( *m_tooltip, g_fonts->get_default_small( ), 12.f );
            std::string render_tooltip = *m_tooltip;

            if ( size.x > max_tooltip_width ) {
                std::vector< std::string > words{ };
                std::string current_word;

                for ( auto c : *m_tooltip ) {
                    if ( c != ' ' ) {
                        current_word += c;
                    } else {
                        words.push_back( current_word );
                        current_word.clear( );
                    }
                }
                if ( !current_word.empty( ) ) {
                    words.push_back( current_word );
                }

                render_tooltip.clear( );
                float x_size = 0.f;

                for ( auto word : words ) {
                    const auto s = g_render->get_text_size( word, g_fonts->get_default_small( ), 12.f );

                    if ( x_size + s.x <= max_tooltip_width ) {
                        x_size += s.x;
                        render_tooltip += word + _( " " );
                    } else {
                        render_tooltip += _( "\n" ) + word + _( " " );
                        x_size = s.x;
                    }
                }

                render_tooltip = render_tooltip.substr( 0, render_tooltip.size( ) - 1 );
                size = g_render->get_text_size( render_tooltip, g_fonts->get_default_small( ), 12.f );
            }

            const auto background_progress = get_animation_state( tooltip_show )->get_progress( );

            draw_list( )->filled_box(
                m_tooltip_hover_start,
                Vec2( size.x + m_parent->get_padding( ) * 2.f, size.y + m_parent->get_padding( ) * 2.f ),
                Color::black( ).alpha( static_cast< int32_t >( background_progress * 255.f ) ),
                0.f,
                std::numeric_limits<int>::max(  )
            );
            draw_list( )->text(
                m_tooltip_hover_start + Vec2( m_parent->get_padding( ), m_parent->get_padding( ) ),
                Color::white( ).alpha( static_cast< int32_t >( background_progress * 255.f ) ),
                g_fonts->get_default_small( ),
                render_tooltip,
                12.f,
                std::numeric_limits<int>::max(  )
            );
        }
    }

    auto c_component_base::process_input( ) -> bool{
        const auto start_position = m_position + Vec2( m_size.x - right_click_menu_width + m_parent->get_padding( ), 0.f );

        if ( is_hovered( ) ) {
            if ( !m_was_hovered_tooltip ) {
                m_was_hovered_tooltip = true;
                get_animation_state( tooltip_delay )->start( );
                m_tooltip_hover_start = m_parent->get_cursor_position( ) + Vec2( 8.f, 0.f );
                m_started_tooltip_fade = false;
            }
            if ( get_animation_state( tooltip_delay )->get_progress( ) == 1.f &&
                !m_started_tooltip_fade
            ) {
                get_animation_state( tooltip_show )->start( );
                m_started_tooltip_fade = true;
            }
        } else {
            m_was_hovered_tooltip = false;
        }

        return false;
    }

    auto c_component_base::is_hovered( ) const -> bool{
        return m_parent->cursor_in_rect( m_position, m_size );
    }

    auto c_component_base::is_hovered( Vec2 position, Vec2 size ) const -> bool{
        return m_parent->cursor_in_rect( position, size );
    }

    auto c_component_base::draw_list( ) const -> c_draw_list*{
        return m_parent->get_draw_list( );
    }

    auto c_component_base::reset( ) -> void{
        for ( auto& state : m_animation_states | std::views::values )
            state.reset( );
        m_selected = false;
    }
}
