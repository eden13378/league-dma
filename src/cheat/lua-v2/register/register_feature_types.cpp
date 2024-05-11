#include "pch.hpp"
#include "../state.hpp"
#include "../../features/feature.hpp"
#include "../../features/runtime/runtime.hpp"
#include "../../features/activator/activator.hpp"
#include "../../features/entity_list.hpp"
#include "../../features/target_selector/ITargetSelector.hpp"
#include "../../features/buff_cache.hpp"
#include "../../features/prediction.hpp"
#include "../../features/evade.hpp"

namespace lua {
    auto LuaState::register_feature_types( sol::state& state ) -> void{
        features::run_lua_initializers( &state );

        // g_entity_list.initialize_lua( &state );


        using Features = features::Features;

        state.new_usertype< Features >(
            "features_t",
            "entity_list",
            sol::readonly( &features::Features::entity_list ),
            "orbwalker",
            sol::readonly( &features::Features::orbwalker ),
            "evade",
            sol::readonly( &features::Features::evade ),
            "prediction",
            sol::readonly( &features::Features::prediction ),
            "target_selector",
            sol::readonly( &features::Features::target_selector ),
            "buff_cache",
            sol::readonly( &features::Features::buff_cache ),
            "activator",
            sol::readonly( &features::Features::activator )
        );

        state.new_usertype< features::Threading >(
            "c_threading",
            "is_feature_thread",
            sol::resolve( &features::Threading::is_feature_thread ),
            "is_render_thread",
            sol::resolve( &features::Threading::is_render_thread )
        );
    }
}
