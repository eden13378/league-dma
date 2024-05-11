#include "pch.hpp"
#include "../state.hpp"
#include "../../overlay/debug_overlay.hpp"

namespace lua {
    auto LuaState::register_debug_overlay( sol::state& state ) -> void{
        state.new_usertype< overlay::DebugOverlay >(
            "c_debug_overlay",
            "track_time",
            sol::resolve( &overlay::DebugOverlay::track_time )
        );
    }

}
