#pragma once
#include <cstdint>

#include "../math/vec3.hpp"
#include "..\..\utils\memory_holder.hpp"

namespace sdk::game {
    class SpellInfo;

    class SpellCastInfo {
    public:
        auto               get_target_index( ) const -> int16_t;
        [[nodiscard]] auto get_spell_info( ) const -> utils::MemoryHolder< SpellInfo >;
        [[nodiscard]] auto get_spell_name( ) const -> std::string;

        auto get_spell_info_raw( ) -> std::optional< SpellInfo >;

        char pad_0000[8]; //0x0000
        class SpellInfo *spell_info; //0x0008
        char pad_0010[8]; //0x0010
        int32_t missile_index; //0x0018
        char pad_001C[128]; //0x001C
        uint32_t missile_nid; //0x009C
        char pad_00A0[32]; //0x00A0
        math::Vec3 start_position; //0x00C0
        math::Vec3 end_position; //0x00CC
        char pad_00D8[36]; //0x00D8
        class N00001506 *N00000FFF; //0x00FC
        char pad_0104[8]; //0x0104
        float windup_time; //0x0110
        char pad_0114[12]; //0x0114
        float total_cast_time; //0x0120
        char pad_0124[14]; //0x0124
        bool is_autoattack; //0x0132
        bool is_special_attack; //0x0133
        char pad_0134[1]; //0x0134
        bool was_autoattack_cast; //0x0135
        char pad_0136[1]; //0x0136
        bool was_autoattack_cast2; //0x0137
        char pad_0138[4]; //0x0138
        UINT32 slot; //0x013C
        char pad_0140[76]; //0x0140
        float server_cast_time; //0x018C
        float end_time; //0x0190
        char pad_0194[20]; //0x0194
        float start_time; //0x01A8
        char pad_01AC[64]; //0x01AC
    };
}
// static_assert(sizeof(SpellCastInfo) == 0x1EC);