#pragma once
#include "../build.hpp"

#if enable_lua

#include "ITargetSelector.hpp"

namespace features {
    class LuaTargetSelector final : public ITargetSelector {
    public:
        ~LuaTargetSelector( ) override = default;

        auto run( ) -> void override;
        auto get_name( ) noexcept -> hash_t override;

        auto get_default_target( bool secondary ) -> Object* override;
        auto get_orbwalker_default_target( bool secondary ) -> Object* override;
        auto get_forced_target( ) -> Object* override;
        auto set_forced_target( int16_t index ) -> bool override;
        auto set_forced_target( Object* object ) -> bool override;
        auto set_forced_target( CHolder* holder ) -> bool override;
        auto reset_forced_target( ) -> void override;

        [[nodiscard]] auto is_forced( ) const -> bool override;

        auto is_bad_target(
            int16_t index,
            bool    ignore_dead,
            bool    ignore_invisible
        ) -> bool override;

        auto get_spell_specific_target(
            float                                  range,
            std::function< float( Object* unit ) > get_travel_time,
            std::function< float( Object* unit ) > get_damage,
            int                                    damage_type
        ) -> Object* override;

        auto get_killsteal_target(
            float                                  range,
            std::function< float( Object* unit ) > get_travel_time,
            std::function< float( Object* unit ) > get_damage,
            int                                    damage_type,
            Vec3                                   source_position
        ) -> Object* override;

        auto get_target_priority( std::string champion_name ) -> int32_t override;
        // auto is_target_overridden( ) const -> bool override;
    };
}

#endif
