#include "pch.hpp"

#include "c_base_window.hpp"

#include <windowsx.h>

#include "menu_options.hpp"
#include "../../renderer/c_renderer.hpp"

namespace menu::framework {
    auto c_base_window::draw( ) -> void{
        if ( !m_opened ) return;

        g_render->filled_box( m_position, Vec2( m_width, m_height ), colors::background, 0 );
        g_render->box( m_position, Vec2( m_width, m_height ), colors::outline, 0, 2 );

        constexpr auto navbar_width = 150.f;

        const auto container_size = Vec2( m_width - navbar_width, m_height );

        // // container divider
        // m_draw_list.line(
        //     m_position + vec2( navbar_width + container_size.x / 2.f, 0.f ),
        //     m_position + vec2( navbar_width + container_size.x / 2.f, m_height ),
        //     colors::outline,
        //     2.f
        // );

        const auto container_position = Vec2( m_position.x + navbar_width, m_position.y + m_topbar->get_height( ) );

        m_draw_list.set_clip_box(
            m_position + Vec2( m_padding, m_padding + m_topbar->get_height( ) ),
            Vec2(
                m_width - ( m_padding * 2.f ),
                m_height - ( m_topbar->get_height( ) + m_padding * 2.f )
            )
        );

        draw_contents( );

        m_draw_list.reset_clip_box( 500 );

        m_draw_list.render( );
        m_top_draw_list.render( );

        m_topbar->set_parent( this );
        m_topbar->set_position( m_position );
        m_topbar->set_width( m_width );
        m_topbar->draw( );

        m_mouse_state = EMouseState::none;
    }

    auto c_base_window::initialize( ) -> void{
        m_topbar = std::make_shared< components::Topbar >( );
        m_topbar->set_parent( this );
    }

    auto c_base_window::wnd_proc_handler( HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param ) -> void{
        if ( !m_opened ) return;


        switch ( msg ) {
        case WM_MOUSEMOVE:
            m_cursor_position.x = static_cast< float >( GET_X_LPARAM( l_param ) );
            m_cursor_position.y = static_cast< float >( GET_Y_LPARAM( l_param ) );
            break;
        case WM_MOUSELEAVE:
            if ( m_dragging ) m_dragging = false;
            m_active_component = nullptr;
            m_topbar->on_mouse_reset( );
            break;
        case WM_MOUSEWHEEL:
        {
        }
        break;
        case WM_MOUSEHWHEEL:
            break;
        // m_navigations[ m_selected ]->scroll_offset -= 10;
        // m_mouse_wheel += static_cast< float >( GET_WHEEL_DELTA_WPARAM( w_param ) ) / static_cast< float >( WHEEL_DELTA );
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
            m_mouse_state = EMouseState::left_down;
            break;
        case WM_LBUTTONUP:
            m_mouse_state = EMouseState::left_up;
            break;
        case WM_RBUTTONDOWN:
        case WM_RBUTTONDBLCLK:
            m_mouse_state = EMouseState::right_down;
            break;
        case WM_RBUTTONUP:
            m_mouse_state = EMouseState::right_up;
            break;
        case WM_KEYDOWN:
        case WM_KEYUP:
            // debug_fn_call( )
            m_last_key_press = static_cast< utils::EKey >( w_param );

            break;
        case WM_XBUTTONDOWN:
        {
            const auto m4 = w_param == MK_XBUTTON1 || w_param == 0x10020;

            debug_log( "wparam 0x{:x}", w_param );

            m_last_key_press = m4 ? utils::EKey::xbutton1 : utils::EKey::xbutton2;
        }
            break;

        default:
        {
            // if ( msg != 36 && msg != 70 )
            //     // debug_log( "mouse move: {}", msg );
        };;
        }
    }
}
