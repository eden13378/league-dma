#pragma once

namespace sdk::game {
    using QWORD = unsigned __int64;


    class ObfuscatedFlag {
        unsigned char    m_is_filled{ };
        unsigned char    m_length_xor32{ }; // correct 0x1
        unsigned char    m_length_xor8{ }; // correct 0x2
        unsigned __int64 m_key{ };
        unsigned char    m_index{ };
        unsigned __int64 m_values[ 4 ]{ }; // 0xC

    public:
        auto get( ) -> int{
            if ( m_length_xor32 > 4 ) return 0;

            __int64 v8{ }; // [rsp+10h] [rbp+10h]

            if ( this->m_is_filled != 1 ) return 0;

            const auto index  = *( unsigned __int8* )( reinterpret_cast< intptr_t >( this ) + 0x8 );
            const int  result = *( DWORD* )( reinterpret_cast< intptr_t >( this ) + 0xC + 4 * index );

            v8 = ( v8 & 0xFFFFFFFF00000000 ) | ( result & 0x00000000FFFFFFFF );

            for ( unsigned __int64 i = 0i64; i < m_length_xor32; ++i ) {
                *( &v8 + i ) ^= ~*( QWORD* )( reinterpret_cast< intptr_t >( this )
                    + 8 * i + ( 0x4 ) );
            }


            if ( this->m_length_xor8 ) {
                const auto start = 4i64 - *( unsigned __int8* )( reinterpret_cast< intptr_t >( this ) + ( 0x2 ) );

                for ( unsigned __int64 j = start; j < 4; ++j ) {
                    *( ( BYTE* )&v8 + j ) ^= ~*( BYTE* )( reinterpret_cast<
                        intptr_t >( this ) + j + ( 0x4 ) );
                }
            }
            return ( unsigned int )v8;
        }

        operator int( ){ return get( ); }
    };

    // #pragma pack( pop )
}
