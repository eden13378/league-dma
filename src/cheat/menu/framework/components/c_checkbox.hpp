#pragma once
#include "c_component.hpp"
#include "../../../config/c_config.hpp"
#include "../../../config/c_config_system.hpp"

#include "../c_base_window.hpp"

namespace menu::framework::components {
    constexpr auto change_selected_animation = ct_hash( "change_toggle" );
    // constexpr auto tooltip_show = ct_hash( "tooltip_show" );
    // constexpr auto tooltip_delay = ct_hash( "tooltip_delay" );

    class c_checkbox final : public c_component {
    public:
        ~c_checkbox( ) override = default;

        auto get_type( ) const -> EComponent override{
            return EComponent::checkbox;
        }

        auto process_input( ) -> bool override;

        explicit c_checkbox( std::string label, const std::shared_ptr< config::ConfigVar > config_var )
            : c_component( ),
              m_value( config_var ){
            m_label = label;
            set_height( 24.f );
            m_animation_states[ change_selected_animation ] = AnimationState( 0.2f );

            right_click_menu( )->add_dropdown(
                _( "MODE" ),
                reinterpret_cast< int32_t* >( &m_value->get_keybind( )->mode ),
                { _( "NO KEYBIND" ), _( "TOGGLE" ), _( "HOLD" ) }
            );
            right_click_menu( )->add_keybind( m_value->get_keybind( ) );
        }

        explicit c_checkbox( std::string label, const std::function< void( bool value ) > callback, bool* checked )
            : c_component( ),
              m_callback( callback ),
              m_secondary_value( checked ){
            m_label = label;
            set_height( 24.f );
            m_animation_states[ change_selected_animation ] = AnimationState( 0.2f );

            // m_keybind = std::make_shared< config::keybind_t >( );
            // m_keybind->on_change = [this]( const bool new_value ) -> void{
            //     if ( m_value )
            //         m_value->get< bool >( ) = new_value;
            //     else if ( m_secondary_value ) *m_secondary_value = new_value;
            // };
            // g_config_system->push_keybind( m_keybind );
        }

        auto draw_component( ) -> void override;

        auto get_height( ) const -> float override{
            // const auto progress = get_animation_state( ct_hash( "select" ) )->get_progress( );
            return get_size(  ).y; //+ ( is_selected( ) ? ( 8.f * progress ) : ( 8.f - 8.f * progress ) );
        }

        auto get_value( ) const -> bool{
            return m_value->get< bool >( );
        }
#if enable_lua
        auto set_value( sol::object value ) const -> void;
#endif

    private:
        // bool m_keybind_opened{ };
        // bool m_waiting_for_keybind{ };
        std::shared_ptr< config::ConfigVar > m_value{ };
        // std::shared_ptr< config::keybind_t > m_keybind{ };
        std::optional< std::function< void( bool value ) > > m_callback{ };
        bool* m_secondary_value{ };
    };
}
