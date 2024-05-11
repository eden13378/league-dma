#include "pch.hpp"

#include "c_component.hpp"

#include <ranges>

#include "../c_window.hpp"
#include "../../../renderer/c_fonts.hpp"

namespace menu::framework::components {
    constexpr auto color_picker_width = 150;
    constexpr auto color_picker_height = 150;

    auto c_component::right_click_component_t::is_hovered( ) const -> bool{
        return parent->cursor_in_rect( position, size );
    }

    void c_component::right_click_checkbox_t::draw( ){
        const auto text_size = g_render->get_text_size( label, g_fonts->get_block( ), 8.f );

        parent->get_draw_list( )->text_shadow(
            position + Vec2( 0.f, ( size.y / 2.f ) - ( text_size.y / 2.f ) ),
            Color::white( ).alpha( 150 ),
            g_fonts->get_block( ),
            label.data( ),
            8.f,
            5000020
        );
    }

    bool c_component::right_click_button_t::process_input( ){
        if ( is_hovered( ) && parent->get_mouse_state( ) == c_window::EMouseState::left_up ) {
            callback( );
            return true;
        }

        return false;
    }

    void c_component::right_click_button_t::draw( ){
        const auto text_size = g_render->get_text_size( label, g_fonts->get_block( ), 8.f );

        parent->get_draw_list( )->text_shadow(
            position + Vec2( ( size.x / 2.f ) - ( text_size.x / 2.f ), ( size.y / 2.f ) - ( text_size.y / 2.f ) ),
            Color::white( ).alpha( 150 ),
            g_fonts->get_block( ),
            label.data( ),
            8.f,
            5000020
        );
    }

    bool c_component::right_click_color_t::process_input( ){
        old_opened = opened;

        const auto old_has_mouse_down_alpha = has_mouse_down_alpha;
        const auto old_has_mouse_down_base_color = has_mouse_down_base_color;
        const auto old_has_mouse_down_color_picker = has_mouse_down_color_picker;

        if ( opened ) {
            const auto picker_position = position + Vec2( size.x - 20.f, 10.f );
            const auto inner_position = picker_position + Vec2( 5, 5 );

            if ( !has_mouse_down_base_color && !has_mouse_down_alpha &&
                parent->cursor_in_rect( Vec2( inner_position.x, inner_position.y ), Vec2( color_picker_width, color_picker_height ) )
            ) {
                if ( !has_mouse_down_color_picker && parent->get_mouse_state( ) == c_window::EMouseState::left_down ) has_mouse_down_color_picker = true;
            }

            if ( !has_mouse_down_color_picker && !has_mouse_down_alpha &&
                parent->cursor_in_rect( Vec2( inner_position.x, inner_position.y + color_picker_height + 10 ), Vec2( color_picker_width, 10 ) )
            ) {
                if ( !has_mouse_down_base_color &&
                    parent->get_mouse_state( ) == c_window::EMouseState::left_down
                )
                    has_mouse_down_base_color = true;
            }

            if ( !has_mouse_down_color_picker && !has_mouse_down_base_color &&
                parent->cursor_in_rect( Vec2( inner_position.x, inner_position.y + color_picker_height + 25 ), Vec2( color_picker_width, 10 ) )
            ) {
                if ( !has_mouse_down_alpha &&
                    parent->get_mouse_state( ) == c_window::EMouseState::left_down
                )
                    has_mouse_down_alpha = true;
            }

            if ( has_mouse_down_base_color ) {
                const auto d = parent->get_cursor_position( ) - Vec2( inner_position.x, inner_position.y + color_picker_height + 10 );
                base_color = percent_to_color( std::clamp( d.x / static_cast< float >( color_picker_width ), 0.f, 1.f ) );

                current_color->get< Color >( ) = position_to_color( base_color, last_color_picker_delta );
                current_color->get< Color >( ).a = last_alpha_value;
            }

            if ( has_mouse_down_alpha ) {
                const auto d = parent->get_cursor_position( ) - Vec2( inner_position.x, inner_position.y + color_picker_height + 25 );

                current_color->get< Color >( ).a = static_cast< int32_t >( 255.f * std::clamp( d.x / static_cast< float >( color_picker_width ), 0.f, 1.f ) );
                last_alpha_value = current_color->get< Color >( ).a;
            }

            if ( has_mouse_down_color_picker ) {
                auto d = parent->get_cursor_position( ) - Vec2( inner_position.x, inner_position.y );
                d.x = static_cast< float >( std::clamp( static_cast< int >( d.x ), 0, color_picker_width ) );
                d.y = static_cast< float >( std::clamp( static_cast< int >( d.y ), 0, color_picker_height ) );

                current_color->get< Color >( ) = position_to_color( base_color, d );
                last_color_picker_delta = d;
                current_color->get< Color >( ).a = last_alpha_value;
            }
        } else {
            const auto s = size - Vec2( parent->get_padding( ) * 2.f, parent->get_padding( ) );
            const auto outline_start = position + Vec2( 0.f, parent->get_padding( ) );

            if ( parent->cursor_in_rect( outline_start + Vec2( s.x - 20.f, 0.f ), Vec2( 20.f, s.y ) ) &&
                parent->get_mouse_state( ) == c_window::EMouseState::left_up
            )
                opened = true;
        }

        if ( parent->get_mouse_state( ) == c_window::EMouseState::left_up ) {
            has_mouse_down_base_color = false;
            has_mouse_down_color_picker = false;
            has_mouse_down_alpha = false;
        }

        const auto picker_position = position + Vec2( size.x - 20.f, 10.f );

        if ( opened &&
            !has_mouse_down_alpha && !old_has_mouse_down_alpha &&
            !has_mouse_down_base_color && !old_has_mouse_down_base_color &&
            !has_mouse_down_color_picker && !old_has_mouse_down_color_picker &&
            parent->get_mouse_state( ) == c_window::EMouseState::left_up &&
            !parent->cursor_in_rect( picker_position, Vec2( color_picker_width + 10, color_picker_height + 45 ) )
        ) {
            opened = false;
        }

        return old_opened;
    }

