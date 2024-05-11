#pragma once

#include "feature.hpp"
#include "menu_order.hpp"
#include "names.hpp"
#include "../menu/menu.hpp"
#include "helper.hpp"

namespace features {
    class Tracker final : public IFeature
    {
    public:
        enum class ERecallType
        {
            unknown   = 0,
            normal    = 6,
            empowered = 11
        };

        enum class ECampType
        {
            unknown = 0,
            krugs   = 1,
            raptors,
            gromp,
            wolves,
            red,
            blue,
            scuttle_crab,
            dragon,
            herald,
            baron
        };

        enum class EDashType
        {
            unknown = 0,
            ezreal_q,
            shaco_q,
            tristana_w,
            urgot_e
        };

        struct WardInstance
        {
            int16_t           index;
            Object::EWardType type{};
            float             expire_time{};
            float             start_time{};
            bool              was_updated{};
            bool              does_expire{};

            float detection_time{};

            bool               should_get_vision_polygon{ true };
            sdk::math::Polygon vision_polygon{};

            bool  is_visible{};
            bool  was_visible{};
            float last_visible_time{};
            bool  was_pinged{};

            bool glow_enabled{};
        };

        struct RecallInfo
        {
            std::string object_name;
            int16_t     index{};
            float       finish_time{};
            bool        canceled{};
            bool        finished{};
            float       cancel_time{};
            float       channel_duration{};
            float       start_time{};

            // new recall tracking things
            int16_t particle_index{};
            bool    should_remove{};

            ERecallType type{};
        };

        struct LastSeenInfo
        {
            int16_t index{};
            float   last_seen_time{};
            Vec3    last_position{};
            Vec3    last_raw_position{};
            Vec3    last_visible_position{};
            Vec3    last_visible_path_end{};
            float   last_visible_time{};
            float   recall_start{};
            float   time_in_recall{};
            bool    is_recalling{};
            float   total_recall_time{};

            float last_death_time{};
            float last_fow_duration{};
            float vision_gain_time{};

            // path tracking
            Vec3              last_path_position{};
            Vec3              path_end{};
            std::vector<Vec3> path{};
            int32_t           next_path_node{};
            bool              is_updated_path{};
            float             last_path_update_time{};

            bool glow_enabled{};
        };

        struct PathingInfo
        {
            int16_t           index{};
            bool              is_moving{};
            int32_t           next_path_node{};
            std::vector<Vec3> path{};
        };

        struct FlashCooldownInfo {

            int16_t index{};
            float cooldown_expire{};
        };

        struct SpellSlotInfo
        {
            bool               is_ready{};
            float              cooldown_expire{};
            float              cooldown{};
            int32_t            level{};
            float              mana_cost{};
            Renderer::Texture *spell_texture{};

            float last_cast_time{};
        };

        struct HeroSpellInfo
        {
            int16_t index{};
            bool    valid{};

            SpellSlotInfo spell_q{};
            SpellSlotInfo spell_w{};
            SpellSlotInfo spell_e{};
            SpellSlotInfo spell_r{};
            SpellSlotInfo summoner_d{};
            SpellSlotInfo summoner_f{};
        };

        struct TurretInfo
        {
            int16_t index{};
        };

        struct ObjectiveInfo
        {
            int16_t  index{};
            unsigned nid{};
            bool     glow_enabled{};
        };

        struct CampObject
        {
            int16_t index{};
            Vec3    position{};
            float   last_visible_time{};
            bool    is_dead{};
            bool    is_main_objective{};
            bool    is_glowing{};
            bool    is_glowing_extra{};
        };

        struct FloatingTextInstance
        {
            int16_t     object_index{};
            std::string message{};
            Color       message_color{};
            float       creation_time{};
        };

        struct CampInstance
        {
            // camp data
            ECampType                          type{};
            Vec3                               position{};
            std::vector<CampObject>            objects{};
            std::string                        name{};
            std::string                        alternative_name{};
            std::string                        display_name{};
            std::shared_ptr<Renderer::Texture> texture{};

            // camp state
            bool    is_ready{};
            bool    is_active{};
            int32_t objective_count{};
            int32_t max_objectives{};
            bool    is_under_attack{};
            bool    should_notify{};

            // pings
            bool did_ping{};

            // respawn track
            float last_update_time{};
            float last_action_time{};
            float last_action_start_time{};
            float death_time{};
            float respawn_time{};
            bool  is_respawning{};
            bool  is_disabled{};
            // notifications
            bool did_attack_notification{};
            bool did_death_notification{};

