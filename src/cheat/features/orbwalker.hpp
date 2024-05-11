#pragma once

#include "feature.hpp"
#include "names.hpp"
#include "../utils/memory_holder.hpp"

namespace features {
    enum class ETargetType {
        unknown = 0,
        hero    = 1,
        minion  = 2,
        turret  = 3,
        misc
    };

    enum class EAnimationType {
        none = 0,
        pulse
    };

    class Orbwalker final : public IFeature {
    public:
        enum class EOrbwalkerMode {
            none = 0,
            combo,
            lasthit,
            laneclear,
            harass,
            flee,
            recalling,
            freeze
        };

        enum class GlowEffect {
            None = 0,
            ShiningGreen,
            OutlineWhite,
            FadingRed
        };

        struct IgnoredMinion {
            int16_t index{ };
            float   end_time{ };
        };

        struct PingInstance {
            uint32_t ping{ };
            float    ping_time{ };
        };

        struct LastHittableUnit {
            int16_t  index{ };
            unsigned network_id{ };
            bool     is_glowing{ };

            float last_update_time{ };
            float minimum_update_delay{ };

            int32_t last_glow_stage{ };

            int        glow_state{ };
            GlowEffect glow_effect{ };

            float death_time{ };
        };

        struct AnimationInstance
        {
            Vec3          position{};

            float radius{};
            Color color{ 255, 255, 255, 255 };

            float start_time{};
            float duration{};

            Vec3 second_position{};
        };

#if enable_lua
        struct OnSpellCastedT {
            OnSpellCastedT( i32 slot, const std::string& name, i16 target );

            sdk::game::Object* object;
            int32_t            spell_slot;
            std::string        spell_name;
        };
#endif

    public:
        ~Orbwalker( ) override = default;

        auto run( ) -> void override;

        auto get_name( ) noexcept -> hash_t override{ return names::orbwalker; }
#if enable_lua
        auto initialize_lua( sol::state* state ) -> void override;
#endif
        auto send_attack( uintptr_t network_id ) -> bool;

        auto send_attack( const Object* object ) -> bool{
            if ( !object ) return false;

            return send_attack( object->network_id );
        }

        [[nodiscard]] auto last_attack_end_time( ) const -> float{ return m_last_attack_end_time; }
        [[nodiscard]] auto last_attack_time( ) const -> float{ return m_last_attack_time; }
        // [[nodiscard]] auto attack_end_time( ) const -> float{ return m_attack_end_time; }
        [[nodiscard]] auto previous_attack_time( ) const -> float{ return m_previous_attack_time; }
        [[nodiscard]] auto last_move_time( ) const -> float{ return m_last_move_time; }
        [[nodiscard]] auto cast_spell_time( ) const -> float{ return m_cast_spell_time; }
        [[nodiscard]] auto last_attack_start_time( ) const -> float{ return m_last_attack_start_time; }
        // [[nodiscard]] auto last_attack_server_time( ) const -> float{ return m_last_attack_server_time; }
        [[nodiscard]] auto last_attack_ending_time( ) const -> float{ return m_last_attack_ending_time; }

        // auto get_target( ) const -> std::optional< int32_t >{ return m_target; }

        auto on_draw( ) -> void override;

#if _DEBUG
        auto get_full_name( ) -> std::string override{ return "c_orbwalker"; }
#endif

        auto initialize_menu( ) -> void override;

        auto intl_send_move_input( Vec3 position, bool force, bool should_lock ) -> bool;

        auto send_move_input( Vec3 position, bool force ) -> bool{
            return intl_send_move_input( position, force, true );
        }

        auto send_move_input( const Vec3 position ) -> bool{ return send_move_input( position, false ); }
        auto send_move_input( ) -> bool{ return send_move_input( Vec3( ), false ); }

        auto lua_send_move_input( const Vec3 position, const bool force ) -> bool{
            return intl_send_move_input( position, force, false );
        }

        auto lua_send_move_input( const Vec3 position ) -> bool{ return intl_send_move_input( position, false, false ); }
        auto lua_send_move_input( ) -> bool{ return intl_send_move_input( Vec3( ), false, false ); }


