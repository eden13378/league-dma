#include "pch.hpp"

#include "c_topbar.hpp"

#include "../c_window.hpp"
#include "..\..\..\utils\keybind_system.hpp"
#include "../../../utils/utils.hpp"

namespace menu::framework::components {
    bool Topbar::process_input( ){
        if ( is_hovered( ) ) {
            if ( m_parent->get_mouse_state( ) == c_base_window::EMouseState::left_down ) {
                m_has_mouse_down = true;
                get_animation_state( topbar_hover_animation )->start( );
                m_parent->on_drag_start( );
                return true;
            }

            if ( ( m_parent->get_mouse_state( ) == c_base_window::EMouseState::left_up || !g_keybind_system->
                is_key_down( utils::EKey::lbutton ) ) && m_has_mouse_down ) {
                get_animation_state( topbar_hover_animation )->start( );
                m_has_mouse_down = false;
            }
        }

        auto size = get_size( );

        if ( g_config ) size.x *= g_config->misc.screen_scaling->get< float >( );

        if ( c_component::is_hovered(
            Vec2( m_position.x + size.x - 16, m_position.y + ( size.y / 2.f ) - 8 ),
            Vec2(
                12.f * g_config->misc.screen_scaling->get< float >( ),
                12.f * g_config->misc.screen_scaling->get< float >( )
            )
        ) ) {
            if ( !m_was_hovered ) get_animation_state( hover_close_animation )->start( );
            m_was_hovered = true;
            if ( m_parent->get_mouse_state( ) == c_base_window::EMouseState::left_up ) {
                g_config_system->save( _( "cfg" ) );
                m_parent->set_opened( false );
                if ( m_on_close ) ( *m_on_close )( );
            }
        } else if ( m_was_hovered ) {
            get_animation_state( hover_close_animation )->start( );
            m_was_hovered = false;
        }

        return m_has_mouse_down;
    }

    bool Topbar::is_hovered( ) const{
        auto size = get_size( );

        if ( g_config ) size.x *= g_config->misc.screen_scaling->get< float >( );

        return m_parent->cursor_in_rect( m_position, size );
    }
}