            bool is_botside{};

            auto is_object_saved(const int16_t index) const -> bool
            {
                for (const auto inst : objects)
                    if (inst.index == index) return true;

                return false;
            }

            auto remove_object(const int16_t index) -> void
            {
                const auto to_remove = std::ranges::remove_if(
                    objects, [&](const CampObject &inst) -> bool { return inst.index == index; });

                if (to_remove.empty()) return;

                objects.erase(to_remove.begin(), to_remove.end());
            }
        };

        struct HudInfo
        {
            int16_t                      index{};
            renderer::Renderer::Texture *champ_img{};
            renderer::Renderer::Texture *summoner_1_img{};
            renderer::Renderer::Texture *summoner_2_img{};
            renderer::Renderer::Texture *ult_img{};
            float                        health_normalized{};
            float                        mana_normalized{};
        };

         enum class notification_type
        {
            neutral = 0,
            aggro,
            death
        };

        struct NotificationInstance
        {
            std::string                        text{};
            float                              start_time{};
            float                              end_time{};
            bool                               animation{};
            bool                               play_sound{};
            std::shared_ptr<Renderer::Texture> texture{};
            notification_type                  type{};
        };

        struct DashData
        {
            bool      valid{};
            EDashType type{};
            float     duration{};
            Vec3      start{};
            Vec3      end{};
            float     speed{};
        };

        struct DashInstance
        {
            int16_t     index{};
            std::string champion_name{};
            float       start_time{};
            float       end_time{};
            bool        is_blink{};
            Vec3        start_position{};
            Vec3        end_position{};
            bool        can_update_position{ true };
            EDashType   type{};
        };

        struct SkillshotInstance
        {
            Vec3 start{};
            Vec3 end{};
            Vec3 position{};
            // static stats
            float speed{};
            float range{};
            float width{};
            bool  edge_range{};
            bool  is_linear{ true };
            // dynamic stats
            float       start_time{};
            float       server_cast_time{};
            float       end_time{};
            std::string name{};
            hash_t      name_hash{};
            // target
            int16_t target_index{};
            Vec3    closest_point{};
            bool    found_missile{};
            int16_t missile_index{};

            auto get_updated_position(const bool next_tick = false) -> Vec3
            {
                if (this->server_cast_time >= *g_time || this->speed == 0) return this->position;

                const auto time_in_flight    = *g_time - this->server_cast_time;
                const auto distance_traveled = time_in_flight * this->speed;

                if (distance_traveled > this->range) return this->position = this->end;

                return this->position =
                           this->start.extend(this->end, distance_traveled + (next_tick ? 0.033f * this->speed : 0.f));
            }
        };

        struct PredictionEvent
        {
            int16_t target_index{};
            Vec3    start_position{};
            Vec3    predicted_position{};
            int32_t calculation_count{};
            float   start_time{};
            float   end_time{};
        };

        struct ExperienceInstance
        {
            int16_t index{};
            int32_t level{};
            float   experience{};
            float   gain_amount{};
            float   experience_gain_time{};
            float   experience_cap{};
            float   required_experience_amount{};
            bool    initialized{};
        };

        struct GankInstance
        {
            int16_t index{};
            float   start_time{};
            float   end_time{};
        };

        struct ItemInstance
        {
            int16_t    owner_index{};
            EItemId    item{};
            ESpellSlot spell_slot{};
            int32_t    inventory_slot{};
            bool       is_ready{};
            float      cooldown_expire{};
            float      cooldown{};
        };

        struct MissingInstance
        {
            int16_t  index{};
            unsigned network_id{};
            float    last_visible_time{};
            bool     is_animating{};
            float    animation_start_time{};
            float    last_cast_time{};
            int32_t  last_cast_value{};
        };

        auto run() -> void override;
        auto on_draw() -> void override;

        auto get_name() noexcept -> hash_t override { return names::tracker; }

        auto get_recalls() -> std::vector<RecallInfo> { return m_recalls; }

        auto get_team_blue_spawn() const -> Vec3 { return m_team_blue_spawn; }

        auto get_team_red_spawn() const -> Vec3 { return m_team_red_spawn; }

        auto get_enemies() -> std::vector<int16_t> { return m_enemies; }

