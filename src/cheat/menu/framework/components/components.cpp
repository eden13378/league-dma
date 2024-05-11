#include "pch.hpp"

#include "c_button.hpp"
#include "c_checkbox.hpp"
#include "c_conditional.hpp"
#include "c_multi_select.hpp"
#include "c_select.hpp"
#if enable_new_lua
#include "../../../lua-v2/lua_def.hpp"
#endif
#if enable_lua
#include <sol/sol.hpp>
#endif

namespace menu::framework::components {
    bool c_checkbox::process_input( ){
        c_component::process_input( );
        if ( is_right_click_menu_opened( ) ) return true;
        if ( m_parent->is_any_right_click_menu_opened( ) ) return false;
        if ( is_hovered( ) ) {
            if ( !m_was_hovered ) { get_animation_state( hover_background_animation )->start( ); }
            m_was_hovered = true;

            if ( m_parent->get_mouse_state( ) == c_base_window::EMouseState::left_down ) {
                m_has_mouse_down = true;
                return true;
            }

            // if ( m_parent->get_mouse_state( ) == c_window::e_mouse_state::right_up ) {
            //     m_keybind_opened = true;
            // }

            if ( m_parent->get_mouse_state( ) == c_base_window::EMouseState::left_up && m_has_mouse_down && !
                is_right_click_menu_opened( ) ) {
                m_has_mouse_down = false;
                if ( m_value ) m_value->get< bool >( ) = !m_value->get< bool >( );
                else if ( m_secondary_value ) *m_secondary_value = !*m_secondary_value;
                if ( m_callback && m_secondary_value ) ( *m_callback )( *m_secondary_value );
                get_animation_state( change_selected_animation )->start( );
            }
        } else if ( m_was_hovered ) {
            get_animation_state( hover_background_animation )->start( );
            m_was_hovered = false;
        } else {
            if ( m_parent->get_mouse_state( ) == c_base_window::EMouseState::left_up && m_has_mouse_down && !
                is_right_click_menu_opened( ) ) { m_has_mouse_down = false; }
        }

        return false;
    }

    auto c_checkbox::draw_component( ) -> void{
        const auto checkbox_position = m_position + Vec2( get_size( ).x - 16, ( get_height( ) - 16 ) / 2.f );

        auto an_prog = get_animation_state( hover_background_animation )->get_progress( );
        if ( is_hovered( ) || an_prog != 0.f ) {
            auto alpha = static_cast< int32_t >( 50.f * an_prog );
            if ( !is_hovered( ) ) alpha = 50 - alpha;
            g_render->filled_box( m_position, get_size( ), Color::black( ).alpha( alpha ), 3.f );
        }

        bool is_active = m_value ? m_value->get< bool >( ) : *m_secondary_value;

        g_render->filled_box( checkbox_position, Vec2( 16, 16 ), colors::outline, 4.f );
        const auto progress = get_animation_state( change_selected_animation )->get_progress( );
        if ( ( ( m_value && m_value->get< bool >( ) ) || ( !m_value && *m_secondary_value ) ) || progress != 1.f ) {
            g_render->filled_box(
                checkbox_position + Vec2( 4, 4 ),
                Vec2( 8, 8 ),
                Color::white( ).alpha(
                    static_cast< int32_t >( is_active ? 255.f * progress : ( 255 - 255.f * progress ) )
                ),
                2.f
            );
            if ( m_has_mouse_down ) {
                g_render->gradient(
                    checkbox_position + Vec2( 4, 4 ),
                    Vec2( 8, 8 ),
                    Color::black( ).alpha( 100 ),
                    Color::black( ).alpha( 100 ),
                    Color::black( ).alpha( 0 ),
                    Color::black( ).alpha( 0 )
                );
            } else {
                g_render->gradient(
                    checkbox_position + Vec2( 4, 4 ),
                    Vec2( 8, 8 ),
                    Color::black( ).alpha( 0 ),
                    Color::black( ).alpha( 0 ),
                    Color::black( ).alpha( 100 ),
                    Color::black( ).alpha( 100 )
                );
            }
        }

        const auto text_height = g_render->get_text_size( m_label, g_fonts->get_default( ), constants::font_size ).y;
        g_render->text(
            m_position + Vec2( 0, ( get_height( ) - text_height ) / 2.f - 1 ),
            Color::white( ),
            g_fonts->get_default( ),
            m_label.data( ),
            constants::font_size
        );
    }

#if enable_lua
    auto c_checkbox::set_value( sol::object value ) const -> void{
        lua_arg_check_ct_v( value, bool, "boolean" )
        m_value->get< bool >( ) = value.as< bool >( );
    }
#endif

