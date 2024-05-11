#pragma once

namespace sdk::game {
#pragma pack( push, 1 )
    template <class t = unsigned long long>
    class Obfuscated {
        unsigned char      m_is_filled{ };
        unsigned char      m_length_xor32{ };
        unsigned char      m_length_xor8{ };
        char               pad_0003[ 5 ]{ }; // 0x0003
        unsigned long long m_key{ };
        unsigned char      m_index{ };
        char               pad_0011[ 7 ]{ }; // 0x0011
        unsigned long long m_values[ 4 ]{ };

    public:
        auto get( ) -> t{
            if ( this->m_is_filled != 1 || m_length_xor32 > 8 || m_length_xor8 > 4 ) return 0;

            unsigned long long result = *reinterpret_cast< unsigned long long* >( &this->m_values[ this->m_index ] );
            if ( !result ) return 0;
            if ( this->m_length_xor32 ) {
                for ( int i = 0; i < this->m_length_xor32; ++i ) {
                    if (
                        !reinterpret_cast< unsigned long long* >(
                            reinterpret_cast< unsigned long long >( &result ) + i )
                    )
                        return 0;

                    *reinterpret_cast< unsigned long long* >( reinterpret_cast< unsigned long long >( &result ) +
                            i ) ^=
                        ~*reinterpret_cast< unsigned long long* >( &this->m_key + i );
                }
            }
            if ( this->m_length_xor8 ) {
                for ( int i = sizeof( t ) - this->m_length_xor8; i < this->m_length_xor8; ++i ) {
                    if ( !reinterpret_cast< unsigned char* >( reinterpret_cast< unsigned long long >( &result ) +
                        i ) )
                        return 0;
                    *reinterpret_cast< unsigned char* >( reinterpret_cast< unsigned long long >( &result ) + i ) ^=
                        ~*reinterpret_cast< unsigned char* >( &this->m_key + i );
                }
            }

            return *reinterpret_cast< t* >( &result );
        }

        operator t( ){ return get( ); }
    };

    template <>
    class Obfuscated< bool > {
    public:
        auto get( ) -> bool{
            return ( *reinterpret_cast< unsigned char* >( reinterpret_cast< unsigned long long >( this ) + 0x8 ) % 2 );
        }
    };
#pragma pack( pop )
}
