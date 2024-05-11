#pragma once

#include "../feature.hpp"
#include "../names.hpp"
#include "../../utils/c_function_caller.hpp"

namespace features {
    class Activator final : public IFeature {
    public:
        enum class EMapSide {
            unknown = 0,
            top,
            bot
        };

        enum class ELane {
            none = 0,
            top,
            mid,
            bot
        };

        struct AllyData {
            int16_t  index{ };
            unsigned network_id{ };

            bool initialized{ };
            bool should_encourage{ };

            int   kill_count{ };
            int   total_kills{ };
            float last_kill_time{ };
            float last_encourage_time{ };

            int validation_count{ };
        };

        struct SpellInfo {
            float last_cast_time{ };
            bool  was_ready{ };

            bool initialized{ };
        };

        struct EnemySpellData {
            int16_t     index{ };
            unsigned    network_id{ };
            std::string champion_name{ };

            SpellInfo ultimate{ };
            SpellInfo summoner1{ };
            SpellInfo summoner2{ };

            bool initialized{ };
        };

        struct SpellCooldownData {
            int16_t  index{ };
            unsigned network_id{ };

            std::string champion_name{ };
            float       distance{ };
            bool        ignore_distance_check{ };

            ESpellSlot slot{ };
            float      last_cast_time{ };
        };

        struct JunglerData {
            int16_t  index{ };
            unsigned network_id{ };

            float    last_seen_time{ };
            float    last_warn_time{ };
            EMapSide last_seen_side{ };

            bool initialized{ };
        };

        struct PingableWard {
            int16_t  index{ };
            unsigned network_id{ };

            bool  was_pinged{ };
            float ping_time{ };
            float random_delay{ };
        };

        struct PingInstance {
            Vec3                 position{ };
            utils::ESendPingType type{ };

            float ping_time{ };
            bool  randomize{ };
        };

        struct TrackedEnemy {
            int16_t  index{ };
            unsigned network_id{ };

            float last_ping_time{ };
            bool  was_pinged{ };
        };

        struct WardPoint {
            Vec3 position{ };

            int value{ };

            bool is_bush{ };
            bool is_control_ward{ };
            bool is_automatic{ };

            sdk::math::Polygon vision_area{ };
            sdk::math::Polygon farsight_vision_area{ };

            float last_ward_time{ };
            float end_time{ };

            bool is_registered{ };
        };

        struct WardOccurrence {
            int16_t           index{ };
            int16_t           owner_index{ };
            Object::EWardType type{ };

            Vec3 position{ };

            float start_time{ };
            float end_time{ };

            bool has_end_time{ true };

            sdk::math::Polygon vision_area{ };
        };

        auto run( ) -> void override;
        auto on_draw( ) -> void override;

        auto get_name( ) noexcept -> hash_t override{ return names::activator; }
        auto initialize_lua( sol::state* state ) -> void override;

        auto initialize_menu( ) -> void override;

    private:
        auto run_auto_potion( ESpellSlot slot ) -> bool;
        auto run_auto_qss( ESpellSlot slot ) -> bool;
        auto run_auto_everfrost( ESpellSlot slot ) -> bool;
        auto run_auto_everfrost_antigapclose( ESpellSlot slot ) -> bool;
        auto run_auto_goredrinker( ESpellSlot slot ) -> bool;
        auto run_auto_mikaels( ESpellSlot slot ) -> bool;
        auto run_auto_redemption( ESpellSlot slot ) -> bool;

        auto run_auto_smite( int buffed, ESpellSlot slot ) -> bool;

        auto run_auto_heal( ESpellSlot slot ) -> bool;
        auto run_auto_barrier( ESpellSlot slot ) -> bool;
        auto run_auto_cleanse( ESpellSlot slot ) -> bool;
        auto run_auto_ignite( ESpellSlot slot ) -> bool;
        auto run_auto_exhaust( ESpellSlot slot ) -> bool;

        // auto chat
        auto auto_question( ) -> bool;
        auto auto_encourage_ally( ) -> bool;
        auto auto_say_cooldowns( ) -> bool;

        auto run_auto_ward_specific( ESpellSlot slot ) -> bool;
        auto run_ward_assist( ) -> void;

        auto draw_ward_spots( ) const -> void;
        auto draw_kill_efect( ) const -> void;

    private:
        // autochat general
        auto update_state( ) -> void;
        auto get_humanized_name( const std::string& name ) const -> std::string;

