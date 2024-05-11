#include "pch.hpp"

#include "../cancelable.hpp"
#include "../json.hpp"
#include "../lua_web_client.hpp"
#include "../state.hpp"

namespace lua {
        auto LuaState::register_lua_only_types( sol::state& state ) -> void{
        state.new_usertype< lua::WebClient >(
            "c_web_client",
            sol::constructors< WebClient( ) >( ),
            "get",
            sol::resolve( &WebClient::get ),
            "async_get",
            sol::resolve( &WebClient::async_get ),
            "post",
            sol::resolve( &WebClient::post ),
            "async_post",
            sol::resolve( &WebClient::async_post ),
            "put",
            sol::resolve( &WebClient::put ),
            "async_put",
            sol::resolve( &WebClient::async_put ),
            "delete",
            sol::resolve( &WebClient::del ),
            "async_delete",
            sol::resolve( &WebClient::async_del ),
            "head",
            sol::resolve( &WebClient::head ),
            "async_head",
            sol::resolve( &WebClient::async_head ),
            "connect",
            sol::resolve( &WebClient::connect ),
            "async_connect",
            sol::resolve( &WebClient::async_connect ),
            "options",
            sol::resolve( &WebClient::options ),
            "async_options",
            sol::resolve( &WebClient::async_options ),
            "trace",
            sol::resolve( &WebClient::trace ),
            "async_trace",
            sol::resolve( &WebClient::async_trace ),
            "patch",
            sol::resolve( &WebClient::patch ),
            "async_patch",
            sol::resolve( &WebClient::async_patch )
        );

        state.new_usertype< WebClient::Response >(
            "c_response",
            "success",
            sol::property( &WebClient::Response::is_success ),
            "status",
            sol::property( &WebClient::Response::get_status_code ),
            "json",
            sol::resolve( &WebClient::Response::json ),
            "text",
            sol::resolve( &WebClient::Response::text ),
            "to_file",
            sol::resolve( &WebClient::Response::to_file ),
            "bytes",
            sol::resolve( &WebClient::Response::bytes )
        );

        state.new_usertype< CancelableAction >(
            "c_cancelable",
            "cancel",
            sol::resolve( &CancelableAction::cancel )
        );

        // register json functions

        state.globals( ).create_named(
            "json",
            "parse",
            sol::resolve( &lua::json::parse ),
            "stringify",
            sol::resolve( &lua::json::stringify )
        );
    }

}
