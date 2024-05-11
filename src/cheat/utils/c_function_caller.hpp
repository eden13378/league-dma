#pragma once

#include "../sdk/math/vec3.hpp"

namespace utils {

    enum class EFloatingTextType : int {
        invulnerable,
        special,
        heal,
        mana_heal,
        mana_damage,
        dodge,
        physical_damage_critical,
        magical_damage_critical,
        true_damage_critical,
        experience,
        gold,
        level,
        disable,
        quest_received,
        quest_completed,
        score,
        physical_damage,
        magical_damage,
        true_damage,
        enemy_physical_damage,
        enemy_magical_damage,
        enemy_true_damage,
        enemy_physical_damage_critical,
        enemy_magical_damage_critical,
        enemy_true_damage_critical,
        countdown,
        on_my_way,
        absorbed,
        debug,
        practice_tool_total,
        practice_tool_last_hit,
        practice_tool_dps,
        score_darkstar,
        score_project0,
        score_project1,
        shield_bonus_damage,
        tft_unit_label
    };

    enum class ESendPingType : unsigned char {
      ping,
      danger = 2,
      enemy_missing,
      omw,
      caution,
      assist_me,
      enemy_vision,
      cleared_vision = 13,
      need_vision,
      push,
      all_in,
      retreat = 18,
      bait,
      hold
    };

    class c_function_caller {
    private:
        bool m_initialized{ false };

        using s_shared_vec_t = struct s_shared_vec_t {
            float x;
            float y;
            float z;
        };

        struct s_shared_vec4_t {
            f32 x, y, z, w;

            s_shared_vec4_t() {
                x = y = z = w = 0.0f;
            }

            s_shared_vec4_t(f32 _x, f32 _y, f32 _z, f32 _w) {
                x = _x;
                y = _y;
                z = _z;
                w = _w;
            }

            static s_shared_vec4_t color(u8 r, u8 g, u8 b, u8 a) {
                float sc = 1.0f / 255.0f;
                return s_shared_vec4_t(f32(r) * sc, f32(g) * sc, f32(b) * sc, f32(a) * sc);
            }
        };

        struct s_shared_vars_t {
            unsigned function_idx{ 0 };
            unsigned enabled{ 0 };
            unsigned zoom_bypass_enabled{ 0 };

            unsigned issue_order_target_nid{ 0 };
            s_shared_vec_t issue_order_position{ 0.0f, 0.0f, 0.0f };

            unsigned return_ping{ 0 };
            float   return_attack_cast_delay{ 0 };
            float    return_attack_delay{ 0 };

            unsigned cast_spell_nid{ 0 };
            unsigned cast_spell_slot{ 0 };
            s_shared_vec_t cast_spell_position{ 0.0f, 0.0f, 0.0f };
            s_shared_vec_t cast_spell_position2{ 0.0f, 0.0f, 0.0f };

            bool release_chargeable_release = false;

            unsigned glow_list_queued_nid{ 0 };
            unsigned glow_list_queued_color{ 0 };
            int      glow_list_queued_size{ 3 };
            int      glow_list_queued_diffusion{ 3 };
            unsigned glow_list_queued_remove = false;
            unsigned glow_list_queued_id{ 0 };

            unsigned turret_draw_allies_enabled{ 0 };
            unsigned turret_draw_enemies_enabled{ 0 };

            unsigned floating_text_nid{ 0 };
            char     floating_text_text[ 255 ]{ 0 };
            int      floating_text_type{ 0 };

            char send_chat_text[ 255 ]{ 0 };
            int  send_chat_all{ 0 };
            bool send_chat_queued{ };

            bool          send_ping_queued{ };
            s_shared_vec_t send_ping_world_position{ 0.0f, 0.0f, 0.0f };
            s_shared_vec_t send_ping_screen_position{ 0.0f, 0.0f, 0.0f };
            unsigned      send_ping_nid{ 0 };
            unsigned char send_ping_type{ 0 };