        // encourage allies
        auto        is_ally_logged( unsigned network_id ) const -> bool;
        static auto is_support( hash_t name ) -> bool;

        // auto say cooldowns
        auto auto_log_cooldowns( ) -> bool;
        auto is_enemy_tracked( unsigned network_id ) const -> bool;

        // auto gank warn
        auto auto_gank_warn( ) -> bool;
        auto get_mapside( const Vec3& position ) -> EMapSide;

        // auto ping general
        auto auto_cancerping( ) -> void;
        auto can_ping( ) const -> bool;
        auto remove_pingable_ward( unsigned network_id ) -> void;
        auto update_ping_timer( ) -> void;
        auto on_ping( ) -> void;

        // auto ping wards
        auto ping_wards( ) -> bool;

        // auto ping world
        auto update_pings( ) -> void;
        auto remove_ping_instance( const PingInstance& instance ) -> void;

        // external autoping
        auto draw_pingable_ward_indicator( ) const -> void;
        auto manual_ping_ward( Vec2 position ) -> void;
        auto update_manual_ping( ) -> void;

        // track missing enemy
        auto update_missing_enemies( ) -> void;
        auto is_enemy_saved( unsigned network_id ) const -> bool;
        auto get_lane( const Vec3& position ) -> ELane;

        // autowarding
        auto draw_autoward( ) const -> void;
        auto draw_controlward_reminder( ) const -> void;

        auto        update_auto_ward( ) -> void;
        auto        run_auto_ward( ) -> void;
        static auto simulate_ward_vision( const Vec3& position, float vision_radius ) -> sdk::math::Polygon;

        auto update_ward_points( ) -> void;
        auto register_ward_points( ) -> void;

        // semi manual ward
        auto semi_manual_autoward( ) -> void;

    private:
        utils::EFloatingTextType m_text_type{ utils::EFloatingTextType::mana_damage };
        float                    m_last_cast_time{ };

        float m_health_difference{ };
        float m_last_health{ };

        float m_last_gold{ };
        float m_gold_difference{ };

        bool  m_can_chat{ };
        float m_idle_time{ };

        // taunt on kill
        float m_last_chat_time{ };
        float m_last_change_time{ };
        int   m_last_kills{ };
        bool  m_should_chat{ };
        bool  m_initialized_chat{ };
        bool  m_autofill_quote{ };

        // warding data
        int m_control_ward_amount{ };
        int m_ward_amount{ };

        bool  m_is_ping_timer_active{ };
        int   m_ping_timer_count{ };
        float m_last_ping_time{ };

        std::vector< EnemySpellData > m_enemies_data{ };

        std::vector< AllyData > m_allies_data{ };
        bool                    m_allies_initialized{ };
        float                   m_last_encourage_time{ };

        bool                             m_enemy_spells_initialized{ };
        float                            m_max_say_cooldown_time{ 1500.f };
        std::vector< SpellCooldownData > m_cooldowns{ };

        std::vector< PingableWard > m_pingable_wards{ };
        std::vector< unsigned >     m_pinged_wards{ };
        int                         m_total_ping_count{ };

        std::vector< TrackedEnemy > m_tracked_enemies{ };
        Vec3                        m_last_ping_position{ };

        ESpellSlot m_ward_slot{ };
        ESpellSlot m_control_slot{ };
        ESpellSlot m_secondary_ward_slot{ };

        std::vector< PingInstance > m_queued_pings{ };


        bool  m_is_manual_pinging_active{ };
        float m_manual_ping_time{ };
        Vec2  m_manual_ping_position{ };
        Vec2  m_manual_ping_return_position{ };
        int   m_manual_ping_state{ };
        float m_manual_ping_next_action_time{ };
        int   m_manual_ping_iteration{ };

        bool m_has_secondary_ward{ };

        bool m_can_ward{ };
        bool m_can_control_ward{ };

        float m_register_start_time{ };
        bool  m_registration_completed{ };

        std::vector< WardPoint > m_ward_points{ };
        std::vector< WardPoint > m_developer_points{ };

