#pragma once
#include <cstdint>

#include "c_component.hpp"
#include "../animation_state_t.hpp"
#include "../c_draw_list.hpp"
#include "../../../sdk/sdk.hpp"
#include "../../../utils/input.hpp"
#include "../../../config/c_config.hpp"



namespace menu {
    namespace framework {
        class c_base_window;
    }
}

namespace menu::framework::components {
    class c_component_base {
    public:
        c_component_base( ){
            const auto select = ct_hash( "select" );
            m_animation_states[ select ] = AnimationState( .5f );
            m_animation_states[ hover_background_animation ] = AnimationState( .5f );

            m_animation_states[ tooltip_show ] = AnimationState( .4f );
            m_animation_states[ tooltip_delay ] = AnimationState( .5f );
        }

        virtual ~c_component_base( ) = default;

        auto draw( ) -> void{
            draw_component( );
            draw_tooltip( );
        }

        virtual auto draw_component( ) -> void = 0;

    private:
        auto draw_tooltip( ) -> void;

    public:
        virtual auto process_input( ) -> bool;

        virtual auto is_hovered( ) const -> bool;
        auto is_hovered( Vec2 position, Vec2 size ) const -> bool;

        virtual auto get_height( ) const -> float{
            return m_size.y;
        }

        virtual auto get_width( ) const -> float{
            return m_size.x;
        }

        auto set_position( const Vec2 position ) -> void{
            m_position = position;
        }

        auto set_size( const Vec2 size ) -> void{
            m_size = size;
        }

        auto set_width( const float width ) -> void{
            m_size.x = width;
        }

        [[nodiscard]] auto has_mouse_down( ) const -> bool{
            return m_has_mouse_down;
        }

        virtual auto draw_as_child( ) const -> bool{
            return true;
        }

        auto on_mouse_reset( ) -> void{
            m_has_mouse_down = false;
        }

        auto set_parent( c_base_window* window ) -> void{
            m_parent = window;
        }

        auto is_selected( ) const -> bool{
            return m_selected;
        }

        [[nodiscard]] auto draw_list( ) const -> c_draw_list*;

        auto set_selected( const bool state ) -> void{
            if ( m_selected != state ) get_animation_state( ct_hash( "select" ) )->start( );
            m_selected = state;
        }

        auto get_time_ms( ) const -> int32_t{
            using namespace std::chrono;
            return static_cast< int32_t >( std::chrono::duration_cast< milliseconds >( high_resolution_clock::now( ).time_since_epoch( ) ).count( ) );
        }

        auto get_animation_state( const hash_t name ) const -> const AnimationState*{
            return &m_animation_states.at( name );
        }

        auto get_animation_state( const hash_t name ) -> AnimationState*{
            return &m_animation_states.at( name );
        }

        auto reset( ) -> void;

        auto get_label( ) const -> std::string{
            return m_label;
        }

        auto set_tooltip( std::optional< std::string > tooltip ) -> void{
            m_tooltip = std::move( tooltip );
        }

    protected:
        std::optional< std::string > m_tooltip{ };
        std::string m_label{ };
        int32_t m_z_depth{ };
        c_base_window* m_parent{ };
        Vec2 m_size;
        Vec2 m_position;
        bool m_selected{ };
        std::unordered_map< hash_t, AnimationState > m_animation_states;
        bool m_has_mouse_down{ };
        bool m_was_hovered{ };

    private:
        bool m_was_hovered_tooltip{ };
        Vec2 m_tooltip_hover_start{ };
        bool m_started_tooltip_fade{ };
    };
}
