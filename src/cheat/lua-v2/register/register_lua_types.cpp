#include "pch.hpp"

#include "../custom_structs.hpp"
#include "../state.hpp"

namespace lua {
    auto LuaState::register_lua_types( sol::state& state ) -> void{
        state.new_usertype< ChampionModuleSpellData >(
            "champion_module_spell_data_t",
            "spell_q",
            &ChampionModuleSpellData::spell_q,
            "spell_w",
            &ChampionModuleSpellData::spell_w,
            "spell_e",
            &ChampionModuleSpellData::spell_e,
            "spell_r",
            &ChampionModuleSpellData::spell_r,
            "target",
            &ChampionModuleSpellData::target
        );
    }

}