        auto get_last_seen_index(int16_t index) const -> std::optional<int32_t>;
        auto get_recall_index(int16_t index) const -> std::optional<int32_t>;
        auto get_recall(int16_t index) const -> std::optional<RecallInfo>;
        auto get_last_seen_data(int16_t index) -> std::optional<LastSeenInfo>;
        auto get_wards() -> std::vector<WardInstance> { return m_wards; }

        auto send_chat_cooldown(std::string champion_name, utils::MemoryHolder<SpellSlot> spell, ESpellSlot slot,
                                unsigned network_id, bool was_manual) -> bool;
        auto add_text_popup(int16_t target_index, const std::string &message, Color message_color) -> void;

#if _DEBUG
        auto get_full_name() -> std::string override { return "c_tracker"; }
#endif

        auto initialize_menu() -> void override
        {
            const auto navigation = g_window->push(_("awareness"), menu_order::awareness);

            const auto players       = navigation->add_section(_("heroes"));
            const auto world         = navigation->add_section(_("world"));
            const auto spelltracker  = navigation->add_section(_("spelltracker"));
            const auto godtracker    = navigation->add_section(_("god jungletracker"));
            const auto itemtracker   = navigation->add_section(_("item tracker"));
            const auto last_position = navigation->add_section(_("position tracker"));
            const auto missing       = navigation->add_section(_("mia tracker"));
            const auto zoom          = navigation->add_section(_("zoomhack"));
            const auto misc          = navigation->add_section(_("misc"));
            const auto hud           = navigation->add_section(_("hud"));

            world->checkbox(_("show wards"), g_config->awareness.ward_tracker);
            world->checkbox(_("show ward vision"), g_config->awareness.show_ward_range);
            world->slider_int(_("ward indicator scale"), g_config->awareness.ward_scale, 100, 300, 1);

            world->checkbox(_("show clones"), g_config->awareness.show_clones)
                ->right_click_menu()
                ->add_color_picker(_("COLOR"), g_config->awareness.show_enemy_paths_color);
            world->checkbox(_("show enemy turret range"), g_config->awareness.show_enemy_turret_range);
            world->checkbox(_("show ally turret range"), g_config->awareness.show_ally_turret_range);

            auto ar2 = players->checkbox(_("show local attack range"), g_config->visuals.local_attack_range)
                           ->right_click_menu();
            ar2->add_dropdown(_("DRAW STYLE"), g_config->visuals.local_attack_range_draw_style,
                              { _("OUTLINE"), _("FILLED"), _("BOTH") });
            ar2->add_color_picker(_("COLOR"), g_config->visuals.local_attack_range_color);

            players->select(_("show enemy attackrange"), g_config->visuals.enemy_attack_range,
                            { _("Off"), _("Dynamic"), _("Always") });
            players->checkbox(_("show enemy spell range"), g_config->awareness.show_enemy_spell_range);
            players->checkbox(_("show recall"), g_config->awareness.recall_tracker);
            players->checkbox(_("show path"), g_config->awareness.show_enemy_paths);
            players->select(_("show experience"), g_config->awareness.show_experience_mode,
                            { _("Off"), _("Enemies only"), _("All") });
            players->select(_("show gold"), g_config->awareness.show_gold_mode,
                            { _("Off"), _("Enemies only"), _("All") });
            players->checkbox(_("gank alert"), g_config->awareness.show_gank_alert);
            players->checkbox(_("orbwalking indicator (?)"), g_config->awareness.show_orbwalking_flag)
                ->set_tooltip(_("Shows a little indicator if enemy is detected as orbwalking."));

            zoom->checkbox(_("enable"), g_config->awareness.zoomhack_toggle);
            zoom->slider_int(_("zoom value"), g_config->awareness.zoomhack_modifier, 0, 3000);
            zoom->checkbox(_("fov enable"), g_config->awareness.fov_toggle);
            zoom->slider_int(_("fov value"), g_config->awareness.fov_modifier, 0, 45);

            last_position->checkbox(_("show last position"), g_config->awareness.last_seen_position);
            last_position->checkbox(_("show nearby enemies"), g_config->awareness.show_nearby_enemies);
            last_position->checkbox(_("simulate fow position (?)"), g_config->awareness.last_seen_simulate_position)
                ->set_tooltip(_("Will simulate position based on enemys last path when entering fog of war"));
            //last_position
            //    ->checkbox(_("try update position in fow (?)"), g_config->awareness.last_seen_look_for_particles)
            //    ->set_tooltip(_("Will look for particles spawned by enemy abilities in fog of war"));
            last_position->select(_("move circle"), g_config->awareness.last_seen_circle_mode,
                                  { _("Minimap + World"), _("Minimap only"), _("Off") });
            last_position->slider_int(_("world scale"), g_config->awareness.last_seen_scale, 100, 300, 1);
            last_position->slider_int(_("minimap scale"), g_config->awareness.last_seen_minimap_scale, 100, 300, 1);
            last_position->slider_int(_("nearby enemies scale"), g_config->awareness.nearby_enemies_scale, 100, 300, 1);

            hud->checkbox(_("enable"), g_config->hud.toggle);
            hud->checkbox(_("vertical"), g_config->hud.vertical);
            hud->checkbox(_("send chat cooldown"), g_config->hud.send_chat_cooldown);
            hud->slider_int(_("x"), g_config->hud.base_x, 0, 3800);
            hud->slider_int(_("y"), g_config->hud.base_y, 0, 2100);
            hud->slider_int(_("scale"), g_config->hud.scale, 1, 100);

            godtracker->checkbox(_("enable"), g_config->awareness.jungletrack_enable);
            godtracker->checkbox(_("show camp population"), g_config->awareness.jungletrack_draw_count);
            godtracker->checkbox(_("camp glow"), g_config->awareness.jungletrack_glow);
            godtracker->checkbox(_("sound alert"), g_config->awareness.jungletrack_sound_alert);
            godtracker->checkbox(_("show camp population"), g_config->awareness.jungletrack_draw_count);
            godtracker->checkbox(_("show notification"), g_config->awareness.jungletrack_notification);
           // godtracker->select(_("notification size"), g_config->awareness.jungletrack_notification_size,
           //                    { _("25%"), _("50%"), _("75%"), _("100%"), _("125%"), _("150%") });
            godtracker->slider_int(_("camp timer size"), g_config->awareness.jungletrack_font_size, 12, 32);
            godtracker->slider_int(_("jungle alert scale"), g_config->awareness.jungle_alert_scale, 75, 250);

            spelltracker->checkbox(_("enable"), g_config->awareness.show_spell_cooldowns);
            spelltracker->checkbox(_("show spell level"), g_config->awareness.show_spell_level);
            spelltracker->select(_("show spells of "), g_config->awareness.show_spells_mode,
                                 { _("Enemies"), _("Enemies + Allies"), _("All") });

            itemtracker->checkbox(_("enable"), g_config->awareness.show_item_cooldowns);
            itemtracker->multi_select(_("show items of "),
                                      { g_config->awareness.show_ally_items, g_config->awareness.show_enemy_items },
                                      { _("Allies"), _("Enemies") });
            itemtracker->slider_int(_("item drawing scale"), g_config->awareness.item_tracker_scale, 75, 300);

            missing->checkbox(_("enable"), g_config->awareness.missing_indicator_enabled);
            missing->slider_int(_("indicator scale"), g_config->awareness.missing_indicator_scale, 100, 300, 1);

            misc->checkbox(_("internal glow"), g_config->awareness.internal_glow_toggle);
        }

