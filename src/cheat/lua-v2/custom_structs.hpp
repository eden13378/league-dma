#pragma once

#include <deque>

// #include "../sdk/game/spell_"

namespace sdk::game {
    class Object;
}

namespace sdk::game {
    class SpellCastInfo;
}

namespace sdk::game {
    class SpellData;
}

namespace sdk::game {
    class SpellInfo;
}

namespace sdk::game {
    class BuffInfo;
    class SpellSlot;
}

namespace lua {
    class Callback {
    public:
        std::deque< sol::function > functions;
        int32_t                     calls;
    };

    class LuaBuff {
    public:
        LuaBuff( const std::shared_ptr< sdk::game::BuffData >& buff );

        LuaBuff(
            const std::shared_ptr< sdk::game::BuffData >& buff,
            const std::shared_ptr< sdk::game::BuffInfo >& buff_info
        );

        [[nodiscard]] auto is_active( ) const -> bool;

        [[nodiscard]] auto is_hard_cc( ) const -> bool;

        [[nodiscard]] auto is_disabling( ) const -> bool;

        [[nodiscard]] auto is_knock_up( ) const -> bool;

        [[nodiscard]] auto is_silence( ) const -> bool;

        [[nodiscard]] auto is_cripple( ) const -> bool;

        [[nodiscard]] auto is_invincible( ) const -> bool;

        [[nodiscard]] auto is_slow( ) const -> bool;

        auto get_source_index( ) -> int16_t;

        auto get_name( ) const -> std::optional< std::string >;

        auto get_type( ) const -> uint8_t;

        auto get_start_time( ) const -> float;

        auto get_end_time( ) const -> float;

        auto get_end_time2( ) const -> float;

        auto get_alt_amount( ) const -> int32_t;

        auto get_amount( ) const -> int32_t;

        auto get_max_stack( ) const -> int32_t;

        auto get_get_amount( ) const -> int32_t;

    private:
        std::shared_ptr< sdk::game::BuffData > m_buff_data;
        std::shared_ptr< sdk::game::BuffInfo > m_buff_info;
    };

    class LuaSpellCastInfo {
    public:
        LuaSpellCastInfo( std::shared_ptr< sdk::game::SpellCastInfo > info );

        auto get_target_index( ) const -> int16_t;

        auto get_missile_index( ) const -> int32_t;

        auto get_missile_nid( ) const -> uint32_t;

        auto get_start_position( ) const -> sdk::math::Vec3;

        auto get_end_position( ) const -> sdk::math::Vec3;

        auto get_windup_time( ) const -> float;

        auto get_total_cast_time( ) const -> float;

        auto is_autoattack( ) const -> bool;

        auto is_special_attack( ) const -> bool;

        auto was_autoattack_cast( ) const -> bool;

        auto was_autoattack_cast2( ) const -> bool;

        auto get_slot( ) const -> int32_t;

        auto get_server_cast_time( ) const -> float;

        auto get_end_time( ) const -> float;

        auto get_start_time( ) const -> float;

        auto get_owner_character( ) const -> std::optional< std::string >;

        auto get_coefficient( ) const -> float;

        auto get_coefficient2( ) const -> float;

        auto get_cooldown_time( ) const -> float;

        auto get_name( ) const -> std::string;

        auto get_mana_cost( ) const -> std::vector< float >;

        auto get_missile_speed( ) const -> float;

        auto get_channel_duration( ) const -> float;

        auto get_missile_width( ) const -> float;

    private:
        std::shared_ptr< sdk::game::SpellCastInfo > m_spell_cast_info;
        std::shared_ptr< sdk::game::SpellInfo >     m_spell_info;
        std::shared_ptr< sdk::game::SpellData >     m_spell_data;
    };

    class LuaSpellSlot {
    public:
        LuaSpellSlot( ) = default;

        LuaSpellSlot( std::shared_ptr< sdk::game::SpellSlot > slot );

        auto get_level( ) const -> int32_t;

        auto get_cooldown_expire( ) const -> float;

        auto get_charges( ) const -> int32_t;

        auto get_final_cooldown_expire( ) const -> float;

        auto get_cooldown( ) const -> float;

        auto get_coefficient( ) const -> float;

        auto get_coefficient2( ) const -> float;

        auto get_cooldown_time( ) const -> float;

        auto get_name( ) const -> std::string;

#if enable_new_lua
        auto get_mana_cost( ) const -> sol::as_table_t< std::vector< float > >;
#endif

        auto get_missile_speed( ) const -> float;

        auto get_channel_duration( ) const -> float;

        auto get_missile_width( ) const -> float;

        auto get_owner_character( ) const -> std::optional< std::string >;

        auto is_ready( ) const -> bool;

    private:
        std::shared_ptr< sdk::game::SpellSlot > m_spell_slot;
        std::shared_ptr< sdk::game::SpellInfo > m_spell_info;
        std::shared_ptr< sdk::game::SpellData > m_spell_data;
    };

    struct ChampionModuleSpellData {
        LuaSpellSlot       spell_q{ };
        LuaSpellSlot       spell_w{ };
        LuaSpellSlot       spell_e{ };
        LuaSpellSlot       spell_r{ };
        sdk::game::Object* target{ };
    };
}
