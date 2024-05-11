#pragma once
#include "../../sdk/sdk.hpp"

namespace sol {
    class state;
}

using namespace sdk::game;

namespace features {
    auto initialize_single_core( ) -> void;
    auto on_lua_reset_single_core( ) -> void;
    auto on_lua_load_single_core( ) -> void;
    auto run_lua_initializers( sol::state* state ) -> void;

    auto initialize_multi_core( ) -> void;
    auto on_lua_reset_multi_core( ) -> void;
    auto on_lua_load_multi_core( ) -> void;
}
