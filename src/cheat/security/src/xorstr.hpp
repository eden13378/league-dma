#pragma once

#include <array>
#include <string>
// ReSharper disable once CppUnusedIncludeDirective
#include "random.hpp"

namespace xbt::security {
    template < size_t size, char key >
    struct xorstr_t {
    private:
        static constexpr char encode( const char c ) {
            return c ^ key;
        }

    public:
        template < size_t... s >
        constexpr __forceinline xorstr_t( const char* str, std::index_sequence< s... > /*unused*/ )
            : m_encrypted{ encode( str[ s ] )... } { }

        __forceinline auto decode( ) -> std::string {
            std::string decoded;
            decoded.resize( size );

            for ( auto i = 0; i < size; i++ ) decoded[ i ] = m_encrypted[ i ] ^ key;

            return decoded;
        }

        __forceinline auto ot( bool decrypt = true ) -> std::string {
            std::string dec;
            dec.resize( size );

            for ( auto i = 0; i < size; i++ ) {
                dec[ i ] = decrypt ?
                               m_encrypted[ i ] ^ key :
                               m_encrypted[ i ];
                m_encrypted[ i ] = '\0';
            }

            return dec;
        }

        std::array< char, size > m_encrypted{ };
    };
}

#define _( string ) xbt::security::xorstr_t< sizeof( string ), xbt::security::random::_char< __COUNTER__ >::value >( string, std::make_index_sequence< sizeof( string ) >( ) ).decode( ).data( )
#define _ot( string ) xbt::security::xorstr_t< sizeof( string ), xbt::security::random::_char< __COUNTER__ >::value>( string, std::make_index_sequence< sizeof( string ) >( ) ).ot( ).data( )
