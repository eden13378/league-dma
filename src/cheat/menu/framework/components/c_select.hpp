#pragma once

#include "c_component.hpp"
#include "..\..\..\config\config_var.hpp"

namespace menu::framework::components {
    class c_select final : public c_component {
    public:
        ~c_select( ) override = default;

        auto get_type( ) const -> EComponent override{
            return EComponent::select;
        }

        auto process_input( ) -> bool override;

        explicit c_select( const std::string& label, std::shared_ptr< config::ConfigVar > config_var, std::vector< std::string > items )
            : c_component( ),
            m_value( config_var ),
            m_items( items ){
            m_label                                         = label;
            set_height( 24.f );
            m_animation_states[ change_selected_animation ] = AnimationState( 0.5f );
        }

        auto calculate_dropdown_box( ) const -> Vec2;

        auto draw_component( ) -> void override;

        auto get_height( ) const -> float override{
            return get_size(  ).y;
        }

        auto get_value( ) const -> int32_t{
            return m_value->get< int32_t >( );
        }

        auto set_value( const int32_t value ) const -> void{
            m_value->get< int32_t >( ) = value;
        }

    private:
        std::shared_ptr< config::ConfigVar > m_value{ };
        std::vector< std::string >              m_items{ };
        bool                                    m_active{ };
        int32_t                                 m_open_time{ };
    };
}
