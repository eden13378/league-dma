#pragma once
#include "../feature.hpp"

namespace sdk::game {
    class Object;
}

namespace features {
    class ITargetSelector : public IFeature {
    public:
        virtual ~ITargetSelector( ) = default;
        virtual auto get_default_target( bool secondary ) -> sdk::game::Object* = 0;
        auto get_default_target( ) -> sdk::game::Object*{ return get_default_target( false ); }
        auto get_secondary_target( ) -> sdk::game::Object*{ return get_default_target( true ); }
        virtual auto get_orbwalker_default_target( bool secondary ) -> sdk::game::Object* = 0;
        auto get_orbwalker_default_target( ) -> sdk::game::Object*{ return get_orbwalker_default_target( false ); }
        auto get_orbwalker_secondary_target( ) -> sdk::game::Object*{ return get_orbwalker_default_target( true ); }
        virtual auto get_forced_target( ) -> sdk::game::Object* = 0;
        virtual auto set_forced_target( int16_t index ) -> bool = 0;
        virtual auto set_forced_target( sdk::game::Object* object ) -> bool = 0;
        virtual auto set_forced_target( CHolder* holder ) -> bool = 0;
        virtual auto reset_forced_target( ) -> void = 0;

#if enable_lua
        auto initialize_lua( sol::state* state ) -> void override;
#endif

#if enable_new_lua
        static auto static_initialize_lua( sol::state* state ) -> void;
#endif

        virtual auto get_champion_range( hash_t champion_name ) -> float;

        [[nodiscard]] virtual auto is_forced( ) const -> bool = 0;

        virtual auto is_bad_target(
            int16_t index,
            bool    ignore_dead,
            bool    ignore_invisible
        ) -> bool = 0;

        auto is_bad_target(
            const int16_t index,
            const bool    ignore_dead
        ) -> bool{ return is_bad_target( index, ignore_dead, false ); }

        auto is_bad_target(
            const int16_t index
        ) -> bool{ return is_bad_target( index, false, false ); }

        virtual auto get_spell_specific_target(
            float range,
            std::function< float( sdk::game::Object* unit ) > get_travel_time = { },
            std::function< float( sdk::game::Object* unit ) > get_damage = { },
            int damage_type = 0 // 0 = true damage, 1 = physical, 2 = magic
        ) -> sdk::game::Object* = 0;
        virtual auto get_killsteal_target(
            float range,
            std::function< float( sdk::game::Object* unit ) > get_travel_time,
            std::function< float( sdk::game::Object* unit ) > get_damage,
            int damage_type = 0 // 0 = true damage, 1 = physical, 2 = magic
            ,
            sdk::math::Vec3 source_position = { }
        ) -> sdk::game::Object* = 0;
        virtual auto get_target_priority( std::string champion_name ) -> int32_t = 0;
#if enable_lua

        auto lua_get_antigapclose_target( sol::object danger_distance ) -> sol::object;
        auto lua_force_target( sol::object target ) -> bool;
        auto lua_is_forced( ) -> bool;
        // auto lua_get_forced_target( ) -> sol::object;
#endif
    };
}
