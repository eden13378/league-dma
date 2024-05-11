#include "pch.hpp"

#include "config_var.hpp"
#include "../lua-v2/lua_def.hpp"
#if enable_lua
#include <sol/sol.hpp>
#endif

namespace config {
#if enable_lua
    auto ConfigVar::lua_set_int( sol::object value ) -> void{
        lua_arg_check_ct_v( value, int32_t, "number" )
        if ( typeid( int32_t ).hash_code( ) != m_value->m_value.type( ).hash_code( ) ) return;
        get< int32_t >( ) = value.as< int32_t >( );
    }

    auto ConfigVar::lua_set_float( sol::object value ) -> void{
        lua_arg_check_ct_v( value, float, "number" )
        if ( typeid( float ).hash_code( ) != m_value->m_value.type( ).hash_code( ) ) return;
        get< float >( ) = value.as< float >( );
    }

    auto ConfigVar::lua_set_bool( sol::object value ) -> void{
        lua_arg_check_ct_v( value, bool, "boolean" )
        if ( typeid( bool ).hash_code( ) != m_value->m_value.type( ).hash_code( ) ) return;
        get< bool >( ) = value.as< bool >( );
    }
#endif
}
