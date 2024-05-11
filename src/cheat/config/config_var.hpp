#pragma once
#include "c_config_var_formatter.hpp"

namespace config {
    struct ConfigVar {
        ConfigVar( )
            : m_name_hash( 0 ){
        }

        ConfigVar(
            const std::shared_ptr< c_config_var_formatter_t > value,
            const std::string&                                name,
            const hash_t                                      name_hash
        )
            : m_value( std::move( value ) ),
            m_name( name ),
            m_name_hash( name_hash ),
            m_hash_string{ std::to_string( name_hash ) }{
        }

        ConfigVar(
            const std::shared_ptr< c_config_var_formatter_t > value,
            const std::string&                                name,
            const hash_t                                      name_hash,
            const hash_t                                      category_hash
        ): m_value{ std::move( value ) },
            m_name{ name },
            m_name_hash{ name_hash },
            m_hash_string{ std::to_string( name_hash ) },
            m_category_hash{ category_hash },
            m_category_hash_string{ std::to_string( category_hash ) }{
        }

        template <typename T>
        auto get( ) -> T&{ return *reinterpret_cast< T* >( std::any_cast< T >( &m_value->get( ) ) ); }

#if enable_lua
        auto lua_get_int( ) -> std::optional< int32_t >{
            if ( typeid( int32_t ).hash_code( ) != m_value->m_value.type( ).hash_code( ) ) return std::nullopt;
            return get< int32_t >( );
        }

        auto lua_get_float( ) -> std::optional< float >{
            if ( typeid( float ).hash_code( ) != m_value->m_value.type( ).hash_code( ) ) return std::nullopt;
            return get< float >( );
        }

        auto lua_get_bool( ) -> std::optional< bool >{
            if ( typeid( bool ).hash_code( ) != m_value->m_value.type( ).hash_code( ) ) return std::nullopt;
            return get< bool >( );
        }

        auto lua_set_int( sol::object value ) -> void;
        auto lua_set_float( sol::object value ) -> void;
        auto lua_set_bool( sol::object value ) -> void;
#endif

        auto get_keybind( ) const -> keybind_t*{
            try { return static_cast< c_config_var< bool >* >( m_value.get( ) )->get_keybind( ); } catch ( ... ) {
                return nullptr;
            }
        }

        auto get_config_formatter( ) const -> std::shared_ptr< c_config_var_formatter_t >{ return m_value; }

        auto get_config_formatter_ref( ) const -> const std::shared_ptr< c_config_var_formatter_t >&{ return m_value; }

        auto get_name_hash( ) const -> hash_t{ return m_name_hash; }

        auto get_name_hash_string( ) const -> const std::string&{ return m_hash_string; }
        auto get_name( ) -> std::string{ return m_name; }

    private:
        std::shared_ptr< c_config_var_formatter_t > m_value;
        std::string                                 m_name;
        hash_t                                      m_name_hash;
        std::string                                 m_hash_string;
        hash_t                                      m_category_hash{ };
        std::string                                 m_category_hash_string{ };
    };
}