            bool block_manual_issue_order{ };
            bool block_manual_cast_spell{ };
            bool block_manual_update_chargeable{ };

            int  cheat_pid{ 0 };
            bool enabled_debug_logging{ false };
            bool starting{ };
            
            unsigned __int64 settings{ 0 };
        };

        struct s_shared_settings_t {
            bool player_aa_range = true;
            bool player_aa_range_rainbow = true;
            s_shared_vec4_t player_aa_range_color = s_shared_vec4_t::color(0, 200, 255, 255);

            bool ally_aa_range = true;
            s_shared_vec4_t ally_aa_range_color = s_shared_vec4_t::color(0, 100, 255, 75);

            bool enemy_aa_range = true;
            s_shared_vec4_t enemy_aa_range_color = s_shared_vec4_t::color(255, 80, 80, 175);

            bool ally_turret_range = true;
            s_shared_vec4_t ally_turret_range_color = s_shared_vec4_t::color(80, 255, 155, 100);

            bool enemy_turret_range = true;
            s_shared_vec4_t enemy_turret_range_color = s_shared_vec4_t::color(255, 200, 80, 155);

            bool player_skin = false;
            unsigned player_skin_id = 0;

            bool disable_circle_glow = false;
        };

        uintptr_t m_stub_base{ 0 };
        uintptr_t m_shared_vars_address{ 0 };
        s_shared_vars_t m_shared_vars{ };
        s_shared_settings_t m_shared_settings{ };

        auto update_shared_vars( ) -> void;
        auto write_shared_vars( ) -> void;
        auto write_shared_settings( ) -> void;

        auto call_with_returned( uint32_t function_index, uintptr_t remote_address ) -> void;
        auto call( uint32_t function_index, uintptr_t remote_address ) -> void;
    public:

        auto set_player_skin_id(uint32_t player_skin_id) -> void
        {
            m_shared_settings.player_skin_id = player_skin_id;
            m_shared_settings.player_skin    = true;
            write_shared_settings();
        }

        auto disable_player_skin() -> void
        {
            m_shared_settings.player_skin = false;
            write_shared_settings();
        }
        ~c_function_caller( );

        auto initialize( ) -> bool;
        auto shutdown( ) -> bool;

        auto issue_order_move( const sdk::math::Vec3& position ) -> void;
        auto issue_order_attack( unsigned network_id ) -> void;

        auto cast_spell( unsigned slot, const sdk::math::Vec3& start, const sdk::math::Vec3& end ) -> void;
        auto cast_spell( unsigned slot, const sdk::math::Vec3& position ) -> void;
        auto cast_spell( unsigned slot, unsigned network_id ) -> void;
        auto cast_spell( unsigned slot ) -> void;
        auto release_chargeable( unsigned slot, const sdk::math::Vec3& position, bool release = true ) -> void;

        auto        enable_glow( unsigned nid, unsigned color_argb, int glow_id = 0, int size = 3, int diffusion = 3, bool remove = false ) -> void;
        auto        set_turret_range_indicator( bool value, bool ally = false ) -> void;
        auto        floating_text(unsigned nid, const char* text, EFloatingTextType type) -> void;
        auto        send_chat(const char* text, bool all_chat) -> void;
        static auto send_ping(const sdk::math::Vec3& world_position, unsigned nid, ESendPingType ping_type) -> void;

        auto set_issue_order_blocked(bool value) -> void;
        auto set_cast_spell_blocked(bool value) -> void;
        auto set_update_chargeable_blocked( bool value ) -> void;

        auto ping( ) -> uint32_t;
        auto attack_delay( ) -> float;
        auto attack_cast_delay( ) -> float;

        auto is_zoom_bypassed() -> bool;
        auto is_glow_queueable() -> bool;

        auto is_movement_blocked( ) -> bool;
        auto is_casting_blocked( ) -> bool;
        auto is_update_chargeable_blocked( ) -> bool;
    private:
        sdk::memory::Process* m_process;
    };

}

extern std::unique_ptr< utils::c_function_caller > g_function_caller;