    void c_component::right_click_color_t::draw( ){
        const auto s = size - Vec2( parent->get_padding( ) * 2.f, parent->get_padding( ) );
        const auto text_size = g_render->get_text_size( label.data( ), g_fonts->get_block( ), 8.f );

        const auto outline_start = position + Vec2( 0.f, parent->get_padding( ) );

        parent->get_draw_list( )->text_shadow(
            outline_start + Vec2( 0.f, s.y / 2.f - ( text_size.y / 2.f ) ),
            Color::white( ).alpha( 150 ),
            g_fonts->get_block( ),
            label.data( ),
            8.f,
            5000020
        );

        parent->get_draw_list( )->filled_box(
            outline_start + Vec2( s.x - 19.f, 1.f ),
            Vec2( 18.f, s.y - 2 ),
            current_color->get< Color >( ),
            0.f,
            5000020
        );
        parent->get_draw_list( )->gradient(
            outline_start + Vec2( s.x - 19.f, 1.f ),
            Vec2( 18.f, s.y - 2 ),
            Color::black( ).alpha( 0 ),
            Color::black( ).alpha( 0 ),
            Color::black( ).alpha( 150 ),
            Color::black( ).alpha( 150 ),
            5000020
        );
        parent->get_draw_list( )->box(
            outline_start + Vec2( s.x - 20.f, 0.f ),
            Vec2( 20.f, s.y ),
            Color::white( ).alpha( 150 ),
            0.f,
            1.f,
            5000021
        );

        if ( opened ) {
            const auto picker_position = position + Vec2( size.x - 20.f, 10.f );

            parent->get_top_draw_list( )->filled_box(
                picker_position,
                Vec2( color_picker_width + 10, color_picker_height + 45 ),
                Color::black( ),
                0.f,
                5000021
            );

            const auto inner_position = picker_position + Vec2( 5, 5 );

            for ( int i = 0; i < color_picker_width; ++i ) {
                parent->get_top_draw_list( )->line(
                    Vec2( inner_position.x + i, inner_position.y + color_picker_height + 10 ),
                    Vec2( inner_position.x + i, inner_position.y + 10 + color_picker_height + 10 ),
                    percent_to_color( static_cast< float >( i ) / static_cast< float >( color_picker_width ) ),
                    1.f,
                    5000022
                );
            }

            for ( int i = 0; i < color_picker_width; ++i ) {
                parent->get_top_draw_list( )->line(
                    Vec2( inner_position.x + i, inner_position.y + color_picker_height + 25 ),
                    Vec2( inner_position.x + i, inner_position.y + 10 + color_picker_height + 25 ),
                    Color::white( ).alpha( static_cast< int32_t >( 255.f * ( static_cast< float >( i ) / static_cast< float >( color_picker_width ) ) ) ),
                    1.f,
                    5000022
                );
            }

            for ( int y = 0; y < color_picker_height; y += 2 ) {
                for ( int x = 0; x < color_picker_width; x += 2 ) {
                    parent->get_top_draw_list( )->filled_box(
                        Vec2( x + inner_position.x, y + inner_position.y ),
                        Vec2( 2.f, 2.f ),
                        position_to_color( base_color, Vec2( static_cast< float >( x ), static_cast< float >( y ) ) ),
                        1.f,
                        5000022
                    );
                }
            }

            parent->get_top_draw_list( )->circle( inner_position + last_color_picker_delta, Color::black( ), 3.f, 10, 3.f, 5000023 );
            parent->get_top_draw_list( )->circle( inner_position + last_color_picker_delta, Color::white( ), 3.f, 10, 1.f, 5000024 );
        }
    }

