#include "pch.hpp"

#include "c_window_manager.hpp"

#include <windowsx.h>

#include "c_window.hpp"
#if enable_new_lua
#include <sol/sol.hpp>

#include "../../lua-v2/lua_def.hpp"
#endif

namespace menu::framework {
#if enable_lua
    auto c_window_manager::lua_add_window( sol::object name ) -> c_base_window*{
        lua_arg_check_ct( name, std::string, "string" )

        auto n = name.as< std::string >( );

        std::ranges::transform( n, n.begin( ), ::tolower );
        auto window = std::make_unique< c_window >( rt_hash( n.data( ) ) );

        m_windows.push_back( std::move( window ) );
        m_lua_windows.push_back( m_windows.back( ).get( ) );

        return m_windows.back( ).get( );
    }
#endif

    auto c_window_manager::wnd_proc_handler( HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param ) -> void{
        if ( !app->should_run( ) ) return;
#if enable_lua
        /*
        if ( g_lua_state ) {
            g_lua->get_callback_system( )->run(
               ct_hash( "core.wnd_proc" ),
               sol::make_object( g_lua_state, lua::structs::wnd_proc_t( msg, w_param ) )
           );
        }
        */
#endif


        if ( !m_active ) {
            if ( !m_windows.empty( ) ) m_active = m_windows.front( ).get( );
            if ( !m_active ) return;
        }

        switch ( msg ) {
        case WM_MOUSEMOVE:
            m_cursor_position.x = static_cast< float >( GET_X_LPARAM( l_param ) );
            m_cursor_position.y = static_cast< float >( GET_Y_LPARAM( l_param ) );
            break;
        case WM_LBUTTONUP:
        case WM_LBUTTONDBLCLK:
        case WM_LBUTTONDOWN:
            if ( m_active->is_hovered( m_cursor_position ) ) { break; }

            for ( const auto& window : m_windows ) {
                if ( window->is_hovered( m_cursor_position ) ) {
                    debug_log( "changing active window" );
                    m_active = window.get( );
                    break;
                }
            }
            break;
        }

        if ( m_active ) m_active->wnd_proc_handler( hwnd, msg, w_param, l_param );
    }
}
