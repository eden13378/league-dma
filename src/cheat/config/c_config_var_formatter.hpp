#pragma once
#include "keybind_t.hpp"
#include "../security/src/hash_t.hpp"

namespace config {
    struct ConfigVar;
}

namespace config {
    class c_config_var_formatter_t {
    public:
        virtual ~c_config_var_formatter_t( ) = default;
        // c_config_var_formatter_t( ) = default;

        c_config_var_formatter_t( ) = default;

        explicit c_config_var_formatter_t( const std::any& value )
            : m_value( value ){
        }

        virtual auto get( ) -> std::any& = 0;
        virtual auto get_default( ) -> std::any = 0;
        virtual auto from_json( nlohmann::json& j ) -> void = 0;
        virtual auto get_json( ) -> nlohmann::json = 0;
#if __DEBUG
        virtual auto get_json_named( ) -> nlohmann::json = 0;
#endif

        // protected:
        std::any m_value{ };
    };

    template <typename T>
    class c_config_var : public c_config_var_formatter_t {
    public:
        c_config_var( )
            : c_config_var_formatter_t( ){
        }
    };

    template <>
    class c_config_var< int32_t >final : public c_config_var_formatter_t {
    public:
        explicit c_config_var( int32_t value )
            : c_config_var_formatter_t( value ),
            m_default_value( value ){
        }

        auto get( ) -> std::any& override{ return m_value; }

        auto get_default( ) -> std::any override{ return m_default_value; }

        auto from_json( nlohmann::json& j ) -> void override{
            if ( j.is_number_integer( ) ) m_value = j.get< int32_t >( );
        }

        auto get_json( ) -> nlohmann::json override{ return std::any_cast< int32_t >( m_value ); }

#if __DEBUG
        auto get_json_named( ) -> nlohmann::json override{ return std::any_cast< int32_t >( m_value ); };
#endif

    private:
        int32_t m_default_value{ };
    };

    template <>
    class c_config_var< float >final : public c_config_var_formatter_t {
    public:
        explicit c_config_var( float value )
            : c_config_var_formatter_t( value ),
            m_default_value( value ){
        }

        auto get( ) -> std::any& override{ return m_value; }

        auto get_default( ) -> std::any override{ return m_default_value; }

        auto from_json( nlohmann::json& j ) -> void override{ if ( j.is_number( ) ) m_value = j.get< float >( ); }

        auto get_json( ) -> nlohmann::json override{ return std::any_cast< float >( m_value ); }

#if __DEBUG
        auto get_json_named( ) -> nlohmann::json override{ return std::any_cast< float >( m_value ); };
#endif

    private:
        float m_default_value{ };
    };

    static auto value_str = std::to_string( ct_hash( "value" ) );

    template <>
    class c_config_var< bool >final : public c_config_var_formatter_t {
    public:
        explicit c_config_var( bool value, std::string name );
        explicit c_config_var( bool value, std::string name, utils::EKey key, keybind_t::EKeybindMode mode );

        auto get( ) -> std::any& override{ return m_value; }

        auto get_default( ) -> std::any override{ return m_default_value; }

        auto from_json( nlohmann::json& j ) -> void override{
            // if ( j[ "value" ].is_boolean( ) ) {
            //         m_value = j[ std::to_string(
            //             ct_hash( "value" )
            //         ) ].get< bool >( );
            // } else
            if ( j[ value_str ].is_boolean( ) ) {
                m_value = j[ std::to_string(
                    ct_hash( "value" )
                ) ].get< bool >( );
                // }
            }
        }

        auto get_json( ) -> nlohmann::json override{
            nlohmann::json j;
            j[ value_str ] = std::any_cast< bool >( m_value );
            // j[ std::to_string( ct_hash( "keybind" ) ) ][ std::to_string( ct_hash( "key" ) ) ] = m_keybind->key;
            // j[ std::to_string( ct_hash( "keybind" ) ) ][ std::to_string( ct_hash( "mode" ) ) ] = static_cast< int32_t >( m_keybind->mode );

            // debug_log( "save as: {}", j.dump( ) );
            return j;
        }

#if __DEBUG
        auto get_json_named( ) -> nlohmann::json override{
            nlohmann::json j;
            j[ "value" ] = std::any_cast< bool >( m_value );

            return j;
        }
#endif