    bool c_component::right_click_color_t::is_hovered( ) const{
        if ( has_mouse_down_alpha || has_mouse_down_base_color || has_mouse_down_color_picker ) return true;

        const auto hov = right_click_component_t::is_hovered( );

        if ( opened ) return true;

        // if ( opened ) {
        //     const auto picker_position = position + vec2( size.x - 20.f, 0.f );
        //     return hov || parent->cursor_in_rect( picker_position, vec2( color_picker_width + 10, color_picker_height + 30 ) );
        // }

        return hov;
    }

    auto c_component::right_click_color_t::position_to_color( Color base_color, Vec2 position ) -> Color{
        const auto mod = ( position.x / static_cast< float >( color_picker_width ) );

        const auto dark_mod = ( ( position.y / static_cast< float >( color_picker_height ) ) );

        const auto calc_color = [&]( int32_t input ) -> int32_t{
            const auto x_clr = 255 - static_cast< int32_t >( ( 255 - input ) * mod );

            return x_clr - static_cast< int32_t >( x_clr * dark_mod );
        };

        return Color(
            calc_color( base_color.r ),
            calc_color( base_color.g ),
            calc_color( base_color.b )
        );
    }

    auto c_component::right_click_color_t::percent_to_color( float p ) -> Color{
        int32_t i = static_cast< int32_t >( p * ( 255 * 6 ) );

        Color c;
        if ( i <= 255 ) {
            c = Color( 255, i, 0 );
        } else if ( i <= 255 * 2 ) {
            c = Color( 255 - ( i - 255 ), 255, 0 );
        } else if ( i <= 255 * 3 ) {
            c = Color( 0, 255, i - ( 255 * 2 ) );
        } else if ( i <= 255 * 4 ) {
            c = Color( 0, 255 - ( i - 255 * 3 ), 255 );
        } else if ( i <= 255 * 5 ) {
            c = Color( i - ( 255 * 4 ), 0, 255 );
        } else if ( i <= 255 * 6 ) {
            c = Color( 255, 0, 255 - ( i - 255 * 5 ) );
        } else {
            return Color( );
        }
        return c;
    }