    private:



        auto draw_ward_tracker() -> void;
        auto draw_recall_tracker() const -> void;
        auto draw_last_seen_position() -> void;
        auto draw_spell_tracker() const -> void;
        auto draw_clones() const -> void;
        auto draw_paths() const -> void;
        auto draw_attack_ranges() const -> void;
        auto draw_hud() -> void;
        auto draw_orbwalker_indicator() const -> void;
        auto draw_jungle_tracker() -> void;
        auto draw_dashes() const -> void;

        auto update_turrets() -> void;
        auto draw_turret_ranges() -> void;

        auto draw_notifications() -> void;

        auto update_wards() -> void;
        auto update_recalls() -> void;
        auto update_clones() -> void;
        auto update_spell_tracker() -> void;
        auto update_enemies() -> void;
        auto update_hud() -> void;
        auto update_jungle_tracker() -> void;
        auto update_last_seen_data() -> void;
        auto update_dashes() -> void;

        auto draw_nearby_enemies() -> void;

        auto draw_local_attack_range() const -> void;

        // ward tracker
        auto is_ward_active(int16_t index) -> bool;
        auto remove_invalid_wards() -> void;

        auto is_invalid_ward(unsigned network_id) -> bool;

        // recall tracker
        auto is_recall_active(int16_t index) -> bool;

