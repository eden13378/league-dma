#pragma once

#include "feature.hpp"
#include "spell_detector.hpp"

#include "menu_order.hpp"

namespace features {
    class Evade final : public IFeature {
    public:
        enum class EMovementMode {
            none = 0,
            walk,
            spell,
            summoner
        };

        struct DangerousSkillshot {
            float time_until_collision{};
            int   danger{};

            std::string name{};
        };

        struct EvadeSpell {
            ESpellSlot slot{ };

            float range{ };
            float delay{ };
            float speed{ };
            bool  is_fixed_range{ };

            bool  has_minimum_range{ };
            float min_range{ };

            bool extends_range_on_wall{ };
            bool penetrates_wall{ true };

            // special
            bool invert{ };
            bool ignores_wall{ };

            [[nodiscard]] auto get_dodge_time( const float distance ) const -> float{
                if ( speed <= 0.f ) return delay;

                return delay + distance / speed;
            }

            [[nodiscard]] auto get_time_to_position( const Vec3& position ) const -> float{
                if ( speed <= 0.f ) return delay + g_features->orbwalker->get_ping( );

                return delay + g_local->position.dist_to( position ) / speed + g_features->orbwalker->get_ping( );
            }
        };

        struct PathInstance {
            Vec3 projected_position{ };
            int  danger{ };
        };

        auto run( ) -> void override;
        auto on_draw( ) -> void override;

        auto get_name( ) noexcept -> hash_t override{ return names::evade; }
#if enable_lua
        auto initialize_lua( sol::state* state ) -> void override;
#endif
#if _DEBUG
        auto get_full_name( ) -> std::string override{ return "c_evade"; }
#endif

        auto is_position_safe(
            const Vec3& position,
            int         lowest_danger_level,
            float       radius_mod,
            bool        is_blink
        ) const -> bool;

        auto is_position_safe(
            const Vec3& position,
            const int   lowest_danger_level,
            const float radius_mod
        ) const -> bool{ return is_position_safe( position, lowest_danger_level, radius_mod, false ); }

        auto is_position_safe(
            const Vec3& position,
            const int   lowest_danger_level
        ) const -> bool{ return is_position_safe( position, lowest_danger_level, 1.f, false ); }

        auto is_position_safe(
            const Vec3& position
        ) const -> bool{ return is_position_safe( position, 0, 1.f, false ); }

        auto is_position_tetherable( const Vec3& position, bool use_tethering_hitbox = false ) const -> bool;

#if enable_lua
        auto is_position_safe_lua(
            const sol::object /*ReSharper disable once CppPassValueParameterByConstReference*/ position,
            const sol::object /*ReSharper disable once CppPassValueParameterByConstReference*/ safe_distance,
            const sol::object /*ReSharper disable once CppPassValueParameterByConstReference*/ radius_mod
        ) const -> bool{
            lua_arg_check( position, Vec3 )
            lua_arg_check_ct( safe_distance, bool, "boolean" )
            if ( radius_mod.get_type( ) == sol::type::number )
                lua_arg_check_ct( radius_mod, float, "number" )

            return is_position_safe(
                position.as< Vec3 >( ),
                safe_distance.as< bool >( ),
                radius_mod.get_type( ) == sol::type::number ? radius_mod.as< float >( ) : 1.1f
            );
        }

        auto get_safe_position_lua( ) -> sol::object;
        auto disable_this_tick_lua( ) -> void;

        static auto get_active_spells_table( ) -> sol::as_table_t< std::vector< SpellDetector::SpellInstance > >{
            return sol::as_table( g_features->spell_detector->get_active_spells( ) );
        }

        static auto get_active_missiles_table( ) -> sol::as_table_t< std::vector< SpellDetector::MissileInstance > >{
            return sol::as_table( g_features->spell_detector->get_active_missiles( ) );
        }
#endif

        auto is_active(bool is_autoattack = false) const -> bool
        {
            return m_is_evading &&
                (!is_autoattack || !g_config->evade.autoattack_sync->get<bool>() || !can_fit_autoattack());
        }

        auto initialize_menu( ) -> void override;

        auto should_tether( Vec3 position = Vec3( ) ) const -> bool;
        auto get_tether_point( bool pre_tether = false ) -> Vec3;
        auto is_tethering_after_evade( ) const -> bool;

        auto get_circle_segment_points(
            Vec3  center,
            float radius,
            int   segments,
            bool  no_modification = false
        ) const -> sdk::math::Polygon;
        auto get_line_segment_points(
            const Vec3& start_pos,
            const Vec3& end_pos,
            float       radius,
            bool        no_modification = false
        ) const -> sdk::math::Polygon;
        static auto get_closest_line_point(
            const Vec3& line_start,
            const Vec3& line_end,
            const Vec3& position
        ) -> Vec3;

        static auto get_polygon_points( const sdk::math::Polygon& hitbox ) -> std::vector< Vec3 >;

        static auto get_linear_tether_points(
            const sdk::math::Polygon& hitbox,
            const Vec3&               source_position
        ) -> std::vector< Vec3 >;

        auto get_bounding_radius( ) const -> float{ return m_raw_bounding_radius; }

