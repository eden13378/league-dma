#pragma once
#include "c_component.hpp"
#include "../c_window.hpp"


namespace menu::framework::components {
    constexpr auto press_animation = ct_hash( "press_toggle" );

    class c_button final : public c_component {
    public:
        ~c_button( ) override = default;

        auto get_type( ) const -> EComponent override{ return EComponent::button; }

        auto process_input( ) -> bool override;

        explicit c_button( std::string label, const std::function< void( ) > on_click )
            : c_component( ),
            m_on_click( on_click ){
            m_label = label;
            c_component::set_height( 24.f );
            m_animation_states[ press_animation ] = AnimationState( .15f );
        }

        auto draw_component( ) -> void override;
        auto get_height( ) const -> float override;

    private:
        std::function< void( ) > m_on_click;
        bool                     m_pressed{ };
        bool                     m_was_pressed{ };
    };
}