        // last seen pos tracker
        auto is_hero_tracked(int16_t index) -> bool;

        static auto get_path_position(const Vec3 &start_position, const std::vector<Vec3> &path, int32_t waypoint_index,
                                      float distance) -> Vec3;

        static auto get_fow_particle_position(hash_t hero_name) -> std::optional<Vec3>;

        // clone tracker
        auto is_clone_tracked(int16_t index) -> bool;
        auto remove_clone(int16_t index) -> void;

        // turret tracker
        auto is_enemy_turret_tracked(int16_t index) -> bool;
        auto remove_turret(int16_t index) -> void;

        // spell tracker
        auto draw_spells(int16_t index) const -> void;
        auto is_hero_spells_tracked(int16_t index) -> bool;
        auto get_hero_spells(int16_t index) const -> std::optional<HeroSpellInfo>;
        auto force_update_hero_spell(int16_t index, hash_t name, float cooldown_expire) -> void;
    

        auto update_summoner_cooldowns() -> void;
        //hud
        auto is_hud_cached( int16_t index ) -> bool;
        auto get_hud_info( int16_t index ) -> HudInfo*;

        // enemy aa range draw
        auto is_enemy_saved( int16_t index ) const -> bool;

        // path tracker
        auto is_path_tracked( int16_t index ) -> bool;

        // objective tracker

        // hud send chat cooldowns
        auto send_cooldown_in_chat(
            Vec2                base_position,
            Vec2                size,
            const LeagueString& champion_name,
            int16_t             index,
            ESpellSlot          slot,
            unsigned            network_id
        ) -> void;

        // god jungle track
        auto is_monster_tracked( int16_t index ) const -> bool;
        auto initialize_camp( std::string name, bool topside ) const -> CampInstance;

        // notification thingy
        auto add_notification(
            const std::string&                          message,
            float                                       duration = 3.f,
            const std::shared_ptr< Renderer::Texture >& texture  = { },
            const notification_type type = notification_type::death
        ) -> void;
        auto remove_notification( const std::string& message ) -> void;

        // floating text external
        auto remove_text_popup( int16_t target_index, float creation_time ) -> void;
        auto is_clone_champion( hash_t name ) const -> bool;

        // dash tracking
        auto remove_dash( int16_t index, EDashType type, float start_time ) -> void;
        auto is_dash_active( int16_t index, Vec3 start_position ) const -> bool;

        // experience tracker
        auto update_experience_tracker( ) -> void;
        auto draw_experience_tracker( ) const -> void;

        // gank alerter
        auto draw_gank_alert( ) -> void;
        auto add_gank( int16_t index ) -> void;
        auto remove_gank( int16_t index ) -> void;

        // skinchanger
        auto update_skin() -> void;

        // enemy spellrange tracker
        auto draw_enemy_spell_ranges( ) const -> void;

        // item tracker
        auto draw_items( ) -> void;
        auto update_items( ) -> void;

        static auto get_item_texture( EItemId item ) -> std::shared_ptr< Renderer::Texture >;

        auto is_item_tracked( int16_t index, EItemId item ) const -> bool;
        auto get_items( int16_t index ) const -> std::vector< ItemInstance >;
        auto remove_item( int16_t index, EItemId item ) -> void;

        // Jungler Tracker
        auto initialize_jungler_tracking( ) -> void;
        auto draw_jungler_tracker( ) -> void;

        auto draw_jungle_alerts() -> void;

    public:
        static auto get_draw_style( int32_t selection ) -> int32_t;

        auto get_rainbow_color( ) const -> Color{ return m_rainbow_color; }

    private:
        std::vector< GankInstance >       m_ganks{ };
        MissingInstance                   m_enemy_jungler{ };
        std::vector< ExperienceInstance > m_experience{ };

        bool m_initialized_jungler_tracking{ };
        bool m_repositioning{ };
        bool m_ignore_changes{ };
        Vec2 m_reposition_start{ };

        bool m_skin_key_pressed_up{ };
        bool m_skin_key_pressed_down{ };
        int  m_skin_value{};

        bool    m_glow_working{ true };
        bool    m_local_glow{ };
        bool    m_rainbow_enabled{ };
        int32_t m_last_glow_id{ };
        int32_t m_glow_layer_count{ };
        int32_t m_last_glow_layer_count{ };
        bool    m_adjusted_layers{ };
        float   m_last_tick{ };
        bool    m_update_color{ };
        int32_t m_color_stage{ };
        int32_t m_color_value{ };