        auto set_cast_time( float time ) -> void;

        static auto get_bounding_radius( ) -> float;

        auto on_cast( ) -> void{
            m_cast_duration   = *g_time + 0.1f;
            m_cast_spell_time = *g_time;
            m_cast_ping       = m_ping;

            disable_for( 0.05f );
        }

        auto override_target( int16_t target_index, bool require_attack ) -> void;
        auto override_target( int16_t target_index ) -> void{ return override_target( target_index, false ); }

        // caitlyn
        auto is_unit_headshottable( int16_t index ) const -> bool;

        auto in_action( ) const -> bool{ return *g_time <= m_cast_duration - get_ping( ) * 0.8f; }

        auto is_autoattack_available() const -> bool { return !is_winding_down(); }

        auto in_attack( ) const -> bool{
            // Function is used in LUA, message @tore if you change args
            return m_in_attack;
        }

        auto is_ignored( int16_t index ) -> bool{
            return std::ranges::find_if(
                m_ignored,
                [ index ]( const IgnoredMinion ignored ){ return ignored.index == index; }
            ) != m_ignored.end( );
        }

        auto ignore_minion( int16_t index, float duration ) -> void;

        auto can_attack( int16_t index ) const -> bool;

        auto can_attack( const Object* object ) const -> bool{
            if ( !object ) return can_attack( static_cast< int16_t >( 0 ) );

            return can_attack( object->index );
        }

        auto can_attack( ) const -> bool{ return can_attack( nullptr ); }

        auto ignore_spell_during_attack( const float expire_time ) -> void{ m_ignore_spell_expire_time = expire_time; }

        auto get_local_server_position( ) const -> Vec3;

        // sticky target
        auto get_sticky_position( ) -> std::optional< Vec3 >;

        auto can_move( ) -> bool;
        auto is_attackable(
            int16_t index,
            float   range,
            bool    edge_range,
            bool    is_autoattack
        ) const -> bool;

        auto is_attackable(
            int16_t index,
            float   range,
            bool    edge_range
        ) const -> bool{ return is_attackable( index, range, edge_range, true ); }

        auto is_attackable(
            int16_t index,
            float   range
        ) const -> bool{ return is_attackable( index, range, true, true ); }

        auto is_attackable(
            int16_t index
        ) const -> bool{ return is_attackable( index, 0.f, true, true ); }

        auto get_attack_cast_delay( ) const -> float{ return m_attack_cast_delay; }

        auto get_attack_delay( ) const -> float{ return m_attack_delay; }

        auto is_winding_down( ) const -> bool{ return *g_time < m_last_attack_time + m_attack_delay; }

        auto get_next_aa_time( ) const -> float{
            return m_last_attack_time + m_attack_delay < *g_time
                       ? *g_time + m_attack_cast_delay
                       : m_last_attack_time + m_attack_delay + m_attack_cast_delay;
        }

        auto get_next_possible_aa_time( ) const -> float{
            return m_last_attack_time + m_attack_delay < *g_time ? *g_time : m_last_attack_time + m_attack_delay;
        }

        auto get_last_target( ) const -> ETargetType{ return m_last_target; }

        auto get_last_target_index( ) const -> int{ return m_last_attack_index; }

        auto get_ping( ) const -> float{ return static_cast< float >( m_ping ) * 0.001f; }

        auto get_mode( ) const -> EOrbwalkerMode{ return m_mode; }

        auto get_pulsing_color( ) const -> Color{ return m_pulsing_color; }
        auto animate_color(
            Color          base_color,
            EAnimationType type,
            int            speed         = 1,
            int            pulse_ceiling = 255
        ) const -> Color;

        auto reset_aa_timer( ) -> void{
            m_last_attack_time     = 0.f;
            m_last_attack_end_time = 0.f;
        }

        auto is_hard_crowdcontrolled( ) const -> bool{ return m_movement_impaired; }

        auto should_reset_aa( ) const -> bool;

        static auto support_limiter_active( ) -> bool;

        auto allow_fast_move( ) -> void{ m_should_fast_move = true; }

