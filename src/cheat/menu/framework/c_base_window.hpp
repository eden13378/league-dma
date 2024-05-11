#pragma once
#include "menu_shared.hpp"
#include "../../sdk/math/vec2.hpp"
#include "../../security/src/hash_t.hpp"
#include "components/c_topbar.hpp"
#include "components/c_topbar_base.hpp"

namespace menu::framework {
    class c_base_window {
    public:
        enum class EMouseState {
            none,
            left_down,
            left_up,
            right_down,
            right_up
        };

        c_base_window( ) = default;

        explicit c_base_window( const hash_t name )
            : m_name( name ){
        }

        c_base_window( const float height, const float width )
            : m_height( height ),
            m_width( width ){
        }

        auto draw( ) -> void;

        virtual auto process_input( ) -> void{
        }

        virtual auto draw_contents( ) -> void{
        }

        virtual auto initialize( ) -> void;

        auto get_padding( ) const -> float{ return m_padding * g_config->misc.screen_scaling->get< float >( ); }

        auto get_height( ) const -> float{ return m_height * g_config->misc.screen_scaling->get< float >( ); }

        auto get_width( ) const -> float{ return m_width * g_config->misc.screen_scaling->get< float >( ); }
        auto get_with_unscaled() const -> float{return m_width;}

        virtual auto wnd_proc_handler( HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param ) -> void;

        auto get_draw_list( ) -> c_draw_list*{ return &m_draw_list; }

        auto get_top_draw_list( ) -> c_draw_list*{ return &m_top_draw_list; }

        auto get_cursor_position( ) const -> Vec2{ return m_cursor_position; }

        auto cursor_in_rect( const Vec2 start, const Vec2 size ) const -> bool{
            return this == nullptr ? false : helper::cursor_in_rect( m_cursor_position, start, size );
        }

        auto get_mouse_state( ) const -> EMouseState{ return m_mouse_state; }

        auto on_drag_start( ) -> void{ m_move_start_delta = m_position - get_cursor_position( ); }

        auto set_position( const Vec2 position ) -> void{ m_position = position; }

        auto set_opened( const bool opened ) -> void{ m_opened = opened; }

        auto get_last_key_pressed( ) const -> std::optional< utils::EKey >{ return m_last_key_press; }

        auto get_position( ) const -> Vec2{ return m_position; }

        virtual auto get_size( ) const -> Vec2{ return { get_width( ), get_height( ) }; }

        auto is_any_right_click_menu_opened( ) const -> bool{ return m_any_right_click_menu_opened; }

        auto get_active_component( ) const -> std::shared_ptr< components::c_component >{ return m_active_component; }

        auto set_active_component( const std::shared_ptr< components::c_component > component ) -> void{
            m_active_component = component;
        }

        auto reset_last_key_pressed( ) -> void{ m_last_key_press.reset( ); }

        auto reset_active_component( ) -> void{ m_active_component = nullptr; }

        auto is_hovered( Vec2 cursor_position ) const -> bool{
            return helper::cursor_in_rect( cursor_position, m_position, { get_width(  ), get_height(  ) } );
        }

        auto set_cursor_position( const Vec2 cursor_position ) -> void{ m_cursor_position = cursor_position; }

        auto set_is_topmost( bool is_topmost ) -> void{ m_is_topmost = is_topmost; }

        auto set_should_show( const std::optional< std::function< bool( ) > >& should_show ) -> void{
            m_should_show = should_show;
        }

        auto should_show( ) const -> bool{
            if ( !m_should_show ) return true;

            return ( m_should_show.value( ) )( );
        }

    protected:
        bool                                       m_opened{ true };
        sdk::math::Vec2                            m_position{ 100.f, 100.f };
        hash_t                                     m_name{ };
        EMouseState                                m_mouse_state{ EMouseState::none };
        sdk::math::Vec2                            m_cursor_position{ };
        std::shared_ptr< components::Topbar >    m_topbar;
        bool                                       m_dragging{ };
        float                                      m_scroll{ };
        std::optional< utils::EKey >               m_last_key_press{ };
        c_draw_list                                m_draw_list{ };
        c_draw_list                                m_top_draw_list{ };
        Vec2                                       m_move_start_delta{ };
        bool                                       m_has_right_click_menu{ };
        bool                                       m_any_right_click_menu_opened{ };
        std::shared_ptr< components::c_component > m_active_component;
        bool                                       m_is_topmost{ };
        std::optional< std::function< bool( ) > >  m_should_show{ };

    private:
        float m_height{ 400.f };
        float m_width{ 600.f };
        float m_padding{ 4.f };
    };
}
