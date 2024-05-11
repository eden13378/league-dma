#include "pch.hpp"

#include "c_topbar_base.hpp"

#include "../c_window.hpp"
#include "..\..\..\utils\keybind_system.hpp"
#include "../../../utils/utils.hpp"
#include "../c_base_window.hpp"

namespace menu::framework::components {
    bool c_topbar_base::process_input( ){
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

        if ( is_hovered(
            Vec2( m_position.x + get_size().x - 16, m_position.y + ( get_size().y / 2.f ) - 8 ),
            Vec2( 12.f, 12.f )
        ) ) {
            if ( !m_was_hovered ) get_animation_state( hover_close_animation )->start( );
            m_was_hovered = true;
            if ( m_parent->get_mouse_state( ) == c_base_window::EMouseState::left_up ) {
                g_config_system->save( _( "cfg" ) );
                m_parent->set_opened( false );
                app->unload( );
            }
        } else if ( m_was_hovered ) {
            get_animation_state( hover_close_animation )->start( );
            m_was_hovered = false;
        }

        return m_has_mouse_down;
    }
}
