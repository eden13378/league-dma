#pragma once
#include "../security/src/hash_t.hpp"

namespace features::names {
    constexpr auto orbwalker       = ct_hash( "orbwalker" );
    constexpr auto visuals         = ct_hash( "visuals" );
    constexpr auto entity_list     = ct_hash( "entity_list" );
    constexpr auto prediction      = ct_hash( "prediction" );
    constexpr auto tracker         = ct_hash( "tracker" );
    constexpr auto evade           = ct_hash( "evade" );
    constexpr auto spell_detector  = ct_hash( "spell_detector" );
    constexpr auto target_selector = ct_hash( "target_selector" );
    constexpr auto auto_combo      = ct_hash( "auto_combo" );
    constexpr auto buff_cache      = ct_hash( "buff_cache" );
    constexpr auto activator       = ct_hash( "activator" );
}