        auto is_movement_disabled( ) const -> bool{ return m_restrict_movement; }


        // Function is used in LUA, message @tore if you change args
        auto allow_movement( const bool allowed ) -> void{ m_restrict_movement = !allowed; }

        // Function is used in LUA, message @tore if you change args
        auto allow_attacks( const bool allowed ) -> void{ m_restrict_attacks = !allowed; }

        auto disable_for( const float duration ) -> void{
            m_autoattack_activation_time = *g_time + duration;
            m_movement_activation_time   = *g_time + duration;
        }

        auto disable_autoattack_until( const float expire_time ) -> void{ m_autoattack_activation_time = expire_time; }
        auto disable_movement_until( const float expire_time ) -> void{ m_movement_activation_time = expire_time; }

        auto get_last_attack_name( ) -> std::string{ return m_last_attack_name; }

        auto set_last_attack_time( const float time ) -> void{ m_last_attack_time = time; }

        auto set_last_cast( ) -> void{ m_last_cast = *g_time; }

        auto is_autospacing( ) const -> bool{ return m_autospacing; }

        auto get_aa_missile_speed( const Object* target = { } ) const -> float;

        auto get_spellfarm_target_index( ) -> std::optional< int16_t >;

        auto get_extrapolated_cursor_position( ) -> Vec3;

    private:
        struct OverrideInstance {
            int16_t index{ };
            float   end_time{ };
        };

        auto draw_minion_healthbar() -> void;

        auto run_orbwalker( const Object* target = nullptr ) -> void;
        auto should_attack_target( const Object* target ) const -> bool;
        auto update_next_attack_times( ) -> void;
        auto update_orb_mode( ) -> void;
        auto update_ignored( ) -> void;
        auto update_aa_missile( ) -> void;

        // orbwalker disable logic
        auto is_disabled( ) const -> bool{ return *g_time < m_sleep_expire; }
        auto is_autoattack_allowed( ) const -> bool{ return *g_time >= m_autoattack_activation_time; }
        auto is_movement_allowed( ) const -> bool{ return *g_time >= m_movement_activation_time; }

        // automatic movement
        auto draw_movement() -> void;
        auto get_automatic_position() -> std::optional<Vec3>;

        std::vector<Vec3> m_automatic_points{};

        // drawings
        auto draw_target( ) -> void;
        auto draw_mode( ) const -> void;
        auto draw_attack_timer( ) const -> void;
        auto draw_hold_radius( ) const -> void;
        auto draw_debug_data( ) const -> void;
        auto draw_ignored_targets( ) const -> void;
        auto draw_local_path( ) -> void;
        auto draw_turret_last_hit( ) const -> void;
#if __DEBUG
        auto debug_shit( ) -> void;
#endif
        auto draw_target_effect( ) -> void;
        auto draw_forced_notification( ) const -> void;

#if __DEBUG
        static auto developer_thing( ) -> void;
        auto        developer_draw( ) -> void;
#endif

    private:
        auto        should_run( ) -> bool;
        static auto is_valid_target( const Object* object ) -> bool;

        auto get_overridden_target( ) -> Object*;
        auto get_orbwalker_target( ) -> Object*;
        auto get_lasthit_target( ) -> Object*;
        auto get_laneclear_target( bool fast_clear = false ) -> Object*;
        auto get_freeze_target( ) -> Object*;
        auto get_recoded_laneclear_target(bool fast_clear = false) -> Object*;
        auto get_senna_soul_target( ) const -> Object*;

        // xayah
        auto get_xayah_passive_harass_target( ) const -> Object*;

        auto get_special_target( ) -> Object*;
        auto get_special_target_low_priority( ) -> Object*;

        auto get_turret_target( ) const -> Object*;
        auto can_attack_turret( ) const -> bool;

        // auto spacing
        auto update_autospacing( ) const -> void;
        auto should_auto_space( ) const -> bool;
        auto get_autospace_position( ) -> std::optional< Vec3 >;
        auto should_autospace_target( int16_t index ) const -> bool;