    auto c_button::process_input( ) -> bool{
        c_component::process_input( );
        if ( is_right_click_menu_opened( ) ) return true;
        if ( m_parent->is_any_right_click_menu_opened( ) ) return false;
        if ( is_hovered( ) ) {
            if ( !m_was_hovered ) get_animation_state( hover_background_animation )->start( );
            m_was_hovered = true;
            if ( m_parent->get_mouse_state( ) == c_base_window::EMouseState::left_down ) {
                m_has_mouse_down = true;
                m_pressed        = true;
                get_animation_state( press_animation )->start( );
                return true;
            }

            if ( m_parent->get_mouse_state( ) == c_base_window::EMouseState::left_up && m_has_mouse_down ) {
                m_has_mouse_down = false;
                m_on_click( );
                m_pressed = false;
            }
        } else if ( m_was_hovered ) {
            get_animation_state( hover_background_animation )->start( );
            m_was_hovered = false;
        }

        return false;
    }

    auto c_button::draw_component( ) -> void{
        auto an_prog = get_animation_state( hover_background_animation )->get_progress( );
        if ( is_hovered( ) || an_prog != 0.f ) {
            auto alpha = static_cast< int32_t >( 50.f * an_prog );
            if ( !is_hovered( ) ) alpha = 50 - alpha;
            g_render->filled_box( m_position, get_size( ), Color::black( ).alpha( alpha ), 3.f );
        }


        if ( m_pressed || m_was_pressed ) {
            const auto progress = get_animation_state( press_animation )->get_progress( );

            g_render->filled_box(
                m_position,
                get_size( ),
                Color::white( ).alpha(
                    ( m_was_pressed
                          ? 20 - static_cast< int32_t >( progress * 20.f )
                          : static_cast< int32_t >( progress * 20.f ) )
                )
            );
        }

        const auto text_height = g_render->get_text_size( m_label, g_fonts->get_default( ), constants::font_size ).y;
        g_render->text(
            m_position + Vec2( m_parent->get_padding( ), ( get_height( ) - text_height ) / 2.f - 1 ),
            Color::white( ),
            g_fonts->get_default( ),
            m_label.data( ),
            constants::font_size
        );
    }

    auto c_button::get_height( ) const -> float{
        const auto progress = get_animation_state( ct_hash( "select" ) )->get_progress( );
        return get_size( ).y + ( is_selected( ) ? ( 8.f * progress ) : ( 8.f - 8.f * progress ) );
    }

    auto c_conditional::draw_component( ) -> void{
        if ( !m_should_enable( ) ) return;
        auto offset = 0.f;
        for ( auto component : m_items.children ) {
            if ( !component->draw_as_child( ) ) continue;
            component->set_position( Vec2( m_position.x, m_position.y + offset ) );
            component->set_width( get_size( ).x );

            if ( !m_parent->get_active_component( ) || m_parent->get_active_component( ) == component ) {
                if ( component->process_input( ) ) { m_parent->set_active_component( component ); } else
                    m_parent->
                        set_active_component( nullptr );
            }
            component->draw_component( );

            offset += component->get_height( ) + m_parent->get_padding( );
        }
    }

