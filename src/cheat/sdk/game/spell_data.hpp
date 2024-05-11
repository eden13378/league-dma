#pragma once
#include <stdint.h>
#include <string>
#include <vector>

namespace sdk::game {
    class SpellData {
    public:
        SpellData( ) = default;

        [[nodiscard]] auto get_effect_amount( ) const -> float{
            return { }; // TODO: update this offset
            // return effect_amount;
        }

        [[nodiscard]] auto get_increase_damage( ) const -> float{
            return { }; // TODO: update this offset
            // return increase_damage;
        }

        [[nodiscard]] auto get_spell_duration( ) const -> float{ return spell_duration; }

        [[nodiscard]] auto get_root_duration( ) const -> float{
            return { }; // TODO: update this offset
            // return root_duration;
        }

        [[nodiscard]] auto get_increase_damage_bonus( ) const -> float{
            return { }; // TODO: update this offset
            // return increase_damage_bonus;
        }

        [[nodiscard]] auto get_coefficient( ) const -> float{ return coefficient; }

        [[nodiscard]] auto get_coefficient2( ) const -> float{ return coefficient2; }

        [[nodiscard]] auto get_cooldown_time( ) const -> float{ return cooldown_time; }

        [[nodiscard]] auto get_name( ) const -> std::string;

        [[nodiscard]] auto get_mana_cost( ) const -> std::vector< float >;

        [[nodiscard]] auto get_missile_speed() const -> float
        {
            return *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(this) + 0x500);
        }

        [[nodiscard]] auto get_channel_duration( ) const -> float{ return channel_duration; }

        [[nodiscard]] auto get_missile_width( ) const -> float{ return missile_width; }

        auto get_cooldown_duration( int level = 1 ) -> float {
            return *reinterpret_cast<float *>(reinterpret_cast<uintptr_t>(this) + ( 0x2F8 + 0x4 * ( level - 1 ) ) );
        }

    private:
        char  pad_0000[128];   // 0x0000
        char* spell_name; //0x0080
        char  pad_0088[ 420 ]; //0x0088
        float spell_duration; //0x022C
        char  pad_0230[ 56 ]; //0x0230
        float coefficient; //0x0268
        float coefficient2; //0x026C
        char  pad_0270[ 12 ]; //0x0270
        float root_duration_maybe; //0x027C
        char  pad_0280[ 100 ]; //0x0280
        float cooldown_time; //0x02E4
        char  pad_02E8[ 8 ]; //0x02E8
        float channel_duration; //0x02F0
        char  pad_02F4[ 360 ]; //0x02F4
        float range; //0x045C
        char  pad_0460[112];   // 0x0460
        float missile_speed; //0x0530
        char  pad_04F0[76];    // 0x04F0
        float missile_width; //0x0550
        char  pad_0544[ 400 ]; //0x0544
    };
}
