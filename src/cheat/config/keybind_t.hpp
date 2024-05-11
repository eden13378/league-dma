#pragma once
#include "../utils/debug_logger.hpp"
#include "..\utils\keybind_system.hpp"

namespace config {
    struct keybind_t {
        enum class EKeybindMode {
            no_keybind,
            toggle,
            hold
        };

        keybind_t( ) = default;

        keybind_t( const int32_t key, const EKeybindMode mode )
            : key( key ),
            mode( mode ){
        }

        keybind_t( const utils::EKey key, const EKeybindMode mode )
            : key( static_cast< int32_t >( key ) ),
            mode( mode ){
        }

        [[nodiscard]] auto is_active( ) const -> bool{
            switch ( mode ) {
            case EKeybindMode::no_keybind:
                return true;
            case EKeybindMode::toggle:
            case EKeybindMode::hold:
                if ( key == -1 ) return false;
                return m_toggled;
            default:
                return false;
            }
        }

        auto update( ) -> void{
            if ( key == -1 ) return;
            switch ( mode ) {
            case EKeybindMode::toggle:
            {
                if ( g_keybind_system->was_key_pressed( static_cast< utils::EKey >( key ) ) ) {
                    m_toggled = !m_toggled;
                    debug_log( "on change" );
                    if ( on_change ) {
                        debug_log( "calling on change" );
                        on_change.value( )( m_toggled );
                    };
                }
            }
            break;
            case EKeybindMode::hold:
            {
                const auto old = m_toggled;
                m_toggled      = g_keybind_system->is_key_down( static_cast< utils::EKey >( key ) );
                if ( old != m_toggled && on_change && key != -1 ) on_change.value()( m_toggled );
                break;
            }
            default: ;
            }
        }

        int32_t                                        key{ -1 };
        EKeybindMode                                 mode{ EKeybindMode::no_keybind };
        std::optional< std::function< void( bool ) > > on_change{ };

    private:
        bool m_toggled{ false };
    };
}