        std::vector< Vec3 > m_ward_positions = {
            Vec3{ 12173.f, 51.f, 1305.f },
            Vec3{ 13405.f, 51.f, 2493.f },
            Vec3{ 11792.f, -70.f, 4129.f },
            Vec3{ 11813.f, 51.f, 6547.f },
            Vec3{ 11662.f, 50.f, 5923.f },
            Vec3{ 10152.f, 49.f, 2965.f },
            Vec3{ 8455.f, 52.f, 4922.f },
            Vec3{ 10450.f, -62.f, 5088.f },
            Vec3{ 6810.f, 53.f, 9253.f },
            Vec3{ 5282.f, -66.f, 8584.f },
            Vec3{ 9779.f, -44.f, 6282.f },
            Vec3{ 9356.f, -71.f, 5679.f },
            Vec3{ 5394.f, -71.f, 9146.f },
            Vec3{ 9778.f, 51.f, 7008.f },
            Vec3{ 3062.f, 51.f, 9023.f },
            Vec3{ 6842.f, 53.f, 11520.f },
            Vec3{ 4392.f, 56.f, 11724.f },
            Vec3{ 5926.f, 52.f, 12743.f },
            Vec3{ 6336.f, 54.f, 10044.f }
        };

        bool  m_initialized_autoward{ };
        float m_last_autoward_time{ };
        Vec3  m_last_autoward_position{ };

        std::vector< WardOccurrence > m_ally_wards{ };
        auto                          is_ward_tracked( int16_t index ) const -> bool;
        auto                          remove_ally_ward( int16_t index ) -> void;


        // idk
        float m_last_camp_action_time{ };
        Vec3  m_last_camp_position{ };

        sdk::math::Polygon m_blueside_bot_poly{ };
        sdk::math::Polygon m_blueside_top_poly{ };
        sdk::math::Polygon m_redside_bot_poly{ };
        sdk::math::Polygon m_redside_top_poly{ };

        sdk::math::Polygon m_toplane_poly{ };
        sdk::math::Polygon m_midlane_poly{ };
        sdk::math::Polygon m_botlane_poly{ };


        bool        m_initialized_lanes{ };
        bool        m_initialized_map{ };
        JunglerData m_jungler{ };

        // anti afk
        Vec3  m_last_position{ };
        float m_last_position_update{ };
        float m_next_update_delay{ 50.f };
        float m_last_path_time{ };

        // kill effect
        float m_last_kill_time{ };
        bool  m_is_pressed{ };

        // vision score
        auto  draw_vision_score( ) const -> void;
        float m_vision_score{ };
        float m_vision_score_addition{ };
        float m_vision_score_change_time{ };

        auto anti_afk( ) -> void;

    public:
        auto add_pingable_ward( int16_t index, unsigned network_id ) -> void;
        auto is_ward_pingable( unsigned network_id ) const -> bool;

        auto add_ping_instance(
            const Vec3&          position,
            utils::ESendPingType type,
            float                delay,
            bool                 randomize_position = true
        ) -> void;

        auto get_enemy_jungler_index( ) -> std::optional< int16_t >{
            return m_jungler.initialized ? std::make_optional( m_jungler.index ) : std::nullopt;
        }

        auto is_cooldown_logged( unsigned network_id, ESpellSlot slot ) const -> bool;
        auto remove_cooldown( unsigned network_id, ESpellSlot slot ) -> void;

        auto set_last_camp( const Vec3& position ) -> void;

        auto add_encouragement( const std::string& encouragement ) -> void{
            m_encouragements.push_back( encouragement );
        }

    private:
        // cancer chat
        float m_last_time{ };
        int   m_chat_index{ };
        float m_last_chat{ };
        bool  m_chatting{ };

        auto cancer_chat( ) -> void;

        std::vector< std::string > m_encouragements = {
            "wp",
            "gj",
            "mvp",
            "nice"
        };
        std::vector< std::string > m_taunts = {
            "?",
            "?",
            "?",
            "mad?",
            "seethe",
            "mald",
            "cope",
            "ff?",
            "inting?"
        };

