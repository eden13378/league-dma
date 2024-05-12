#pragma once
#include "c_config_system.hpp"

#include "../features/orbwalker.hpp"

#include "../security/src/xorstr.hpp"

#define config_var( var_name, _struct, value ) std::shared_ptr< ConfigVar > var_name = g_config_system->add_item( value, std::format(("{}_{}"), _(#var_name), _(#_struct)), rt_hash( std::format(("{}_{}"), _(#var_name), _(#_struct)).c_str() ) );
#define config_var_raw( var_name, _struct, value, key, mode ) \
    std::shared_ptr< ConfigVar > var_name = \
        g_config_system->add_item_raw( c_config_var<bool>(value, #var_name, key, mode), std::format(("{}_{}"), _(#var_name), _(#_struct)), rt_hash( std::format(("{}_{}"), _(#var_name), _(#_struct)).c_str() ) );

namespace config {
    class c_config {
    public:
        struct {
            config_var( toggle, orbwalker, true )
            config_var_raw( orbwalker, orbwalker, false, utils::EKey::space, keybind_t::EKeybindMode::hold )
            config_var( target_selection_mode, orbwalker, 3 )
            config_var_raw( lasthit, orbwalker, false, utils::EKey::X, keybind_t::EKeybindMode::hold )
            config_var_raw( harass, orbwalker, false, utils::EKey::C, keybind_t::EKeybindMode::hold )
            config_var_raw( lane_clear, orbwalker, false, utils::EKey::V, keybind_t::EKeybindMode::hold )
            config_var_raw( flee, orbwalker, false, utils::EKey::Z, keybind_t::EKeybindMode::hold )
            config_var( draw_target_mode, orbwalker, 1 )
            config_var( draw_mode, orbwalker, true )
            config_var( draw_attack_timer, orbwalker, false )
            config_var( draw_hold_radius, orbwalker, true )
            config_var( draw_rainbow_attack_range, orbwalker, true )
            config_var( extra_delay, orbwalker, 0 )
            config_var( windup_ping_modifier, orbwalker, 2 )
            config_var( draw_lasthittable_minions, orbwalker, true )
            config_var( draw_path, orbwalker, true )

            config_var( disable_aa_level, orbwalker, 31 )
            config_var( support_mode, orbwalker, false )
            config_var( windwall_check, orbwalker, true )
            config_var( attack_plants, orbwalker, true )
            config_var( melee_magnet, orbwalker, true )

            config_var( ignore_missile_speed, orbwalker, false )
            config_var( new_farm_logic, orbwalker, true )
            config_var( lasthit_predict_ping, orbwalker, true )

            config_var( extrapolate_cursor, orbwalker, true )

            config_var( reaction_time, orbwalker, 1 )
            config_var( laneclear_priority, orbwalker, 0 )
            config_var_raw(
                autospacing_toggle,
                orbwalker,
                true,
                utils::EKey::xbutton1,
                keybind_t::EKeybindMode::toggle
            )

            config_var( spacing_modifier, orbwalker, 50 )

            config_var(glow_layers, orbwalker, 8)
            config_var( external_glow_size, orbwalker, 2 )

            config_var( draw_turret_minion, orbwalker, false )
            config_var( draw_current_minion_tower, orbwalker, false )
            config_var( draw_next_minion_tower, orbwalker, true )

            config_var( orbwalk_necessary_only, orbwalker, true )
            config_var( shorten_path, orbwalker, false )

            config_var( draw_leftclick_target_indicator, orbwalker, true )


            config_var( glow_preset, orbwalker, 0 )

            // glow
            config_var( glow_mode, orbwalker, 2 )

            config_var( enable_first_glow, orbwalker, true )
            config_var( glow_style, orbwalker, 0 )
            config_var( glow_size, orbwalker, 3 )
            config_var( glow_diffusion, orbwalker, 0 )
            config_var( glow_color, orbwalker, Color::white( ).alpha( 200 ) )

            config_var( enable_second_glow, orbwalker, true )
            config_var( second_glow_style, orbwalker, 0 )
             config_var( second_glow_size, orbwalker, 3 )
             config_var( second_glow_diffusion, orbwalker, 0 )
             config_var( second_glow_color, orbwalker, Color::white( ).alpha( 200 ) )

             config_var( enable_third_glow, orbwalker, true )
            config_var( third_glow_style, orbwalker, 0 )
             config_var( third_glow_size, orbwalker, 3 )
             config_var( third_glow_diffusion, orbwalker, 0 )
             config_var( third_glow_color, orbwalker, Color::white( ).alpha( 200 ) )


        } orbwalker;

        struct {
            config_var( selector_mode, target_selector, 1 )
            config_var( selector_aa_to_kill_priority, target_selector, 4 )
            config_var( selector_left_click_target, target_selector, true )
        } target_selector;

        struct {
            // priority 1

            config_var( alistar_priority, target_selector, 1 )
            config_var( amumu_priority, target_selector, 1 )
            config_var( bard_priority, target_selector, 1 )
            config_var( blitzcrank_priority, target_selector, 1 )
            config_var( braum_priority, target_selector, 1 )
            config_var( chogath_priority, target_selector, 1 )
            config_var( drmundo_priority, target_selector, 1 )
            config_var( garen_priority, target_selector, 1 )
            config_var( gnar_priority, target_selector, 1 )
            config_var( hecarim_priority, target_selector, 1 )
            config_var( illaoi_priority, target_selector, 1 )
            config_var( janna_priority, target_selector, 1 )
            config_var( jervaniv_priority, target_selector, 1 )
            config_var( ksante_priority, target_selector, 1 )
            config_var( leona_priority, target_selector, 1 )
            config_var( lulu_priority, target_selector, 1 )
            config_var( malphite_priority, target_selector, 1 )
            config_var( nami_priority, target_selector, 1 )
            config_var( nasus_priority, target_selector, 1 )
            config_var( nautilus_priority, target_selector, 1 )
            config_var( nunu_priority, target_selector, 1 )
            config_var( olaf_priority, target_selector, 1 )
            config_var( ornn_priority, target_selector, 1 )
            config_var( rammus_priority, target_selector, 1 )
            config_var( rell_priority, target_selector, 1 )
            config_var( renekton_priority, target_selector, 1 )
            config_var( sejuani_priority, target_selector, 1 )
            config_var( shen_priority, target_selector, 1 )
            config_var( shyvana_priority, target_selector, 1 )
            config_var( singed_priority, target_selector, 1 )
            config_var( sion_priority, target_selector, 1 )
            config_var( skarner_priority, target_selector, 1 )
            config_var( sona_priority, target_selector, 1 )
            config_var( tahmkench_priority, target_selector, 1 )
            config_var( taric_priority, target_selector, 1 )
            config_var( thresh_priority, target_selector, 1 )
            config_var( yorick_priority, target_selector, 1 )
            config_var( volibear_priority, target_selector, 1 )
            config_var( warwick_priority, target_selector, 1 )
            config_var( yuumi_priority, target_selector, 1 )
            config_var( monkeyking_priority, target_selector, 1 )
            config_var( zac_priority, target_selector, 1 )


            //priority 2


            config_var( aatrox_priority, target_selector, 2 )
            config_var( camille_priority, target_selector, 2 )
            config_var( darius_priority, target_selector, 2 )
            config_var( elise_priority, target_selector, 2 )
            config_var( evelynn_priority, target_selector, 2 )
            config_var( galio_priority, target_selector, 2 )
            config_var( gangplank_priority, target_selector, 2 )
            config_var( gragas_priority, target_selector, 2 )
            config_var( irelia_priority, target_selector, 2 )
            config_var( ivern_priority, target_selector, 2 )
            config_var( jax_priority, target_selector, 2 )
            config_var( kled_priority, target_selector, 2 )
            config_var( leesin_priority, target_selector, 2 )
            config_var( lillia_priority, target_selector, 2 )
            config_var( maokai_priority, target_selector, 2 )
            config_var( morgana_priority, target_selector, 2 )
            config_var( nocturne_priority, target_selector, 2 )
            config_var( pantheon_priority, target_selector, 2 )
            config_var( poppy_priority, target_selector, 2 )
            config_var( reksai_priority, target_selector, 2 )
            config_var( rakan_priority, target_selector, 2 )
            config_var( rengar_priority, target_selector, 2 )
            config_var( rumble_priority, target_selector, 2 )
            config_var( ryze_priority, target_selector, 2 )
            config_var( sett_priority, target_selector, 2 )
            config_var( swain_priority, target_selector, 2 )
            config_var( sylas_priority, target_selector, 2 )
            config_var( trundle_priority, target_selector, 2 )
            config_var( tryndamere_priority, target_selector, 2 )
            config_var( udyr_priority, target_selector, 2 )
            config_var( urgot_priority, target_selector, 2 )
            config_var( vi_priority, target_selector, 2 )
            config_var( xinzhao_priority, target_selector, 2 )


            //priority 3


            config_var( akali_priority, target_selector, 3 )
            config_var( diana_priority, target_selector, 3 )
            config_var( ekko_priority, target_selector, 3 )
            config_var( fiddlesticks_priority, target_selector, 3 )
            config_var( fiora_priority, target_selector, 3 )
            config_var( fizz_priority, target_selector, 3 )
            config_var( gwen_priority, target_selector, 3 )
            config_var( heimerdinger_priority, target_selector, 3 )
            config_var( jayce_priority, target_selector, 3 )
            config_var( karma_priority, target_selector, 3 )
            config_var( kassadin_priority, target_selector, 3 )
            config_var( kayle_priority, target_selector, 3 )
            config_var( kayn_priority, target_selector, 3 )
            config_var( khazix_priority, target_selector, 3 )
            config_var( lissandra_priority, target_selector, 3 )
            config_var( mordekaiser_priority, target_selector, 3 )
            config_var( milio_priority, target_selector, 3 )
            config_var( neeko_priority, target_selector, 3 )
            config_var( nidalee_priority, target_selector, 3 )
            config_var( pyke_priority, target_selector, 3 )
            config_var( qiyana_priority, target_selector, 3 )
            config_var( renataglasc_priority, target_selector, 3 )
            config_var( riven_priority, target_selector, 3 )
            config_var( senna_priority, target_selector, 3 )
            config_var( shaco_priority, target_selector, 3 )
            config_var( taliyah_priority, target_selector, 3 )
            config_var( viego_priority, target_selector, 3 )
            config_var( vladimir_priority, target_selector, 3 )
            config_var( yasuo_priority, target_selector, 3 )
            config_var( zilean_priority, target_selector, 3 )
            config_var( zyra_priority, target_selector, 3 )


            //priority 4


            config_var( ahri_priority, target_selector, 4 )
            config_var( akshan_priority, target_selector, 4 )
            config_var( anivia_priority, target_selector, 4 )
            config_var( annie_priority, target_selector, 4 )
            config_var( aphelios_priority, target_selector, 4 )
            config_var( ashe_priority, target_selector, 4 )
            config_var( aurelionson_priority, target_selector, 4 )
            config_var( azir_priority, target_selector, 4 )
            config_var( belveth_priority, target_selector, 4 )
            config_var( brand_priority, target_selector, 4 )
            config_var( caitlyn_priority, target_selector, 4 )
            config_var( cassiopeia_priority, target_selector, 4 )
            config_var( corki_priority, target_selector, 4 )
            config_var( draven_priority, target_selector, 4 )
            config_var( ezreal_priority, target_selector, 4 )
            config_var( graves_priority, target_selector, 4 )
            config_var( jhin_priority, target_selector, 4 )
            config_var( jinx_priority, target_selector, 4 )
            config_var( kaisa_priority, target_selector, 4 )
            config_var( kalista_priority, target_selector, 4 )
            config_var( karthus_priority, target_selector, 4 )
            config_var( katarina_priority, target_selector, 4 )
            config_var( kennen_priority, target_selector, 4 )
            config_var( kindred_priority, target_selector, 4 )
            config_var( kogmaw_priority, target_selector, 4 )
            config_var( leblanc_priority, target_selector, 4 )
            config_var( lucian_priority, target_selector, 4 )
            config_var( lux_priority, target_selector, 4 )
            config_var( malzahar_priority, target_selector, 4 )
            config_var( masteryi_priority, target_selector, 4 )
            config_var( missfortune_priority, target_selector, 4 )
            config_var( nilah_priority, target_selector, 4 )
            config_var( orianna_priority, target_selector, 4 )
            config_var( quinn_priority, target_selector, 4 )
            config_var( samira_priority, target_selector, 4 )
            config_var( seraphine_priority, target_selector, 4 )
            config_var( sivir_priority, target_selector, 4 )
            config_var( syndra_priority, target_selector, 4 )
            config_var( talon_priority, target_selector, 4 )
            config_var( teemo_priority, target_selector, 4 )
            config_var( tristana_priority, target_selector, 4 )
            config_var( twistedfate_priority, target_selector, 4 )
            config_var( twitch_priority, target_selector, 4 )
            config_var( varus_priority, target_selector, 4 )
            config_var( vayne_priority, target_selector, 4 )
            config_var( veigar_priority, target_selector, 4 )
            config_var( velkoz_priority, target_selector, 4 )
            config_var( vex_priority, target_selector, 4 )
            config_var( viktor_priority, target_selector, 4 )
            config_var( xayah_priority, target_selector, 4 )
            config_var( xerath_priority, target_selector, 4 )
            config_var( yone_priority, target_selector, 4 )
            config_var( zed_priority, target_selector, 4 )
            config_var( zeri_priority, target_selector, 4 )
            config_var( ziggs_priority, target_selector, 4 )
            config_var( zoe_priority, target_selector, 4 )
            
            // priority 5

            config_var( soraka_priority, target_selector, 5 )

        } target_priorities;

        struct {
            config_var( autopotion_toggle, activator, true )
            config_var( autopotion_only_combo, activator, true )
            config_var( autopotion_disable_early, activator, true )
            config_var( autopotion_health_threshold, activator, 70 )

            config_var( heal_toggle, activator, true )
            config_var( heal_predict, activator, true )
            config_var( heal_percent_threshold, activator, 20 )
            config_var( heal_logic, activator, 1 )

            config_var( barrier_toggle, activator, true )
            config_var( barrier_predict, activator, true )
            config_var( barrier_percent_threshold, activator, 10 )
            config_var( barrier_logic, activator, 1 )

            config_var( ignite_toggle, activator, true )
            config_var( ignite_predict, activator, true )
            config_var( ignite_mode, activator, 1 )

            config_var( exhaust_toggle, activator, true )
            config_var_raw( exhaust_hotkey, activator, false, utils::EKey::F, keybind_t::EKeybindMode::hold )

            config_var( smite_toggle, activator, true )
            config_var( smite_buffs, activator, false )
            config_var( smite_crabs, activator, false )
            config_var( smite_predict_health, activator, true )

            config_var( qss_toggle, activator, true )
            config_var( qss_smart, activator, true )
            config_var( qss_delay, activator, 10 )

            config_var( cleanse_toggle, activator, true )
            config_var( cleanse_smart, activator, true )
            config_var( cleanse_delay, activator, 10 )

            // cleanse options
            config_var( cleanse_stun, activator, true )
            config_var( cleanse_root, activator, true )
            config_var( cleanse_sleep, activator, true )
            config_var( cleanse_drowsy, activator, false )
            config_var( cleanse_suppression, activator, true )
            config_var( cleanse_fear, activator, true )
            config_var( cleanse_charm, activator, true )
            config_var( cleanse_berserk, activator, true )
            config_var( cleanse_taunt, activator, true )
            config_var( cleanse_polymorph, activator, true )
            config_var( cleanse_exhaust, activator, true )
            config_var( cleanse_nasus_w, activator, true )
            config_var( cleanse_mordekaiser_r, activator, true )
            config_var( cleanse_yasuo_r, activator, true )

            config_var( cleanse_disarm, activator, false )
            config_var( cleanse_blind, activator, false )
            config_var( cleanse_ignite, activator, false )

            config_var( everfrost_toggle, activator, true )
            config_var( everfrost_only_in_combo, activator, true )
            config_var( everfrost_hitchance, activator, 2 )

            config_var( warding_toggle, activator, true )
            config_var( warding_fully_automatic, activator, true )
            config_var( warding_controlward_reminder, activator, true )
            config_var( wardhelper_toggle, activator, true )

            config_var( goredrinker_toggle, activator, true )
            config_var( goredrinker_only_in_combo, activator, true )
            config_var( goredrinker_HP_thresh_toggle, activator, true )
            config_var( goredrinker_min_hit, activator, 2 )
            config_var( goredrinker_HP_thresh, activator, 30 )

            config_var( mikaels_toggle, activator, true )

            config_var( redemption_toggle, activator, true )
            config_var( redemption_ally_hp_threshold, activator, 10 )

            config_var( locket_toggle, activator, true )
            config_var( locket_ally_hp_threshold, activator, 10 )
        } activator;

        struct {
            config_var( toggle, hud, true )
            config_var( vertical, hud, false )
            config_var( send_chat_cooldown, hud, true )
            config_var( base_x, hud, 0 )
            config_var( base_y, hud, 0 )
            config_var( scale, hud, 60 )
        } hud;

        struct {
            config_var_raw( toggle, evade, true, utils::EKey::K, keybind_t::EKeybindMode::toggle )
            config_var( draw_spells_mode, evade, 1 )
            config_var( draw_spells_style, evade, 2 )
            config_var( draw_spells_color_circular, evade, Color::red().alpha( 50 ) )
            config_var( draw_spells_color_linear, evade, Color::white().alpha( 200 ) )
            config_var( show_position, evade, true )
            config_var( show_text, evade, true )
            config_var( combo_min_danger_level, evade, 2 )
            config_var( min_danger_level, evade, 1 )

            config_var( minion_collision, evade, true )
            config_var( champion_collision, evade, true )

            config_var( humanizer_delay, evade, 0 )
            config_var( humanizer_maximum_angle, evade, 2 )
            config_var( humanizer_dangerous_delay, evade, 0 )
            config_var( humanizer_dangerous_maximum_angle, evade, 0 )
            config_var( humanizer_dangerous_threshold, evade, 4 )
            config_var( humanizer_crowdcontrol_dangerous, evade, true )
            config_var( humanizer_randomize_pattern, evade, true )
            config_var( visualize_maximum_angle, evade, false )

            config_var( dodge_linear, evade, true )
            config_var( dodge_circle, evade, true )
            config_var( dodge_traps, evade, true )

            config_var( autoattack_sync, evade, true )

            config_var( extra_dodge_time, evade, 3 )
            config_var( calculation_accuracy, evade, 50 )
            config_var( default_dodge_priority, evade, 1 )
            config_var( combo_dodge_priority, evade, 0 )
            config_var( spell_dodge_priority, evade, 1 )

            config_var( preset_mode, evade, 0)

            config_var( force_longest_path, evade, false )

            config_var( extra_distance, evade, 0 )
            config_var( tether_distance, evade, 40 )
            config_var( use_server_position, evade, false )
            config_var( new_tether_logic, evade, false )
            config_var( include_ping_mode, evade, 0 )
            config_var( hitbox_modifier, evade, 2 )
            config_var( block_input, evade, true )
            config_var( smooth_pathing, evade, true )
            config_var( dodge_fow, evade, true )
            config_var( alternative_tether_logic, evade, false )

            config_var( simulate_spells, evade, false )
            config_var( simulation_delay, evade, 5 )


            config_var( toggle_spells, evade, true )
            config_var( spell_min_danger_level, evade, 3 )

            config_var( toggle_summoner, evade, true )
            config_var( summoner_min_danger_level, evade, 5 )

            config_var( draw_spells_blue, evade, true )
        } evade;

        struct {
            //aatrox
            config_var( aatrox_q_enabled, evade_spells, true )
            config_var( aatrox_q_danger, evade_spells, 3 )
            config_var( aatrox_q_min_dodge_health, evade_spells, 100 )

            config_var( aatrox_w_enabled, evade_spells, true )
            config_var( aatrox_w_danger, evade_spells, 2 )
            config_var( aatrox_w_min_dodge_health, evade_spells, 100 )

            //ahri
            config_var( ahri_q_enabled, evade_spells, true )
            config_var( ahri_q_danger, evade_spells, 1 )
            config_var( ahri_q_min_dodge_health, evade_spells, 100 )

            config_var( ahri_e_enabled, evade_spells, true )
            config_var( ahri_e_danger, evade_spells, 4 )
            config_var( ahri_e_min_dodge_health, evade_spells, 100 )

            //akali
            config_var( akali_e_enabled, evade_spells, true )
            config_var( akali_e_danger, evade_spells, 3 )
            config_var( akali_e_min_dodge_health, evade_spells, 100 )

            //akshan
            config_var( akshan_q_enabled, evade_spells, true )
            config_var( akshan_q_danger, evade_spells, 1 )
            config_var( akshan_q_min_dodge_health, evade_spells, 100 )

            //alistar
            config_var( alistar_q_enabled, evade_spells, true )
            config_var( alistar_q_danger, evade_spells, 3 )
            config_var( alistar_q_min_dodge_health, evade_spells, 100 )

            //amumu
            config_var( amumu_q_enabled, evade_spells, true )
            config_var( amumu_q_danger, evade_spells, 3 )
            config_var( amumu_q_min_dodge_health, evade_spells, 100 )

            config_var( amumu_r_enabled, evade_spells, true )
            config_var( amumu_r_danger, evade_spells, 5 )
            config_var( amumu_r_min_dodge_health, evade_spells, 100 )

            //anivia
            config_var( anivia_q_enabled, evade_spells, true )
            config_var( anivia_q_danger, evade_spells, 3 )
            config_var( anivia_q_min_dodge_health, evade_spells, 100 )

            //annie
            config_var( annie_w_enabled, evade_spells, true )
            config_var( annie_w_danger, evade_spells, 2 )
            config_var( annie_w_min_dodge_health, evade_spells, 100 )

            config_var( annie_r_enabled, evade_spells, true )
            config_var( annie_r_danger, evade_spells, 5 )
            config_var( annie_r_min_dodge_health, evade_spells, 100 )

            //aphelios
            config_var( aphelios_q_enabled, evade_spells, true )
            config_var( aphelios_q_danger, evade_spells, 2 )
            config_var( aphelios_q_min_dodge_health, evade_spells, 100 )

            config_var( aphelios_r_enabled, evade_spells, true )
            config_var( aphelios_r_danger, evade_spells, 2 )
            config_var( aphelios_r_min_dodge_health, evade_spells, 100 )

            //ashe
            config_var( ashe_w_enabled, evade_spells, true )
            config_var( ashe_w_danger, evade_spells, 1 )
            config_var( ashe_w_min_dodge_health, evade_spells, 100 )

            config_var( ashe_r_enabled, evade_spells, true )
            config_var( ashe_r_danger, evade_spells, 5 )
            config_var( ashe_r_min_dodge_health, evade_spells, 100 )

            //asol
            config_var( asol_q_enabled, evade_spells, true )
            config_var( asol_q_danger, evade_spells, 3 )
            config_var( asol_q_min_dodge_health, evade_spells, 100 )

            config_var( asol_r_enabled, evade_spells, true )
            config_var( asol_r_danger, evade_spells, 4 )
            config_var( asol_r_min_dodge_health, evade_spells, 100 )

            //azir
            config_var( azir_r_enabled, evade_spells, true )
            config_var( azir_r_danger, evade_spells, 3 )
            config_var( azir_r_min_dodge_health, evade_spells, 100 )

            //bard
            config_var( bard_q_enabled, evade_spells, true )
            config_var( bard_q_danger, evade_spells, 3 )
            config_var( bard_q_min_dodge_health, evade_spells, 100 )

            config_var( bard_r_enabled, evade_spells, true )
            config_var( bard_r_danger, evade_spells, 4 )
            config_var( bard_r_min_dodge_health, evade_spells, 100 )
            //brand
            config_var( brand_w_enabled, evade_spells, true )
            config_var( brand_w_danger, evade_spells, 2 )
            config_var( brand_w_min_dodge_health, evade_spells, 100 )

            config_var( brand_q_enabled, evade_spells, true )
            config_var( brand_q_danger, evade_spells, 2 )
            config_var( brand_q_min_dodge_health, evade_spells, 100 )

            //belveth
            config_var( belveth_w_enabled, evade_spells, true )
            config_var( belveth_w_danger, evade_spells, 3 )
            config_var( belveth_w_min_dodge_health, evade_spells, 100 )

            //blitzcrank
            config_var( blitzcrank_q_enabled, evade_spells, true )
            config_var( blitzcrank_q_danger, evade_spells, 4 )
            config_var( blitzcrank_q_min_dodge_health, evade_spells, 100 )

            //braum
            config_var( braum_q_enabled, evade_spells, true )
            config_var( braum_q_danger, evade_spells, 3 )
            config_var( braum_q_min_dodge_health, evade_spells, 100 )

            config_var( braum_r_enabled, evade_spells, true )
            config_var( braum_r_danger, evade_spells, 5 )
            config_var( braum_r_min_dodge_health, evade_spells, 100 )

            //caitlyn
            config_var( caitlyn_q_enabled, evade_spells, true )
            config_var( caitlyn_q_danger, evade_spells, 2 )
            config_var( caitlyn_q_min_dodge_health, evade_spells, 100 )

            config_var( caitlyn_e_enabled, evade_spells, true )
            config_var( caitlyn_e_danger, evade_spells, 2 )
            config_var( caitlyn_e_min_dodge_health, evade_spells, 100 )

            //camille
            config_var( camille_e_enabled, evade_spells, true )
            config_var( camille_e_danger, evade_spells, 2 )
            config_var( camille_e_min_dodge_health, evade_spells, 100 )

            //cassiopeia
            config_var( cass_q_enabled, evade_spells, true )
            config_var( cass_q_danger, evade_spells, 2 )
            config_var( cass_q_min_dodge_health, evade_spells, 100 )

            config_var( cass_r_enabled, evade_spells, true )
            config_var( cass_r_danger, evade_spells, 5 )
            config_var( cass_r_min_dodge_health, evade_spells, 100 )

            //chogath
            config_var( chogath_q_enabled, evade_spells, true )
            config_var( chogath_q_danger, evade_spells, 3 )
            config_var( chogath_q_min_dodge_health, evade_spells, 100 )

            config_var( chogath_w_enabled, evade_spells, true )
            config_var( chogath_w_danger, evade_spells, 1 )
            config_var( chogath_w_min_dodge_health, evade_spells, 100 )

            //corki
            config_var( corki_q_enabled, evade_spells, true )
            config_var( corki_q_danger, evade_spells, 1 )
            config_var( corki_q_min_dodge_health, evade_spells, 100 )

            config_var( corki_r_enabled, evade_spells, true )
            config_var( corki_r_danger, evade_spells, 3 )
            config_var( corki_r_min_dodge_health, evade_spells, 100 )

            //darius
            config_var( darius_e_enabled, evade_spells, true )
            config_var( darius_e_danger, evade_spells, 4 )
            config_var( darius_e_min_dodge_health, evade_spells, 100 )

            //diana
            config_var( diana_q_enabled, evade_spells, true )
            config_var( diana_q_danger, evade_spells, 2 )
            config_var( diana_q_min_dodge_health, evade_spells, 100 )

            config_var( diana_r_enabled, evade_spells, true )
            config_var( diana_r_danger, evade_spells, 5 )
            config_var( diana_r_min_dodge_health, evade_spells, 100 )

            //mundo
            config_var( mundo_q_enabled, evade_spells, true )
            config_var( mundo_q_danger, evade_spells, 2 )
            config_var( mundo_q_min_dodge_health, evade_spells, 100 )

            //draven
            config_var( draven_e_enabled, evade_spells, true )
            config_var( draven_e_danger, evade_spells, 2 )
            config_var( draven_e_min_dodge_health, evade_spells, 100 )

            config_var( draven_r_enabled, evade_spells, true )
            config_var( draven_r_danger, evade_spells, 4 )
            config_var( draven_r_min_dodge_health, evade_spells, 100 )

            //ekko
            config_var( ekko_q_enabled, evade_spells, true )
            config_var( ekko_q_danger, evade_spells, 1 )
            config_var( ekko_q_min_dodge_health, evade_spells, 100 )

            config_var( ekko_w_enabled, evade_spells, true )
            config_var( ekko_w_danger, evade_spells, 0 )
            config_var( ekko_w_min_dodge_health, evade_spells, 100 )

            //elise
            config_var( elise_e_enabled, evade_spells, true )
            config_var( elise_e_danger, evade_spells, 3 )
            config_var( elise_e_min_dodge_health, evade_spells, 100 )

            //evelynn
            config_var( evelynn_q_enabled, evade_spells, true )
            config_var( evelynn_q_danger, evade_spells, 3 )
            config_var( evelynn_q_min_dodge_health, evade_spells, 100 )

            config_var( evelynn_r_enabled, evade_spells, true )
            config_var( evelynn_r_danger, evade_spells, 4 )
            config_var( evelynn_r_min_dodge_health, evade_spells, 100 )

            //ezreal
            config_var( ezreal_q_enabled, evade_spells, true )
            config_var( ezreal_q_danger, evade_spells, 2 )
            config_var( ezreal_q_min_dodge_health, evade_spells, 100 )

            config_var( ezreal_w_enabled, evade_spells, true )
            config_var( ezreal_w_danger, evade_spells, 1 )
            config_var( ezreal_w_min_dodge_health, evade_spells, 100 )

            config_var( ezreal_r_enabled, evade_spells, true )
            config_var( ezreal_r_danger, evade_spells, 3 )
            config_var( ezreal_r_min_dodge_health, evade_spells, 100 )

            //fiora
            config_var( fiora_w_enabled, evade_spells, true )
            config_var( fiora_w_danger, evade_spells, 2 )
            config_var( fiora_w_min_dodge_health, evade_spells, 100 )

            //fizz
            config_var( fizz_r_enabled, evade_spells, true )
            config_var( fizz_r_danger, evade_spells, 4 )
            config_var( fizz_r_min_dodge_health, evade_spells, 100 )

            //galio
            config_var( galio_q_enabled, evade_spells, true )
            config_var( galio_q_danger, evade_spells, 2 )
            config_var( galio_q_min_dodge_health, evade_spells, 100 )

            config_var( galio_e_enabled, evade_spells, true )
            config_var( galio_e_danger, evade_spells, 3 )
            config_var( galio_e_min_dodge_health, evade_spells, 100 )

            //gnar
            config_var( gnar_q_enabled, evade_spells, true )
            config_var( gnar_q_danger, evade_spells, 1 )
            config_var( gnar_q_min_dodge_health, evade_spells, 100 )

            config_var( gnar_w_enabled, evade_spells, true )
            config_var( gnar_w_danger, evade_spells, 3 )
            config_var( gnar_w_min_dodge_health, evade_spells, 100 )

            config_var( gnar_r_enabled, evade_spells, true )
            config_var( gnar_r_danger, evade_spells, 4 )
            config_var( gnar_r_min_dodge_health, evade_spells, 100 )

            //gragas
            config_var( gragas_q_enabled, evade_spells, true )
            config_var( gragas_q_danger, evade_spells, 2 )
            config_var( gragas_q_min_dodge_health, evade_spells, 100 )

            config_var( gragas_e_enabled, evade_spells, true )
            config_var( gragas_e_danger, evade_spells, 3 )
            config_var( gragas_e_min_dodge_health, evade_spells, 100 )

            config_var( gragas_r_enabled, evade_spells, true )
            config_var( gragas_r_danger, evade_spells, 5 )
            config_var( gragas_r_min_dodge_health, evade_spells, 100 )

            //graves
            config_var( graves_q_enabled, evade_spells, true )
            config_var( graves_q_danger, evade_spells, 1 )
            config_var( graves_q_min_dodge_health, evade_spells, 100 )

            config_var( graves_w_enabled, evade_spells, true )
            config_var( graves_w_danger, evade_spells, 0 )
            config_var( graves_w_min_dodge_health, evade_spells, 100 )

            //hecarim
            config_var( hecarim_r_enabled, evade_spells, true )
            config_var( hecarim_r_danger, evade_spells, 5 )
            config_var( hecarim_r_min_dodge_health, evade_spells, 100 )

            //heimerdinger
            config_var( heimer_w_enabled, evade_spells, true )
            config_var( heimer_w_danger, evade_spells, 2 )
            config_var( heimer_w_min_dodge_health, evade_spells, 100 )

            config_var( heimer_e_enabled, evade_spells, true )
            config_var( heimer_e_danger, evade_spells, 2 )
            config_var( heimer_e_min_dodge_health, evade_spells, 100 )

            //illaoi
            config_var( illaoi_q_enabled, evade_spells, true )
            config_var( illaoi_q_danger, evade_spells, 2 )
            config_var( illaoi_q_min_dodge_health, evade_spells, 100 )

            config_var( illaoi_e_enabled, evade_spells, true )
            config_var( illaoi_e_danger, evade_spells, 3 )
            config_var( illaoi_e_min_dodge_health, evade_spells, 100 )

            //irelia
            config_var( irelia_e_enabled, evade_spells, true )
            config_var( irelia_e_danger, evade_spells, 3 )
            config_var( irelia_e_min_dodge_health, evade_spells, 100 )

            config_var( irelia_r_enabled, evade_spells, true )
            config_var( irelia_r_danger, evade_spells, 3 )
            config_var( irelia_r_min_dodge_health, evade_spells, 100 )

            //ivern
            config_var( ivern_q_enabled, evade_spells, true )
            config_var( ivern_q_danger, evade_spells, 3 )
            config_var( ivern_q_min_dodge_health, evade_spells, 100 )

            //janna
            config_var( janna_q_enabled, evade_spells, true )
            config_var( janna_q_danger, evade_spells, 3 )
            config_var( janna_q_min_dodge_health, evade_spells, 100 )

            //jarvan
            config_var( jarvan_q_enabled, evade_spells, true )
            config_var( jarvan_q_danger, evade_spells, 3 )
            config_var( jarvan_q_min_dodge_health, evade_spells, 100 )

            //jayce
            config_var( jayce_q_enabled, evade_spells, true )
            config_var( jayce_q_danger, evade_spells, 3 )
            config_var( jayce_q_min_dodge_health, evade_spells, 100 )

            //jhin
            config_var( jhin_w_enabled, evade_spells, true )
            config_var( jhin_w_danger, evade_spells, 2 )
            config_var( jhin_w_min_dodge_health, evade_spells, 100 )

            config_var( jhin_r_enabled, evade_spells, true )
            config_var( jhin_r_danger, evade_spells, 4 )
            config_var( jhin_r_min_dodge_health, evade_spells, 100 )

            //jinx
            config_var( jinx_w_enabled, evade_spells, true )
            config_var( jinx_w_danger, evade_spells, 3 )
            config_var( jinx_w_min_dodge_health, evade_spells, 100 )

            config_var( jinx_r_enabled, evade_spells, true )
            config_var( jinx_r_danger, evade_spells, 4 )
            config_var( jinx_r_min_dodge_health, evade_spells, 100 )

            //kaisa
            config_var( kaisa_w_enabled, evade_spells, true )
            config_var( kaisa_w_danger, evade_spells, 2 )
            config_var( kaisa_w_min_dodge_health, evade_spells, 100 )

            //karma
            config_var( karma_q_enabled, evade_spells, true )
            config_var( karma_q_danger, evade_spells, 3 )
            config_var( karma_q_min_dodge_health, evade_spells, 100 )

            //karthus
            config_var( karthus_q_enabled, evade_spells, true )
            config_var( karthus_q_danger, evade_spells, 1 )
            config_var( karthus_q_min_dodge_health, evade_spells, 100 )

            //kassadin
            config_var( kassadin_r_enabled, evade_spells, true )
            config_var( kassadin_r_danger, evade_spells, 3 )
            config_var( kassadin_r_min_dodge_health, evade_spells, 100 )

            //kayle
            config_var( kayle_q_enabled, evade_spells, true )
            config_var( kayle_q_danger, evade_spells, 1 )
            config_var( kayle_q_min_dodge_health, evade_spells, 100 )

            //kayn
            config_var( kayn_q_enabled, evade_spells, true )
            config_var( kayn_q_danger, evade_spells, 2 )
            config_var( kayn_q_min_dodge_health, evade_spells, 100 )

            config_var( kayn_w_enabled, evade_spells, true )
            config_var( kayn_w_danger, evade_spells, 2 )
            config_var( kayn_w_min_dodge_health, evade_spells, 100 )

            //kennen
            config_var( kennen_q_enabled, evade_spells, true )
            config_var( kennen_q_danger, evade_spells, 2 )
            config_var( kennen_q_min_dodge_health, evade_spells, 100 )

            //khazix
            config_var( khazix_w_enabled, evade_spells, true )
            config_var( khazix_w_danger, evade_spells, 2 )
            config_var( khazix_w_min_dodge_health, evade_spells, 100 )

            //kled
            config_var( kled_q_enabled, evade_spells, true )
            config_var( kled_q_danger, evade_spells, 3 )
            config_var( kled_q_min_dodge_health, evade_spells, 100 )

            config_var( kled_e_enabled, evade_spells, true )
            config_var( kled_e_danger, evade_spells, 2 )
            config_var( kled_e_min_dodge_health, evade_spells, 100 )

            //kogmaw
            config_var( kogmaw_q_enabled, evade_spells, true )
            config_var( kogmaw_q_danger, evade_spells, 1 )
            config_var( kogmaw_q_min_dodge_health, evade_spells, 100 )

            config_var( kogmaw_e_enabled, evade_spells, true )
            config_var( kogmaw_e_danger, evade_spells, 1 )
            config_var( kogmaw_e_min_dodge_health, evade_spells, 100 )

            config_var( kogmaw_r_enabled, evade_spells, true )
            config_var( kogmaw_r_danger, evade_spells, 2 )
            config_var( kogmaw_r_min_dodge_health, evade_spells, 100 )

            //leblanc
            config_var( leblanc_e_enabled, evade_spells, true )
            config_var( leblanc_e_danger, evade_spells, 3 )
            config_var( leblanc_e_min_dodge_health, evade_spells, 100 )

            //leesin
            config_var( leesin_q_enabled, evade_spells, true )
            config_var( leesin_q_danger, evade_spells, 3 )
            config_var( leesin_q_min_dodge_health, evade_spells, 100 )

            //leona
            config_var( leona_e_enabled, evade_spells, true )
            config_var( leona_e_danger, evade_spells, 4 )
            config_var( leona_e_min_dodge_health, evade_spells, 100 )

            config_var( leona_r_enabled, evade_spells, true )
            config_var( leona_r_danger, evade_spells, 2 )
            config_var( leona_r_min_dodge_health, evade_spells, 100 )

            //lillia
            config_var( lillia_e_enabled, evade_spells, true )
            config_var( lillia_e_danger, evade_spells, 2 )
            config_var( lillia_e_min_dodge_health, evade_spells, 100 )

            config_var( lillia_w_enabled, evade_spells, true )
            config_var( lillia_w_danger, evade_spells, 3 )
            config_var( lillia_w_min_dodge_health, evade_spells, 100 )

            //lissandra
            config_var( lissandra_q_enabled, evade_spells, true )
            config_var( lissandra_q_danger, evade_spells, 2 )
            config_var( lissandra_q_min_dodge_health, evade_spells, 100 )

            config_var( lissandra_e_enabled, evade_spells, true )
            config_var( lissandra_e_danger, evade_spells, 2 )
            config_var( lissandra_e_min_dodge_health, evade_spells, 100 )

            //lucian
            config_var( lucian_q_enabled, evade_spells, true )
            config_var( lucian_q_danger, evade_spells, 2 )
            config_var( lucian_q_min_dodge_health, evade_spells, 100 )

            config_var( lucian_w_enabled, evade_spells, true )
            config_var( lucian_w_danger, evade_spells, 1 )
            config_var( lucian_w_min_dodge_health, evade_spells, 100 )

            //lulu
            config_var( lulu_q_enabled, evade_spells, true )
            config_var( lulu_q_danger, evade_spells, 1 )
            config_var( lulu_q_min_dodge_health, evade_spells, 100 )

            //lux
            config_var( lux_q_enabled, evade_spells, true )
            config_var( lux_q_danger, evade_spells, 3 )
            config_var( lux_q_min_dodge_health, evade_spells, 100 )

            config_var( lux_e_enabled, evade_spells, true )
            config_var( lux_e_danger, evade_spells, 1 )
            config_var( lux_e_min_dodge_health, evade_spells, 100 )

            config_var( lux_r_enabled, evade_spells, true )
            config_var( lux_r_danger, evade_spells, 4 )
            config_var( lux_r_min_dodge_health, evade_spells, 100 )

            //malphite
            config_var( malphite_r_enabled, evade_spells, true )
            config_var( malphite_r_danger, evade_spells, 5 )
            config_var( malphite_r_min_dodge_health, evade_spells, 100 )

            //malzahar
            config_var( malz_q_enabled, evade_spells, true )
            config_var( malz_q_danger, evade_spells, 1 )
            config_var( malz_q_min_dodge_health, evade_spells, 100 )

            //miss_fortune
            config_var( mf_e_enabled, evade_spells, true )
            config_var( mf_e_danger, evade_spells, 0 )
            config_var( mf_e_min_dodge_health, evade_spells, 100 )

            //mordekaiser
            config_var( mord_q_enabled, evade_spells, true )
            config_var( mord_q_danger, evade_spells, 1 )
            config_var( mord_q_min_dodge_health, evade_spells, 100 )

            config_var( mord_e_enabled, evade_spells, true )
            config_var( mord_e_danger, evade_spells, 3 )
            config_var( mord_e_min_dodge_health, evade_spells, 100 )

            //morgana
            config_var( morg_q_enabled, evade_spells, true )
            config_var( morg_q_danger, evade_spells, 3 )
            config_var( morg_q_min_dodge_health, evade_spells, 100 )

            //nami
            config_var( nami_q_enabled, evade_spells, true )
            config_var( nami_q_danger, evade_spells, 3 )
            config_var( nami_q_min_dodge_health, evade_spells, 100 )

            config_var( nami_r_enabled, evade_spells, true )
            config_var( nami_r_danger, evade_spells, 4 )
            config_var( nami_r_min_dodge_health, evade_spells, 100 )

            //nautilus
            config_var( naut_q_enabled, evade_spells, true )
            config_var( naut_q_danger, evade_spells, 3 )
            config_var( naut_q_min_dodge_health, evade_spells, 100 )

            //neeko
            config_var( neeko_q_enabled, evade_spells, true )
            config_var( neeko_q_danger, evade_spells, 2 )
            config_var( neeko_q_min_dodge_health, evade_spells, 100 )

            config_var( neeko_e_enabled, evade_spells, true )
            config_var( neeko_e_danger, evade_spells, 3 )
            config_var( neeko_e_min_dodge_health, evade_spells, 100 )

            //nidalee
            config_var( nidalee_q_enabled, evade_spells, true )
            config_var( nidalee_q_danger, evade_spells, 2 )
            config_var( nidalee_q_min_dodge_health, evade_spells, 100 )

            //nocturne
            config_var( nocturne_q_enabled, evade_spells, true )
            config_var( nocturne_q_danger, evade_spells, 2 )
            config_var( nocturne_q_min_dodge_health, evade_spells, 100 )

            //nunu
            config_var( nunu_r_enabled, evade_spells, true )
            config_var( nunu_r_danger, evade_spells, 0 )
            config_var( nunu_r_min_dodge_health, evade_spells, 100 )

            //olaf
            config_var( olaf_q_enabled, evade_spells, true )
            config_var( olaf_q_danger, evade_spells, 2 )
            config_var( olaf_q_min_dodge_health, evade_spells, 100 )

            //orianna
            config_var( orianna_q_enabled, evade_spells, true )
            config_var( orianna_q_danger, evade_spells, 2 )
            config_var( orianna_q_min_dodge_health, evade_spells, 100 )

            //ornn
            config_var( ornn_r_enabled, evade_spells, true )
            config_var( ornn_r_danger, evade_spells, 3 )
            config_var( ornn_r_min_dodge_health, evade_spells, 100 )

            //pantheon
            config_var( panth_q_enabled, evade_spells, true )
            config_var( panth_q_danger, evade_spells, 2 )
            config_var( panth_q_min_dodge_health, evade_spells, 100 )

            config_var( panth_r_enabled, evade_spells, true )
            config_var( panth_r_danger, evade_spells, 3 )
            config_var( panth_r_min_dodge_health, evade_spells, 100 )

            //poppy
            config_var( poppy_q_enabled, evade_spells, true )
            config_var( poppy_q_danger, evade_spells, 2 )
            config_var( poppy_q_min_dodge_health, evade_spells, 100 )

            config_var( poppy_r_enabled, evade_spells, true )
            config_var( poppy_r_danger, evade_spells, 4 )
            config_var( poppy_r_min_dodge_health, evade_spells, 100 )

            //pyke
            config_var( pyke_q_enabled, evade_spells, true )
            config_var( pyke_q_danger, evade_spells, 3 )
            config_var( pyke_q_min_dodge_health, evade_spells, 100 )

            config_var( pyke_e_enabled, evade_spells, true )
            config_var( pyke_e_danger, evade_spells, 3 )
            config_var( pyke_e_min_dodge_health, evade_spells, 100 )

            config_var( pyke_r_enabled, evade_spells, true )
            config_var( pyke_r_danger, evade_spells, 5 )
            config_var( pyke_r_min_dodge_health, evade_spells, 100 )

            //Qiyana
            config_var( qiyana_q_enabled, evade_spells, true )
            config_var( qiyana_q_danger, evade_spells, 2 )
            config_var( qiyana_q_min_dodge_health, evade_spells, 100 )

            config_var( qiyana_r_enabled, evade_spells, true )
            config_var( qiyana_r_danger, evade_spells, 4 )
            config_var( qiyana_r_min_dodge_health, evade_spells, 100 )

            //Quinn
            config_var( quinn_q_enabled, evade_spells, true )
            config_var( quinn_q_danger, evade_spells, 2 )
            config_var( quinn_q_min_dodge_health, evade_spells, 100 )

            //rakan
            config_var( rakan_q_enabled, evade_spells, true )
            config_var( rakan_q_danger, evade_spells, 1 )
            config_var( rakan_q_min_dodge_health, evade_spells, 100 )

            config_var( rakan_w_enabled, evade_spells, true )
            config_var( rakan_w_danger, evade_spells, 3 )
            config_var( rakan_w_min_dodge_health, evade_spells, 100 )

            //reksai
            config_var( reksai_q_enabled, evade_spells, true )
            config_var( reksai_q_danger, evade_spells, 1 )
            config_var( reksai_q_min_dodge_health, evade_spells, 100 )

            //rell
            config_var( rell_r_enabled, evade_spells, true )
            config_var( rell_r_danger, evade_spells, 4 )
            config_var( rell_r_min_dodge_health, evade_spells, 100 )

            //renata
            config_var( renata_q_enabled, evade_spells, true )
            config_var( renata_q_danger, evade_spells, 3 )
            config_var( renata_q_min_dodge_health, evade_spells, 100 )

            config_var( renata_e_enabled, evade_spells, true )
            config_var( renata_e_danger, evade_spells, 2 )
            config_var( renata_e_min_dodge_health, evade_spells, 100 )

            config_var( renata_r_enabled, evade_spells, true )
            config_var( renata_r_danger, evade_spells, 4 )
            config_var( renata_r_min_dodge_health, evade_spells, 100 )

            //rengar
            config_var( rengar_e_enabled, evade_spells, true )
            config_var( rengar_e_danger, evade_spells, 3 )
            config_var( rengar_e_min_dodge_health, evade_spells, 100 )

            //riven
            config_var( riven_r_enabled, evade_spells, true )
            config_var( riven_r_danger, evade_spells, 3 )
            config_var( riven_r_min_dodge_health, evade_spells, 100 )

            //rumble
            config_var( rumble_e_enabled, evade_spells, true )
            config_var( rumble_e_danger, evade_spells, 2 )
            config_var( rumble_e_min_dodge_health, evade_spells, 100 )

            //ryze
            config_var( ryze_q_enabled, evade_spells, true )
            config_var( ryze_q_danger, evade_spells, 1 )
            config_var( ryze_q_min_dodge_health, evade_spells, 100 )

            //samira
            config_var( samira_q_enabled, evade_spells, true )
            config_var( samira_q_danger, evade_spells, 1 )
            config_var( samira_q_min_dodge_health, evade_spells, 100 )

            //sejuani
            config_var( sejuani_r_enabled, evade_spells, true )
            config_var( sejuani_r_danger, evade_spells, 4 )
            config_var( sejuani_r_min_dodge_health, evade_spells, 100 )

            //senna
            config_var( senna_q_enabled, evade_spells, true )
            config_var( senna_q_danger, evade_spells, 1 )
            config_var( senna_q_min_dodge_health, evade_spells, 100 )

            config_var( senna_w_enabled, evade_spells, true )
            config_var( senna_w_danger, evade_spells, 3 )
            config_var( senna_w_min_dodge_health, evade_spells, 100 )

            config_var( senna_r_enabled, evade_spells, true )
            config_var( senna_r_danger, evade_spells, 3 )
            config_var( senna_r_min_dodge_health, evade_spells, 100 )

            //seraphine
            config_var( seraphine_q_enabled, evade_spells, true )
            config_var( seraphine_q_danger, evade_spells, 1 )
            config_var( seraphine_q_min_dodge_health, evade_spells, 100 )

            config_var( seraphine_e_enabled, evade_spells, true )
            config_var( seraphine_e_danger, evade_spells, 2 )
            config_var( seraphine_e_min_dodge_health, evade_spells, 100 )

            config_var( seraphine_r_enabled, evade_spells, true )
            config_var( seraphine_r_danger, evade_spells, 3 )
            config_var( seraphine_r_min_dodge_health, evade_spells, 100 )

            //shen
            config_var( shen_e_enabled, evade_spells, true )
            config_var( shen_e_danger, evade_spells, 3 )
            config_var( shen_e_min_dodge_health, evade_spells, 100 )

            //shyvana
            config_var( shyv_e_enabled, evade_spells, true )
            config_var( shyv_e_danger, evade_spells, 3 )
            config_var( shyv_e_min_dodge_health, evade_spells, 100 )

            //sion
            config_var( sion_q_enabled, evade_spells, true )
            config_var( sion_q_danger, evade_spells, 0 )
            config_var( sion_q_min_dodge_health, evade_spells, 100 )

            config_var( sion_e_enabled, evade_spells, true )
            config_var( sion_e_danger, evade_spells, 2 )
            config_var( sion_e_min_dodge_health, evade_spells, 100 )

            //sivir
            config_var( sivir_q_enabled, evade_spells, true )
            config_var( sivir_q_danger, evade_spells, 1 )
            config_var( sivir_q_min_dodge_health, evade_spells, 100 )

            //sona
            config_var( sona_r_enabled, evade_spells, true )
            config_var( sona_r_danger, evade_spells, 5 )
            config_var( sona_r_min_dodge_health, evade_spells, 100 )

            //soraka
            config_var( soraka_q_enabled, evade_spells, true )
            config_var( soraka_q_danger, evade_spells, 1 )
            config_var( soraka_q_min_dodge_health, evade_spells, 100 )

            //swain
            config_var( swain_w_enabled, evade_spells, true )
            config_var( swain_w_danger, evade_spells, 1 )
            config_var( swain_w_min_dodge_health, evade_spells, 100 )

            config_var( swain_e_enabled, evade_spells, true )
            config_var( swain_e_danger, evade_spells, 3 )
            config_var( swain_e_min_dodge_health, evade_spells, 100 )

            //sylas
            config_var( sylas_q_enabled, evade_spells, true )
            config_var( sylas_q_danger, evade_spells, 2 )
            config_var( sylas_q_min_dodge_health, evade_spells, 100 )

            config_var( sylas_e_enabled, evade_spells, true )
            config_var( sylas_e_danger, evade_spells, 3 )
            config_var( sylas_e_min_dodge_health, evade_spells, 100 )

            //syndra
            config_var( syndra_q_enabled, evade_spells, true )
            config_var( syndra_q_danger, evade_spells, 1 )
            config_var( syndra_q_min_dodge_health, evade_spells, 100 )

            config_var( syndra_e_enabled, evade_spells, true )
            config_var( syndra_e_danger, evade_spells, 3 )
            config_var( syndra_e_min_dodge_health, evade_spells, 100 )
            //kench
            config_var( kench_q_enabled, evade_spells, true )
            config_var( kench_q_danger, evade_spells, 2 )
            config_var( kench_q_min_dodge_health, evade_spells, 100 )

            config_var( kench_w_enabled, evade_spells, true )
            config_var( kench_w_danger, evade_spells, 3 )
            config_var( kench_w_min_dodge_health, evade_spells, 100 )

            //taliyah
            config_var( taliyah_q_enabled, evade_spells, true )
            config_var( taliyah_q_danger, evade_spells, 2 )
            config_var( taliyah_q_min_dodge_health, evade_spells, 100 )

            config_var( taliyah_w_enabled, evade_spells, true )
            config_var( taliyah_w_danger, evade_spells, 3 )
            config_var( taliyah_w_min_dodge_health, evade_spells, 100 )

            //talon
            config_var( talon_w_enabled, evade_spells, true )
            config_var( talon_w_danger, evade_spells, 2 )
            config_var( talon_w_min_dodge_health, evade_spells, 100 )

            //thresh
            config_var( thresh_q_enabled, evade_spells, true )
            config_var( thresh_q_danger, evade_spells, 3 )
            config_var( thresh_q_min_dodge_health, evade_spells, 100 )

            config_var( thresh_e_enabled, evade_spells, true )
            config_var( thresh_e_danger, evade_spells, 2 )
            config_var( thresh_e_min_dodge_health, evade_spells, 100 )

            //tryndamere
            config_var( trynd_e_enabled, evade_spells, true )
            config_var( trynd_e_danger, evade_spells, 2 )
            config_var( trynd_e_min_dodge_health, evade_spells, 100 )

            //TF
            config_var( tf_q_enabled, evade_spells, true )
            config_var( tf_q_danger, evade_spells, 1 )
            config_var( tf_q_min_dodge_health, evade_spells, 100 )

            //twitch
            config_var( twitch_w_enabled, evade_spells, true )
            config_var( twitch_w_danger, evade_spells, 0 )
            config_var( twitch_w_min_dodge_health, evade_spells, 100 )

            //urgot
            config_var( urgot_q_enabled, evade_spells, true )
            config_var( urgot_q_danger, evade_spells, 2 )
            config_var( urgot_q_min_dodge_health, evade_spells, 100 )

            config_var( urgot_e_enabled, evade_spells, true )
            config_var( urgot_e_danger, evade_spells, 3 )
            config_var( urgot_e_min_dodge_health, evade_spells, 100 )

            config_var( urgot_r_enabled, evade_spells, true )
            config_var( urgot_r_danger, evade_spells, 3 )
            config_var( urgot_r_min_dodge_health, evade_spells, 100 )

            //varus
            config_var( varus_q_enabled, evade_spells, true )
            config_var( varus_q_danger, evade_spells, 2 )
            config_var( varus_q_min_dodge_health, evade_spells, 100 )

            config_var( varus_e_enabled, evade_spells, true )
            config_var( varus_e_danger, evade_spells, 1 )
            config_var( varus_e_min_dodge_health, evade_spells, 100 )

            config_var( varus_r_enabled, evade_spells, true )
            config_var( varus_r_danger, evade_spells, 4 )
            config_var( varus_r_min_dodge_health, evade_spells, 100 )

            //veigar
            config_var( veigar_q_enabled, evade_spells, true )
            config_var( veigar_q_danger, evade_spells, 1 )
            config_var( veigar_q_min_dodge_health, evade_spells, 100 )

            config_var( veigar_w_enabled, evade_spells, true )
            config_var( veigar_w_danger, evade_spells, 2 )
            config_var( veigar_w_min_dodge_health, evade_spells, 100 )

            //velkoz
            config_var( velkoz_q_enabled, evade_spells, true )
            config_var( velkoz_q_danger, evade_spells, 1 )
            config_var( velkoz_q_min_dodge_health, evade_spells, 100 )

            config_var( velkoz_w_enabled, evade_spells, true )
            config_var( velkoz_w_danger, evade_spells, 2 )
            config_var( velkoz_w_min_dodge_health, evade_spells, 100 )

            config_var( velkoz_e_enabled, evade_spells, true )
            config_var( velkoz_e_danger, evade_spells, 3 )
            config_var( velkoz_e_min_dodge_health, evade_spells, 100 )

            //vex
            config_var( vex_q_enabled, evade_spells, true )
            config_var( vex_q_danger, evade_spells, 2 )
            config_var( vex_q_min_dodge_health, evade_spells, 100 )

            config_var( vex_e_enabled, evade_spells, true )
            config_var( vex_e_danger, evade_spells, 2 )
            config_var( vex_e_min_dodge_health, evade_spells, 100 )

            config_var( vex_r_enabled, evade_spells, true )
            config_var( vex_r_danger, evade_spells, 4 )
            config_var( vex_r_min_dodge_health, evade_spells, 100 )

            //vi
            config_var( vi_q_enabled, evade_spells, true )
            config_var( vi_q_danger, evade_spells, 3 )
            config_var( vi_q_min_dodge_health, evade_spells, 100 )

            //viktor
            config_var( viktor_w_enabled, evade_spells, true )
            config_var( viktor_w_danger, evade_spells, 3 )
            config_var( viktor_w_min_dodge_health, evade_spells, 100 )

            config_var( viktor_e_enabled, evade_spells, true )
            config_var( viktor_e_danger, evade_spells, 2 )
            config_var( viktor_e_min_dodge_health, evade_spells, 100 )

            //viego
            config_var( viego_q_enabled, evade_spells, true )
            config_var( viego_q_danger, evade_spells, 2 )
            config_var( viego_q_min_dodge_health, evade_spells, 100 )

            config_var( viego_w_enabled, evade_spells, true )
            config_var( viego_w_danger, evade_spells, 3 )
            config_var( viego_w_min_dodge_health, evade_spells, 100 )

            config_var( viego_r_enabled, evade_spells, true )
            config_var( viego_r_danger, evade_spells, 3 )
            config_var( viego_r_min_dodge_health, evade_spells, 100 )

            //warwick
            config_var( warwick_r_enabled, evade_spells, true )
            config_var( warwick_r_danger, evade_spells, 5 )
            config_var( warwick_r_min_dodge_health, evade_spells, 100 )

            //xayah
            config_var( xayah_q_enabled, evade_spells, true )
            config_var( xayah_q_danger, evade_spells, 1 )
            config_var( xayah_q_min_dodge_health, evade_spells, 100 )

            //xerath
            config_var( xerath_q_enabled, evade_spells, true )
            config_var( xerath_q_danger, evade_spells, 2 )
            config_var( xerath_q_min_dodge_health, evade_spells, 100 )

            config_var( xerath_w_enabled, evade_spells, true )
            config_var( xerath_w_danger, evade_spells, 2 )
            config_var( xerath_w_min_dodge_health, evade_spells, 100 )

            config_var( xerath_e_enabled, evade_spells, true )
            config_var( xerath_e_danger, evade_spells, 3 )
            config_var( xerath_e_min_dodge_health, evade_spells, 100 )

            config_var( xerath_r_enabled, evade_spells, true )
            config_var( xerath_r_danger, evade_spells, 2 )
            config_var( xerath_r_min_dodge_health, evade_spells, 100 )

            //xin zhao
            config_var( xin_w_enabled, evade_spells, true )
            config_var( xin_w_danger, evade_spells, 3 )
            config_var( xin_w_min_dodge_health, evade_spells, 100 )

            //yasuo
            config_var( yasuo_q_enabled, evade_spells, true )
            config_var( yasuo_q_danger, evade_spells, 1 )
            config_var( yasuo_q_min_dodge_health, evade_spells, 100 )

            config_var( yasuo_q3_enabled, evade_spells, true )
            config_var( yasuo_q3_danger, evade_spells, 3 )
            config_var( yasuo_q3_min_dodge_health, evade_spells, 100 )

            //yone
            config_var( yone_q_enabled, evade_spells, true )
            config_var( yone_q_danger, evade_spells, 3 )
            config_var( yone_q_min_dodge_health, evade_spells, 100 )

            config_var( yone_r_enabled, evade_spells, true )
            config_var( yone_r_danger, evade_spells, 5 )
            config_var( yone_r_min_dodge_health, evade_spells, 100 )

            //zac
            config_var( zac_q_enabled, evade_spells, true )
            config_var( zac_q_danger, evade_spells, 4 )
            config_var( zac_q_min_dodge_health, evade_spells, 100 )

            //zed
            config_var( zed_q_enabled, evade_spells, true )
            config_var( zed_q_danger, evade_spells, 2 )
            config_var( zed_q_min_dodge_health, evade_spells, 100 )

            //zeri
            config_var( zeri_q_enabled, evade_spells, true )
            config_var( zeri_q_danger, evade_spells, 0 )
            config_var( zeri_q_min_dodge_health, evade_spells, 100 )

            config_var( zeri_w_enabled, evade_spells, true )
            config_var( zeri_w_danger, evade_spells, 2 )
            config_var( zeri_w_min_dodge_health, evade_spells, 100 )

            //ziggs
            config_var( ziggs_q_enabled, evade_spells, true )
            config_var( ziggs_q_danger, evade_spells, 1 )
            config_var( ziggs_q_min_dodge_health, evade_spells, 100 )

            config_var( ziggs_w_enabled, evade_spells, true )
            config_var( ziggs_w_danger, evade_spells, 2 )
            config_var( ziggs_w_min_dodge_health, evade_spells, 100 )

            config_var( ziggs_e_enabled, evade_spells, true )
            config_var( ziggs_e_danger, evade_spells, 2 )
            config_var( ziggs_e_min_dodge_health, evade_spells, 100 )

            config_var( ziggs_r_enabled, evade_spells, true )
            config_var( ziggs_r_danger, evade_spells, 0 )
            config_var( ziggs_r_min_dodge_health, evade_spells, 100 )

            //zilean
            config_var( zilean_q_enabled, evade_spells, true )
            config_var( zilean_q_danger, evade_spells, 3 )
            config_var( zilean_q_min_dodge_health, evade_spells, 100 )

            //zoe
            config_var( zoe_q_enabled, evade_spells, true )
            config_var( zoe_q_danger, evade_spells, 1 )
            config_var( zoe_q_min_dodge_health, evade_spells, 100 )

            config_var( zoe_e_enabled, evade_spells, true )
            config_var( zoe_e_danger, evade_spells, 3 )
            config_var( zoe_e_min_dodge_health, evade_spells, 100 )

            //zyra
            config_var( zyra_e_enabled, evade_spells, true )
            config_var( zyra_e_danger, evade_spells, 3 )
            config_var( zyra_e_min_dodge_health, evade_spells, 100 )

            config_var( zyra_r_enabled, evade_spells, true )
            config_var( zyra_r_danger, evade_spells, 4 )
            config_var( zyra_r_min_dodge_health, evade_spells, 100 )
        } evade_spells;

        struct {
            config_var( toggle, awareness, true )
            config_var( last_seen_position, awareness, true )
            config_var( recall_tracker, awareness, true )
            config_var( ward_tracker, awareness, true )
            config_var( objective_tracker, awareness, true )
            config_var( show_gold, awareness, true )

            config_var( show_nearby_enemies, awareness, true)
            config_var( nearby_enemies_scale, awareness, 100)

            config_var( ward_scale, awareness, 100 )
            config_var( show_ward_range, awareness, true )

            config_var( show_clones, awareness, true )
            config_var( show_clones_color, awareness, Color::white( ) )
            config_var( show_enemy_turret_range, awareness, true )
            config_var( show_ally_turret_range, awareness, false )
            config_var( show_turret_range_internally, awareness, false )
            config_var( show_enemy_paths, awareness, false )
            config_var( show_enemy_paths_color, awareness, Color::white( ) )
            config_var( show_path_time, awareness, false )
            config_var( show_orbwalking_flag, awareness, true )
            config_var( show_gank_alert, awareness, true )

            config_var( jungletrack_enable, awareness, true )
            config_var( jungletrack_update_aggro, awareness, false )
            config_var( jungletrack_notification, awareness, true )
            config_var( jungletrack_sound_alert, awareness, true )
            config_var( jungletrack_draw_count, awareness, false )
            config_var( jungletrack_notification_size, awareness, 3 )
            config_var( jungletrack_font_size, awareness, 16 )
            config_var( jungletrack_glow, awareness, true )

            config_var( jungle_alert_position_x, awareness, 0 )
            config_var( jungle_alert_position_y, awareness, 0 )
            config_var( jungle_alert_scale, awareness, 100 )

            config_var( show_spell_cooldowns, awareness, true )
            config_var( show_spell_level, awareness, true )
            config_var( show_spells_mode, awareness, 2 )

            config_var( show_item_cooldowns, awareness, true )
            config_var( show_enemy_items, awareness, true )
            config_var( show_ally_items, awareness, true )
            config_var( item_tracker_scale, awareness, 100 )

            config_var( show_enemy_spell_range, awareness, false )

            config_var( show_experience_mode, awareness, 1 )

            config_var( show_gold_mode, awareness, 1 )

            config_var( internal_glow_toggle, awareness, true )

            config_var( last_seen_circle_mode, awareness, 0 )
            config_var( last_seen_simulate_position, awareness, true )
            config_var( last_seen_look_for_particles, awareness, true )
            config_var( last_seen_scale, awareness, 100 )
            config_var( last_seen_minimap_scale, awareness, 100 )

            config_var( missing_indicator_enabled, awareness, true )
            config_var( missing_indicator_x, awareness, 0 )
            config_var( missing_indicator_y, awareness, 0 )
            config_var( missing_indicator_scale, awareness, 100 )

            config_var( zoomhack_toggle, awareness, false )
            config_var( zoomhack_modifier, awareness, 0 )
            config_var( fov_toggle, awareness, false )
            config_var( fov_modifier, awareness, 0 )

            config_var( enable_cannon_wave_tracker, awareness, false )
        } awareness;

        struct {
            config_var( q_hitchance, autocombo, 2 )
            config_var( w_hitchance, autocombo, 2 )
            config_var( e_hitchance, autocombo, 2 )
            config_var( r_hitchance, autocombo, 2 )
            config_var( prediction_visualize, autocombo, true )

            config_var( q_enabled, autocombo, true )
            config_var( w_enabled, autocombo, true )
            config_var( e_enabled,autocombo, true )
            config_var( r_enabled, autocombo, true )

            config_var_raw( semi_manual_ult_enabled, autocombo, false, utils::EKey::T, keybind_t::EKeybindMode::hold )

            config_var( enable_spell_laneclear, autocombo, true )
            config_var( enable_spell_lasthit, autocombo, true )
        } autocombo;

        struct {
            config_var( q_range, visuals, true )
            config_var( local_attack_range, visuals, true )
            config_var( local_attack_range_draw_style, visuals, 0 )
            config_var( local_attack_range_color, visuals, Color::white( ).alpha( 200 ) )
            config_var( enemy_attack_range, visuals, 1 )
            config_var( enemy_attack_range_draw_style, visuals, 0 )
            config_var( enemy_attack_range_color, visuals, Color::red( ).alpha( 150 ) )
        } visuals;


        struct {
            config_var( q_enabled, ezreal, true )
            config_var( q_killsteal, ezreal, true )
            config_var( q_auto_harass, ezreal, true )
            config_var( q_hitchance, ezreal, 2 )
            config_var( q_fullcombo_hitchance, ezreal, 0 )
            config_var( q_max_range, ezreal, 95 )

            config_var( w_enabled, ezreal, true )
            config_var( w_preferred, ezreal, true )
            config_var( w_hitchance, ezreal, 2 )
            config_var( w_fullcombo_hitchance, ezreal, 1 )
            config_var( w_max_range, ezreal, 95 )

            config_var( r_killsteal, ezreal, true )
            config_var( r_on_immobile, ezreal, true )
            config_var( r_multihit, ezreal, true )
            config_var( r_recall_ult, ezreal, true )

            config_var( r_killsteal_hitchance, ezreal, 2 )
            config_var( r_multihit_count, ezreal, 3 )
            config_var( r_recall_ult_delay, ezreal, true )

            config_var( lasthit_use_q, ezreal, true )
            config_var( q_lasthit_mode, ezreal, 1 )
            config_var( laneclear_use_q, ezreal, true )
             config_var( jungleclear_q, ezreal, true )

            config_var( combat_mode_enabled, ezreal, true )
            config_var_raw(
                combat_mode_toggle,
                ezreal,
                false,
                utils::EKey::xbutton1,
                keybind_t::EKeybindMode::toggle
            )

            config_var( combat_ignore_q_hitchance, ezreal, true )
            config_var( combat_ignore_w_hitchance, ezreal, false )
            config_var( combat_ignore_w_prefer, ezreal, true )
            config_var( combat_disable_r_logic, ezreal, true )

            config_var( combat_max_threshold, ezreal, 800 )


            config_var( q_draw_range, ezreal, true )
            config_var( w_draw_range, ezreal, false )
            config_var( e_draw_range, ezreal, false )
            config_var( dont_draw_on_cooldown, ezreal, true )
            config_var( e_draw_blink_position, ezreal, true )
            config_var( combat_draw_threshold, ezreal, true )
            config_var( combat_draw_text_mode, ezreal, 1 )

            config_var( show_debug_prediction, ezreal, false )
        } ezreal;

        struct {
            config_var( q_enabled, xerath, true )
            config_var( q_harass, xerath, true )
            config_var( q_multihit, xerath, true )
            config_var( q_overcharge, xerath, true )
            config_var( q_multihit_mode, xerath, 1 )
            config_var( q_hitchance, xerath, 2 )
            config_var( q_draw, xerath, true )
            config_var( q_debug_prediction, xerath, true )
            config_var( q_targeting_mode, xerath, 0 )

            config_var( w_enabled, xerath, true )
            config_var( w_flee, xerath, true )
            config_var( w_harass, xerath, true )
            config_var( w_antigapclose, xerath, true )
            config_var( w_multihit, xerath, true )
            config_var( w_prefer_center, xerath, true )
            config_var( w_multihit_mode, xerath, 1 )
            config_var( w_hitchance, xerath, 2 )
            config_var( w_killsteal, xerath, true )
            config_var( w_draw, xerath, false )

            config_var( e_enabled, xerath, true )
            config_var( e_flee, xerath, true )
            config_var( e_antigapclose, xerath, true )
            config_var( e_autointerrupt, xerath, true )
            config_var( e_hitchance, xerath, 2 )
            config_var( e_draw_missile, xerath, true )

            config_var( r_enabled, xerath, true )
            config_var_raw( r_fast_mode, xerath, false, utils::EKey::space, keybind_t::EKeybindMode::hold )
            config_var( r_hitchance, xerath, 2 )
            config_var( r_multihit, xerath, true )
            config_var( r_draw_range, xerath, true )

            // misc 
            config_var( prefer_w_over_q, xerath, true )
            config_var( e_delay_spells_during_travel, xerath, true )
        } xerath;

        struct {
            config_var( q_enabled, twitch, true )

            config_var( e_enabled, twitch, true )
            config_var( e_spellclear, twitch, true )
            config_var( e_predict_damage, twitch, true )
            config_var( e_coup_de_grace_rune, twitch, false )
            config_var( e_mode, twitch, 0 )
            config_var( e_damage_prediction_mode, twitch, 2)

            config_var( w_enabled, twitch, true )
            config_var( w_aa_reset, twitch, true )
            config_var( w_hitchance, twitch, 2 )

            config_var( e_draw_damage, twitch, true )
            config_var( r_draw_range, twitch, true )
            config_var( q_draw_time_left, twitch, true )
            config_var( r_draw_duration, twitch, true )
        } twitch;

        struct {
            config_var( q_enabled, jinx, true )

            config_var( w_enabled, jinx, true )
            config_var( w_only_out_of_range, jinx, true )
            config_var( w_hitchance, jinx, 2 )

            config_var( e_enabled, jinx, true )
            config_var( e_autointerrupt, jinx, true )
            config_var( e_antigapclose, jinx, true )
            config_var( e_antistasis, jinx, true )
            config_var( e_hitchance, jinx, 4 )


            config_var( r_recall_ult, jinx, true )
            config_var( r_recall_ult_delay, jinx, true )

            config_var( q_draw_range, jinx, true )
        } jinx;

        struct {
            config_var( q_enabled, brand, true )
            config_var( q_hitchance, brand, 4 )
            config_var( q_only_ablaze, brand, true )

            config_var( w_enabled, brand, true )
            config_var( w_hitchance, brand, 2 )
            config_var( w_multihit, brand, true )

            config_var( e_enabled, brand, true )

            config_var( r_enabled, brand, true )
            config_var( r_min_enemy, brand, 2 )


            config_var( q_draw_range, brand, false )
            config_var( w_draw_range, brand, true )
            config_var( e_draw_range, brand, false )
            config_var( r_draw_range, brand, false )
            config_var( dont_draw_on_cooldown, brand, true )
        } brand;

        struct {
            config_var( q_enabled, blitzcrank, true )
            config_var( q_autointerrupt, blitzcrank, true )
            config_var( q_hitchance, blitzcrank, 2 )
            config_var( q_ignore_range_full_combo, blitzcrank, true )
            config_var( q_max_range, blitzcrank, 80 )

            config_var( e_enabled, blitzcrank, true )
            config_var( e_reset_aa, blitzcrank, true )

            config_var( draw_q, blitzcrank, true )
            config_var( draw_r, blitzcrank, false )
        } blitzcrank;

        struct {
            config_var( q_enabled, kayle, true )
            config_var( q_hitchance, kayle, 4 )

            config_var( w_enabled, kayle, true )
            config_var( w_logic, kayle, 1 )
            config_var( w_only_combo, kayle, true )
            config_var( w_min_mana, kayle, 25 )
            config_var( w_min_hp_percent, kayle, 75 )

            config_var( e_enabled, kayle, true )
            config_var( e_lane_clear, kayle, true )

            config_var( r_enabled, kayle, true )
            config_var( r_only_full_combo, kayle, true )
            config_var( r_health_threshold, kayle, 10 )

            config_var( dont_draw_on_cooldown, kayle, true )
            config_var( q_draw_range, kayle, true )
            config_var( e_draw_range, kayle, true )
        } kayle;

        struct {
            config_var( q_enabled, karthus, true )
            config_var( q_harass, karthus, true )
            config_var( q_hitchance, karthus, 2 )
            config_var( q_multihit, karthus, true )
            config_var( q_prioritize_no_minion, karthus, true )
            config_var( q_killsteal, karthus, true )

            config_var( q_automatic_if_dead, karthus, true )
            config_var( q_ignore_hitchance_in_full_combo, karthus, true )

            config_var( e_enabled, karthus, true )
            config_var( e_min_enemy, karthus, 1 )
            config_var( e_min_mana, karthus, 15 )
            config_var( e_disable_under_min_mana, karthus, false )

            config_var( w_enabled, karthus, true )
            config_var( w_hitchance, karthus, 2 )
            config_var( w_max_range, karthus, 80 )

            config_var( r_enabled, karthus, true )
            config_var( r_min_kill, karthus, 1 )
            config_var( r_disable_enemy_within, karthus, 750 )

            config_var( q_draw_range, karthus, true )
            config_var( w_draw_range, karthus, false )
            config_var( e_draw_range, karthus, false )
            config_var( r_draw_kill_indcator, karthus, true )
            config_var( dont_draw_on_cooldown, karthus, true )

            config_var( q_laneclear, karthus, true )
            config_var( q_lasthit, karthus, true )
            config_var( e_laneclear, karthus, false )
            config_var( laneclear_min_mana, karthus, 40 )

            config_var( q_jungleclear, karthus, true )
            config_var( e_jungleclear, karthus, true )
            config_var( e_min_hitcount, karthus, 2 )

        } karthus;

        struct {
            config_var( q_enabled, zed, true )
            config_var( q_harass, zed, true )
            config_var( q_lasthit, zed, true )
            config_var( q_draw_range, zed, true )
            config_var( q_hitchance, zed, 2 )

            config_var( w_draw_set, zed, false )
            config_var( w_enabled, zed, true )
            config_var( w_full_combo_only, zed, false )

            config_var( e_enabled, zed, true )

            config_var( r_draw_range, zed, true )

            config_var( dont_draw_on_cooldown, zed, true )
        } zed;

        struct {
            config_var( q_enabled, kalista, true )
            config_var( q_hitchance, kalista, 2 )

            config_var( e_enabled, kalista, true )
            config_var( e_to_reset_cooldown, kalista, true )
            config_var( e_coup_de_grace, kalista, false )
            config_var( e_laneclear, kalista, true )

            config_var( combo_attack_unit, kalista, true )
            config_var( jump_exploit, kalista, true )

            config_var( q_draw_range, kalista, true )
            config_var( e_draw_range, kalista, false )
            config_var( e_draw_damage, kalista, true )
            config_var( dont_draw_on_cooldown, kalista, true )
        } kalista;

        struct {
            config_var( q_enabled, syndra, true )
            config_var( q_harass, syndra, true )
            config_var( q_hitchance, syndra, 2 )

            config_var( w_enabled, syndra, true )
            config_var( w_hitchance, syndra, 2 )

            config_var( e_enabled, syndra, true )
            config_var( e_autointerrupt, syndra, true )
            config_var( e_antigapclose, syndra, true )
            config_var( e_snipe_enabled, syndra, true )
            config_var( e_hitchance, syndra, 2 )

            config_var( r_enabled, syndra, true )
            config_var( r_predict_q_damage, syndra, true )

            config_var( lasthit_q, syndra, true )
            config_var( laneclear_q, syndra, true )

            config_var( q_draw_range, syndra, true )
            config_var( eq_draw_range, syndra, true )
            config_var( r_draw_range, syndra, true )
            config_var( r_draw_damage, syndra, true )
        } syndra;

        struct {
            config_var( q_enabled, kaisa, true )
            config_var( q_harass, kaisa, true )

            config_var( w_enabled, kaisa, true )
            config_var( w_killsteal, kaisa, true )
            config_var( w_hitchance, kaisa, 2 )
            config_var( w_minimum_stacks, kaisa, 1 )
            config_var( w_semi_manual, kaisa, true )
            config_var( w_semi_manual_hitchance, kaisa, 2 )
            config_var( w_spellweaving, kaisa, true )

            config_var( e_enabled, kaisa, true )
            config_var( flee_e, kaisa, true )

            config_var( flash_w_enabled, kaisa, true )

            config_var( q_draw_range, kaisa, false )
            config_var( w_draw_range, kaisa, true )
            config_var( r_draw_range, kaisa, false )
            config_var( dont_draw_on_cooldown, kaisa, true )
        } kaisa;

        struct {
            config_var( q_enabled, ashe, true )
            config_var( q_laneclear, ashe, true )

            config_var( w_enabled, ashe, true )
            config_var( w_hitchance, ashe, 2 )

            config_var( r_enabled, ashe, true )
            config_var( r_hitchance, ashe, 2 )

            config_var( w_draw_range, ashe, true )
            config_var( dont_draw_on_cooldown, ashe, true )
        } ashe;

        struct {
            config_var( q_enabled, tristana, true )
            config_var( q_laneclear, tristana, true )

            config_var( e_enabled, tristana, true )

            config_var( r_enabled, tristana, true )
            config_var( r_antigapclose, tristana, true )
            config_var( r_autointerrupt, tristana, true )
        } tristana;

        struct {
            config_var( q_enabled, kogmaw, true )
            config_var( q_killsteal, kogmaw, true )
            config_var( q_hitchance, kogmaw, 2 )

            config_var( w_enabled, kogmaw, true )
            config_var( w_extend_range, kogmaw, false )
            config_var( w_disable_spells_in_combo, kogmaw, true )

            config_var( e_enabled, kogmaw, true )
            config_var( e_flee, kogmaw, true )
            config_var( e_hitchance, kogmaw, 2 )
            config_var( e_full_combo_only, kogmaw, true )

            config_var( r_enabled, kogmaw, true )
            config_var( r_killsteal, kogmaw, true )
            config_var( r_hitchance, kogmaw, 2 )
            config_var( r_min_health_percent, kogmaw, 50 )
            config_var( r_max_stacks, kogmaw, 5 )

            config_var( prefer_weaving_spells, kogmaw, false )
            config_var( w_save_mana, kogmaw, true )

            config_var( q_draw_range, kogmaw, false )
            config_var( w_draw_range, kogmaw, true )
            config_var( r_draw_range, kogmaw, true )
            config_var( dont_draw_on_cooldown, kogmaw, true )
            config_var( w_draw_duration, kogmaw, true )
            config_var( r_draw_damage, kogmaw, true )

            //dont_draw_on_cooldown
        } kogmaw;

        struct {
            config_var( q_enabled, tahm_kench, true )
            config_var( q_hitchance, tahm_kench, 2 )
            config_var( q_draw_range, tahm_kench, false )
        } tahm_kench;

        struct {
            config_var( q_enabled, teemo, true )
            config_var( q_harass, teemo, true )
            config_var( q_aa_reset, teemo, true )
            config_var( q_draw_range, teemo, false )
            config_var( q_draw_spell_target, teemo, true )
        } teemo;

        struct {
            config_var( q_enabled, varus, true )
            config_var( q_slow_prediction, varus, true )
            config_var( q_hitchance, varus, 2 )
            config_var( q_ap_draw, varus, 800 )

            config_var( e_enabled, varus, true )
            config_var( e_multihit, varus, true )
            config_var( e_hitchance, varus, 2 )

            config_var( r_enabled, varus, true )
            config_var( r_only_if_can_spread, varus, true )
            config_var( r_antigapclose, varus, true )
            config_var( r_hitchance, varus, 3 )

            config_var( q_draw_range, varus, true )
            config_var( e_draw_range, varus, false )
            config_var( r_draw_range, varus, false )
            config_var( q_ap_draw_range, varus, true )

            config_var( prefer_e_over_q, varus, true )

            config_var( e_spellclear, varus, true )
            config_var( spellclear_min_mana, varus, 40 )
        } varus;

        struct {
            config_var( e_autoslide_enable, zeri, true )

            config_var( w_enabled, zeri, true )
            config_var( w_extend_wall, zeri, true )
            config_var( w_closerange_limit, zeri, true )
            config_var( w_hitchance, zeri, 2 )

            config_var( r_enabled, zeri, true )
            config_var( r_minimum_hitcount, zeri, 2 )

            config_var( q_draw_range, zeri, true )
            config_var( w_draw_range, zeri, false )
            config_var( r_draw_range, zeri, false )
            config_var( dont_draw_on_cooldown, zeri, true )

            config_var( use_zeri_orbwalker, zeri, true )
            config_var( allow_aa_on_fullcharge, zeri, true )
            config_var( allow_aa_on_low_hp_enemy, zeri, true )
            config_var( dont_aa_minion_on_full_charge, zeri, false )

            config_var( comp_q_windup, zeri, true )
            config_var( buffer_q_cast, zeri, true )
        } zeri;


        struct {
            config_var( q_enabled, jax, true )
            config_var( q_only_out_of_range, jax, true )

            config_var( w_enabled, jax, true )
            config_var( w_laneclear, jax, true )

            config_var( e_enabled, jax, true )
            config_var( e_only_on_autoattack, jax, true )
            config_var( e_recast, jax, false )

            config_var( q_draw_range, jax, true )
            config_var( w_draw_range, jax, false )
            config_var( e_draw_range, jax, false )
            config_var( dont_draw_on_cooldown, jax, true )
        } jax;

        struct {
            config_var( q_enabled, kassadin, true )
            config_var( q_autointerrupt, kassadin, true )
            config_var( q_harass, kassadin, true )

            config_var( w_enabled, kassadin, true )
            config_var( w_laneclear, kassadin, false )

            config_var( e_enabled, kassadin, true )
            config_var( e_hitchance, kassadin, 0 )

            config_var( r_enabled, kassadin, true )
            config_var( r_hitchance, kassadin, 2 )
            config_var( r_max_stacks, kassadin, 3 )
            config_var( r_out_of_range, kassadin, true )

            config_var( q_draw_range, kassadin, true )
            config_var( w_draw_range, kassadin, false )
            config_var( e_draw_range, kassadin, false )
            config_var( r_draw_range, kassadin, true )

            config_var( dont_draw_on_cooldown, kassadin, true )
        } kassadin;


        struct {
            config_var( q_enabled, ryze, true )
            config_var( q_hitchance, ryze, 0 )

            config_var( w_enabled, ryze, true )
            config_var( w_only_root, ryze, true )

            config_var( e_enabled, ryze, true )
            config_var( e_only_on_cooldown, ryze, true )

            config_var( q_draw_range, ryze, true )
            config_var( w_draw_range, ryze, false )
            config_var( e_draw_range, ryze, false )
            config_var( dont_draw_on_cooldown, ryze, true )
        } ryze;

        struct {
            config_var( q_enabled, leesin, true )
            config_var( q_killsteal, leesin, true )
            config_var( q_hitchance, leesin, 2 )
            config_var( q_gapclose_minimum, leesin, 1400 )

            config_var( w_enabled, leesin, true )
            config_var( w_semi_manual, leesin, true )
            config_var( w_jungleclear, leesin, true )

            config_var( e_enabled, leesin, true )
            config_var( e_jungleclear, leesin, true )

            config_var( combo_delay_spells_if_passive, leesin, 1 )

            config_var( q_draw_range, leesin, true )
            config_var( e_draw_range, leesin, false )
            config_var( ward_hop_target_draw, leesin, true )
            config_var( dont_draw_on_cooldown, leesin, true )
        } leesin;

        struct {
            config_var( q_enabled, leblanc, true )
            config_var( q_harass, leblanc, true )

            config_var( e_enabled, leblanc, true )
            config_var( e_hitchance, leblanc, 0 )

            config_var( q_draw_range, leblanc, true )
            config_var( w_draw_range, leblanc, false )
            config_var( e_draw_range, leblanc, true )
            config_var( dont_draw_on_cooldown, leblanc, true )
        } leblanc;

        struct {
            config_var( q_enabled, pyke, true )
            config_var( q_melee, pyke, true )
            config_var( q_hitchance, pyke, 2 )

            config_var( e_enabled, pyke, true )
            config_var( e_mode, pyke, 0 )
            config_var( e_safe_position_check, pyke, true )

            config_var( r_enabled, pyke, true )
            config_var( r_interrupt_q, pyke, true )
            config_var( r_hitchance, pyke, 0 )
            config_var( r_new_hitchance, pyke, 0 )

            config_var( q_draw_range, pyke, true )
            config_var( e_draw_range, pyke, false )
            config_var( r_draw_range, pyke, false )
            config_var( dont_draw_on_cooldown, pyke, true )
        } pyke;

        struct {
            config_var( q_enabled, ahri, true )
            config_var( q_hitchance, ahri, 2 )
            config_var( q_harass, ahri, true )

            config_var( w_enabled, ahri, true )

            config_var( e_enabled, ahri, true )
            config_var( e_autointerrupt, ahri, true )
            config_var( e_antigapclose, ahri, true )
            config_var( e_hitchance, ahri, 2 )

            config_var( q_draw_range, ahri, true )
            config_var( w_draw_range, ahri, false )
            config_var( e_draw_range, ahri, true )
            config_var( dont_draw_on_cooldown, ahri, true )
        } ahri;

        struct {
            config_var( q_enabled, veigar, true )
            config_var( q_hitchance, veigar, 1 )
            config_var( q_harass, veigar, true )
            config_var( q_lasthit, veigar, true )

            config_var( w_enabled, veigar, true )
            config_var( w_hitchance, veigar, 2 )

            config_var( e_enabled, veigar, true )
            config_var( e_prefer_stun, veigar, true )
            config_var( e_hitchance, veigar, 2 )

            config_var( r_enabled, veigar, true )

            config_var( q_draw_range, veigar, true )
            config_var( w_draw_range, veigar, false )
            config_var( e_draw_range, veigar, false )
            config_var( r_draw_range, veigar, false )
            config_var( dont_draw_on_cooldown, veigar, true )
            config_var( r_draw_damage, veigar, true )
        } veigar;

        struct {
            config_var( q_enabled, lucian, true )
            config_var( q_hitchance, lucian, 1 )
            config_var( q_harass, lucian, true )
            config_var( q_aa_reset, lucian, true )

            config_var( w_enabled, lucian, true )
            config_var( w_only_use_after_q, lucian, true )
            config_var( w_aa_reset, lucian, true )
            config_var( w_hitchance, lucian, 0 )

            config_var( e_enabled, lucian, true )
            config_var( e_last_in_combo, lucian, false )
            config_var( e_only_reset_aa, lucian, true )

            config_var( disable_spells_on_passive, lucian, true )

            config_var( q_draw_range, lucian, true )
            config_var( w_draw_range, lucian, false )
            config_var( e_draw_range, lucian, true )
            config_var( dont_draw_on_cooldown, lucian, true )
        } lucian;


        struct {
            config_var( q_enabled, jhin, true )
            config_var( q_harass, jhin, true )

            config_var( w_enabled, jhin, true )
            config_var( w_only_on_root, jhin, true )
            config_var( w_killsteal, jhin, true )
            config_var( w_hitchance, jhin, 2 )

            config_var( e_enabled, jhin, true )
            config_var( e_antigapcloser, jhin, true )
            config_var( e_hitchance, jhin, 4 )

            config_var( r_enabled, jhin, true )
            config_var( r_hitchance, jhin, 2 )

            config_var( force_crit, jhin, true )

            config_var( q_draw_range, jhin, true )
            config_var( w_draw_range, jhin, false )
            config_var( e_draw_range, jhin, true )
            config_var( dont_draw_on_cooldown, jhin, true )
        } jhin;

        struct {
            config_var( q_enabled, cassiopeia, true )
            config_var( q_multihit, cassiopeia, true )
            config_var( q_harass, cassiopeia, true )
            config_var( q_hitchance, cassiopeia, 2 )

            config_var( q_spellfarm, cassiopeia, true )
            config_var( spellclear_min_mana, cassiopeia, 40 )

            config_var( w_enabled, cassiopeia, true )
            config_var( w_max_range, cassiopeia, 75 )
            config_var( w_antigapclose, cassiopeia, true )
            config_var( w_hitchance, cassiopeia, 3 )

            config_var( e_enabled, cassiopeia, true )
            config_var( e_spellfarm, cassiopeia, true )
            config_var( e_only_poisoned, cassiopeia, true )
            config_var( e_integrate_lasthitting_in_orbwalker, cassiopeia, true )

            config_var( r_enabled, cassiopeia, true )
            config_var( r_hitchance, cassiopeia, 3 )
            config_var( r_full_combo_single_target, cassiopeia, true )
            config_var( r_minimum_multihit, cassiopeia, 3 )

            config_var( combat_mode_enabled, cassiopeia, true )
            config_var( combat_ignore_q_hitchance, cassiopeia, false )
            config_var( combat_cast_w, cassiopeia, true )
            config_var( combat_allow_r_stun, cassiopeia, true )

            config_var( combat_max_threshold, cassiopeia, 475 )

            config_var( calculate_coup_de_grace, cassiopeia, true )

            config_var( q_draw_range, cassiopeia, true )
            config_var( w_draw_range, cassiopeia, false )
            config_var( e_draw_range, cassiopeia, false )
            config_var( r_draw_range, cassiopeia, false )
            config_var( dont_draw_on_cooldown, cassiopeia, true )
            config_var( combat_draw_threshold, cassiopeia, true )
            config_var( combat_draw_text_mode, cassiopeia, 1 )
            config_var( highlight_poison, cassiopeia, true )
        } cassiopeia;

        struct {
            config_var( q_enabled, lux, true )
            config_var( q_hitchance, lux, 2 )
            config_var( q_harass, lux, false )
            config_var( q_flee, lux, true )
            config_var( q_antigapclose, lux, true )

            config_var( w_enabled, lux, true )
            config_var( w_shield_self, lux, true )
            config_var( w_shield_allies, lux, true )
            config_var( w_autoshield, lux, true )
            config_var( w_shield_threshold, lux, 50 )

            config_var( e_enabled, lux, true )
            config_var( e_multihit, lux, true )
            config_var( e_hitchance, lux, 2 )
            config_var( e_harass, lux, true )
            config_var( e_antigapclose, lux, true )
            config_var( e_killsteal, lux, true )
            config_var( e_flee, lux, true )
            config_var( e_delay_recast, lux, true )

            config_var( r_enabled, lux, true )
            config_var( r_killsteal, lux, true )
            config_var( r_hitchance, lux, 2 )
            config_var( r_recall_ult, lux, true )
            config_var( r_recall_ult_delay, lux, true )

            config_var( q_draw_range, lux, true )
            config_var( w_draw_range, lux, false )
            config_var( e_draw_range, lux, false )
            config_var( r_draw_range, lux, true )
            config_var( dont_draw_on_cooldown, lux, true )
            config_var( r_draw_damage, lux, true )
        } lux;

        struct {
            config_var( q_enabled, mundo, true )
            config_var( q_hitchance, mundo, 2 )
            config_var( q_harass, mundo, true )
            config_var( q_flee, mundo, true )
            config_var( q_lasthit, mundo, true )

            config_var( e_enabled, mundo, true )
            config_var( e_laneclear, mundo, true )

            config_var( q_draw_range, mundo, true )
            config_var( dont_draw_on_cooldown, mundo, true )
        } mundo;

        struct {
            config_var( w_enabled, trynda, true )
            config_var( w_hitchance, trynda, 1 )

            config_var( e_enabled, trynda, true )
            config_var( e_hitchance, trynda, 1 )

            config_var( w_draw_range, trynda, false )
            config_var( e_draw_range, trynda, true )
            config_var( dont_draw_on_cooldown, trynda, true )
        } trynda;

        struct {
            config_var( q_enabled, corki, true )
            config_var( q_multihit, corki, true )
            config_var( q_hitchance, corki, 2 )
            config_var( q_harass, corki, true )
            config_var(q_immobile, corki, true )
            config_var( q_increase_range, corki, true )
            config_var( q_killsteal, corki, true )

            config_var( w_flee, corki, true )
            config_var( w_antigapclose, corki, true )
            config_var( w_antimelee, corki, true )

            config_var( e_enabled, corki, true )
            config_var( e_logic, corki, 1 )

            config_var( r_enabled, corki, true )
            config_var( r_harass, corki, true )
            config_var( r_hitchance, corki, 2 )
            config_var( r_killsteal, corki, true )
            config_var( r_max_range, corki, 95 )

            config_var( q_spellclear, corki, true )

            config_var( q_draw_range, corki, true )
            config_var( w_draw_range, corki, false )
            config_var( e_draw_range, corki, false )
            config_var( r_draw_range, corki, true )
            config_var( dont_draw_on_cooldown, corki, true )
             config_var( r_draw_damage, corki, true )
        } corki;

        struct {
            config_var( q_enabled, zyra, true )
            config_var( q_fertilizer, zyra, true )
            config_var( q_min_fertilizer_count, zyra, 2 )
            config_var( q_hitchance, zyra, 2 )
            config_var( q_harass, zyra, true )
            config_var_raw( q_poke_toggle, zyra, false, utils::EKey::U, keybind_t::EKeybindMode::toggle )

            config_var( e_enabled, zyra, true )
            config_var( e_autointerrupt, zyra, true )
            config_var( e_antigapclose, zyra, true )
            config_var( e_multihit, zyra, true )
            config_var( e_flee, zyra, true )
            config_var( e_hitchance, zyra, 2 )
            config_var( e_max_range, zyra, 95 )

            config_var( w_enabled, zyra, true )
            config_var( w_turret_check, zyra, true )
            config_var( w_save_charge, zyra, true )
            config_var( w_preferred_plant, zyra, 1 )

            config_var( r_enabled, zyra, true )
            config_var( r_only_full_combo, zyra, true )
            config_var( r_multihit, zyra, true )
            config_var( r_hitchance, zyra, 4 )

            config_var( q_draw_range, zyra, false )
            config_var( w_draw_range, zyra, false )
            config_var( e_draw_range, zyra, true )
            config_var( r_draw_range, zyra, false )
            config_var( dont_draw_on_cooldown, zyra, true )
        } zyra;

        struct {
            config_var( q_enabled, morgana, true )
            config_var( q_antigapclose, morgana, true )
            config_var( q_autointerrupt, morgana, true )
            config_var( q_hitchance, morgana, 2 )
            config_var( q_harass, morgana, true )

            config_var( w_enabled, morgana, true )
            config_var( w_hitchance, morgana, 4 )

            config_var( w_laneclear, morgana, true )
            config_var( spellclear_min_mana, morgana, 40 )

            config_var( e_enabled, morgana, true )

            config_var( r_enabled, morgana, true )
            config_var( r_minimum_hitcount, morgana, 2 )

            config_var( q_draw_range, morgana, false )
            config_var( w_draw_range, morgana, true )
            config_var( r_draw_range, morgana, false )
            config_var( dont_draw_on_cooldown, morgana, true )
        } morgana;

        struct {
            config_var( q_enabled, yasuo, true )
            config_var( q_tornado_hitchance, yasuo, 2 )
            config_var( q_harass, yasuo, true )
            config_var( q_stack_on_minion, yasuo, true )
            config_var( q_exploit, yasuo, false )
            config_var( q_lasthit, yasuo, true )
            config_var( q_tornado_lasthit, yasuo, true )

            config_var( e_enabled, yasuo, true )
            config_var( e_lasthit, yasuo, true )
            config_var( e_flee, yasuo, true )
            config_var( e_underturret, yasuo, false )

            config_var( r_enabled, yasuo, true )
            config_var( r_min_targets, yasuo, 3 )
            config_var( r_min_health_percent, yasuo, 70 )

            config_var( q_lasthit_mode, yasuo, 2 )
            config_var( q_laneclear_mode, yasuo, 1 )
            config_var( q_fastclear_mode, yasuo, 2 )
            config_var( q_jungleclear_mode, yasuo, 2 )

            config_var( q_draw_range, yasuo, true )
            config_var( r_draw_range, yasuo, false )
            config_var( draw_exploit_area, yasuo, false )
            config_var( dont_draw_on_cooldown, yasuo, true )
        } yasuo;

        struct {
            config_var( q_enabled, yone, true )
            config_var( q_tornado_hitchance, yone, 2 )
            config_var( q_harass, yone, true )
            config_var( q_tornado_turret_check, yone, true )

            config_var( q_lasthit_mode, yone, 2 )
            config_var( q_laneclear_mode, yone, 1 )
            config_var( q_fastclear_mode, yone, 2 )
            config_var( q_jungleclear_mode, yone, 2 )

            config_var( w_enabled, yone, true )
            config_var( w_harass, yone, true )
            config_var( w_only_in_aa_range, yone, true )
            config_var( w_mode, yone, 1 )
            config_var( w_hitchance, yone, 2 )

            config_var( e_enabled, yone, true )
            config_var( e_min_snap_kill, yone, 2 )

            config_var( r_enabled, yone, true )
            config_var( r_full_combo_check, yone, true )
            config_var( r_multihit, yone, true )
            config_var( r_hitchance, yone, 4 )
            config_var( r_multihit_count, yone, 3 )

            config_var( q_draw_range, yone, true )
            config_var( w_draw_range, yone, false )
            config_var( r_draw_range, yone, false )
            config_var( dont_draw_on_cooldown, yone, true )
        } yone;

        struct {
            config_var( q_enabled, riven, true )
            config_var( q_harass, riven, true )

            config_var( w_enabled, riven, true )
            config_var( w_hitchance, riven, 0 )

            config_var( e_combo_w, riven, true )
        } riven;

        struct {
            config_var( q_enabled, viktor, true )
            config_var( q_harass, viktor, true )
            config_var( q_flee, viktor, true )

            config_var( w_enabled, viktor, true )
            config_var( w_antigapclose, viktor, true )
            config_var( w_autointerrupt, viktor, true )
            config_var( w_ignore_hitchance, viktor, true )
            config_var( w_flee, viktor, true )
            config_var( w_hitchance, viktor, 2 )

            config_var( e_enabled, viktor, true )
            config_var( e_hitchance, viktor, 2 )
            config_var( e_harass, viktor, true )
            config_var( e_multihit, viktor, true )
            config_var( e_flee, viktor, true )
            config_var( e_killsteal, viktor, true )
            config_var( e_max_range, viktor, 95 )

            config_var( r_enabled, viktor, true )
            config_var( r_autointerrupt, viktor, true )
            config_var( r_autofollow, viktor, true )
            config_var( r_hitchance, viktor, 2 )
            config_var( r_ticks_lethal, viktor, 3 )
            config_var( r_killsteal, viktor, true )

            config_var( lasthit_q_mode, viktor, 1 )
            config_var( spellclear_q, viktor, true )
            config_var( spellclear_e, viktor, true )
            config_var( spellclear_e_min_hitcount, viktor, 4 )

            config_var( q_draw_range, viktor, false )
            config_var( w_draw_range, viktor, false )
            config_var( e_draw_range, viktor, true )
            config_var( dont_draw_on_cooldown, viktor, true )
        } viktor;

        struct {
            config_var( q_enabled, senna, true )
            config_var( q_heal_allies, senna, true )
            config_var( q_hitchance, senna, 1 )
            config_var( q_harass, senna, true )
            config_var( q_extend_for_soul_only, senna, false )
            config_var( q_aa_reset, senna, true )

            config_var( w_enabled, senna, true )
            config_var( w_flee, senna, true )
            config_var( w_autointerrupt, senna, true )
            config_var( w_antigapclose, senna, true )
            config_var( w_hitchance, senna, 2 )

            config_var( w_instant_snare, senna, true )
            config_var( w_instant_snare_hitchance, senna, 0 )
            config_var( w_snare_use_q_allowed, senna, true )
            config_var( w_max_snare_distance, senna, 200 )

            config_var( e_flee, senna, true )

            config_var( r_silent_baseult, senna, true )
            config_var( r_prefer_delay, senna, true )

            config_var( r_killsteal, senna, true )
            config_var( r_killsteal_hitchance, senna, 1 )

            config_var( q_draw_range, senna, true )
            config_var( w_draw_range, senna, false )
            config_var( draw_soul_timer, senna, true )
            config_var( highlight_souls, senna, true )
            config_var( dont_draw_on_cooldown, senna, true )
            config_var( souls_per_minute_indicator, senna, true )

            config_var( combo_collect_souls, senna, true )
        } senna;

        struct {
            config_var( q_enabled, vayne, true )
            config_var( q_harass, vayne, true )
            config_var( q_direction_mode, vayne, 0 )
            config_var( q_check_mouse_range, vayne, true )
            config_var( q_check_turret_range, vayne, true )
            config_var( q_max_enemy_count, vayne, 2 )
            config_var( q_require_aa_reset, vayne, true )

            // q conditions
            config_var( q_position_for_e, vayne, true )
            config_var( q_proc_w, vayne, true )
            config_var( q_engage, vayne, false )

            config_var( e_enabled, vayne, true )
            config_var( e_antigapclose, vayne, true )
            config_var( e_autointerrupt, vayne, true )
            config_var( e_killsteal, vayne, true )
            config_var( e_mode, vayne, 0 )

            config_var( q_draw_range, vayne, true )
            config_var( e_draw_range, vayne, false )
            config_var( dont_draw_on_cooldown, vayne, true )
            config_var( e_draw_push, vayne, true )
        } vayne;

        struct {
            config_var( q_enabled, kindred, true )
            config_var( q_hitchance, kindred, 1 )
            config_var( q_harass, kindred, true )
            config_var( q_laneclear, kindred, true )
            config_var( q_aa_reset, kindred, true )
            config_var( q_machinegun, kindred, true )
            config_var( q_wallhop, kindred, true )

            config_var( w_enabled, kindred, true )

            config_var( e_enabled, kindred, true )
            config_var( e_aa_reset, kindred, true )
            config_var( e_jungleclear, kindred, true )
            config_var( e_execute_champ_threshold, kindred, true )
            config_var( e_execute_camp_threshold, kindred, true )

            config_var( r_enabled, kindred, true )
            config_var( r_only_full_combo, kindred, true )
            config_var( r_health_threshold, kindred, 10 )

            config_var( q_draw_range, kindred, true )
            config_var( dont_draw_on_cooldown, kindred, true )
            config_var( draw_stack_tracker, kindred, 1 )
            config_var( stack_tracker_show_possible_camps, kindred, true )
        } kindred;

        struct {
            config_var( show_menu_on_start, misc, true )
            config_var( extra_eye_candy, misc, true )
            config_var( chat_taunt_on_kill, misc, 0 )
            config_var( chat_encourage_allies, misc, false )
            config_var( chat_spell_cooldowns, misc, 1 )
            config_var( chat_auto_gank_warn, misc, 0 )

            config_var( ping_wards, misc, true )
            config_var( ping_objectives, misc, true )
            config_var( ping_missing, misc, true )
            config_var( ping_spam_ally, misc, true )
            config_var( ping_spam_mode, misc, 1 )

            config_var( draw_pingable_indicator, misc, true )
            config_var( pingable_indicator_scale, misc, 100 )
            config_var( pingable_indicator_x, misc, 375 )
            config_var( pingable_indicator_y, misc, 600 )

            config_var( pred_pattern_tracking, misc, false )

            config_var( audio_volume, misc, 1.f )

            config_var( show_vision_score, misc, true )
            config_var( antiafk_toggle, misc, true )
            config_var( show_spells_hitbox, misc, false )
            config_var( kill_effect_mode, misc, 3 )

            config_var( core_update_delay, misc, 3 )
            config_var( core_update_threads_, misc, 4 )
            config_var( core_fast_update_threads, misc, false )
            config_var( core_league_topmost_check, misc, true )
            config_var( core_entity_list_high_performance_mode, misc, false )

            config_var( exploit_enabled, misc, true )
            config_var( exploit_slot, misc, 2 )
            config_var( exploit_type, misc, 0 )


            config_var( toggle_menu_with_shift, misc, false )

            config_var(limit_overlay_fps, misc,false)

            config_var( test_int, misc, 0 )

            config_var(enable_vsync__, misc, true)

            config_var(high_performace_mode, misc, false)

            // config_var(use_multi_core_runtime, misc, false)

            config_var(download_lua_sdk, misc, true)
            config_var(lua_show_memory_uage, misc, false)

            config_var(screen_scaling, misc, 2.f)
            config_var(screen_scaling_select_, misc, 0)
        } misc;

        struct {
            config_var( mode, prediction, 0 )
            config_var( slower_prediction, prediction, true )
            config_var( no_recent_path, prediction, true )
            config_var( compensate_full_hitbox, prediction, true )

            config_var( speed_calculation_mode, prediction, 0 )
            config_var( adjust_for_dodges, prediction, true )
            config_var( hitbox_compensation_mode, prediction, 0 )
            config_var( wall_prediction, prediction, true )
            config_var( hitchance_rating_logic, prediction, 1 )
            config_var( ping_delay, prediction, 1 )
            config_var( travel_reduction, prediction, 2 )
            config_var( include_cast_delay, prediction, true )
            config_var( ping_compensation, prediction, 1 )

            config_var( ward_distance, prediction, 75 )
            config_var( ward_angle, prediction, 150 )
            config_var( ward_radius, prediction, 50 )
        } prediction;

        struct {
            config_var( q_enabled, belveth, true )
            config_var( q_harass, belveth, true )
            config_var( q_mode, belveth, 1 )

            config_var( w_enabled, belveth, true )
            config_var( w_autointerrupt, belveth, true )
            config_var( w_only_out_of_range, belveth, true )
            config_var( w_hitchance, belveth, 2 )

            config_var( e_enabled, belveth, true )
            config_var( e_disable_under_turret, belveth, true )
            config_var( e_min_health_percent, belveth, 50 )

            config_var( r_enabled, belveth, true )
            config_var( r_disable_if_buffed, belveth, true )

            config_var( q_laneclear, belveth, true )
            config_var( w_laneclear, belveth, true )

            config_var( q_draw_range, belveth, true )
            config_var( w_draw_range, belveth, false )
            config_var( e_draw_range, belveth, false )

            config_var( dont_draw_on_cooldown, belveth, true )
        } belveth;

        struct {
            config_var( q_enabled, sylas, true )
            config_var( q_prefer_q2, sylas, true )
            config_var( q_hitchance, sylas, 2 )

            config_var( w_enabled, sylas, true )
            config_var( w_min_health_percent, sylas, 60 )

            config_var( e_enabled, sylas, true )
            config_var( e_use_dash, sylas, true )
            config_var( e_hitchance, sylas, 2 )

            config_var( q_draw_range, sylas, true )
            config_var( w_draw_range, sylas, false )
            config_var( e_draw_range, sylas, false )
            config_var( r_draw_range, sylas, false )
            config_var( dont_draw_on_cooldown, sylas, true )
        } sylas;

        struct {
            config_var( q_enabled, katarina, true )
            config_var( q_harass, katarina, true )
            config_var( q_lasthit, katarina, true )

            config_var( w_enabled, katarina, true )
            config_var( w_after_shunpo, katarina, true )

            config_var( e_enabled, katarina, true )
            config_var( e_killsteal, katarina, true )
            config_var( e_dagger, katarina, true )
            config_var( e_moving_dagger, katarina, true )

            config_var( r_enabled, katarina, true )
            config_var( r_execute, katarina, true )
            config_var( r_overpredict, katarina, true )
            config_var( r_min_targets, katarina, 2 )

            config_var( q_draw_range, katarina, true )
            config_var( e_draw_range, katarina, false )
            config_var( r_draw_range, katarina, true )
            config_var( draw_dagger_range, katarina, true )

            config_var( dont_draw_on_cooldown, katarina, true )
        } katarina;

        struct {
            config_var( q_enabled, chogath, true )
            config_var( q_harass, chogath, true )
            config_var( q_hitchance, chogath, 2 )

            config_var( w_enabled, chogath, true )
            config_var( w_autointerrupt, chogath, true )
            config_var( w_hitchance, chogath, 2 )

            config_var( e_enabled, chogath, true )

            config_var( r_enabled, chogath, true )
            config_var( r_objs, chogath, true )

            config_var( q_draw_range, chogath, true )
            config_var( w_draw_range, chogath, false )
            config_var( dont_draw_on_cooldown, chogath, true )
        } chogath;

        struct {
            config_var( q_enabled, olaf, true )
            config_var( q_harass, olaf, true )
            config_var( q_hitchance, olaf, 2 )
            config_var( q_jungleclear, olaf, true )
            config_var( q_laneclear, olaf, true )

            config_var( w_enabled, olaf, true )
            config_var( w_laneclear, olaf, false )

            config_var( e_enabled, olaf, true )
            config_var( e_weave_in_aa, olaf, true )
            config_var( e_laneclear, olaf, true )

            config_var( q_draw_range, olaf, true )
            config_var( e_draw_range, olaf, false )
            config_var( dont_draw_on_cooldown, olaf, true )
        } olaf;

        struct {
            config_var( q_enabled, ziggs, true )
            config_var( q_harass, ziggs, true )
            config_var( q_minion_impact, ziggs, true )
            config_var( q_hitchance, ziggs, 2 )

            config_var( q_direct, ziggs, true )
            config_var( q_bounce, ziggs, true )
            config_var( q_double_bounce, ziggs, true )

            config_var( w_enabled, ziggs, true )
            config_var( w_antigapclose, ziggs, true )
            config_var( w_autointerrupt, ziggs, true )
            config_var( w_pull_enemies, ziggs, true )
            config_var( w_hitchance, ziggs, 2 )

            config_var( e_enabled, ziggs, true )
            config_var( e_antigapclose, ziggs, true )
            config_var( e_hitchance, ziggs, 2 )

            config_var( r_enabled, ziggs, true )
            config_var_raw( r_semi_manual, ziggs, false, utils::EKey::T, keybind_t::EKeybindMode::hold )
            config_var( r_semi_manual_hitchance, ziggs, 2 )

            config_var( r_multihit, ziggs, true )
            config_var( r_min_multihit_count, ziggs, 3 )

            config_var( combat_mode_enabled, ziggs, true )
            config_var_raw(
                combat_mode_toggle,
                ziggs,
                false,
                utils::EKey::xbutton1,
                keybind_t::EKeybindMode::toggle
            )
            config_var( combat_ignore_q_hitchance, ziggs, false )
            config_var( combat_direct_q_only, ziggs, true )
            config_var( combat_w_allowed, ziggs, true )
            config_var( combat_r_disabled, ziggs, true )
            config_var( combat_max_threshold, ziggs, 800 )


            config_var( q_draw_range, ziggs, true )
            config_var( q_draw_bounce_range, ziggs, false )
            config_var( q_draw_double_bounce_range, ziggs, false )
            config_var( w_draw_range, ziggs, false )
            config_var( e_draw_range, ziggs, false )
            config_var( r_draw_range_minimap, ziggs, true )
            config_var( dont_draw_on_cooldown, ziggs, true )
            config_var( r_show_indicator, ziggs, true )
            config_var( combat_draw_threshold, ziggs, true )
        } ziggs;

        struct {
            config_var( q_enabled, samira, true )
            config_var( q_harass, samira, true )
            config_var( q_farm, samira, true )
            config_var( q_hitchance, samira, 2 )

            config_var( w_enabled, samira, true )
            config_var( w_aa_reset, samira, false )

            config_var( e_enabled, samira, true )
            config_var( e_weave_aa, samira, true )
            config_var( e_mode, samira, 1 )
            config_var( e_force_dash_range, samira, 200 )

            config_var( r_enabled, samira, true )
            config_var( r_min_targets, samira, 2 )

            config_var( passive_orbwalk, samira, false )

            config_var( q_draw_range, samira, true )
            config_var( q_melee_draw_range, samira, false )
            config_var( w_draw_range, samira, false )
            config_var( e_draw_range, samira, true )
            config_var( r_draw_range, samira, false )
            config_var( dont_draw_on_cooldown, samira, true )

            config_var( e_draw_dash_area, samira, true )
        } samira;

        struct {
            config_var( q_enabled, seraphine, true )
            config_var( q_harass, seraphine, true )
            config_var( q_hitchance, seraphine, 2 )

            config_var( w_enabled, seraphine, true )

            config_var( e_enabled, seraphine, true )
            config_var( e_hitchance, seraphine, 2 )

            config_var( r_enabled, seraphine, true )
            config_var( r_hitchance, seraphine, 2 )
            config_var( r_mode, seraphine, 1 )
            config_var( r_multihit_count, seraphine, 3 )

            config_var( q_draw_range, seraphine, true )
            config_var( w_draw_range, seraphine, false )
            config_var( e_draw_range, seraphine, false )
            config_var( r_draw_range, seraphine, false )
            config_var( dont_draw_on_cooldown, seraphine, true )
        } seraphine;

        struct {
            config_var( q_enabled, sett, true )
            config_var( q_laneclear, sett, true )

            config_var( w_enabled, sett, true )
            config_var( w_hitchance, sett, 2 )
            config_var( w_min_grit_percent, sett, 30 )

            config_var( e_enabled, sett, true )
            config_var( e_disable_aa_range, sett, true )
            config_var( e_only_stun, sett, true )
            config_var( e_hitchance, sett, 2 )

            config_var( q_draw_range, sett, true )
            config_var( w_draw_range, sett, false )
            config_var( e_draw_mode, sett, 2 )
            config_var( dont_draw_on_cooldown, sett, true )
        } sett;

        struct {
            config_var( q_enabled, gnar, true )
            config_var( q_harass, gnar, true )
            config_var( q_hitchance, gnar, 2 )

            config_var( w_enabled, gnar, true )
            config_var( w_autointerrupt, gnar, true )
            config_var( w_hitchance, gnar, 2 )

            config_var( e_enabled, gnar, true )
            config_var( e_disable_aa_range, gnar, true )
            config_var( e_hitchance, gnar, 2 )

            config_var( q_draw_range, gnar, true )
            config_var( e_draw_range, gnar, false )
            config_var( dont_draw_on_cooldown, gnar, true )
        } gnar;

        struct {
            config_var( q_enabled, sion, true )
            config_var( q_hitchance, sion, 2 )

            config_var( w_enabled, sion, true )

            config_var( e_enabled, sion, true )
            config_var( e_hitchance, sion, 2 )

            config_var( r_enabled, sion, true )

            config_var( q_draw_range, sion, true )
            config_var( e_draw_range, sion, false )
            config_var( dont_draw_on_cooldown, sion, true )
        } sion;

        struct {
            config_var( q_enabled, nilah, true )
            config_var( q_harass, nilah, true )
            config_var( q_hitchance, nilah, 2 )
            config_var( q_spellclear, nilah, true )

            config_var( w_enabled, nilah, true )
            config_var( w_min_health_percent, nilah, 80 )

            config_var( e_enabled, nilah, true )
            config_var( e_exploit, nilah, true )
            config_var( e_on_killable, nilah, true )
            config_var( e_min_dash_range, nilah, 350 )
            config_var( e_force_dash_range, nilah, 200 )

            config_var( r_enabled, nilah, true )
            config_var( r_min_targets, nilah, 2 )

            config_var( q_draw_range, nilah, true )
            config_var( e_draw_range, nilah, false )
            config_var( r_draw_range, nilah, false )
            config_var( dont_draw_on_cooldown, nilah, true )

            config_var( e_draw_dash_area, nilah, true )
        } nilah;

        struct {
            config_var( q_enabled, sivir, true )
            config_var( q_harass, sivir, true )
            config_var( q_hitchance, sivir, 2 )
            config_var( q_max_minions, sivir, 2 )

            config_var( w_enabled, sivir, true )
            config_var( w_aa_reset, sivir, true )

            config_var( e_enabled, sivir, true )

            config_var( q_draw_range, sivir, true )
            config_var( dont_draw_on_cooldown, sivir, true )
        } sivir;

        struct {
            config_var( q_enabled, illaoi, true )
            config_var( q_harass, illaoi, true )
            config_var( q_hitchance, illaoi, 2 )
            config_var( q_max_range, illaoi, 95 )

            config_var( w_enabled, illaoi, true )
            config_var( w_aa_reset, illaoi, true )

            config_var( e_enabled, illaoi, true )
            config_var( e_hitchance, illaoi, 2 )
            config_var( e_max_range, illaoi, 90 )

            config_var( r_enabled, illaoi, true )
            config_var( r_min_targets, illaoi, 2 )

            config_var( q_draw_range, illaoi, true )
            config_var( e_draw_range, illaoi, false )
            config_var( dont_draw_on_cooldown, illaoi, true )
        } illaoi;

        struct {
            config_var( q_enabled, vladimir, true )
            config_var( q_harass, vladimir, true )
            config_var( q_lasthit, vladimir, true )

            config_var( w_enabled, vladimir, true )

            config_var( e_enabled, vladimir, true )
            config_var( e_hitchance, vladimir, 2 )

            config_var( r_enabled, vladimir, true )
            config_var( r_min_targets, vladimir, 2 )

            config_var( q_draw_range, vladimir, true )
            config_var( e_draw_range, vladimir, false )
            config_var( dont_draw_on_cooldown, vladimir, true )
        } vladimir;

        struct {
            config_var( q_enabled, irelia, true )
            config_var( q_gapclose, irelia, true )
            config_var( q_prekill, irelia, true )
            config_var( q_spellfarm, irelia, true )
            config_var( q_predict_health, irelia, true )

            config_var( w_enabled, irelia, true )

            config_var( e_enabled, irelia, true )
            config_var( e_hide, irelia, true )
            config_var( e_only_recast, irelia, false )
            config_var( e_hitchance, irelia, 2 )

            config_var( r_enabled, irelia, true )
            config_var( r_only_full_combo, irelia, false )
            config_var( r_min_health_percent, irelia, 60 )

            config_var( q_draw_range, irelia, true )
            config_var( e_draw_range, irelia, false )
            config_var( r_draw_range, irelia, false )
            config_var( dont_draw_on_cooldown, irelia, true )
        } irelia;

        struct {
            config_var( q_enabled, zoe, true )
            config_var( q_harass, zoe, true )
            config_var_raw( q_fast_combo, zoe, false, utils::EKey::shift, keybind_t::EKeybindMode::toggle )
            config_var( q_delay_cast, zoe, true )
            config_var( q_override_hitchance, zoe, true )
            config_var( q_only_redirect, zoe, false )
            config_var( q_hitchance, zoe, 2 )

            config_var( q_magnet, zoe, true )
            config_var( q_magnet_distance, zoe, 300 )


            config_var( w_enabled, zoe, true )

            config_var( e_enabled, zoe, true )
            config_var( e_flee, zoe, true )
            config_var( e_flee_only_direct, zoe, true )
            config_var( e_antigapcloser, zoe, true )
            config_var( e_autointerrupt, zoe, true )
            config_var( e_through_walls, zoe, true )
            config_var_raw( e_hide_animation, zoe, true, utils::EKey::f1, keybind_t::EKeybindMode::toggle )
            config_var( e_hitchance, zoe, 2 )

            config_var( r_enabled, zoe, true )
            config_var( r_underturret, zoe, 1 )
            config_var( r_delay_cast, zoe, true )

            config_var( q_draw_range, zoe, true )
            config_var( q_draw_extend_range, zoe, true )
            config_var( e_draw_range, zoe, false )
            config_var( dont_draw_on_cooldown, zoe, true )

            config_var( e_priority_combo, zoe, true )
            config_var( aa_passive_check, zoe, true )
        } zoe;

        struct {
            config_var( q_enabled, kennen, true )
            config_var( q_hitchance, kennen, 2 )
            config_var( q_harass, kennen, true )

            config_var( q_draw_range, kennen, false )
            config_var( dont_draw_on_cooldown, kennen, true )
        } kennen;

        struct {
            config_var( q_enabled, draven, true )
            config_var( q_harass, draven, true )
            config_var( q_keep_buff, draven, true )
            config_var( q_farm, draven, true )

            config_var( w_enabled, draven, true )
            config_var( w_on_slow, draven, true )
            config_var( w_attackspeed, draven, true )
            config_var( w_flee, draven, true )
            config_var( w_chase, draven, false )

            config_var( e_antigapcloser, draven, true )
            config_var( e_anti_melee, draven, true )

            config_var( r_killsteal, draven, true )
            config_var( r_hitchance, draven, 2 )
            config_var( r_recall_ult, draven, true )
            config_var( r_recall_ult_delay, draven, true )

            config_var( q_catch_axes, draven, true )
            config_var( q_slow_order, draven, false )
            config_var( q_prefer_dps, draven, true )
            config_var( catch_axe_range, draven, 500 )
            config_var( catch_mode, draven, 1 )

            config_var( draw_catch_circle, draven, true )
            config_var( dont_draw_on_cooldown, draven, true )
        } draven;

        struct {
            config_var( q_enabled, akali, true )
            config_var( q_harass, akali, true )
            config_var( q_hitchance, akali, 0 )

            config_var( w_enabled, akali, true )
            config_var( w_restore_energy, akali, true )
            config_var( w_multiple_enemy, akali, false )
            config_var( w_cancel_casts, akali, true )
            config_var( w_antigapcloser, akali, true )

            config_var( e_enabled, akali, true )
            config_var( e_hitchance, akali, 2 )
            config_var( e_on_killable, akali, true )
            config_var( e_in_full_combo, akali, true )
            config_var( e_hold_in_full_combo, akali, true )

            config_var( r_enabled, akali, true )
            config_var( r_in_full_combo, akali, true )
            config_var( r_animation_cancel, akali, true )
            config_var( r_killsteal, akali, true )
            config_var( r_hitchance, akali, 2 )

            config_var( magnet_orbwalk, akali, true )
            config_var( magnet_turret_check, akali, true )
            config_var( magnet_distance, akali, 275 )
            config_var( magnet_block_cast, akali, true )

            config_var( passive_block_cast, akali, true )

            config_var( q_draw_range, akali, true )
            config_var( e_draw_range, akali, false )
            config_var( r_draw_range, akali, false )
            config_var( dont_draw_on_cooldown, akali, true )
        } akali;

        struct {
            config_var( q_enabled, missfortune, true )
            config_var( q_hitchance, missfortune, 2 )
            config_var( q_harass, missfortune, true )
            config_var( q_ignore_moving_minion, missfortune, true )
            config_var( q_max_range, missfortune, 80 )

            config_var( w_enabled, missfortune, true )
            config_var( w_harass, missfortune, true )

            config_var( e_enabled, missfortune, true )
            config_var( e_harass, missfortune, true )
            config_var( e_antigapcloser, missfortune, true )
            config_var( e_only_out_of_range, missfortune, true )
            config_var( e_hitchance, missfortune, 2 )

            config_var( q_draw_range, missfortune, false )
            config_var( e_draw_range, missfortune, true )
            config_var( r_draw_range, missfortune, false )
            config_var( dont_draw_on_cooldown, missfortune, true )
        } missfortune;

        struct {
            config_var( debug_console, lua, false )
            config_var(reload_on_unknown_exception, lua, true)
        } lua;

        struct {
            config_var( q_enabled, caitlyn, true )
            config_var( q_harass, caitlyn, true )
            config_var( q_only_out_of_range, caitlyn, true )
            config_var( q_hitchance, caitlyn, 2 )

            config_var( w_enabled, caitlyn, true )
            config_var( w_antimelee, caitlyn, true )
            config_var( w_autointerrupt, caitlyn, true )
            config_var( w_antigapclose, caitlyn, true )
            config_var( w_animation_cancel, caitlyn, true )
            config_var( w_save_charge_for_cc, caitlyn, true )
            config_var( w_hitchance, caitlyn, 3 )

            config_var( e_enabled, caitlyn, true )
            config_var( e_antigapcloser, caitlyn, true )
            config_var( e_max_range, caitlyn, 75 )
            config_var( e_hitchance, caitlyn, 2 )

            config_var( r_enabled, caitlyn, true )

            config_var( combo_logic, caitlyn, 2 )
            config_var( w_fast_combo, caitlyn, true )
            config_var( galeforce_max_angle, caitlyn, 50 )

            config_var( q_draw_range, caitlyn, true )
            config_var( w_draw_range, caitlyn, false )
            config_var( e_draw_range, caitlyn, false )
            config_var( r_draw_range, caitlyn, false )
            config_var( dont_draw_on_cooldown, caitlyn, true )
        } caitlyn;

        struct {
            config_var( q_enabled, aatrox, true )
            config_var( q_harass, aatrox, true )
            config_var( q_accurate_hitbox, aatrox, true )
            config_var( q_hitchance, aatrox, 2 )

            config_var( w_enabled, aatrox, true )
            config_var( w_hitchance, aatrox, 2 )

            config_var( e_enabled, aatrox, true )

            config_var( q_draw_range, aatrox, true )
            config_var( w_draw_range, aatrox, false )
            config_var( e_draw_range, aatrox, false )
            config_var( dont_draw_on_cooldown, aatrox, true )
        } aatrox;

        struct {
            config_var( q_enabled, vex, true )
            config_var( q_harass, vex, true )
            config_var( q_hitchance, vex, 2 )

            config_var( w_enabled, vex, true )

            config_var( e_enabled, vex, true )
            config_var( e_hitchance, vex, 2 )

            config_var( r_recall_ult, vex, true )
            config_var( r_recall_ult_delay, vex, true )

            config_var( combo_override_hitchance, vex, true )

            config_var( q_draw_range, vex, true )
            config_var( w_draw_range, vex, true )
            config_var( e_draw_range, vex, false )
            config_var( r_draw_range, vex, false )
            config_var( dont_draw_on_cooldown, vex, true )
        } vex;

        struct {
            config_var( q_enabled, yorick, true )
            config_var( q_lasthit, yorick, true )
            config_var( q_AA_reset, yorick, true )

            config_var( w_enabled, yorick, true )
            config_var( w_hitchance, yorick, 2 )

            config_var( e_enabled, yorick, true )
            config_var( e_hitchance, yorick, 2 )

            config_var( r_enabled, yorick, true )

            config_var( w_draw_range, yorick, true )
            config_var( e_draw_range, yorick, false )
            config_var( dont_draw_on_cooldown, yorick, true )
        } yorick;

        struct {
            config_var( q_enabled, darius, true )
            config_var( q_harass, darius, true )
            config_var( q_magnet, darius, true )
            config_var( q_magnet_turret_check, darius, true )
            config_var( q_aa_range_limit, darius, true )

            config_var( w_enabled, darius, true )
            config_var( w_aa_reset, darius, true )

            config_var( e_enabled, darius, true )
            config_var( e_aa_range_limit, darius, true )
            config_var( e_hitchance, darius, 2 )

            config_var( r_enabled, darius, true )

            config_var( prefer_e_over_q, darius, true )

            config_var( q_draw_range, darius, true )
            config_var( e_draw_range, darius, false )
            config_var( dont_draw_on_cooldown, darius, true )
        } darius;

        struct {
            config_var( q_enabled, taliyah, true )
            config_var( q_harass, taliyah, true )
            config_var( q_magnet, taliyah, true )
            config_var( q_hitchance, taliyah, 2 )

            config_var( w_enabled, taliyah, true )
            config_var( w_flee, taliyah, true )
            config_var( w_autointerrupt, taliyah, true )
            config_var( w_hitchance, taliyah, 2 )

            config_var( e_enabled, taliyah, true )
            config_var( e_antigapcloser, taliyah, true )

            config_var( q_draw_range, taliyah, false )
            config_var( w_draw_range, taliyah, false )
            config_var( e_draw_range, taliyah, false )
            config_var( dont_draw_on_cooldown, taliyah, true )
        } taliyah;

        struct {
            config_var( q_enabled, swain, true )
            config_var( q_harass, swain, true )
            config_var( q_hitchance, swain, 0 )

            config_var( w_enabled, swain, true )
            config_var( w_allow_lower_hitchance, swain, true )
            config_var( w_hitchance, swain, 4 )

            config_var( e_enabled, swain, true )
            config_var( e_harass, swain, true )
            config_var( e_flee, swain, true )
            config_var( e_autointerrupt, swain, true )
            config_var( e_hitchance, swain, 2 )

            config_var( r_enabled, swain, true )
            config_var( r_execute, swain, true )
            config_var( r_full_combo, swain, false )
            config_var( r_min_enemy_count, swain, 2 )

            config_var( q_draw_range, swain, false )
            config_var( w_draw_range, swain, true )
            config_var( e_draw_range, swain, false )
            config_var( dont_draw_on_cooldown, swain, true )
        } swain;

        struct {
            config_var( q_enabled, orianna, true )
            config_var( q_harass, orianna, true )
            config_var( q_killsteal, orianna, true )
            config_var( q_increase_range, orianna, true )
            config_var( q_position_multihit_r, orianna, true )
            config_var( q_hitchance, orianna, 2 )
            config_var( q_flee, orianna, true )
            config_var( q_max_ball_travel_distance, orianna, 1400 )

            config_var( w_enabled, orianna, true )
            config_var( w_hitchance, orianna, 2 )
            config_var( w_autoharass, orianna, true )

            config_var( e_enabled, orianna, true )
            config_var( e_semi_automatic, orianna, true )
            config_var( e_autoshield, orianna, true )
            config_var( e_shield_self, orianna, true )
            config_var( e_shield_allies, orianna, true )
            config_var( e_damage, orianna, true )
            config_var( e_position_ult, orianna, true )
            config_var( e_hitchance, orianna, 2 )
            config_var( e_shield_threshold, orianna, 75 )

            config_var( ally_priority_enabled, orianna, true )
            config_var( ally_shield_on_damage, orianna, true )
            config_var( ally_shield_on_chase, orianna, true )
            config_var( ally_shield_on_flee, orianna, false )
            config_var( ally_shield_in_combat, orianna, false )

            config_var( ally_buff_on_chase, orianna, true )
            config_var( ally_buff_on_flee, orianna, true )
            config_var( ally_buff_in_combat, orianna, false )

            config_var( r_enabled, orianna, true )
            config_var( r_autointerrupt, orianna, true )
            config_var( r_single_target_full_combo, orianna, true )
            config_var( r_hitchance, orianna, 4 )

            config_var( r_multihit, orianna, true )
            config_var( r_min_multihit, orianna, 3 )

            config_var( q_draw_range, orianna, true )
            config_var( w_draw_hitbox, orianna, true )
            config_var( e_draw_range, orianna, true )
            config_var( r_draw_hitbox, orianna, false )
            config_var( dont_draw_on_cooldown, orianna, true )
            config_var( draw_ball_effect, orianna, true )
        } orianna;

        struct {
            config_var( q_enabled, viego, true )
            config_var( q_hitchance, viego, 1 )
            config_var( q_draw_range, viego, true )

            config_var( w_enabled, viego, true )
            config_var( w_hitchance, viego, 1 )
            config_var( w_draw_range, viego, true )

            config_var( r_enabled, viego, true )
            config_var( r_hitchance, viego, 3 )
            config_var( r_draw_range, viego, true )

            config_var( dont_draw_on_cooldown, viego, true )
        } viego;

        struct {
            config_var( q_enabled, lulu, true )
            config_var( q_multihit, lulu, true )
            config_var( q_flee, lulu, true )
            config_var( q_hitchance, lulu, 2 )
            config_var( q_harass, lulu, true )
            config_var( q_killsteal, lulu, true )
            config_var( q_antigapclose, lulu, true )
            config_var( q_max_range, lulu, 95 )

            config_var( w_enabled, lulu, true )
            config_var( w_enemies, lulu, true )
            config_var( w_autointerrupt, lulu, true )
            config_var( w_flee, lulu, true )

            config_var( e_enabled, lulu, true )
            config_var( e_killsteal, lulu, true )
            config_var( e_block_damage_from_spells, lulu, true )
            config_var( e_block_damage_from_autoattacks, lulu, true )
            config_var( e_block_damage_from_turret_shots, lulu, true )
            config_var( e_block_damage_from_ignite, lulu, true )
            config_var( e_block_damage_from_poison, lulu, true )
            config_var( e_block_damage_from_item_burn, lulu, true )
            config_var( e_skillshot_minimum_danger, lulu, 2 )
            config_var( e_increase_damage, lulu, true )
            config_var( e_self, lulu, true )

            config_var( semi_manual_we, lulu, true )
            config_var( semi_manual_e, lulu, true )
            config_var( semi_manual_w, lulu, true )

            config_var( ally_priority_enabled, lulu, true )
            config_var( ally_buff_on_chase, lulu, false )
            config_var( ally_buff_on_flee, lulu, false )
            config_var( ally_buff_in_combat, lulu, false )
            config_var( only_buff_priority_ally, lulu, true )

            config_var( r_enabled, lulu, true )
            config_var( r_logic, lulu, 1 )
            config_var( r_multihit_count, lulu, 2 )
            config_var( r_antigapclose, lulu, true )
            config_var( r_antimelee, lulu, true )
            config_var( r_autointerrupt, lulu, true )
            config_var( r_health_threshold, lulu, 40 )

            config_var( q_draw_range, lulu, true )
            config_var( e_draw_range, lulu, true )
            config_var( dont_draw_on_cooldown, lulu, true )
        } lulu;

        struct {
            config_var( q_enabled, soraka, true )
            config_var( q_hitchance, soraka, 2 )
            config_var( q_harass, soraka, true )
            config_var( q_antigapclose, soraka, true )

            config_var( w_enabled, soraka, true )
            config_var( w_min_hp, soraka, 10 )
            config_var( w_min_hp_ally, soraka, 30 )

            config_var( e_enabled, soraka, true )
            config_var( e_hitchance, soraka, 1 )
            config_var( e_harass, soraka, true )
            config_var( e_autointerrupt, soraka, true )
            config_var( e_antigapclose, soraka, true )

            config_var( r_enabled, soraka, true )
            config_var( r_min_hp, soraka, 30 )
            config_var( r_min_ally_danger, soraka, 2 )
            config_var( r_danger_count_override, soraka, true )

            config_var( q_draw_range, soraka, true )
            config_var( e_draw_range, soraka, true )
            config_var( r_draw_counter, soraka, true )
            config_var( dont_draw_on_cooldown, soraka, true )
        } soraka;

        struct {
            config_var( q_enabled, diana, true )
            config_var( q_hitchance, diana, 2 )
            config_var( q_harass, diana, true )
            config_var( q_laneclear, diana, true )
            config_var( q_laneclear_min_mana, diana, 20 )
            config_var( q_jungleclear, diana, true )

            config_var( w_enabled, diana, true )
            config_var( w_incoming_damage, diana, true )
            config_var( w_jungleclear, diana, true )

            config_var( e_enabled, diana, true )
            config_var( e_tower_range_check, diana, true )
            config_var( e_only_to_passive, diana, true )
            config_var( e_on_killable, diana, true )
            config_var( e_force_dash_range, diana, 400 )
            config_var( e_jungleclear, diana, true )

            config_var( r_enabled, diana, true )
            config_var( r_if_killable, diana, true )
            config_var( r_min_enemy_count, diana, 2 )

            config_var( q_draw_range, diana, true )
            config_var( e_draw_range, diana, false )
            config_var( r_draw_range, diana, false )
            config_var( e_draw_dash_area, diana, true )
            config_var( dont_draw_on_cooldown, diana, true )
        } diana;

        struct {
            config_var( q_enabled, velkoz, true )
            config_var( q_harass, velkoz, true )
            config_var( q_flee, velkoz, true )
            config_var( q_hitchance, velkoz, 2 )
            config_var( q_mode, velkoz, 0 )
            config_var( q_max_indirect_duration, velkoz, 0 )
            config_var( q_split_max_range, velkoz, 80 )
            config_var( q_direct_max_range, velkoz, 95 )
            config_var( q_split_logic, velkoz, 1 )


            config_var( w_enabled, velkoz, true )
            config_var( w_hitchance, velkoz, 2 )
            config_var( w_flee, velkoz, false )
            config_var( w_max_range, velkoz, 95 )

            config_var( e_enabled, velkoz, true )
            config_var( e_antigapclose, velkoz, true )
            config_var( e_autointerrupt, velkoz, true )
            config_var( e_flee, velkoz, true )
            config_var( e_hitchance, velkoz, 2 )

            config_var( r_enabled, velkoz, true )
            config_var( r_targeting_mode, velkoz, 0 )

            config_var( q_draw_range, velkoz, false )
            config_var( q_draw_max_range, velkoz, true )
            config_var( w_draw_range, velkoz, false )
            config_var( e_draw_range, velkoz, false )
            config_var( r_draw_range, velkoz, false )
            config_var( r_draw_hitbox, velkoz, true )

            config_var( dont_draw_on_cooldown, velkoz, true )
        } velkoz;

        struct {
            config_var( q_enabled, anivia, true )
            config_var( q_hitchance, anivia, 2 )
            config_var( q_harass, anivia, true )

            config_var( w_enabled, anivia, true )
            config_var( w_autointerrupt, anivia, true )
            config_var( w_antigapclose, anivia, true )

            config_var( e_enabled, anivia, true )
            config_var( e_only_if_frosted, anivia, true )

            config_var( r_enabled, anivia, true )
            config_var( r_laneclear, anivia, true )
            config_var( r_recast, anivia, 500 )
            config_var( r_laneclear_min_mana, anivia, 20 )

            config_var( q_draw_range, anivia, true )
            config_var( e_draw_range, anivia, false )
            config_var( r_draw_range, anivia, false )
            config_var( dont_draw_on_cooldown, anivia, true )
        } anivia;

        struct {
            config_var( q_enabled, aurelion, true )
            config_var( q_harass, aurelion, true )
            config_var( q_max_range, aurelion, 80 )

            config_var( w_enabled, aurelion, true )

            config_var( e_enabled, aurelion, true )
            config_var( e_flee, aurelion, true )
            config_var( e_killsteal, aurelion, true )
            config_var( e_antigapclose, aurelion, true )
            config_var( e_hitchance, aurelion, 2 )

            config_var( r_enabled, aurelion, true )
            config_var( r_hitchance, aurelion, 2 )
            config_var( r_min_multihit_count, aurelion, 3 )

            config_var( prefer_e_over_q, aurelion, 1 )

            config_var( q_draw_range, aurelion, true )
            config_var( w_draw_range, aurelion, false )
            config_var( e_draw_range, aurelion, false )
            config_var( r_draw_range, aurelion, false )
            config_var( dont_draw_on_cooldown, aurelion, true )
        } aurelion;

        struct {
            config_var( q_enabled, gangplank, true )
            config_var( q_harass, gangplank, true )
            config_var( q_lasthit, gangplank, true )

            config_var( e_enabled, gangplank, true )
            config_var( e_chain_barrel_logic, gangplank, 1 )
            config_var( e_hitchance, gangplank, 2 )
            config_var( e_hitbox_compensation_value, gangplank, 300 )
            config_var( e_simulated_radius, gangplank, 365 )

            config_var( humanize_barrel_placement, gangplank, true )
            config_var( humanizer_strength, gangplank, 25 )

            config_var( q_draw_range, gangplank, true )
            config_var( e_draw_range, gangplank, true )
            config_var( dont_draw_on_cooldown, gangplank, true )
            config_var( draw_simulated_barrel, gangplank, true )
        } gangplank;

        struct {
            config_var( q_enabled, annie, true )
            config_var( q_harass, annie, true )
            config_var( q_killsteal, annie, true )
            config_var( q_lasthit, annie, true )

            config_var( w_enabled, annie, true )
            config_var( w_harass, annie, false )
            config_var( w_hitchance, annie, 2 )

            config_var( e_enabled, annie, true )
            config_var( e_flee, annie, true )
            config_var( e_shielding, annie, true )

            config_var( r_enabled, annie, true )
            config_var( r_killsteal, annie, true )
            config_var( r_hitchance, annie, 2 )

            config_var( q_draw_range, annie, true )
            config_var( w_draw_range, annie, false )
            config_var( e_draw_range, annie, false )
            config_var( r_draw_range, annie, false )
            config_var( dont_draw_on_cooldown, annie, true )
        } annie;

        struct {
            config_var( q_enabled, fiora, true )
            config_var( q_harass, fiora, true )
            config_var( q_flee, fiora, true )
            config_var( q_only_vitals, fiora, true )
            config_var( q_vital_hitchance, fiora, 1 )

            config_var( vital_max_angle, fiora, 75 )
            config_var( vital_max_range, fiora, 80 )

            config_var( e_enabled, fiora, true )
            config_var( e_aa_reset, fiora, true )
            config_var( e_turretclear, fiora, true )

            config_var( q_draw_range, fiora, true )
            config_var( w_draw_range, fiora, false )
            config_var( dont_draw_on_cooldown, fiora, true )
            config_var( draw_vitals, fiora, true )
        } fiora;

        struct {
            config_var( q_enabled, jarvan, true )

            config_var( w_enabled, jarvan, true )

            config_var( e_enabled, jarvan, true )
            config_var( eq_flee, jarvan, true )
            config_var( eq_hitchance, jarvan, 0 )
            config_var( eq_autointerrupt, jarvan, true)

            config_var( q_jungleclear, jarvan, true )
            config_var( w_jungleclear, jarvan, true )
            config_var( e_jungleclear, jarvan, true )

            config_var( eq_draw_range, jarvan, true )
            config_var( r_draw_range, jarvan, true )
            config_var( dont_draw_on_cooldown, jarvan, true )
        } jarvan;

        struct {
            config_var( q_enabled, xayah, true )
            config_var( q_harass, xayah, true )
            config_var( q_killsteal, xayah, true )
            config_var( q_immobile, xayah, true )
            config_var( q_aa_reset, xayah, true )
            config_var( q_hitchance, xayah, 2 )

            config_var( w_enabled, xayah, true )
            config_var( w_turretclear, xayah, true )

            config_var( e_enabled, xayah, true )
            config_var( e_killsteal, xayah, true )
            config_var( e_single_target_min_feathers, xayah, 4 )
            config_var( e_multihit_min_feathers, xayah, 3 )
            config_var( e_multihit_min_targets, xayah, 2 )
            config_var( e_full_combo_force_root, xayah, true )

            config_var( r_enabled, xayah, true )
            config_var( r_hitchance, xayah, 2 )

            config_var( passive_harass, xayah, true )

            config_var( q_draw_range, xayah, true )
            config_var( r_draw_range, xayah, false )
            config_var( w_draw_duration, xayah, true )
            config_var( dont_draw_on_cooldown, xayah, true )
            config_var( draw_feathers, xayah, true )
            config_var( e_draw_damage, xayah, true )
        } xayah;

        struct {
            config_var( q_enabled, nami, true )
            config_var( q_harass, nami, true )
            config_var( q_flee, nami, true )
            config_var( q_antigapclose, nami, true )
            config_var( q_autointerrupt, nami, true )
            config_var( q_hitchance, nami, 2 )

            config_var( w_enabled, nami, true )
            config_var( w_bounce_logic, nami, 1 )
            config_var( w_simulated_bounce_max_range, nami, 90 )
            config_var( w_heal_threshold, nami, 65 )

            config_var( e_enabled, nami, true )
            config_var( e_on_ally_autoattack, nami, true )
            config_var( e_on_self_autoattack, nami, true )

            config_var( r_enabled, nami, true )
            config_var( r_minimum_hitcount, nami, 3)

            config_var( q_draw_range, nami, true )
            config_var( w_draw_range, nami, false )
            config_var( e_draw_range, nami, false )
            config_var( r_draw_range, nami, false )
            config_var( dont_draw_on_cooldown, nami, true )
        } nami;

        struct {
            config_var( q_enabled, mordekaiser, true )
            config_var( q_harass, mordekaiser, true )
            config_var( q_hitchance, mordekaiser, 2 )
            config_var( q_aa_reset_ignore_hitchance, mordekaiser, true )

            config_var( w_enabled, mordekaiser, true )
            config_var( w_on_incoming_damage, mordekaiser, true )
            config_var( w_health_threshold, mordekaiser, 60 )
            config_var( w_min_mana, mordekaiser, 35 )

            config_var( e_enabled, mordekaiser, true )
            config_var( e_hitchance, mordekaiser, 2 )
            config_var( e_autointerrupt, mordekaiser, true )
            config_var( e_min_range, mordekaiser, 50 )

            config_var( q_draw_range, mordekaiser, true )
            config_var( e_draw_range, mordekaiser, false )
            config_var( r_draw_range, mordekaiser, false )
            config_var( dont_draw_on_cooldown, mordekaiser, true )
        } mordekaiser;

        struct {
            config_var( q_enabled, amumu, true )
            config_var( q_hitchance, amumu, 2 )
            config_var( q_antigapclose, amumu, true )
            config_var( q_autointerrupt, amumu, true )
            config_var( q_killsteal, amumu, true )

            config_var( w_enabled, amumu, true )
            config_var( w_jungleclear, amumu, true )
            config_var( w_min_mana, amumu, 10 )
            config_var( w_disable_under_min_mana, amumu, true )
            config_var( w_min_enemy, amumu, 1 )
            config_var( w_min_jungle, amumu, 2 )

            config_var( e_enabled, amumu, true )
            config_var( e_jungleclear, amumu, true )

            config_var( r_enabled, amumu, true )
            config_var( r_min_enemy, amumu, 2 )
            config_var( r_ignore_multi_FC, amumu, true )

            config_var( q_draw_range, amumu, true )
            config_var( dont_draw_on_cooldown, amumu, true )
        } amumu;

        struct {
            config_var( q_enabled, zilean, true )
            config_var( q_hitchance, zilean, 2 )
            config_var( q_hold_to_stun, zilean, true )
            config_var( q_antigapclose, zilean, true )
            config_var( q_autointerrupt, zilean, true )
            config_var( q_killsteal, zilean, true )
            config_var( q_laneclear, zilean, true )

            config_var( w_enabled, zilean, true )
            config_var( w_laneclear, zilean, true )

            config_var( e_enabled, zilean, true )
            config_var( e_antimelee, zilean, true )
            config_var( e_gapclose, zilean, true )
            config_var( e_flee, zilean, true )

            config_var( r_enabled, zilean, true )
            config_var( only_buff_priority_ally, zilean, true )
            config_var( r_min_hp, zilean, 10 )

            config_var( q_draw_range, zilean, true )
            config_var( r_draw_range, zilean, true )
            config_var( dont_draw_on_cooldown, zilean, true )
        } zilean;

        struct {
            config_var( q_enabled, yuumi, true )
            config_var( q_auto_first_cast, yuumi, false )
            config_var( q_auto_harass, yuumi, true )
            config_var( q_hitchance, yuumi, 2 )

            config_var( e_enabled, yuumi, true )
            config_var( e_block_damage_from_spells, yuumi, true )
            config_var( e_block_damage_from_autoattacks, yuumi, true )
            config_var( e_block_damage_from_turret_shots, yuumi, true )
            config_var( e_block_damage_from_ignite, yuumi, true )
            config_var( e_block_damage_from_poison, yuumi, true )
            config_var( e_block_damage_from_item_burn, yuumi, true )
            config_var( e_skillshot_minimum_danger, yuumi, 2 )
            config_var( e_increase_damage, yuumi, true )

             config_var( q_spellslot, yuumi, 50 )

            config_var( q_draw_range, yuumi, true )
            config_var( w_draw_range, yuumi, false )
            config_var( r_draw_range, yuumi, false )
            config_var( dont_draw_on_cooldown, yuumi, true )
        } yuumi;

        struct {
            config_var( q_enabled, janna, true )
            config_var( q_recast, janna, true )
            config_var( q_antigapclose, janna, true )
            config_var( q_autointerrupt, janna, true )
            config_var( q_on_crowdcontrol, janna, true )
            config_var( q_flee, janna, true )
            config_var( q2_hitchance, janna, 2 )
            config_var( q_antimelee, janna, true )
            config_var( q_antimelee_max_range, janna, 40 )

            config_var( w_enabled, janna, true )
            config_var( w_harass, janna, true )
            config_var( w_flee, janna, true )

            config_var( e_enabled, janna, true )
            config_var( e_block_damage_from_spells, janna, true )
            config_var( e_block_damage_from_autoattacks, janna, true )
            config_var( e_block_damage_from_turret_shots, janna, true )
            config_var( e_block_damage_from_ignite, janna, true )
            config_var( e_block_damage_from_poison, janna, true )
            config_var( e_block_damage_from_item_burn, janna, true )
            config_var( e_skillshot_minimum_danger, janna, 2 )
            config_var( e_increase_damage, janna, true )

            config_var( ally_priority_enabled, janna, true )
            config_var( ally_buff_on_chase, janna, false )
            config_var( ally_buff_on_flee, janna, false )
            config_var( ally_buff_in_combat, janna, false )
            config_var( only_buff_priority_ally, janna, true )

            config_var( q_draw_range, janna, false )
            config_var( q_draw_max_range, janna, true )
            config_var( w_draw_range, janna, false )
            config_var( e_draw_range, janna, false )
            config_var( r_draw_range, janna, false )
            config_var( dont_draw_on_cooldown, janna, true )
            config_var( q_draw_hitbox, janna, true )
        } janna;

        struct {
            config_var( q_enabled, malzahar, true )
            config_var( q_direct, malzahar, true )
            config_var( q_normal, malzahar, true )
            config_var( q_harass, malzahar, true )
            config_var( q_killsteal, malzahar, true )
            config_var( q_autointerrupt, malzahar, true )
            config_var( q_hitchance, malzahar, 2 )

            config_var( w_enabled, malzahar, true )
            config_var( w_min_spawn_count, malzahar, 2 )

            config_var( e_enabled, malzahar, true )

            config_var( r_enabled, malzahar, true )
            config_var( r_in_full_combo, malzahar, true )

            config_var( q_draw_range, malzahar, true )
            config_var( e_draw_range, malzahar, false )
            config_var( r_draw_range, malzahar, false )
            config_var( dont_draw_on_cooldown, malzahar, true )
        } malzahar;

        struct {
            config_var( q_enabled, thresh, true )
            config_var( q_antigapclose, thresh, true )
            config_var( q_hitchance, thresh, 2 )
            config_var( q_autointerrupt, thresh, true )

            config_var( w_enabled, thresh, true )
            config_var( w_block_inc_damage, thresh, true )
            config_var( w_percent_hp_to_block, thresh, 10 )

            config_var( e_enabled, thresh, true )
            config_var( e_antigapclose, thresh, true )
            config_var( e_autointerrupt, thresh, true )

            config_var( r_enabled, thresh, true )
            config_var( r_in_full_combo, thresh, true )

            config_var( q_draw_range, thresh, true )
            config_var( w_draw_range, thresh, false )
            config_var( r_draw_range, thresh, false )
            config_var( dont_draw_on_cooldown, thresh, true )
        } thresh;

        struct {
            config_var( q_enabled, milio, true )
            config_var( q_flee, milio, true )
            config_var( q_harass, milio, true )
            config_var( q_killsteal, milio, true )
            config_var( q_antigapclose, milio, true )
            config_var( q_autointerrupt, milio, true )
            config_var( q_direct, milio, true )
            config_var( q_bounce, milio, true )
            config_var( q_bounce_ignore_moving, milio, false )
            config_var( q_max_range, milio, 95 )
            config_var( q_hitchance, milio, 2 )

            config_var( w_enabled, milio, true )
            config_var( w_on_autoattack, milio, true )
            config_var( w_full_combo, milio, true )

            config_var( e_enabled, milio, true )
            config_var( e_flee, milio, true )
            config_var( e_block_damage_from_spells, milio, true )
            config_var( e_block_damage_from_autoattacks, milio, true )
            config_var( e_block_damage_from_turret_shots, milio, true )
            config_var( e_block_damage_from_ignite, milio, true )
            config_var( e_block_damage_from_poison, milio, true )
            config_var( e_block_damage_from_item_burn, milio, true )
            config_var( e_skillshot_minimum_danger, milio, 2 )
            config_var( e_increase_damage, milio, true )
            config_var( e_self, milio, true )
            config_var( e_delay_movespeed, milio, true )
            config_var( e_delay_shield, milio, true )
            config_var( e_delay_passive, milio, true )

            config_var( semi_manual_we, milio, true )
            config_var( semi_manual_e, milio, true )
            config_var( semi_manual_w, milio, true )

            config_var( ally_priority_enabled, milio, true )
            config_var( ally_buff_on_chase, milio, false )
            config_var( ally_buff_on_flee, milio, false )
            config_var( ally_buff_in_combat, milio, true )
            config_var( only_buff_priority_ally, milio, true )
            config_var( ally_override_cleanse_count, milio, true )

            config_var( r_enabled, milio, true )
            config_var( r_multihit_count, milio, 2 )
            config_var( r_heal_multihit_count, milio, 3 )
            config_var( r_heal_threshold, milio, 30 )

            // ult cleanse options
            config_var( cleanse_stun, milio, true )
            config_var( cleanse_root, milio, true )
            config_var( cleanse_sleep, milio, true )
            config_var( cleanse_drowsy, milio, true )
            config_var( cleanse_suppression, milio, true )
            config_var( cleanse_fear, milio, true )
            config_var( cleanse_charm, milio, true )
            config_var( cleanse_berserk, milio, true )
            config_var( cleanse_taunt, milio, true )
            config_var( cleanse_polymorph, milio, true )
            config_var( cleanse_exhaust, milio, true )
            config_var( cleanse_nasus_w, milio, true )
            config_var( cleanse_yasuo_r, milio, true )
            config_var( cleanse_disarm, milio, true )

            config_var( cleanse_silence, milio, false )
            config_var( cleanse_blind, milio, false )
            config_var( cleanse_ignite, milio, false )
            config_var( cleanse_delay, milio, 15 )

            config_var( q_draw_range, milio, true )
            config_var( q_draw_max_range, milio, false )
            config_var( w_draw_range, milio, false )
            config_var( e_draw_range, milio, true )
            config_var( r_draw_range, milio, false )
            config_var( dont_draw_on_cooldown, milio, true )
        } milio;

        struct {
            config_var( q_enabled, akshan, true )
            config_var( q_aa_reset, akshan, true )
            config_var( q_hitchance, akshan, 2 )

            config_var( e_enabled, akshan, true )

            config_var( q_draw_range, akshan, true )
            config_var( e_draw_range, akshan, false )
            config_var( r_draw_range, akshan, true )
            config_var( dont_draw_on_cooldown, akshan, true )
        } akshan;

        struct {
            config_var( q_enabled, evelynn, true )
            config_var( q_killsteal, evelynn, true )
            config_var( q_hitchance, evelynn, 2 )
            config_var( q_spellclear, kindred, true )

            config_var( w_enabled, evelynn, true )

            config_var( e_enabled, evelynn, true )
            config_var( e_aa_reset, evelynn, true )
            config_var( e_killsteal, evelynn, true )
            config_var( e_spellclear, evelynn, true )

            config_var( r_enabled, evelynn, true )
            config_var( r_only_full_combo, evelynn, true )
            config_var( r_health_threshold, evelynn, 10 )
            config_var( r_killsteal, evelynn, true )

            config_var( delay_qe_for_stun, evelynn, true )
            config_var( full_combo_ignore_delay, evelynn, true )

            config_var( q_draw_range, evelynn, true )
            config_var( r_draw_range, evelynn, true )
            config_var( dont_draw_on_cooldown, evelynn, true )
        } evelynn;

        struct {
            config_var( q_enabled, trundle, true )
            config_var( q_spellclear, trundle, true )

            config_var( w_enabled, trundle, true )
            config_var( w_spellclear, trundle, true )

            config_var( e_enabled, trundle, true )
            config_var( e_autointerrupt, trundle, true )
            config_var( e_antigapclose, trundle, true )
            config_var( e_hitchance, trundle, 2 )

            config_var( w_draw_range, trundle, false )
            config_var( e_draw_range, trundle, true )
            config_var( r_draw_range, trundle, false )
            config_var( dont_draw_on_cooldown, trundle, true )
        } trundle;

        struct {
            config_var( q_enabled, azir, true )
            config_var( q_hitchance, azir, 2 )
            config_var( q_harass, azir, true )
            config_var( q_only_two_soldiers, azir, true )

            config_var( w_enabled, azir, true )
            config_var( w_harass, azir, true )

            config_var( e_enabled, azir, true )
            config_var( e_shuffle_on_flee, azir, true )

            config_var( r_full_combo_shuffle, azir, true )
            config_var( r_shuffle_key, azir, true )
            config_var( r_direction, azir, 0 )

            config_var( q_draw_range, azir, true )
            config_var( w_draw_range, azir, false )
            config_var( w_draw_duration, azir, true )
            config_var( e_draw_range, azir, true )
            config_var( r_draw_shuffle_mode, azir, true )
            config_var( dont_draw_on_cooldown, azir, true )
        } azir;
    };
}

extern config::c_config* g_config;
