#pragma once

#include "c_component.hpp"
#include "..\..\..\config\config_var.hpp"

namespace menu::framework::components {
    class c_multi_select final : public c_component {
    public:
        ~c_multi_select( ) override = default;


        auto get_type( ) const -> EComponent override{
            return EComponent::multi_select;
        }
        
        auto process_input( ) -> bool override;

        explicit c_multi_select( const std::string& label, std::vector< std::shared_ptr< config::ConfigVar > > config_vars, std::vector< std::string > items )
            : c_component( ),
              m_values( config_vars ),
              m_items( items ){
            m_label = label;
            set_height( 24.f );
            m_animation_states[ change_selected_animation ] = AnimationState( 0.5f );
#if _DEBUG
            if ( config_vars.size( ) != items.size( ) ) throw std::runtime_error( _("c_multi_select: config_vars and items must be the same size") );
#endif

        }

        auto calculate_dropdown_box( ) const -> Vec2;

        auto draw_component( ) -> void override;

        auto get_height( ) const -> float override{
            return get_size(  ).y;
        }

    private:
        std::vector< std::shared_ptr< config::ConfigVar > > m_values{ };
        std::vector< std::string > m_items{ };
        bool m_active{ };
        int32_t m_open_time{ };
    };
}
