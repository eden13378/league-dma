#include "pch.hpp"

#include "c_config_var_formatter.hpp"

#include "c_config_system.hpp"

namespace config {
    c_config_var< bool >::c_config_var( bool value, std::string name )
        : c_config_var_formatter_t( value ),
          m_default_value( value ){
        const auto keybind_name = std::format( ( "{}_keybind" ), name );

        m_keybind = g_config_system->add_item(
            keybind_t( ),
            keybind_name,
            rt_hash( keybind_name.data() )
        );
        this->m_value = value;

        g_config_system->push_keybind( m_keybind );
    }

    c_config_var< bool >::c_config_var( bool value, std::string name, utils::EKey key, const keybind_t::EKeybindMode mode )
        : c_config_var_formatter_t( value ),
          m_default_value( value ){
        {
            const auto keybind_name = std::format( ( "{}_keybind" ), name );

            m_keybind = g_config_system->add_item(
                keybind_t(
                    key,
                    mode
                ),
                keybind_name,
                rt_hash( keybind_name.data() )
            );
            this->m_value = value;

            g_config_system->push_keybind( m_keybind );
        }
    }

    auto c_config_var< bool >::get_keybind( ) const -> keybind_t*{
        return &m_keybind->get< keybind_t >( );
    }
}
