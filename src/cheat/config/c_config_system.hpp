#pragma once
#include "config_var.hpp"

namespace config {
    class ConfigSystem {
    public:
        ConfigSystem( ) = default;

        template <typename t>
        auto add_item( t default_value, hash_t var_name ) -> std::shared_ptr< ConfigVar >{
            for ( auto& var : m_config_vars ) { if ( var->get_name_hash( ) == var_name ) return var; }

            std::unique_lock lock( m_mutex );

            auto name_str = std::to_string( var_name );

            if constexpr ( std::_Is_character_or_bool< t >::value ) {
                auto formatter = std::make_shared< c_config_var< bool > >( default_value, name_str );
                auto ptr       = std::make_shared< ConfigVar >( formatter, name_str, var_name );
                m_config_vars.push_back( ptr );
                m_bool_config_vars.push_back( ptr.get( ) );
                return ptr;
            } else {
                auto formatter = std::make_shared< c_config_var< t > >( default_value );
                auto ptr       = std::make_shared< ConfigVar >( formatter, name_str, var_name );
                m_config_vars.push_back( ptr );

                return ptr;
            }
        }

        template <typename T>
        auto add_item(
            T           value,
            std::string var_name,
            hash_t      var_name_hashed,
            hash_t      category
        ) -> std::shared_ptr< ConfigVar >{
            // check if config var already exists
            for ( auto& var : m_config_vars ) { if ( var->get_name_hash( ) == var_name_hashed ) return var; }


            if constexpr ( std::is_same_v< T, keybind_t > ) {
                auto formatter = std::make_shared< c_config_var< T > >( value );
                auto ptr       = std::make_shared< ConfigVar >( formatter, var_name, var_name_hashed, category );
                m_config_vars.push_back( ptr );

                return ptr;
            }
            std::unique_lock lock( m_mutex );

            if constexpr ( std::_Is_character_or_bool< T >::value ) {
                auto formatter = std::make_shared< c_config_var< bool > >( value, var_name );
                auto ptr       = std::make_shared< ConfigVar >( formatter, var_name, var_name_hashed, category );
                m_config_vars.push_back( ptr );
                m_bool_config_vars.push_back( ptr.get( ) );
                return ptr;
            } else {
                auto formatter = std::make_shared< c_config_var< T > >( value );
                auto ptr       = std::make_shared< ConfigVar >( formatter, var_name, var_name_hashed, category );
                m_config_vars.push_back( ptr );

                return ptr;
            }
        }

        template <typename t>
        auto add_item( t value, std::string var_name, hash_t var_name_hashed ) -> std::shared_ptr< ConfigVar >{
            for ( auto& var : m_config_vars ) { if ( var->get_name_hash( ) == var_name_hashed ) return var; }


            if constexpr ( std::is_same< t, keybind_t >::value ) {
                auto formatter = std::make_shared< c_config_var< t > >( value );
                auto ptr       = std::make_shared< ConfigVar >( formatter, var_name, var_name_hashed );
                m_config_vars.push_back( ptr );

                return ptr;
            }
            std::unique_lock lock( m_mutex );

            if constexpr ( std::_Is_character_or_bool< t >::value ) {
                auto formatter = std::make_shared< c_config_var< bool > >( value, var_name );
                auto ptr       = std::make_shared< ConfigVar >( formatter, var_name, var_name_hashed );
                m_config_vars.push_back( ptr );
                m_bool_config_vars.push_back( ptr.get( ) );
                return ptr;
            } else {
                auto formatter = std::make_shared< c_config_var< t > >( value );
                auto ptr       = std::make_shared< ConfigVar >( formatter, var_name, var_name_hashed );
                m_config_vars.push_back( ptr );

                return ptr;
            }
        }

        template <typename t>
        auto add_item_raw(
            c_config_var< t > value,
            std::string       var_name,
            hash_t            var_name_hashed
        ) -> std::shared_ptr< ConfigVar >{
            std::unique_lock lock( m_mutex );
            auto             formatter = std::make_shared< c_config_var< t > >( value );
            auto             ptr       = std::make_shared< ConfigVar >( formatter, var_name, var_name_hashed );
            m_config_vars.push_back( ptr );

            if ( typeid( t ).hash_code( ) == typeid( bool ).hash_code( ) ) {
                m_bool_config_vars.push_back( ptr.get( ) );
            }

            return ptr;
        }

        auto push_keybind( std::shared_ptr< ConfigVar > shared ) -> void{
            // std::unique_lock lock( m_mutex );
            m_keybinds.push_back( shared );
        }

        auto find_by_name( hash_t name ) -> std::shared_ptr< ConfigVar >{
            const auto found = std::ranges::find_if(
                m_config_vars,
                [&]( std::shared_ptr< ConfigVar > var ) -> bool{ return var->get_name_hash( ) == name; }
            );

            if ( found != m_config_vars.end( ) ) { return *found; }

            return nullptr;
        }

#if enable_lua
        auto lua_add_int( int32_t value, sol::object var_name ) -> std::shared_ptr< ConfigVar >;
        auto lua_add_float( float value, sol::object var_name ) -> std::shared_ptr< ConfigVar >;
        auto lua_add_bool( bool value, sol::object var_name ) -> std::shared_ptr< ConfigVar >;
#endif

    public:
#if __DEBUG
        auto export_named( std::string out_path ) -> void;
#endif

        auto save( const char* name ) -> void;
        auto load( const char* name ) -> void;

        auto get_keybinds( ) const -> const std::vector< std::shared_ptr< ConfigVar > >&{ return m_keybinds; }

        auto get_bool_vars( ) const -> const std::vector< ConfigVar* >&{ return m_bool_config_vars; }

    private:
        std::vector< std::shared_ptr< ConfigVar > > m_config_vars{ };
        std::vector< std::shared_ptr< ConfigVar > > m_keybinds{ };
        std::vector< ConfigVar* >                   m_bool_config_vars{ };
        nlohmann::json                              m_unknown_data{ };
        std::mutex                                  m_mutex{ };
    };
}

extern config::ConfigSystem* g_config_system;