    auto c_multi_select::process_input( ) -> bool{
        c_component::process_input( );
        if ( is_right_click_menu_opened( ) ) return true;
        if ( m_parent->is_any_right_click_menu_opened( ) ) return false;
        if ( c_component::is_hovered( m_position, get_size( ) ) ) {
            if ( !m_was_hovered ) get_animation_state( hover_background_animation )->start( );
            m_was_hovered = true;
        } else if ( m_was_hovered ) {
            get_animation_state( hover_background_animation )->start( );
            m_was_hovered = false;
        }

        if ( c_component::is_hovered( m_position, get_size( ) ) && !m_active ) {
            if ( m_parent->get_mouse_state( ) == c_window::EMouseState::left_up &&
                !m_parent->get_active_component( ) )
                m_active = true;
            return false;
        }

        if ( m_active ) reinterpret_cast< c_window* >( m_parent )->set_active_component( this );

        const auto dropdown_size = calculate_dropdown_box( );
        if ( !c_component::is_hovered(
                m_position + Vec2( get_size( ).x - dropdown_size.x + m_parent->get_padding( ), 0.f ),
                dropdown_size
            ) &&
            m_parent->get_mouse_state( ) == c_window::EMouseState::left_up ) { m_active = false; }

        if ( m_active ) {
            float   draw_y = m_parent->get_padding( );
            int32_t i      = 0;
            for ( auto item : m_items ) {
                const auto size           = g_render->get_text_size( item, g_fonts->get_default( ), 16.f );
                const auto selected_start = m_position + Vec2(
                    get_size( ).x - dropdown_size.x + m_parent->get_padding( ),
                    draw_y
                );
                const auto selected_size = Vec2(
                    dropdown_size.x - m_parent->get_padding( ) * 2.f,
                    size.y - m_parent->get_padding( ) + 1.f
                );
                if ( c_component::is_hovered( selected_start, selected_size ) &&
                    m_parent->get_mouse_state( ) == c_window::EMouseState::left_up
                ) { m_values[ i ]->get< bool >( ) = !m_values[ i ]->get< bool >( ); }
                draw_y += size.y + m_parent->get_padding( );
                ++i;
            }
        }

        return m_active;
    }

    auto c_multi_select::calculate_dropdown_box( ) const -> Vec2{
        Vec2 s = { 100.f, 0.f };
        for ( auto item : m_items ) {
            const auto text_size = g_render->get_text_size( item, g_fonts->get_default( ), constants::font_size );
            const auto w         = text_size.x + m_parent->get_padding( ) * 2.f;
            s.y += text_size.y + m_parent->get_padding( );
            if ( w > s.x ) s.x = w;
        }
        s.y += m_parent->get_padding( ) - 3.f;
        return s;
    }

    auto c_multi_select::draw_component( ) -> void{
        auto an_prog = get_animation_state( hover_background_animation )->get_progress( );
        if ( is_hovered( ) || an_prog != 0.f ) {
            auto alpha = static_cast< int32_t >( 50.f * an_prog );
            if ( !is_hovered( ) ) alpha = 50 - alpha;
            g_render->filled_box( m_position, get_size( ), Color::black( ).alpha( alpha ), 3.f );
        }

        const auto triangle_start = m_position + Vec2(
            get_size( ).x - 8.f - m_parent->get_padding( ),
            get_size( ).y / 2.f - 4.f
        );
        g_render->filled_triangle(
            triangle_start,
            triangle_start + Vec2( 8.f, 0.f ),
            triangle_start + Vec2( 4.f, 8.f ),
            Color::white( )
        );

        const auto text_height = g_render->get_text_size( m_label, g_fonts->get_default( ), constants::font_size ).y;
        g_render->text(
            m_position + Vec2( 0, ( get_height( ) - text_height ) / 2.f - 1 ),
            Color::white( ),
            g_fonts->get_default( ),
            m_label.data( ),
            constants::font_size
        );

        if ( m_active ) {
            const auto s = calculate_dropdown_box( );
            draw_list( )->filled_box(
                m_position + Vec2( get_size( ).x - s.x, 0.f ),
                Vec2( s.x, s.y ),
                colors::background,
                0.f,
                5000
            );
            draw_list( )->box(
                m_position + Vec2( get_size( ).x - s.x - 1.f, -1.f ),
                Vec2( s.x + 1.f, s.y + 1.f ),
                colors::outline,
                0.f,
                2.f,
                5001
            );
            int32_t i = 0;
            float draw_y = m_parent->get_padding( );
            for ( auto item : m_items ) {
                const auto size = g_render->get_text_size( item, g_fonts->get_default( ), constants::font_size );
                const auto selected_start = m_position + Vec2( get_size( ).x - s.x + m_parent->get_padding( ), draw_y );
                const auto selected_size = Vec2(
                    s.x - m_parent->get_padding( ) * 2.f,
                    size.y - m_parent->get_padding( ) + 1.f
                );
                if ( c_component::is_hovered( selected_start, selected_size ) || m_values[ i ]->get<
                    bool >( ) )
                    draw_list( )->filled_box(
                        selected_start,
                        selected_size,
                        Color::black( ).alpha( 50 ),
                        0,
                        5002
                    );
                draw_list( )->text(
                    m_position + Vec2( get_size( ).x - s.x + m_parent->get_padding( ), draw_y - 2.f ),
                    Color::white( ),
                    m_values[ i ]->get< bool >( ) ? g_fonts->get_default_bold( ) : g_fonts->get_default( ),
                    item,
                    constants::font_size,
                    5003
                );
                draw_y += size.y + m_parent->get_padding( );
                ++i;
            }
        }
    }

