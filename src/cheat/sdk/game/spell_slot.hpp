#pragma once
#include <cstdint>
#include <string>
#include "..\..\utils\memory_holder.hpp"

namespace sdk::game {
    class SpellInfo;
    class SpellDetails;

    enum class ESpellSlot {
        q = 0,
        w = 1,
        e = 2,
        r = 3,
        d = 4,
        f = 5,
        item1,
        item2,
        item3,
        item4,
        item5,
        item6,
        item7,
        recall = 13,
        max
    };

    class SpellSlot {
    public:
        auto get_spell_info( ) const -> utils::MemoryHolder< SpellInfo >;
        auto get_details( ) const -> utils::MemoryHolder< SpellDetails >;
        auto get_usable_state( ) const -> int8_t;
        auto get_active_state( ) const -> int;
        auto get_spell_effect_value() const -> float;
        auto get_name( ) const -> std::string;
        auto get_cooldown() const -> float;
        auto is_ready( bool is_local = false ) const -> bool;
        auto get_manacost( ) const -> float;

        auto get_spell_info_raw( ) const -> std::optional< SpellInfo >;
        auto get_details_raw( ) const -> std::optional< SpellDetails >;
       
        
        char                pad_0000[ 40 ]; //0x0000
        int32_t             level; //0x0028
        char                pad_002C[ 4 ]; //0x002C
        float               cooldown_expire; //0x0030
        float               cast_start; //0x0034
        char                pad_0038[ 36 ]; //0x0038
        int32_t             charges; //0x005C
        char                pad_0060[ 8 ]; //0x0060
        float               final_cooldown_expire; //0x0068
        float               recharge_cooldown; //0x006C
        char                pad_0070[ 4 ]; //0x0070
        float               cooldown; //0x0074
        char                pad_0078[ 176 ]; //0x0078
        class SpellDetails* spell_details; //0x0128
        class SpellInfo*    spell_info; //0x0130
        char                pad_0138[ 224 ]; //0x0138
    };
}