        // magnet
        auto get_magnet_position( ) -> std::optional< Vec3 >;
        auto get_kogmaw_magnet( ) const -> std::optional< Vec3 >;

        static auto is_wall_in_line( const Vec3& start, const Vec3& end ) -> bool;

        // dev debug
        auto draw_developer_data( ) -> void;

#if _DEBUG
        static auto print_local_buffs( ) -> void;
        static auto print_target_buffs( ) -> void;
#endif

        auto add_spellfarm_target( int16_t index ) -> void;

        // under turret last hitting

        /**
         * \brief 
         * \param turret 
         * \return 
         */
        auto get_turret_current_target( const Object* turret ) const -> Object*;

        auto get_turret_next_target( const Object* turret, uint32_t ignored_nid ) const -> Object*;

        auto get_turret_next_target( const Object* turret ) const -> Object*{
            return get_turret_next_target( turret, 0 );
        }

        static auto is_in_turret_range( const Object* object ) -> bool;
        static auto get_nearest_turret( const Object* object ) -> Object*;
        static auto get_turret_shot_damage( const Object* obj ) -> float;
        static auto is_position_under_turret( const Vec3& position ) -> bool;

        // update local debuffs
        auto update_debuff_state( ) -> void;

        // caitlyn empowered aa specific
        auto update_overrides( ) -> void;
        auto is_override_allowed( int16_t index, bool is_net ) const -> bool;
        auto remove_override( int16_t index, bool is_net ) -> void;
        auto can_override_attack( int16_t index ) const -> bool;
        auto set_override_cooldown( int16_t index, bool is_net, float end_time ) -> void;

        // color effect stuff
        auto update_color_state( ) -> void;

        // samira passive aa
        static auto samira_get_empowered_range( ) -> float;
        auto        samira_update_overrides( ) -> void;
        auto        samira_remove_override( int16_t index ) -> void;
        auto        samira_can_override_attack( int16_t index ) const -> bool;
        auto        samira_is_override_logged( int16_t index ) const -> bool;
        auto        samira_add_override_instance( int16_t index, float end_time ) -> void;

        auto update_last_hittable_glow( ) -> void;;
        auto is_unit_glowing( unsigned network_id ) const -> bool;
        auto remove_last_hittable_glow( unsigned network_id ) -> void;

        auto update_pings( ) -> void;
        auto remove_outdated_pings( ) -> void;

        // overriding target
        auto clear_target_override( ) -> void{
            m_is_overridden           = false;
            m_override_expire_time    = 0.f;
            m_override_require_attack = false;
            m_override_target_index   = 0;
        }

    private:
        Vec3 m_sticky_start{ };
        Vec3 m_sticky_end{ };

        bool                m_autospacing{ };
        int16_t             m_autospace_target_index{ };
        std::vector< Vec3 > possible_space_points{ };

        // path drawing
        Vec3 m_current_path_end{};
        Vec3 m_previous_path_end{ };
        float m_last_path_time{ };

        float   m_last_attack_end_time{ };
        float   m_last_attack_time{ };
        float   m_attack_end_time{ };
        int16_t m_last_attack_index{ };

        bool        m_did_print_cast{ };
        std::string m_last_name{ };
        bool        was_attack_cast{ };
        float       m_previous_attack_time{ };

        bool did_tether{};

        // orbwalker disable logic
        float m_sleep_expire{ };

        float m_autoattack_activation_time{ };
        float m_movement_activation_time{ };

        float m_ignore_spell_expire_time{ };

        // using for kalista
        float m_last_attack_input{ };

        float m_last_move_time{ };
        float m_last_force_time{ };
        float m_attack_cast_delay{ };
        float m_attack_delay{ };
        float m_last_attack_speed{ };
        float m_move_delay{ };

        float m_cast_duration{ };
        float m_last_cast{ };
        Vec3  m_last_position{ };

        bool was_active{ };
        Vec3 m_cast_start{ };
        Vec3 m_cast_end{ };

        // DEBUG DEBUG DEBUG
        float    m_cast_spell_time{ };
        bool     m_slow_detected{ };
        float    m_slow_movespeed{ };
        float    m_slow_end_time{ };
        unsigned m_cast_ping{ };