    void c_component::right_click_dropdown_t::draw( ){
        const auto s = size - Vec2( parent->get_padding( ) * 2.f, parent->get_padding( ) );
        const auto text_size = g_render->get_text_size( label.data( ), g_fonts->get_block( ), 8.f );

        const auto outline_start = position + Vec2( 0.f, parent->get_padding( ) );

        parent->get_draw_list( )->text_shadow(
            outline_start + Vec2( 0.f, s.y / 2.f - ( text_size.y / 2.f ) ),
            Color::white( ).alpha( 150 ),
            g_fonts->get_block( ),
            label.data( ),
            8.f,
            5000020
        );

        const auto box_start = outline_start + Vec2( text_size.x + parent->get_padding( ), 0.f );
        const auto box_size = s - Vec2( text_size.x + parent->get_padding( ), 0.f );
        parent->get_draw_list( )->box(
            box_start,
            box_size,
            Color::white( ).alpha( 50 ),
            0.f,
            1.f,
            5000020
        );

        auto sel = selected ?
                       &selected->get< int32_t >( ) :
                       selected_raw;

        const auto item_size = g_render->get_text_size( items[ *sel ], g_fonts->get_block( ), 8.f );

        if ( !opened ) {
            parent->get_draw_list( )->text_shadow(
                box_start + Vec2( box_size.x / 2.f - item_size.x / 2.f, box_size.y / 2.f - item_size.y / 2.f ),
                Color::white( ).alpha( 150 ),
                g_fonts->get_block( ),
                items[ *sel ].data( ),
                8.f,
                5000020
            );
        }

        if ( opened ) {
            parent->get_draw_list( )->filled_box(
                box_start,
                Vec2( box_size.x, static_cast< float >( items.size( ) ) * ( box_size.y + parent->get_padding( ) ) ),
                Color::black( ),
                0.f,
                5000080
            );

            float offset_y = parent->get_padding( );
            int32_t j = 0;
            for ( auto i : items ) {
                auto       s           = g_render->get_text_size( i, g_fonts->get_block( ), 8.f );
                const auto hover_start = box_start + Vec2( parent->get_padding( ), offset_y - parent->get_padding( ) );
                const auto hover_size  = box_size - Vec2( parent->get_padding( ) * 2.f, 0.f );
                if ( j == hovered ) {
                    parent->get_draw_list( )->filled_box(
                        hover_start,
                        hover_size,
                        Color::white( ).alpha( 25 ),
                        0.f,
                        5000081
                    );
                } else if ( *sel == j ) {
                    parent->get_draw_list( )->filled_box(
                        hover_start,
                        hover_size,
                        Color::white( ).alpha( 15 ),
                        0.f,
                        5000081
                    );
                }
                parent->get_draw_list( )->text_shadow(
                    box_start + Vec2( box_size.x / 2.f - s.x / 2.f, offset_y + 2.f ),
                    Color::white( ).alpha( 150 ),
                    g_fonts->get_block( ),
                    i.data( ),
                    8.f,
                    5000082
                );
                offset_y += box_size.y + parent->get_padding( );
                ++j;
            }
        }
    }

    auto c_component::right_click_dropdown_t::process_input( ) -> bool{
        const auto s = size - Vec2( parent->get_padding( ) * 2.f, parent->get_padding( ) );
        const auto text_size = g_render->get_text_size( label.data( ), g_fonts->get_block( ), 8.f );

        const auto outline_start = position + Vec2( 0.f, parent->get_padding( ) );

        const auto box_start = outline_start + Vec2( text_size.x + parent->get_padding( ), 0.f );
        const auto box_size = s - Vec2( text_size.x + parent->get_padding( ), 0.f );

        auto sel = selected ?
                       &selected->get< int32_t >( ) :
                       selected_raw;

        if ( opened ) {
            float offset_y = parent->get_padding( );
            int32_t j = 0;
            hovered = -1;
            for ( auto i : items ) {
                auto       s           = g_render->get_text_size( i, g_fonts->get_block( ), 8.f );
                const auto hover_start = box_start + Vec2( parent->get_padding( ), offset_y - parent->get_padding( ) );
                const auto hover_size  = box_size - Vec2( parent->get_padding( ) * 2.f, 0.f );

                if ( parent->cursor_in_rect( hover_start, hover_size ) ) {
                    hovered = j;
                    if ( parent->get_mouse_state( ) == c_window::EMouseState::left_up ) *sel = j;
                }

                offset_y += box_size.y + parent->get_padding( );
                ++j;
            }
        }

        if ( !opened && is_hovered( ) && parent->get_mouse_state( ) == c_window::EMouseState::left_up ) opened = true;
        else if ( hovered == -1 && parent->get_mouse_state( ) == c_window::EMouseState::left_up )
            opened = false;

        return opened;
    }

