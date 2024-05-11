#pragma once

#define NOMINMAX
#include <Windows.h>

#include "key.hpp"

namespace utils {
    class KeybindSystem {
        struct KeyState {
            bool is_down{ };
            bool was_pressed{ };
        };

    public:
        auto update( ) -> void{
            for ( uint32_t i{ }; i < m_key_states.size( ); ++i ) {
                if ( const auto is_down = GetAsyncKeyState( i ); !m_key_states[ i ].is_down && is_down ) {
                    auto& k       = m_key_states[ i ];
                    k.is_down     = true;
                    k.was_pressed = true;
                } else if ( is_down ) {
                    auto& k       = m_key_states[ i ];
                    k.is_down     = true;
                    k.was_pressed = false;
                } else {
                    auto& k       = m_key_states[ i ];
                    k.is_down     = false;
                    k.was_pressed = false;
                }
            }

            for ( auto [ key, callback ] : m_keybinds ) if ( was_key_pressed( key ) ) callback( );
        }

        [[nodiscard]] auto is_key_down( const EKey key ) const -> bool{
            if ( static_cast< int32_t >( key ) == -1 || static_cast< size_t >( key ) >= m_key_states.size( ) ) {
                return
                    false;
            }
            return m_key_states.at( static_cast< int32_t >( key ) ).is_down;
        }

        [[nodiscard]] auto was_key_pressed( const EKey key ) const -> bool{
            if ( static_cast< int32_t >( key ) == -1 || static_cast< size_t >( key ) >= m_key_states.size( ) ) {
                return
                    false;
            }
            return m_key_states.at( static_cast< int32_t >( key ) ).was_pressed;
        }

        auto register_keybind( const EKey key, const std::function< void ( ) >& callback ) -> void{
            m_keybinds.push_back( { key, callback } );
        }

    private:
        std::array< KeyState, 0x87 >                                 m_key_states{ };
        std::vector< std::pair< EKey, std::function< void ( ) > > > m_keybinds;
    };
}

extern utils::KeybindSystem* g_keybind_system;