    bool c_select::process_input( ){
        c_component::process_input( );

        if ( is_right_click_menu_opened( ) ) return true;
        if ( m_parent->is_any_right_click_menu_opened( ) ) return false;

        if ( c_component::is_hovered( m_position, get_size( ) ) ) {
            if ( !m_was_hovered ) get_animation_state( hover_background_animation )->start( );
            m_was_hovered = true;
        } else if ( m_was_hovered ) {
            get_animation_state( hover_background_animation )->start( );
            m_was_hovered = false;
        }

        if ( c_component::is_hovered( m_position, get_size( ) ) && !m_active ) {
            if (
                m_parent->get_mouse_state( ) == c_base_window::EMouseState::left_up &&
                !m_parent->get_active_component( )
            ) { m_active = true; }
            return false;
        }

        if ( m_active ) reinterpret_cast< c_window* >( m_parent )->set_active_component( this );

        const auto dropdown_size = calculate_dropdown_box( );
        if ( !c_component::is_hovered(
                m_position + Vec2( get_size( ).x - dropdown_size.x + m_parent->get_padding( ), 0.f ),
                dropdown_size
            ) &&
            m_parent->get_mouse_state( ) == c_base_window::EMouseState::left_up ) { m_active = false; }

        if ( m_active ) {
            float   draw_y = m_parent->get_padding( );
            int32_t i      = 0;
            for ( auto item : m_items ) {
                const auto size           = g_render->get_text_size( item, g_fonts->get_default( ), 16.f );
                const auto selected_start = m_position + Vec2(
                    get_size( ).x - dropdown_size.x + m_parent->get_padding( ),
                    draw_y
                );
                const auto selected_size = Vec2(
                    dropdown_size.x - m_parent->get_padding( ) * 2.f,
                    size.y - m_parent->get_padding( ) + 1.f
                );
                if ( c_component::is_hovered( selected_start, selected_size ) &&
                    m_parent->get_mouse_state( ) == c_base_window::EMouseState::left_up
                ) { m_value->get< int32_t >( ) = i; }
                draw_y += size.y + m_parent->get_padding( );
                ++i;
            }
        }

        return m_active;
    }

    auto c_select::calculate_dropdown_box( ) const -> Vec2{
        Vec2 s = { 100.f * g_config->misc.screen_scaling->get< float >( ), 0.f };
        for ( auto item : m_items ) {
            const auto text_size = g_render->get_text_size( item, g_fonts->get_default( ), constants::font_size );
            const auto w         = text_size.x + m_parent->get_padding( ) * 2.f;
            s.y += text_size.y + m_parent->get_padding( );

            // set max size
            if ( w > s.x ) s.x = w;
        }
        s.y += m_parent->get_padding( ) - 3.f * g_config->misc.screen_scaling->get< float >( );
        return s;
    }