        // ward helper
        std::array< Vec3, 11 > m_walk_position = {
            Vec3{ 9322, -71.2406f, 4508 },
            // DRAGON PIT BEHIND BUSH
            Vec3{ 11772, -71.2406f, 4608 },
            // BOT REDSIDE TRIBUSH
            Vec3{ 1824.f, 52.8381f, 11106.f },
            // TOP BLUESIDE RIVERBUSH
            Vec3{ 3486.f, -62.3959f, 11744.f },
            // TOP RIVER REDSIDE TRIBUSH
            Vec3{ 4824.f, -80.0779f, 10906.f },
            // TOP REDSIDE TRIBUSH FROM BARON PIT
            Vec3{ 5496.f, -72.7833f, 10472.f },
            // BARONPIT BEHIND BUSH,
            Vec3{ 5674.f, 51.8081f, 6508.f },
            // MID BLUESIDE TOWERBUSH
            Vec3{ 9122.f, 53.7441f, 8356.f },
            // MID REDSIDE TOWERBUSH
            Vec3{ 10072.f, -71.2406f, 3908.f },
            // DRAGONPIT BLUESIDE TRIBUSH
            Vec3{ 11286.f, -69.2616f, 3276.f },
            // BOT RIVER BLUESIDE TRIBUSH
            Vec3{ 12972.f, 51.3669f, 3658.f } // BOT REDSIDE RIVERBUSH
        };
        std::array< Vec3, 11 > m_cast_position = {
            Vec3{ 8706.7f, 52.8665f, 4606.11f },
            // DRAGON PIT BEHIND BUSH
            Vec3{ 12221.f, 51.7292f, 5037.11f },
            // BOT REDSIDE TRIBUSH
            Vec3{ 2427.37f, -70.0608f, 10969.9f },
            // TOP BLUESIDE RIVERBUSH
            Vec3{ 4027.15f, 40.4761f, 11849.4f },
            // TOP RIVER REDSIDE TRIBUSH
            Vec3{ 4642.96f, 51.8846f, 11404.2f },
            // TOP REDSIDE TRIBUSH FROM BARON PIT
            Vec3{ 6093.34f, 55.5834f, 10451.f },
            // BARONPIT BEHIND BUSH
            Vec3{ 5390.94f, 51.2194f, 6976.92f },
            // MID BLUESIDE TOWERBUSH
            Vec3{ 9533.54f, 73.1332f, 8020.57f },
            // MID REDSIDE TOWERBUSH
            Vec3{ 10278.7f, 50.0381f, 3382.33f },
            // DRAGONPIT BLUESIDE TRIBUSH
            Vec3{ 10735.3f, 40.6562f, 3077.39f },
            // BOT RIVER BLUESIDE TRIBUSH
            Vec3{ 12463.5f, -63.8353f, 3861.18f } // BOT REDSIDE RIVERBUSH
        };

        std::vector< Vec3 > m_blueside_bot = {
            Vec3{ 4874, 50.7865f, 4158 },
            Vec3{ 5446, 50.6999f, 1766 },
            Vec3{ 11272, 49.234f, 1920 },
            Vec3{ 12046, -66.2472f, 3700 },
            Vec3{ 11338, -70.8941f, 5152 },
            Vec3{ 8290, 49.798f, 7190 }
        };

        std::vector< Vec3 > m_blueside_top = {
            Vec3{ 4148, 50.5043f, 5014 },
            Vec3{ 7276, 19.0531f, 8108 },
            Vec3{ 5758, -68.1933f, 9452 },
            Vec3{ 4908, -69.9212f, 9776 },
            Vec3{ 3930, -79.0842f, 10534 },
            Vec3{ 3792, -46.0694f, 11528 },
            Vec3{ 3136, -1.34652f, 12076 },
            Vec3{ 1992, 52.8381f, 11570 },
            Vec3{ 1850, 52.8381f, 9808 },
            Vec3{ 1824, 55.8489f, 5528 }
        };

        std::vector< Vec3 > m_redside_top = {
            Vec3{ 9366, 52.8377f, 13046 },
            Vec3{ 5788, 52.8381f, 13010 },
            Vec3{ 3654, 53.9935f, 12808 },
            Vec3{ 2566, -13.653f, 11556 },
            Vec3{ 3474, -64.6676f, 9806 },
            Vec3{ 4712, -32.3474f, 8710 },
            Vec3{ 6606, 48.3145f, 7506 },
            Vec3{ 8966, 50.44f, 9608 },
            Vec3{ 9990, 52.3063f, 10734 },
            Vec3{ 9416, 52.3063f, 11656 }
        };

        std::vector< Vec3 > m_redside_bot = {
            Vec3{ 12972, 52.3063f, 9356 },
            Vec3{ 11802, 52.3063f, 9396 },
            Vec3{ 10714, 52.4236f, 9838 },
            Vec3{ 8910, 51.5309f, 7952 },
            Vec3{ 7610, 19.1307f, 6556 },
            Vec3{ 9172, -71.2406f, 5400 },
            Vec3{ 10280, -67.4454f, 4948 },
            Vec3{ 11064, -64.8615f, 3802 },
            Vec3{ 11508, -64.9214f, 2998 },
            Vec3{ 12958, 51.7294f, 5108 }
        };

