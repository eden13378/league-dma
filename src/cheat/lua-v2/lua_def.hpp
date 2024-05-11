#pragma once
#include "../build.hpp"
#if enable_lua
#include <sol/forward.hpp>
#include "logger.hpp"
#include <format>

#define with_lua( def ) def
#define lua_arg_check_ct( arg, type, type_name ) if(!arg.is<type>()) { \
    throw std::runtime_error(std::format("arg '{}' must be of type {}.", _(#arg), _(type_name)).c_str(  )); \
    return {}; \
    }
#define lua_arg_check( arg, type ) lua_arg_check_ct(arg, type, #type)
#define lua_arg_check_ct_v( arg, type, type_name ) if(!arg.is<type>()) { \
    throw std::runtime_error(std::format("Arg '{}' must be of type {}.", _(#arg), _(type_name)).c_str(  )); \
    return; \
    }
#define lua_arg_check_v( arg, type ) lua_arg_check_ct_v(arg, type, #type)

#define lua_c_call( fn ) fn
//sol::c_call<sol::wrap< decltype( fn ), fn> >
#else
#define with_lua( def ) 
#define lua_arg_check_ct( arg, type, type_name ) 
#define lua_arg_check( arg, type ) 
#define lua_arg_check_ct_v( arg, type, type_name ) 
#define lua_arg_check_v( arg, type ) 
#endif

#if enable_lua
extern lua_State* g_lua_state;
#endif