        bool m_blinded{ };
        bool m_movement_impaired{ };
        bool m_polymorph{ };

        uint32_t m_ping{ };
        float    m_last_data_update_time{ };
        int16_t  m_last_target_index{ };

        bool m_in_attack{ };
        bool m_attack_calculated{ };
        bool m_did_debug_print{ };

        bool m_attack_reset{ };
        bool m_was_dashing{ };

        // evade and orb move sync
        bool  m_should_fast_move{ };
        float m_last_fast_move{ };
        Vec3  m_last_move_position{ };


        // aa target tracking
        ETargetType m_last_target{ };
        float       m_attack_damage{ };


        // debug shit
        bool        m_pressed{ };
        std::string m_last_attack_name{ };

        Vec3 last_path_end{ };

        // riven aa reset detection
        std::tuple< bool, bool, bool > m_riven_aa_reset{ };

        // jhin crit exploit
        bool m_jhin_block{ };

        // attack damage processing
        int16_t m_latest_lasthit_index{ };

        // spellfarming integration
        int16_t m_spellfarm_target_index{ };
        float   m_spellfarm_target_expire_time{ };
        bool    m_spellfarm_confirmed{ };

        std::vector< OverrideInstance > m_trap_overrides{ };
        std::vector< OverrideInstance > m_net_overrides{ };

        std::chrono::time_point< std::chrono::steady_clock > m_last_color_update_time{
            std::chrono::steady_clock::now( )
        };

        //draven axe catch
        bool m_restrict_movement{ };
        bool m_restrict_attacks{ };

        // glow color
        Color                           m_pulsing_color{ };
        std::vector< OverrideInstance > m_samira_overrides{ };

        // aa missile speed detection
        float    m_autoattack_missile_speed{ 0.f };
        uint32_t m_autoattack_missile_nid{ };
        bool     m_missile_checked{ };

        // kalista flymachine
        float m_min_attack_cast_delay{ 0.242f };
        bool  m_sent_move{ };
        float m_last_attack_start_time{ };
        float m_last_attack_server_time{ };
        float m_last_attack_ending_time{ };
        Vec3  m_attack_start_position{ };

        // cursor extrapolation
        Vec3 m_last_cursor_position{ };
        float m_last_cursor_time{};

        bool m_valid_attack{ };

        bool m_is_dashing{ };
        bool m_should_move{ };
        bool m_allow_move{ };
        bool m_exploit_valid{ };

        // kogmaw thing
        bool m_kogmaw_in_passive{ };
        bool m_kogmaw_did_death_autoattack{ };

        int16_t m_override_target_index{ };
        float   m_override_expire_time{ };
        bool    m_is_overridden{ };
        bool    m_override_require_attack{ };

        // debug
        Vec3  m_last_path_end{ };
        float m_last_path_update_time{ };
        bool  m_did_cast{ };

        // ping storing
        std::vector< PingInstance > m_pings{ };
        uint32_t                    m_last_ping{ };
        uint32_t                    m_highest_ping{ };

        // lasthit glow
        std::vector< LastHittableUnit > m_last_hittable_units{ };

        float m_dash_time{ };
        float m_last_dash_time{ };

        // selected target indicator
        bool     m_glowing{ };
        unsigned m_target_network_id{ };

        float m_draw_cycle_time{ };
        float m_effect_animation_time{ };

        EOrbwalkerMode m_mode{ };
        EOrbwalkerMode m_last_mode{ };
        EOrbwalkerMode m_previous_mode{ };

        void ClearAnimations();
        void CreateAnimation(Vec3 start, Vec3 end, Color color, float duration = 0.5f, float customThickness = 0.f) {
            animationList.push_back({ start, customThickness, color, *g_time, duration, end });
        }
        void DrawAnimations();

        std::vector<AnimationInstance> animationList{};

        std::chrono::time_point< std::chrono::steady_clock > m_last_mode_update_time{
            std::chrono::steady_clock::now( )
        };

        std::vector< IgnoredMinion > m_ignored{ };
    };
}