    auto c_select::draw_component( ) -> void{
        static std::once_flag flag;
        std::call_once(
            flag,
            [&]( ) -> void{
                m_value->get< int32_t >( ) = std::clamp(
                    m_value->get< int32_t >( ),
                    0,
                    static_cast< int32_t >( m_items.size( ) - 1 )
                );
            }
        );

        auto an_prog = get_animation_state( hover_background_animation )->get_progress( );
        if ( is_hovered( ) || an_prog != 0.f ) {
            auto alpha = static_cast< int32_t >( 50.f * an_prog );
            if ( !is_hovered( ) ) alpha = 50 - alpha;
            g_render->filled_box( m_position, get_size( ), Color::black( ).alpha( alpha ), 3.f );
        }

        const auto triangle_start = m_position + Vec2(
            get_size( ).x - 8.f - m_parent->get_padding( ),
            get_size( ).y / 2.f - 4.f
        );

        const auto selected = std::clamp(
            m_value->get< int32_t >( ),
            0,
            static_cast< int32_t >( m_items.size( ) - 1 )
        );

        const auto selected_size = g_render->get_text_size(
            m_items[ selected ],
            g_fonts->get_default( ),
            constants::font_size
        );
        const auto text_pos = Vec2(
            triangle_start.x - m_parent->get_padding( ),
            m_position.y + get_size( ).y / 2.f - selected_size.y / 2.f
        ) - Vec2( selected_size.x + m_parent->get_padding( ) * 2.f, 0.f );
        g_render->filled_box(
            text_pos,
            Vec2( selected_size.x + m_parent->get_padding( ) * 2.f + 16.f, selected_size.y ),
            colors::background,
            0.f
        );
        g_render->box(
            text_pos,
            Vec2( selected_size.x + m_parent->get_padding( ) * 2.f + 16.f, selected_size.y ),
            colors::outline,
            0.f,
            2.f
        );
        g_render->filled_triangle(
            triangle_start,
            triangle_start + Vec2( 8.f, 0.f ),
            triangle_start + Vec2( 4.f, 8.f ),
            Color::white( )
        );
        g_render->text(
            text_pos + Vec2( m_parent->get_padding( ), -1.f ),
            Color::white( ),
            g_fonts->get_default( ),
            m_items[ selected ].data( ),
            constants::font_size
        );

        const auto text_height = g_render->get_text_size( m_label, g_fonts->get_default( ), constants::font_size ).y;
        g_render->text(
            m_position + Vec2( 0, ( get_height( ) - text_height ) / 2.f - 1 ),
            Color::white( ),
            g_fonts->get_default( ),
            m_label.data( ),
            constants::font_size
        );

        if ( m_active ) {
            const auto dropdown_box = calculate_dropdown_box( );

            auto size = get_size( );

            // if ( g_config ) size.x *= g_config->misc.screen_scaling->get< float >( );

            // g_render->filled_box(
            //     m_position + Vec2( size.x - dropdown_box.x, 0.f ),
            //     Vec2( dropdown_box.x, dropdown_box.y ),
            //     colors::background,
            //     0.f
            //     // 6000
            // );
            //
            // g_render->box(
            //     m_position + Vec2( size.x - dropdown_box.x - 1.f, -1.f ),
            //     Vec2( dropdown_box.x + 1.f, dropdown_box.y + 1.f ),
            //     colors::outline,
            //     0.f,
            //     2.f
            // );
            draw_list(  )->reset_clip_box(  std::numeric_limits<int32_t>::max(  ) -5 );
            // dropdown box
            draw_list( )->filled_box(
                m_position + Vec2( size.x - dropdown_box.x, 0.f ),
                Vec2( dropdown_box.x, dropdown_box.y ),
                colors::background,
                0.f,
                std::numeric_limits<int32_t>::max(  ) -4
            );
            // dropdown outline
            draw_list( )->box(
                m_position + Vec2( size.x - dropdown_box.x - 1.f, -1.f ),
                Vec2( dropdown_box.x + 1.f, dropdown_box.y + 1.f ),
                colors::outline,
                0.f,
                2.f,
                std::numeric_limits<int32_t>::max(  ) - 3
            );
            int32_t i = 0;
            float draw_y = m_parent->get_padding( );
            for ( auto item : m_items ) {
                const auto text_size = g_render->get_text_size( item, g_fonts->get_default( ), constants::font_size );
                const auto selected_start = m_position + Vec2(
                    size.x - dropdown_box.x + m_parent->get_padding( ),
                    draw_y
                );
                const auto selected_size = Vec2(
                    dropdown_box.x - m_parent->get_padding( ) * 2.f,
                    text_size.y - m_parent->get_padding( ) + 1.f
                );
                if ( c_component::is_hovered( selected_start, selected_size ) || selected == i )
                    draw_list( )->
                        filled_box( selected_start, selected_size, Color::black( ).alpha( 50 ), 0, std::numeric_limits<int32_t>::max(  ) - 2 );
                draw_list( )->text(
                    m_position + Vec2( size.x - dropdown_box.x + m_parent->get_padding( ), draw_y - 2.f ),
                    Color::white( ),
                    selected == i ? g_fonts->get_default_bold( ) : g_fonts->get_default( ),
                    item,
                    constants::font_size,
                    std::numeric_limits<int32_t>::max(  ) - 1
                );
                draw_y += text_size.y + m_parent->get_padding( );
                ++i;
            }
        }
    }
}
