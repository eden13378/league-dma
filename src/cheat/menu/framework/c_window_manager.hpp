#pragma once

#include "c_base_window.hpp"

namespace menu::framework {
    class c_window_manager {
    public:
        auto add_window( std::unique_ptr< c_base_window > window ) -> c_base_window*{
            m_windows.push_back( std::move( window ) );

            return m_windows.back( ).get( );
        }

#if enable_lua
        auto lua_add_window( sol::object name ) -> c_base_window*;
#endif

        auto set_opened( bool opened ) -> void{
            m_opened = opened;
        }

        auto draw( ) -> void{
            if ( g_keybind_system->was_key_pressed( utils::EKey::insert ) ||
                g_keybind_system->was_key_pressed( utils::EKey::delete_key ) ||
                ( g_config->misc.toggle_menu_with_shift->get< bool >( ) && g_keybind_system->was_key_pressed( utils::EKey::shift ) )
            ) {
                m_opened = !m_opened;
                if ( !m_opened ) g_config_system->save( _( "cfg" ) );
            }

            for ( auto& element : m_windows ) {
                element->set_cursor_position( m_cursor_position );
                element->set_opened( m_opened );
            }

            if ( !m_active && !m_windows.empty( ) ) {
                m_active = m_windows.front( ).get( );
            }

            if ( !m_opened ) return;

            for ( auto& window : m_windows ) {
                if ( window.get( ) == m_active ) continue;

                if ( !window->should_show( ) ) continue;

                window->set_is_topmost( false );
                window->draw( );
                g_render->filled_box( window->get_position( ), window->get_size( ), Color::black( ).alpha( 125 ) );
            }

            if ( m_active && m_active->should_show( ) ) {
                m_active->set_is_topmost( true );
                m_active->process_input( );
                m_active->draw( );
            } else {
                for ( auto& window : m_windows ) {
                    if ( window->should_show( ) ) {
                        m_active = window.get( );
                        break;
                    }
                }
            }
        }

        

        auto wnd_proc_handler( HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param ) -> void;

    private:
        std::deque< std::unique_ptr< c_base_window > > m_windows;
        std::deque< c_base_window* > m_lua_windows;
        c_base_window* m_active{ };
        sdk::math::Vec2 m_cursor_position{ };
        bool m_opened{ true };
    };
}
