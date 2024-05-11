#pragma once
#include "console.hpp"
#include "framework/c_window.hpp"
#include "framework/c_window_manager.hpp"

extern std::unique_ptr< menu::framework::c_window_manager > g_windows;
extern menu::framework::c_window* g_window;
extern void* g_selected_window;

namespace menu {
    enum class ESettingType {
        none,
        checkbox,
        slider
    };

    struct ESpecialSetting {
        ESettingType type{ };

        int slider_max{ };
        int slider_min{ };

        std::shared_ptr< config::ConfigVar > config_var{ };
    };

    struct SpellConfigT {
        std::shared_ptr< config::ConfigVar > spell_enabled{ };
        std::shared_ptr< config::ConfigVar > spell_danger{ };
        std::shared_ptr< config::ConfigVar > spell_health_threshold{ };
    };

    auto initialize( ) -> void;

     auto get_spell_config( hash_t champion_name, ESpellSlot slot, bool special_spell = false ) -> std::optional< SpellConfigT >;
}
