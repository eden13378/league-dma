#pragma once
#include <algorithm>

#include "encryption.hpp"

#undef max
#undef min 

#define obf_return( v ) { ____return_value_->set( v ); throw ____return_value_; }

#define obf_start(type) try { auto ____return_value_ = std::make_unique<user::obf_store<type>>();
#define obf_end(type) } catch(std::unique_ptr<user::obf_store<type>>& ex) { return ex->get(); }

namespace user {
    template <typename T>
    class obf_store {
    public:
        virtual auto set( T value ) -> void{
            reset( );
            auto ptr = new T;
            *ptr = std::move( value );

            m_key1 = encryption::get_random_char( ).at( 0 );
            m_key = static_cast< int32_t >( generate_key( ) );
            m_value_key1 = encryption::get_random_char( ).at( 0 );
            m_value_key2 = encryption::get_random_char( ).at( 0 );
            while ( m_value_key2 == m_value_key1 ) {
                m_value_key2 = encryption::get_random_char( ).at( 0 );
            }

            for ( auto i = 0u; i < sizeof( T ); ++i ) {
                auto p = reinterpret_cast< char* >( ptr ) + i;
                if ( i % 2 == 0 ) {
                    *p ^= m_value_key1;
                } else {
                    *p ^= m_value_key2;
                }
            }

            m_value = reinterpret_cast< intptr_t >( ptr ) ^ m_key ^ m_key1;
            m_has_value = true;
        }

        virtual __forceinline auto get( ) const -> T{
            auto ptr = get_ptr( );
            if ( !ptr ) return { };

            T copy = *static_cast< T* >( ptr );

            for ( auto i = 0u; i < sizeof( T ); ++i ) {
                auto p = reinterpret_cast< char* >( &copy ) + i;
                if ( i % 2 == 0 ) {
                    *p ^= m_value_key1;
                } else {
                    *p ^= m_value_key2;
                }
            }

            return copy;
        }

        virtual auto reset( ) -> void{
            auto value = get_ptr( );

            if ( !value ) return;
            delete static_cast< T* >( value );
            m_has_value = false;
        }

        virtual ~obf_store( ){
            reset( );
        }

        virtual __forceinline auto get_ptr( ) const -> void*{
            if ( !m_has_value ) return nullptr;
            return reinterpret_cast< void* >( ( m_value ^ m_key ^ m_key1 ) );
        }

        __forceinline auto generate_key( ) -> uint32_t{
            std::random_device dev;
            std::mt19937 rng( dev( ) );
            std::uniform_int_distribution< std::mt19937::result_type > dist( 0, std::numeric_limits< int32_t >::max( ) );

            return dist( rng );
        }

    private:
        char m_key1{ };
        intptr_t m_value{ };
        int32_t m_key{ };
        char m_value_key1{ };
        bool m_has_value{ };
        char m_value_key2{ };
    };
}
