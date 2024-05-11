#pragma once
#include "c_component.hpp"
#include "../c_window.hpp"

namespace menu::framework::components {
    class c_conditional final : public c_component {
    public:
        ~c_conditional( ) override = default;

        explicit c_conditional( std::function< bool( ) > should_enable )
            : m_should_enable( std::move( should_enable ) ){
            m_items = c_window::item_container_t( );
        }

        auto get_type( ) const -> EComponent override{
            return EComponent::conditional;
        }

        auto process_input( ) -> bool override{
            return false;
        }

        auto draw_component( ) -> void override;

        auto get_height( ) const -> float override{
            if ( !m_should_enable( ) ) return 0.f;
            auto height = 0.f;
            for ( const auto item : m_items.children ) height += item->get_height( ) + m_parent->get_padding( );

            return height;
        }

        auto get( ) -> c_window::item_container_t*{
            m_items.parent = m_parent;
            return &m_items;
        }

    private:
        c_window::item_container_t m_items;
        std::function< bool( ) > m_should_enable{ };
    };


}