    bool c_component::right_click_dropdown_t::is_hovered( ) const{
        if ( opened ) return true;
        return right_click_component_t::is_hovered( );
    }

    void c_component::right_click_keybind_t::draw( ){
        const auto s = size - Vec2( parent->get_padding( ) * 2.f, parent->get_padding( ) );
        const auto text_size = g_render->get_text_size( _( "KEYBIND" ), g_fonts->get_block( ), 8.f );

        const auto outline_start = position + Vec2( 0.f, parent->get_padding( ) );

        // debug_log( "keybind: {} {}", keybind->key, keybind->mode );

        parent->get_draw_list( )->box( outline_start, s, Color::white( ).alpha( 50 ), 0.f, 1.f, 5000020 );
        parent->get_draw_list( )->text_shadow(
            outline_start + Vec2( 4.f, -( text_size.y / 2.f ) + 1.f ),
            Color::white( ).alpha( 150 ),
            g_fonts->get_block( ),
            _( "KEYBIND" ),
            8.f,
            5000020
        );

        const auto get_waiting = []( ) -> std::string{
            const auto since_epoch = std::chrono::steady_clock::now( ).time_since_epoch( );
            const auto time = std::chrono::duration_cast< std::chrono::milliseconds >( since_epoch ).count( ) / 500;

            switch ( time % 4 ) {
            case 0:
                return _( "waiting" );
            case 1:
                return _( "waiting." );
            case 2:
                return _( "waiting.." );
            case 3:
                return _( "waiting..." );
            default: ;
            }

            return _( "" );
        };

        std::string text = waiting_for_input ?
                               get_waiting( ) :
                               keybind->key == -1 ?
                               _( "NONE" ) :
                               helper::key_to_string( static_cast< utils::EKey >( keybind->key ) );

        std::ranges::transform( text, text.begin( ), ::toupper );
        const auto text_size2 = g_render->get_text_size( text, g_fonts->get_block( ), 8.f );

        parent->get_draw_list( )->text_shadow(
            outline_start + Vec2( s.x / 2.f - text_size2.x / 2.f, s.y / 2.f - text_size2.y / 2.f ),
            Color::white( ).alpha( 150 ),
            g_fonts->get_block( ),
            text.data( ),
            8.f,
            5000020
        );

    }

    auto c_component::right_click_keybind_t::process_input( ) -> bool{
        if ( is_hovered( ) ) {
            if ( parent->get_mouse_state( ) == c_window::EMouseState::left_up ) {
                waiting_for_input = true;
                parent->reset_last_key_pressed( );
            }
        } else {
            if ( parent->get_mouse_state( ) == c_window::EMouseState::left_up ) {
                waiting_for_input = false;
            }
        }

        auto pressed = parent->get_last_key_pressed( );
        if ( waiting_for_input && pressed ) {
            if ( *pressed != utils::EKey::escape ) keybind->key = static_cast< int32_t >( *pressed );
            else keybind->key = -1;
            waiting_for_input = false;
        }

        return waiting_for_input;
    }

    auto c_component::right_click_menu_t::get_height( ) const -> float{
        float height = 0.f;
        for ( const auto c : components ) {
            height += c->get_height( ) + parent->get_padding( );
        }
        return height;
    }

