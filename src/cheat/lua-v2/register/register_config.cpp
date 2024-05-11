#include "pch.hpp"
#include "../state.hpp"
#include "../../config/c_config.hpp"

namespace lua {
    auto LuaState::register_config( sol::state& state ) -> void{
        state.new_usertype< config::ConfigVar >(
            "config_var_t",
            "get_int",
            sol::resolve( &config::ConfigVar::lua_get_int ),
            "get_float",
            sol::resolve( &config::ConfigVar::lua_get_float ),
            "get_bool",
            sol::resolve( &config::ConfigVar::lua_get_bool ),
            "set_int",
            sol::resolve( &config::ConfigVar::lua_set_int ),
            "set_float",
            sol::resolve( &config::ConfigVar::lua_set_float ),
            "set_bool",
            sol::resolve( &config::ConfigVar::lua_set_bool )
        );

        state.new_usertype< config::ConfigSystem >(
            "c_config",
            "add_int",
            sol::resolve( &config::ConfigSystem::lua_add_int ),
            "add_float",
            sol::resolve( &config::ConfigSystem::lua_add_float ),
            "add_bool",
            sol::resolve( &config::ConfigSystem::lua_add_bool )
        );

        state.globals( ).set( "g_config", g_config_system );
    }

}
