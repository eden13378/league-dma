#pragma once
#include <cstdint>

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
    constexpr auto hover_background_animation = ct_hash( "check_hover" );

    constexpr auto tooltip_show  = ct_hash( "tooltip_show" );
    constexpr auto tooltip_delay = ct_hash( "tooltip_delay" );

    constexpr auto right_click_menu_width = 130.f;

    enum class EComponent {
        unknown,
        button,
        checkbox,
        conditional,
        multi_select,
        select,
        slider_int,
        slider_float,
        topbar,
        topbar_base
    };

    class c_component {
        struct right_click_component_t {
            right_click_component_t( float width, float height )
                : size( width, height ){
            }

            virtual ~right_click_component_t( ) = default;

            virtual auto draw( ) -> void{
            }

            virtual auto process_input( ) -> bool{ return false; }

            virtual auto is_hovered( ) const -> bool;

            virtual auto get_width( ) -> float{ return 0.f; }

            virtual auto get_height( ) -> float{ return 0.f; }

            auto set_position( const Vec2 position ) -> void{ this->position = position; }

            auto set_parent( c_base_window* parent ) -> void{ this->parent = parent; }

            Vec2           position{ };
            Vec2           size{ };
            c_base_window* parent{ };
        };

        struct right_click_checkbox_t final : right_click_component_t {
            explicit right_click_checkbox_t( std::string label )
                : right_click_component_t( right_click_menu_width, 24.f ),
                label( std::move( label ) ){ std::ranges::transform( this->label, this->label.begin( ), ::toupper ); }

            ~right_click_checkbox_t( ) override = default;

            auto draw( ) -> void override;
            // auto process_input( ) -> void override;

            auto get_height( ) -> float override{ return 24.f; }

            std::string label{ };
        };

        struct right_click_button_t final : right_click_component_t {
            explicit right_click_button_t( std::string label, std::function< void( ) > callback )
                : right_click_component_t( right_click_menu_width, 24.f ),
                label( std::move( label ) ),
                callback( callback ){ std::ranges::transform( this->label, this->label.begin( ), ::toupper ); }

            ~right_click_button_t( ) override = default;

            auto process_input( ) -> bool override;
            auto draw( ) -> void override;
            // auto process_input( ) -> void override;

            auto get_height( ) -> float override{ return 24.f; }

            std::string              label{ };
            std::function< void( ) > callback{ };
        };

        struct right_click_color_t final : right_click_component_t {
            explicit right_click_color_t( std::string label, std::shared_ptr< config::ConfigVar > clr )
                : right_click_component_t( right_click_menu_width, 24.f ),
                label( std::move( label ) ),
                base_color( clr->get< Color >( ) ),
                current_color( clr ),
                last_alpha_value( clr->get< Color >( ).a ){
                std::ranges::transform( this->label, this->label.begin( ), ::toupper );
            }

            ~right_click_color_t( ) override = default;

            auto process_input( ) -> bool override;
            auto draw( ) -> void override;

            auto get_height( ) -> float override{ return 24.f; }

            auto is_hovered( ) const -> bool override;

        private:
            auto position_to_color( Color base_color, Vec2 position ) -> Color;
            auto percent_to_color( float p ) -> Color;

        public:
            std::string                          label{ };
            Color                                base_color{ 255, 0, 0 };
            std::shared_ptr< config::ConfigVar > current_color{ };
            bool                                 opened{ false };
            bool                                 old_opened{ false };
            bool                                 has_mouse_down_base_color{ };
            bool                                 has_mouse_down_color_picker{ };
            bool                                 has_mouse_down_alpha{ };
            Vec2                                 last_color_picker_delta{ 150.f, 0.f };
            int32_t                              last_alpha_value{ 255 };
        };

        struct right_click_dropdown_t final : right_click_component_t {
            explicit right_click_dropdown_t(
                std::string                          label,
                std::shared_ptr< config::ConfigVar > selected,
                std::vector< std::string >           items
            )
                : right_click_component_t( right_click_menu_width, 24.f ),
                label( label ),
                items( items ),
                selected( selected ){
            }

            explicit right_click_dropdown_t( std::string label, int32_t* selected, std::vector< std::string > items )
                : right_click_component_t( right_click_menu_width, 24.f ),
                label( label ),
                items( items ),
                selected_raw( selected ){
            }

            ~right_click_dropdown_t( ) override = default;
            auto draw( ) -> void override;
            auto process_input( ) -> bool override;

            auto get_height( ) -> float override{ return 24.f; }

            [[nodiscard]] auto is_hovered( ) const -> bool override;

            std::string                          label{ };
            bool                                 opened{ false };
            std::vector< std::string >           items{ };
            int32_t                              hovered{ -1 };
            std::shared_ptr< config::ConfigVar > selected{ };
            int32_t*                             selected_raw{ };
        };

        struct right_click_keybind_t final : right_click_component_t {
            explicit right_click_keybind_t( config::keybind_t* keybind )
                : right_click_component_t( right_click_menu_width, 24.f ),
                keybind( keybind ){
            }

            ~right_click_keybind_t( ) override = default;
            auto draw( ) -> void override;
            auto process_input( ) -> bool override;

            auto get_height( ) -> float override{ return 24.f; }

            config::keybind_t* keybind{ };
            bool               waiting_for_input{ };
        };

        struct right_click_menu_t {
            std::deque< std::shared_ptr< right_click_component_t > > components{ };
            c_base_window*                                           parent;
            std::shared_ptr< right_click_component_t >               active_component;

            auto add_dropdown(
                std::string                          label,
                std::shared_ptr< config::ConfigVar > selected,
                std::vector< std::string >           items
            ) -> std::shared_ptr< right_click_dropdown_t >{
                auto ptr = std::make_shared< right_click_dropdown_t >( label, selected, items );
                components.push_back( ptr );
                return ptr;
            }

            auto add_dropdown(
                std::string                label,
                int32_t*                   selected,
                std::vector< std::string > items
            ) -> std::shared_ptr< right_click_dropdown_t >{
                auto ptr = std::make_shared< right_click_dropdown_t >( label, selected, items );
                components.push_back( ptr );
                return ptr;
            }

            auto add_checkbox( ) -> std::shared_ptr< right_click_checkbox_t >{
                auto ptr = std::make_shared< right_click_checkbox_t >( _( "test label" ) );
                components.push_back( ptr );
                return ptr;
            }

            auto add_keybind( config::keybind_t* keybind ) -> std::shared_ptr< right_click_keybind_t >{
                auto ptr = std::make_shared< right_click_keybind_t >( keybind );
                components.push_back( ptr );
                return ptr;
            }

            auto add_color_picker(
                std::string                          label,
                std::shared_ptr< config::ConfigVar > color
            ) -> std::shared_ptr< right_click_color_t >{
                auto ptr = std::make_shared< right_click_color_t >( label, color );
                components.push_back( ptr );
                return ptr;
            }

            auto add_button(
                std::string              label,
                std::function< void( ) > callback
            ) -> std::shared_ptr< right_click_button_t >{
                auto ptr = std::make_shared< right_click_button_t >( label, callback );
                components.push_back( ptr );
                return ptr;
            }

            [[nodiscard]] auto get_height( ) const -> float;
            [[nodiscard]] auto is_hovered( Vec2 position ) const -> bool;
        };

    public:
        c_component( ){
            const auto select = ct_hash( "select" );

            m_animation_states[ select ]                     = AnimationState( .5f );
            m_animation_states[ hover_background_animation ] = AnimationState( .5f );

            m_animation_states[ tooltip_show ]  = AnimationState( .4f );
            m_animation_states[ tooltip_delay ] = AnimationState( .5f );
        }

        virtual ~c_component( ) = default;

        auto draw( ) -> void{
            draw_component( );
            if ( !m_right_click_menu_open ) draw_tooltip( );
            draw_right_click_menu( );
        }

        virtual auto get_type( ) const -> EComponent{ return EComponent::unknown; }

        virtual auto draw_component( ) -> void = 0;
        auto         set_height( float y ) -> void{ m_size.y = y; }

    private:
        auto draw_tooltip( ) -> void;
        auto draw_right_click_menu( ) -> void;

    public:
        virtual auto process_input( ) -> bool;

        virtual auto is_hovered( ) const -> bool;
        auto         is_hovered( Vec2 position, Vec2 size ) const -> bool;

        virtual auto get_height( ) const -> float{ return m_size.y; }

        virtual auto get_width( ) const -> float{ return m_size.x; }

        auto set_position( const Vec2 position ) -> void{ m_position = position; }

        auto set_size( const Vec2 size ) -> void{ m_size = size; }

        auto set_width( const float width ) -> void{ m_size.x = width; }

        auto get_size( ) const -> Vec2{
            return {
                m_size.x,
                m_size.y * g_config->misc.screen_scaling->get< float >( )
            };
        }


        [[nodiscard]] auto has_mouse_down( ) const -> bool{ return m_has_mouse_down; }

        virtual auto draw_as_child( ) const -> bool{ return true; }

        auto on_mouse_reset( ) -> void{ m_has_mouse_down = false; }

        auto set_parent( c_base_window* window ) -> void{ m_parent = window; }

        auto is_selected( ) const -> bool{ return m_selected; }

        [[nodiscard]] auto draw_list( ) const -> c_draw_list*;

        auto set_selected( const bool state ) -> void{
            if ( m_selected != state ) get_animation_state( ct_hash( "select" ) )->start( );
            m_selected = state;
        }

        auto get_time_ms( ) const -> int32_t{
            using namespace std::chrono;
            return static_cast< int32_t >( std::chrono::duration_cast< milliseconds >(
                high_resolution_clock::now( ).time_since_epoch( )
            ).count( ) );
        }

        auto get_animation_state( const hash_t name ) const -> const AnimationState*{
            return &m_animation_states.at( name );
        }

        auto get_animation_state( const hash_t name ) -> AnimationState*{ return &m_animation_states.at( name ); }

        auto reset( ) -> void;

        auto get_label( ) const -> std::string{ return m_label; }

        auto set_tooltip( std::optional< std::string > tooltip ) -> void{
            if ( !tooltip ) return;
            m_tooltip = tooltip;
        }

        auto right_click_menu( ) -> right_click_menu_t*{ return &m_right_click_menu; }

        [[nodiscard]] auto is_right_click_menu_opened( ) const -> bool{ return m_right_click_menu_open_last; }

    protected:
        std::optional< std::string > m_tooltip{ };
        std::string                  m_label{ };
        int32_t                      m_z_depth{ };
        c_base_window*               m_parent{ };

    private:
        Vec2 m_size;

    protected:
        Vec2                                         m_position;
        bool                                         m_selected{ };
        std::unordered_map< hash_t, AnimationState > m_animation_states;
        bool                                         m_has_mouse_down{ };
        bool                                         m_was_hovered{ };

    private:
        bool               m_was_hovered_tooltip{ };
        Vec2               m_tooltip_hover_start{ };
        bool               m_started_tooltip_fade{ };
        bool               m_right_click_menu_open{ };
        bool               m_right_click_menu_open_last{ };
        right_click_menu_t m_right_click_menu{ };
    };
}