        auto get_dangerous_spells(
            const Vec3& position,
            bool        allow_evade_logic    = true,
            float       unit_bounding_radius = 0.f
        ) const -> std::vector< SpellDetector::SpellInstance >;
        auto get_dangerous_spells_table(
            const Vec3& position,
            bool        allow_evade_logic    = true,
            float       unit_bounding_radius = 0.f
        ) const -> sol::as_table_t< std::vector< SpellDetector::SpellInstance > >;
        auto get_dangerous_missiles(
            const Vec3& position,
            bool        allow_evade_logic    = true,
            float       unit_bounding_radius = 0.f
        ) const -> std::vector< SpellDetector::MissileInstance >;
        auto get_dangerous_objects( const Vec3& position ) const -> std::vector< SpellDetector::ObjectInstance >;

        // post evade logic
        auto get_post_evade_point( ) -> std::optional< Vec3 >;

        auto can_fit_autoattack() const -> bool;

    private:
        auto move_to_safe_position( ) -> void;
        auto calculate_safe_position( ) -> void;

        auto update_preset_settings() -> void;

        static auto get_time_until_collision( const SpellDetector::SpellInstance& spell ) -> float;

        auto        get_smart_position(  Vec3 goal_position = Vec3( ), bool force_fastest = false ) -> Vec3;
        auto        calculate_minimum_dodge_time(Vec3 goal_position = Vec3()) const -> float;
        static auto get_position_weight( const Vec3& point ) -> float;

        auto get_cone_segment_points(
            Vec3  start_pos,
            Vec3  end_pos,
            float range,
            float angle
        ) const -> sdk::math::Polygon;

        // path calculation
        auto        get_units_to_safety( const Vec3& start, const Vec3& path_end ) const -> float;
        auto        get_path_danger( const Vec3& start, const Vec3& end, float move_speed ) const -> int;
        static auto calculate_path_danger(
            const Vec3& path_start,
            const Vec3& path_end,
            float       movement_speed
        ) -> PathInstance;

        // collision
        static auto get_collision_point( const SpellDetector::SpellInstance& spell ) -> std::optional< Vec3 >;

        // spell evade
        static auto get_evade_spell( ) -> std::optional< EvadeSpell >;
        static auto get_blink_position( const Vec3& cast_position, float range ) -> Vec3;

        auto set_disable_time( const float duration ) -> void{ m_disable_time = *g_time + duration; }

        // new spell evade
        auto try_spell_evade( const SpellDetector::SpellInstance& spell ) -> bool;
        auto try_summoner_evade( const SpellDetector::SpellInstance& spell ) -> bool;

        auto update_tether_points( ) -> void;

        auto extend_evade_point(
            const SpellDetector::SpellInstance& spell,
            const Vec3&                         point,
            float                               extend_distance
        ) const -> Vec3;

        // evade-orb sync
        auto get_colliding_skillshot(int16_t index) const -> std::optional<DangerousSkillshot>;

    private:
        std::vector< Vec3 > m_last_evade_points{ };

        bool m_disable_this_tick_lua{ };

        Vec3 m_goal_position{ };
        Vec3 m_start_position{ };
        Vec3 m_adjusted_goal{ };

        Vec3 m_evade_direction{ };

        std::vector< Vec3 > m_post_evade_tether_points{ };

        bool  m_adjusted{ };
        float m_evade_start_time{ };
        float m_last_calc_time{ };
        float m_bounding_radius{ 0.f };
        float m_raw_bounding_radius{ };
        float m_free_time_left{ };
        bool  m_was_forced_fastest{};
        bool  m_cancel_autoattack{};

        float m_last_free_duration{};

        int m_last_radius_modifier{ 0 };

        bool  m_is_evading{ };
        float m_safe_distance{ 0.f };
        bool  m_dynamic_safe_distance{ true };
        float m_last_evade_time{ };

        int m_minimum_danger{ };

        // logic
        bool m_should_fix_height{ false };

        // float m_last_toggle_time{ };

        // spell evade logic stuff
        bool  m_reset_path{ };
        Vec3  m_spell_end_position{ };
        float m_spellcast_end_time{ };
        bool  m_spell_evading{ };

        std::vector< Vec3 > m_spell_points{ };
        std::vector< Vec3 > m_adjusted_spell_points{ };
        float               m_last_spell_calculation_time{ };

        std::vector< Vec3 > m_unsafe_positions{ };

        bool m_blocked_input{ };
        bool m_is_input_blocked{ };

        Vec3  m_last_goal_pos{ };
        float m_last_move_time{ };

        float m_last_spell_time{ };
        float m_disable_time{ };

        // post evade
        Vec3  m_post_evade_point{ };
        Vec3  m_post_evade_start_point{ };
        float m_post_evade_expire_time{ };

        std::vector< Vec3 > m_tether_points{ };
        std::vector< Vec3 > m_render_tether_points{ };
        std::vector< Vec3 > m_valid_tether_points{ };
        Vec3                m_tether_point{ };

        Vec3  m_tether_direction_start{ };
        Vec3  m_tether_direction{ };
        float m_last_tether_direction_update{ };

        float m_last_tether_time{ };

        Vec3 m_alt_tether_point{ };
    };
}