    auto c_component::right_click_menu_t::is_hovered( Vec2 position ) const -> bool{
        if ( !parent ) return false;
        const auto h = parent->cursor_in_rect( position, Vec2( right_click_menu_width, get_height( ) ) );

        if ( h ) return true;

        return std::ranges::any_of(
            components,
            []( const std::shared_ptr< right_click_component_t >& c ) -> bool{
                return c->is_hovered( );
            }
        );
    }

    auto c_component::draw_tooltip( ) -> void{
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
                5000000
            );
            draw_list( )->text(
                m_tooltip_hover_start + Vec2( m_parent->get_padding( ), m_parent->get_padding( ) ),
                Color::white( ).alpha( static_cast< int32_t >( background_progress * 255.f ) ),
                g_fonts->get_default_small( ),
                render_tooltip,
                12.f,
                5000001
            );
        }
    }

    auto c_component::draw_right_click_menu( ) -> void{
        if ( !m_right_click_menu_open || m_right_click_menu.components.empty( ) ) return;

        m_right_click_menu.parent = m_parent;

        const auto start_position = m_position + Vec2( m_size.x - right_click_menu_width + m_parent->get_padding( ), 0.f );

        m_parent->get_draw_list( )->filled_box( start_position, Vec2( right_click_menu_width, m_right_click_menu.get_height( ) ), Color::black( ), 0.f, 5000010 );

        float offset_y = 0.f;
        for ( const auto component : m_right_click_menu.components ) {
            component->set_parent( m_parent );
            if ( !m_right_click_menu.active_component || m_right_click_menu.active_component == component ) {
                if ( component->process_input( ) )
                    m_right_click_menu.active_component = component;
                else if ( component == m_right_click_menu.active_component ) m_right_click_menu.active_component.reset( );
            }

            component->set_position( Vec2( start_position.x + m_parent->get_padding( ), start_position.y + offset_y ) );
            component->draw( );
            offset_y += component->get_height( ) + m_parent->get_padding( );
        }
    }

    auto c_component::process_input( ) -> bool{
        m_right_click_menu_open_last = m_right_click_menu_open;
        const auto start_position = m_position + Vec2( m_size.x - right_click_menu_width + m_parent->get_padding( ), 0.f );

        if ( is_hovered( ) ) {
            if ( !m_was_hovered_tooltip ) {
                m_was_hovered_tooltip = true;
                get_animation_state( tooltip_delay )->start( );
                m_tooltip_hover_start = m_parent->get_cursor_position( ) + Vec2( 8.f, 0.f );
                m_started_tooltip_fade = false;
            }

            if ( m_parent->get_mouse_state( ) == c_base_window::EMouseState::right_up ) {
                m_right_click_menu_open = !m_right_click_menu_open;
            }

            if ( m_right_click_menu_open &&
                m_parent->get_mouse_state( ) == c_base_window::EMouseState::left_up &&
                !m_right_click_menu.is_hovered( start_position )
            )
                m_right_click_menu_open = false;

            if ( get_animation_state( tooltip_delay )->get_progress( ) == 1.f &&
                !m_started_tooltip_fade
            ) {
                get_animation_state( tooltip_show )->start( );
                m_started_tooltip_fade = true;
            }
        } else {
            m_was_hovered_tooltip = false;

            if ( m_parent->get_mouse_state( ) == c_base_window::EMouseState::left_up ||
                m_parent->get_mouse_state( ) == c_base_window::EMouseState::right_up
            ) {
                if ( m_right_click_menu.parent && !m_right_click_menu.is_hovered( start_position ) )
                    m_right_click_menu_open = false;
            }
        }

        return false;
    }

    auto c_component::is_hovered( ) const -> bool{
        return m_parent->cursor_in_rect( m_position, m_size );
    }

    auto c_component::is_hovered( Vec2 position, Vec2 size ) const -> bool{
        return m_parent->cursor_in_rect( position, size );
    }

    auto c_component::draw_list( ) const -> c_draw_list*{
        return m_parent->get_draw_list( );
    }

    auto c_component::reset( ) -> void{
        for ( auto& state : m_animation_states | std::views::values )
            state.reset( );
        m_selected = false;
    }
}