        auto get_summoner_cooldown( int16_t index ) const -> float {

            for (auto inst : m_summoner_cooldown_info)
            {
                if (inst.index != index) continue;

                return inst.cooldown_expire;
            }

            return 0.f;
        }

        std::vector<FlashCooldownInfo>      m_summoner_cooldown_info{};
        std::vector< WardInstance >         m_wards{ };
        std::vector< RecallInfo >           m_recalls{ };
        std::vector< LastSeenInfo >         m_last_seen_data{ };
        std::vector< int16_t >              m_clones{ };
        std::vector< int16_t >              m_enemy_turrets{ };
        std::mutex                          m_mutex;
        std::vector< PathingInfo >          m_paths{ };
        std::vector< HeroSpellInfo >        m_hero_spells{ };
        std::vector< int16_t >              m_enemies{ };
        std::vector< ObjectiveInfo >        m_objectives{ };
        std::vector< CampInstance >         m_jungle_camps{ };
        std::vector< NotificationInstance > m_notifications{ };
        std::vector< FloatingTextInstance > m_text_popups{ };
        std::vector< SkillshotInstance >    m_skillshots{ };
        std::vector< DashInstance >         m_dashes{ };
        std::vector< ItemInstance >         m_hero_items{ };
        std::vector< MissingInstance >      m_missing_enemies{ };
        std::vector< HudInfo >              m_hud_infos{ };

        Vec3 m_team_blue_spawn = { 394.f, 182.133f, 461.f };
        Vec3 m_team_red_spawn  = { 14340.f, 171.978f, 14390.f };

        bool    m_cached_enemies{ };
        bool    m_cached_turrets{ };
        int32_t m_last_zoom_value{ };
        int32_t m_last_fov_value{ };

        Vec3 m_topside_blue    = Vec3( 3821.49f, 52.0359f, 7901.05f );
        Vec3 m_topside_wolves  = Vec3( 3780.63f, 52.4632f, 6443.98f );
        Vec3 m_topside_gromp   = Vec3( 2110.63f, 51.7773f, 8450.98f );
        Vec3 m_topside_red     = Vec3( 7101.87f, 56.2827f, 10900.5f );
        Vec3 m_topside_krugs   = Vec3( 6317.09f, 56.4768f, 12146.5f );
        Vec3 m_topside_raptors = Vec3( 7987.f, 52.3479f, 9471.39f );
        Vec3 m_topside_crab    = Vec3( 4400.f, -66.5308f, 9600.f );

        Vec3 m_botside_blue    = Vec3( 11031.7f, 51.7236f, 6990.84f );
        Vec3 m_botside_wolves  = Vec3( 11008.2f, 62.0905f, 8387.41f );
        Vec3 m_botside_gromp   = Vec3( 12703.6f, 51.6908f, 6443.98f );
        Vec3 m_botside_red     = Vec3( 7765.24f, 53.9564f, 4020.19f );
        Vec3 m_botside_krugs   = Vec3( 8482.47f, 50.6481f, 2705.95f );
        Vec3 m_botside_raptors = Vec3( 6823.9f, 54.7828f, 5507.76f );
        Vec3 m_botside_crab    = Vec3( 10500.f, -62.8102f, 5170.f );

        Vec3 m_dragon = Vec3( 9866.15f, -71.2406f, 4414.01f );
        Vec3 m_herald = Vec3( 4932.f, -71.2406f, 10370.f );

        std::vector< hash_t > m_clone_champions = {
            ct_hash( "Leblanc" ),
            ct_hash( "MonkeyKing" ),
            ct_hash( "Shaco" ),
            ct_hash( "Neeko" )
        };

        Color                   m_rainbow_color{ };
        std::vector< unsigned > m_invalid_wards{ };

        // new recall track thing
        std::vector< unsigned > m_ignored_particles{ };
        auto                    is_particle_ignored( unsigned network_id ) const -> bool;

        // debug shit
        std::vector< unsigned > m_ignored_objects{ };
        auto                    is_ignored_object( unsigned network_id ) const -> bool;

        float m_last_chat_time{ };
        bool  m_did_click{ };

        //wave tracking
        bool   m_next_wave_is_cannon{ };
        int8_t m_wave_to_cannon{ };
        float  m_last_wave_time{ };
        float  m_last_cannon_notification_time{ };
    };
}