        // lanes
        std::vector< Vec3 > m_midlane = {
            Vec3{ 4816.f, 50.f, 5771.f },
            Vec3{ 4431.f, 52.f, 5909.f },
            Vec3{ 4800.f, 50.f, 6253.f },
            Vec3{ 5225.f, 50.f, 6150.f },
            Vec3{ 5894.f, 51.f, 6886.f },
            Vec3{ 5492.f, 51.f, 7431.f },
            Vec3{ 5714.f, 51.f, 7883.f },
            Vec3{ 6649.f, -45.f, 8854.f },
            Vec3{ 7180.f, 52.f, 9270.f },
            Vec3{ 7928.f, 52.f, 8742.f },
            Vec3{ 8903.f, 50.f, 9546.f },
            Vec3{ 8821.f, 50.f, 9954.f },
            Vec3{ 10051.f, 51.f, 9206.f },
            Vec3{ 10045.f, 49.f, 8698.f },
            Vec3{ 9593.f, 55.f, 8787.f },
            Vec3{ 9096.f, 53.f, 8254.f },
            Vec3{ 8919.f, 51.f, 7907.f },
            Vec3{ 9384.f, 52.f, 7465.f },
            Vec3{ 9201.f, 52.f, 7068.f },
            Vec3{ 8186.f, -71.f, 6077.f },
            Vec3{ 7571.f, 52.f, 5677.f },
            Vec3{ 6785.f, 52.f, 6185.f },
            Vec3{ 5892.f, 51.f, 5384.f },
            Vec3{ 6021.f, 48.f, 4855.f },
            Vec3{ 5538.f, 49.f, 4923.f }
        };

        std::vector< Vec3 > m_toplane = {
            Vec3{ 1738.f, 52.f, 9259.f },
            Vec3{ 2131.f, 52.f, 9874.f },
            Vec3{ 3359.f, -68.f, 11135.f },
            Vec3{ 4656.f, 56.f, 11563.f },
            Vec3{ 5043.f, 56.f, 12501.f },
            Vec3{ 4953.f, 52.f, 14140.f },
            Vec3{ 3716.f, 52.f, 14095.f },
            Vec3{ 2537.f, 52.f, 13756.f },
            Vec3{ 1984.f, 57.f, 13663.f },
            Vec3{ 1413.f, 57.f, 13969.f },
            Vec3{ 705.f, 55.f, 13344.f },
            Vec3{ 803.f, 55.f, 13016.f },
            Vec3{ 1040.f, 52.f, 12707.f },
            Vec3{ 958.f, 52.f, 12045.f },
            Vec3{ 745.f, 52.f, 10974.f },
            Vec3{ 648.f, 52.f, 9494.f }
        };

        std::vector< Vec3 > m_botlane = {
            Vec3{ 9585.f, 52.f, 1792.f },
            Vec3{ 10141.f, 49.f, 2677.f },
            Vec3{ 10040.f, 49.f, 3361.f },
            Vec3{ 10889.f, -23.f, 3715.f },
            Vec3{ 11925.f, -71.f, 4216.f },
            Vec3{ 12719.f, 51.f, 5523.f },
            Vec3{ 14208.f, 52.f, 5517.f },
            Vec3{ 14231.f, 52.f, 4164.f },
            Vec3{ 13965.f, 51.f, 3327.f },
            Vec3{ 13530.f, 51.f, 2465.f },
            Vec3{ 13916.f, 48.f, 1782.f },
            Vec3{ 13681.f, 36.f, 1283.f },
            Vec3{ 13237.f, 44.f, 1004.f },
            Vec3{ 12814.f, 51.f, 1240.f },
            Vec3{ 12064.f, 51.f, 1105.f },
            Vec3{ 10706.f, 49.f, 783.f },
            Vec3{ 9494.f, 50.f, 741.f }
        };

        float m_ward_range{ 625.f };

        bool  m_placing_ward{ };
        float m_last_ward_time{ };

        bool  m_is_dead{ };
        float m_death_time{ };

        Vec3       m_ward_cast_position{ };
        Vec3       m_ward_walk_position{ };
        ESpellSlot ward_slot{ };
        float      next_auto_ward_update_time{ };
    };
}
