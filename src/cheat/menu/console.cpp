#include "pch.hpp"

#include "console.hpp"

#if enable_lua

#include "../features/champion_modules/ahri_module.hpp"
// #include "../lua/c_lua.hpp"
#include "../renderer/c_renderer.hpp"
#include "framework/menu_options.hpp"

namespace menu {
    constexpr auto console_text_field_height = 24.f;

    auto CConsole::draw_contents( ) -> void{
        if ( !g_lua2 ) return;
        // const auto& logs = g_lua2->get_last_logs( );
        //
        // if ( !logs.empty( ) ) {
        //     const auto text_input_box_start = m_position + Vec2(
        //         4.f,
        //         m_height - ( m_padding + console_text_field_height )
        //     );
        //
        //     auto y_of = -( m_padding * 5.f );
        //     for ( auto i = 0u; i < logs.size( ); i++ ) {
        //         if ( text_input_box_start.y + y_of < m_position.y ) break;
        //
        //         auto& log     = logs.at( ( logs.size( ) - 1 ) - i );
        //         auto  lines   = 0;
        //         auto  last_nl = false;
        //         for ( auto& c : log ) {
        //             if ( c == '\n' || c == '\r' ) {
        //                 if ( last_nl ) {
        //                     last_nl = false;
        //                     continue;
        //                 }
        //                 last_nl = true;
        //                 lines++;
        //                 continue;
        //             }
        //             last_nl = false;
        //         }
        //         auto s               = g_render->get_text_size( log, g_fonts->get_default( ), 16.f );
        //         auto one_line_height = s.y;
        //         s.y *= static_cast< float >( lines );
        //         const auto hovered = framework::helper::cursor_in_rect(
        //             m_cursor_position,
        //             text_input_box_start + Vec2( 0.f, y_of - ( s.y - one_line_height ) ),
        //             s
        //         );
        //         m_draw_list.text_shadow(
        //             text_input_box_start + Vec2( 0.f, y_of - ( s.y - one_line_height ) ),
        //             Color::white( ),
        //             g_fonts->get_default( ),
        //             log.data( ),
        //             16.f,
        //             hovered ?
        //                 std::numeric_limits< int >::max( ) :
        //                 -1
        //         );
        //         y_of -= s.y + 2.f;
        //     }
        // }

        draw_text_input_field( );
    }

    void CConsole::wnd_proc_handler( HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param ){
        c_base_window::wnd_proc_handler( hwnd, msg, w_param, l_param );

        if ( !m_is_topmost ) return;

        static int   nCharWidth = 0;
        static int   nCaretPosX = 0; // horizontal position of caret 
        static int   nCaretPosY = 0;
        static DWORD dwCharY;
        TCHAR        ch;

        switch ( msg ) {
        case WM_KEYDOWN:
        {
            debug_fn_call( )

            const auto key = static_cast< utils::EKey >( w_param );
            switch ( key ) {
            case utils::EKey::return_key:
                m_input_position = 0;
            // g_lua->_lock.lock( );
            // g_lua->run( m_input_buffer );
            // g_lua->_lock.unlock( );
                m_input_buffer.clear( );
                break;
            case utils::EKey::right:
                m_input_position = std::min( m_input_position + 1, static_cast< int32_t >( m_input_buffer.size( ) ) );
                break;
            case utils::EKey::left:
                m_input_position = std::max( m_input_position - 1, 0 );
                break;
            }
        }
        break;
        case WM_CHAR:
        {
            switch ( w_param ) {
            case 0x08:
                if ( !m_input_buffer.empty( ) ) {
                    m_input_buffer.pop_back( );
                    m_input_position--;
                }
                break;
            case 0x0A: // linefeed 
            case 0x1B: // escape 
                break;
            case 0x09: // tab
                m_input_buffer += "   ";
                break;

            case 0x0D: // carriage return 
                //
                // // Record the carriage return and position the 
                // // caret at the beginning of the new line.
                //
                // pchInputBuf[ cch++ ] = 0x0D;
                // nCaretPosX = 0;
                // nCaretPosY += 1;
                break;

            default: // displayable character 
                ch = ( TCHAR )w_param;
                auto hdc = GetDC( hwnd );
                GetCharWidth32( hdc, ( UINT )w_param, ( UINT )w_param, &nCharWidth );
                TextOut(
                    hdc,
                    nCaretPosX,
                    nCaretPosY * dwCharY,
                    &ch,
                    1
                );
                ReleaseDC( hwnd, hdc );

                if ( ( int )ch == 1 ) return;

                std::wstring s{ };
                s += ch;

                if ( m_input_position != static_cast< int32_t >( m_input_buffer.size( ) ) ) {
                    auto split1    = m_input_buffer.substr( 0, m_input_position );
                    auto split2    = m_input_buffer.substr( m_input_position );
                    m_input_buffer = split1 + std::string( s.begin( ), s.end( ) ) + split2;
                } else { m_input_buffer += std::string( s.begin( ), s.end( ) ); }
                m_input_position++;
            }
        }
        break;
        }
    }

    void CConsole::process_input( ){
        const auto screen_size = g_render->get_screensize( );

        m_topbar->process_input( );

        if ( m_topbar->has_mouse_down( ) ) {
            set_position(
                Vec2(
                    std::clamp( get_cursor_position( ).x, 50.f, screen_size.x - 50.f ),
                    std::clamp( get_cursor_position( ).y, 0.f, screen_size.y - 50.f )
                ) + m_move_start_delta
            );
        }
    }

    auto CConsole::draw_text_input_field( ) -> void{
        const auto text_input_box_start = m_position + Vec2(
            4.f,
            get_height( ) - ( get_padding( ) + console_text_field_height )
        );

        m_draw_list.box(
            text_input_box_start,
            Vec2( get_width( ) - get_padding( ) * 2.f, console_text_field_height ),
            framework::colors::outline,
            -1.f,
            2.f
        );

        const auto time = std::chrono::duration_cast< std::chrono::milliseconds >(
            std::chrono::steady_clock::now( ).time_since_epoch( )
        ).count( ) % 1000;

        Vec2 text_cursor_position;
        if ( m_input_position != static_cast< int32_t >( m_input_buffer.size( ) ) ) {
            text_cursor_position = g_render->get_text_size(
                m_input_buffer.substr( 0, m_input_position ),
                g_fonts->get_default( ),
                16.f
            );
        } else text_cursor_position = g_render->get_text_size( m_input_buffer, g_fonts->get_default( ), 16.f );

        if ( m_is_topmost ) {
            m_draw_list.line(
                text_input_box_start + Vec2( get_padding( ) + text_cursor_position.x, get_padding( ) ),
                text_input_box_start + Vec2(
                    get_padding( ) + text_cursor_position.x,
                    get_padding( ) + ( console_text_field_height - get_padding( ) * 2.f )
                ),
                Color::white( ).alpha(
                    std::abs( static_cast< int32_t >( ( static_cast< float >( time - 500 ) / 500.f ) * 255.f ) )
                ),
                2.f
            );
        }

        m_draw_list.text(
            text_input_box_start + Vec2( get_padding( ), get_padding( ) ),
            Color::white( ),
            g_fonts->get_default( ),
            m_input_buffer,
            16.f
        );
    }
}

#endif
