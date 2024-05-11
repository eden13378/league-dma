#pragma once
#include <vcruntime_string.h>
#undef min
#undef max

namespace utils {
    enum class EDynamicError {
        unknown,
        items_is_null,
        could_not_free_items,
        size_matches_new_size,
        could_not_create_copy
    };

    template <typename T>
    class Dynamic {
    public:
        Dynamic( ) = default;

        explicit Dynamic( const size_t size ): m_size( size ){ m_items = new T[ size ]; }

        ~Dynamic( ){ free( ); }

        auto reset( ) -> std::expected< void, EDynamicError >{
            try {
                if ( !m_items ) return std::unexpected( EDynamicError::items_is_null );

                memset( m_items, 0, sizeof( T ) * m_size );
                return { };
            } catch ( ... ) { return std::unexpected( EDynamicError::unknown ); }
        }

        auto free( ) -> std::expected< void, EDynamicError >{
            try {
                if ( m_items && m_size ) delete[] m_items;
                m_size = 0;
                return { };
            } catch ( ... ) { return std::unexpected( EDynamicError::could_not_free_items ); }
        }

        auto at( size_t index ) const -> const T&{ return m_items[ index ]; }
        auto at( size_t index ) -> T&{ return m_items[ index ]; }

        auto operator[]( const size_t index ) -> T&{ return at( index ); }
        auto operator[]( const size_t index ) const -> const T&{ return at( index ); }

        auto is_valid( ) const -> bool{ return !!m_items; }

        /**
         * \brief Resize capacity of dynamic storage and copy old data to new pointer. 
         * \param new_size sizeof( t ) * new_size
         */
        auto resize( const size_t new_size ) -> std::expected< void, EDynamicError >{
            if ( new_size == m_size ) return std::unexpected( EDynamicError::size_matches_new_size );

            try {
                T* new_items = new T[ new_size ];
                // copy old items
                std::memcpy( new_items, m_items, sizeof( T ) * std::min( m_size, new_size ) );
                free( );
                m_size  = new_size;
                m_items = new_items;
                return { };
            } catch ( ... ) { return std::unexpected( EDynamicError::unknown ); }
        }

        auto data( ) -> T*{ return m_items; }
        auto data( ) const -> const T*{ return m_items; }
        auto size( ) const -> size_t{ return m_size; }

        auto copy( ) const -> std::expected< Dynamic< T >, EDynamicError >{
            try {
                Dynamic< T > d( m_size );
                std::memcpy( d.data( ), data( ), sizeof( T ) * m_size );
                return d;
            } catch ( ... ) { return std::unexpected( EDynamicError::could_not_create_copy ); }
        }

    private:
        size_t m_size;
        T*     m_items = nullptr;
    };
}