        auto get_keybind( ) const -> keybind_t*;

    private:
        bool                         m_default_value{ };
        std::shared_ptr< ConfigVar > m_keybind{ };
        // keybind_t* m_keybind{ };
    };

    static auto key_string  = std::to_string( ct_hash( "key" ) );
    static auto mode_string = std::to_string( ct_hash( "mode" ) );

    template <>
    class c_config_var< keybind_t >final : public c_config_var_formatter_t {
    public:
        explicit c_config_var( keybind_t value )
            : c_config_var_formatter_t( value ),
            m_default_value( value ){
        }

        auto get( ) -> std::any& override{ return m_value; }

        auto get_default( ) -> std::any override{ return m_default_value; }

        auto from_json( nlohmann::json& j ) -> void override{
            keybind_t keybind{ };
            keybind.key  = j[ key_string ].get< int32_t >( );
            keybind.mode = static_cast< keybind_t::EKeybindMode >( j[ mode_string ].get<
                int32_t >( ) );

            if ( m_value.has_value( ) ) keybind.on_change = std::any_cast< keybind_t >( m_value ).on_change;

            m_value = keybind;
        }

        auto get_json( ) -> nlohmann::json override{
            nlohmann::json j;
            j[ key_string ]  = std::any_cast< keybind_t >( m_value ).key;
            j[ mode_string ] = static_cast< int32_t >( std::any_cast< keybind_t >( m_value ).mode );

            return j;
        }

#if __DEBUG
        auto get_json_named( ) -> nlohmann::json override{
            nlohmann::json j;
            j[ "key" ]  = std::any_cast< keybind_t >( m_value ).key;
            j[ "mode" ] = static_cast< int32_t >( std::any_cast< keybind_t >( m_value ).
                mode );

            return j;
        }
#endif

        auto get_keybind( ) -> keybind_t*{ return &m_default_value; }

    private:
        keybind_t m_default_value{ };
    };

    static auto r_string = std::to_string( ct_hash( "r" ) );
    static auto g_string = std::to_string( ct_hash( "g" ) );
    static auto b_string = std::to_string( ct_hash( "b" ) );
    static auto a_string = std::to_string( ct_hash( "a" ) );

    template <>
    class c_config_var< Color >final : public c_config_var_formatter_t {
    public:
        explicit c_config_var( Color value )
            : c_config_var_formatter_t( value ),
            m_default_value( value ){
        }

        auto get( ) -> std::any& override{ return m_value; }

        auto get_default( ) -> std::any override{ return m_default_value; }

        auto from_json( nlohmann::json& j ) -> void override{
            Color clr;
            clr.r = j[ r_string ].get< int32_t >( );
            clr.g = j[ g_string ].get< int32_t >( );
            clr.b = j[ b_string ].get< int32_t >( );
            clr.a = j[ a_string ].get< int32_t >( );

            m_value = clr;
        }

        auto get_json( ) -> nlohmann::json override{
            nlohmann::json j;
            j[ r_string ] = std::any_cast< Color >( m_value ).r;
            j[ g_string ] = std::any_cast< Color >( m_value ).g;
            j[ b_string ] = std::any_cast< Color >( m_value ).b;
            j[ a_string ] = std::any_cast< Color >( m_value ).a;

            return j;
        }

#if __DEBUG
        auto get_json_named( ) -> nlohmann::json override{
            nlohmann::json j;
            j[ "r" ] = std::any_cast< Color >( m_value ).r;
            j[ "g" ] = std::any_cast< Color >( m_value ).g;
            j[ "b" ] = std::any_cast< Color >( m_value ).b;
            j[ "a" ] = std::any_cast< Color >( m_value ).a;

            return j;
        }
#endif

    private:
        Color m_default_value{ };
    };
}
