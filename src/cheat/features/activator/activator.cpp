#include "pch.hpp"

#include "activator.hpp"
#include "../buff_cache.hpp"
#include "../orbwalker.hpp"
// #include "../buff_cache.hpp"
#include "../entity_list.hpp"
#include "../evade.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"
#include "../tracker.hpp"
#include "../../utils/input.hpp"
#include "../../sdk/game/pw_hud.hpp"
#include "../../sdk/game/ai_manager.hpp"
#include "../../sdk/game/hud_manager.hpp"

#include "../../sdk/audio/audio.hpp"
#include "../../sdk/game/render_manager.hpp"

// #include "../sdk/game/spell_info.hpp"
#include "../../sdk/game/spell_data.hpp"
#include "../../utils/directory_manager.hpp"
#include "../../utils/path.hpp"

#include "../champion_modules/module.hpp"

#pragma comment( lib, "winmm.lib" )

namespace features {
    const std::array< std::string, 13 > FOURTEEN_WORDS = {
        "WE",
        "MUST",
        "SECURE",
        "THE",
        "EXISTENCE",
        "OF",
        "OUR",
        "PEOPLE",
        "AND",
        "A FUTURE",
        "FOR",
        "WHITE",
        "CHILDREN!"
    };

    void Activator::initialize_menu( ){
        const auto navigation    = g_window->push( _( "activator" ), menu_order::activator );
        const auto warding       = navigation->add_section( _( "autoward" ) );
        const auto heal          = navigation->add_section( _( "heal" ) );
        const auto ignite        = navigation->add_section( _( "ignite" ) );
        const auto barrier       = navigation->add_section( _( "barrier" ) );
        const auto exhaust       = navigation->add_section( _( "exhaust" ) );
        const auto cleanse       = navigation->add_section( _( "cleanse" ) );
        const auto qss           = navigation->add_section( _( "qss" ) );
        const auto smite         = navigation->add_section( _( "smite" ) );
        const auto pot           = navigation->add_section( _( "auto potion" ) );
        const auto everfrost     = navigation->add_section( _( "everfrost" ) );
        const auto goredrinker   = navigation->add_section( _( "goredrinker/whip" ) );
        const auto support_items = navigation->add_section( _( "support items " ) );


        heal->checkbox( _( "enable" ), g_config->activator.heal_toggle );
        heal->checkbox( _( "predict damage" ), g_config->activator.heal_predict );
        heal->select(
            _( "logic mode" ),
            g_config->activator.heal_logic,
            { _( "Always" ), _( "In combo" ), _( "In full combo" ) }
        );
        heal->slider_int( _( "hp% threshold" ), g_config->activator.heal_percent_threshold, 5, 50, 1 );

        barrier->checkbox( _( "enable" ), g_config->activator.barrier_toggle );
        barrier->checkbox( _( "predict damage" ), g_config->activator.barrier_predict );
        barrier->select(
            _( "logic mode" ),
            g_config->activator.barrier_logic,
            { _( "Always" ), _( "In combo" ), _( "In full combo" ) }
        );
        barrier->slider_int( _( "hp% threshold" ), g_config->activator.barrier_percent_threshold, 5, 50, 1 );

        ignite->checkbox( _( "enable" ), g_config->activator.ignite_toggle );
        ignite->checkbox( _( "predict damage" ), g_config->activator.ignite_predict );
        ignite->select(
            _( "logic mode" ),
            g_config->activator.ignite_mode,
            { _( "Always" ), _( "In combo" ), _( "In full combo" ) }
        );

        exhaust->checkbox( _( "enable" ), g_config->activator.exhaust_toggle );
        const auto hotkey_checkbox = exhaust->checkbox( _( "hotkey (?)" ), g_config->activator.exhaust_hotkey );
        hotkey_checkbox->set_tooltip( _( "Right click this checkbox to set your preferred hotkey" ) );


        smite->checkbox( _( "enable" ), g_config->activator.smite_toggle );
        smite->checkbox( _( "buffs" ), g_config->activator.smite_buffs );
        smite->checkbox( _( "river crab" ), g_config->activator.smite_crabs );
        smite->checkbox( _( "predict monster health" ), g_config->activator.smite_predict_health );

        qss->checkbox( _( "enable" ), g_config->activator.qss_toggle );
        qss->checkbox( _( "only in combo" ), g_config->activator.qss_smart );
        qss->slider_int( _( "use delay (?)" ), g_config->activator.qss_delay, 0, 500, 5 )->set_tooltip(
            _( "Delay cast by X milliseconds" )
        );

        cleanse->checkbox( _( "enable" ), g_config->activator.cleanse_toggle );
        cleanse->checkbox( _( "only in combo" ), g_config->activator.cleanse_smart );
        cleanse->multi_select(
            _( "cleanse whitelist" ),
            {
                g_config->activator.cleanse_stun,
                g_config->activator.cleanse_root,
                g_config->activator.cleanse_charm,
                g_config->activator.cleanse_fear,
                g_config->activator.cleanse_suppression,
                g_config->activator.cleanse_taunt,
                g_config->activator.cleanse_blind,
                g_config->activator.cleanse_sleep,
                g_config->activator.cleanse_polymorph,
                g_config->activator.cleanse_berserk,
                g_config->activator.cleanse_disarm,
                g_config->activator.cleanse_drowsy,
                g_config->activator.cleanse_nasus_w,
                g_config->activator.cleanse_mordekaiser_r,
                g_config->activator.cleanse_yasuo_r,
                g_config->activator.cleanse_ignite,
                g_config->activator.cleanse_exhaust
            },
            {
                _( "Stun" ),
                _( "Root" ),
                _( "Charm" ),
                _( "Fear" ),
                _( "Suppression" ),
                _( "Taunt" ),
                _( "Blind" ),
                _( "Asleep" ),
                _( "Polymorph" ),
                _( "Berserk" ),
                _( "Disarm" ),
                _( "Drowsy" ),
                _( "Nasus W" ),
                _( "Mordekaiser R" ),
                _( "Yasuo R" ),
                _( "Ignite" ),
                _( "Exhaust" )
            }
        );

        cleanse->slider_int( _( "use delay (?)" ), g_config->activator.cleanse_delay, 0, 500, 5 )->set_tooltip(
            _( "Delay cast by X milliseconds" )
        );

        pot->checkbox( _( "enable" ), g_config->activator.autopotion_toggle );
        pot->checkbox( _( "only in combo" ), g_config->activator.autopotion_only_combo );
        pot->checkbox( _( "dont use before 1:30 time" ), g_config->activator.autopotion_disable_early );
        pot->slider_int( _( "hp% threshold" ), g_config->activator.autopotion_health_threshold, 5, 85, 1 );

        everfrost->checkbox( _( "enable" ), g_config->activator.everfrost_toggle );
        everfrost->checkbox( _( "only in combo" ), g_config->activator.everfrost_only_in_combo );
        everfrost->select(
            _( "hitchance" ),
            g_config->activator.everfrost_hitchance,
            { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
        );

        goredrinker->checkbox( _( "enable" ), g_config->activator.goredrinker_toggle );
        goredrinker->checkbox( _( "enable only in combo" ), g_config->activator.goredrinker_only_in_combo );
        goredrinker->checkbox(
            _( "enable HP threshold activation" ),
            g_config->activator.goredrinker_HP_thresh_toggle
        );
        goredrinker->slider_int( _( "targets within range" ), g_config->activator.goredrinker_min_hit, 1, 5, 1 );
        goredrinker->slider_int(
            _( "HP threshold activation" ),
            g_config->activator.goredrinker_HP_thresh,
            1,
            75,
            1
        );

        support_items->checkbox( _( "enable mikaels blessing" ), g_config->activator.mikaels_toggle );
        support_items->checkbox( _( "enable redemption" ), g_config->activator.redemption_toggle );
        support_items->slider_int(
            _( "redemption: Ally HP Threshold" ),
            g_config->activator.redemption_ally_hp_threshold,
            1,
            50,
            1
        );
        //support_items->checkbox(_("enable locket of solari"), g_config->activator.redemption_toggle);
        //support_items->slider_int(_("locket: Ally HP Threshold"), g_config->activator.redemption_ally_hp_threshold, 1, 50, 1);

        warding->checkbox( _( "enable" ), g_config->activator.warding_toggle );
        warding->checkbox( _( "fully automatic warding" ), g_config->activator.warding_fully_automatic );
        warding->checkbox( _( "controlward reminder" ), g_config->activator.warding_controlward_reminder );
        warding->checkbox( _( "semi-manual ward [ hotkey: N ]" ), g_config->activator.wardhelper_toggle );
    }

    auto Activator::on_draw( ) -> void{
        if ( g_local->is_dead( ) ) return;

        draw_autoward( );
        draw_controlward_reminder( );
        draw_ward_spots( );
        draw_kill_efect( );
        draw_vision_score( );
        draw_pingable_ward_indicator( );
    }

    auto Activator::run( ) -> void{
        // manual ping
        update_manual_ping( );

        update_ping_timer( );
        update_state( );
        auto_encourage_ally( );
        auto_log_cooldowns( );
        auto_say_cooldowns( );
        auto_gank_warn( );

        update_auto_ward( );

        ping_wards( );
        update_missing_enemies( );
        update_pings( );
        auto_cancerping( );

        auto_question( );

        if ( g_local->is_dead( ) ) {
            if ( !m_is_dead ) {
                m_is_dead    = true;
                m_death_time = *g_time;
            }
            return;
        }

        run_auto_ward( );
        semi_manual_autoward( );

        anti_afk( );

        m_is_dead = false;
        if (!g_local.update( )) return; 

        if ( static_cast< int >( m_last_health ) != static_cast< int >( g_local->health ) ) {
            m_health_difference = m_last_health - g_local->health;
            m_last_health       = g_local->health;

            /* if (g_config->misc.extra_eye_candy->get<bool>() && m_health_difference < 0 && m_health_difference > -10)
             {
                 int value = std::ceil(-m_health_difference);
                 std::string text = "+" + std::to_string(value);
                 g_function_caller->floating_text(g_local->network_id, text.c_str(), utils::e_floating_text_type::heal);
             }*/
        }


        // TODO: re-add this
        if ( g_config->misc.show_vision_score->get< bool >( ) && m_vision_score < g_local->get_vision_score( ) ) {
            m_vision_score_addition    = g_local->get_vision_score( ) - m_vision_score;
            m_vision_score             = g_local->get_vision_score( );
            m_vision_score_change_time = m_vision_score_addition < 5.f ? *g_time : 0.f;

            std::cout << "[o] Updated vision score, gained: " << m_vision_score_addition << std::endl;
        }


        //cancer_chat( );

        if ( *g_time - m_last_cast_time <= 0.5f ) return;

        run_ward_assist( );

        for ( auto i = 0; i < 2; i++ ) {
            auto spell = i == 0
                             ? g_local->spell_book.get_spell_slot( ESpellSlot::d )
                             : g_local->spell_book.get_spell_slot( ESpellSlot::f );
            const auto slot = i == 0 ? ESpellSlot::d : ESpellSlot::f;
            if ( !spell || !spell->is_ready( ) ) continue;

            switch ( rt_hash( spell->get_name().c_str() ) ) {
            case ct_hash( "SummonerSmite" ):
                if ( spell->charges > 0 ) run_auto_smite( 0, slot );
                break;
            case ct_hash( "S5_SummonerSmiteDuel" ):
            case ct_hash( "S5_SummonerSmitePlayerGanker" ):
                if ( spell->charges > 0 ) run_auto_smite( 1, slot );
                break;
            case ct_hash( "SummonerSmiteAvatarUtility" ):
            case ct_hash( "SummonerSmiteAvatarOffensive" ):
            case ct_hash( "SummonerSmiteAvatarDefensive" ):
                if ( spell->charges > 0 ) run_auto_smite( 2, slot );
                break;
            case ct_hash( "SummonerHeal" ):
                run_auto_heal( slot );
                break;
            case ct_hash( "SummonerBarrier" ):
                run_auto_barrier( slot );
                break;
            case ct_hash( "SummonerBoost" ):
                run_auto_cleanse( slot );
                break;
            case ct_hash( "SummonerDot" ):
                run_auto_ignite( slot );
                break;
            case ct_hash( "SummonerExhaust" ):
                run_auto_exhaust( slot );
                break;
            default:
                break;
            }
        }

        for ( auto i = 1; i < 8; i++ ) // cuz Inventory Slot starts at 1, so we iterate all, 7 is trinket so no check
        {
            const auto spell_slot_object = static_cast< ESpellSlot >( 5 + i );
            auto       spell_slot        = g_local->spell_book.get_spell_slot( spell_slot_object );

            if ( !spell_slot || !spell_slot->is_ready( ) ) continue;

            auto slot = g_local->inventory.get_inventory_slot( i );
            if ( !slot ) continue;
            auto item_base = slot->get_base_item( );
            if ( !item_base ) continue;
            auto item_data = item_base->get_item_data( );
            if ( !item_data ) continue;

            //if (i == 7) std::cout << "name: " << spell_slot->get_spell_info()->get_spell_data()->get_name() << "\n" << "id: " << item_data->id << std::endl;

            switch ( static_cast< EItemId >( item_data->id ) ) {
            default:
                //if ( i == 3 )
                //    std::cout << "id: " << std::dec << item_data->id << " | ready: " << spell_slot->cooldown_expire
                //             << " | charges: " << spell_slot->charges << "| addy " << std::hex << slot.get_address(  ) << std::endl;
                break;
            case EItemId::refillable_potion:
            case EItemId::corrupting_potion:
                if ( item_base->stacks_left == 0 ) break;
            case EItemId::health_potion:
                run_auto_potion( spell_slot_object );
            case EItemId::mercurial_scimitar:
            case EItemId::quicksilver_sash:
            case EItemId::silvermere_dawn:
                run_auto_qss( spell_slot_object );
                break;
            case EItemId::everfrost:
                if ( run_auto_everfrost_antigapclose( spell_slot_object ) ) break;
                run_auto_everfrost( spell_slot_object );
                break;
            case EItemId::ward_trinket:
            case EItemId::control_ward:
            case EItemId::frostfang:
            case EItemId::shard_of_true_ice:
            case EItemId::harrowing_cresent:
            case EItemId::blackmist_scythe:
                if ( spell_slot->charges == 0 && item_base->stacks_left == 0 && static_cast< EItemId >( item_data->id )
                    != EItemId::control_ward )
                    break;

                run_auto_ward_specific( spell_slot_object );
                break;
            case EItemId::ironspike_whip:
            case EItemId::goredrinker:
                run_auto_goredrinker( spell_slot_object );
                break;
            case EItemId::mikaels_blessing:
                run_auto_mikaels( spell_slot_object );
                break;
            case EItemId::redemption:
                run_auto_redemption( spell_slot_object );
                break;
            }
        }
    }

    auto Activator::run_auto_cleanse( const ESpellSlot slot ) -> bool{
        if ( !g_config->activator.cleanse_toggle->get< bool >( ) ||
            g_config->activator.cleanse_smart->get< bool >( ) && g_features->orbwalker->get_mode( ) !=
            Orbwalker::EOrbwalkerMode::combo )
            return false;

        bool should_cast{ };
        bool prevent_cast{ };

        for ( const auto buff : g_features->buff_cache->get_all_buffs( g_local->index ) ) {
            if ( !buff ) continue;

            const auto type = static_cast< EBuffType >( buff->buff_data->type );
            bool       is_buff_cleansable{ };
            switch ( type ) {
            case EBuffType::snare:
                if ( !g_config->activator.cleanse_root->get< bool >( ) ) continue;

                is_buff_cleansable = true;
                break;
            case EBuffType::charm:
                if ( !g_config->activator.cleanse_charm->get< bool >( ) ) continue;

                is_buff_cleansable = true;
                break;
            case EBuffType::taunt:
                if ( !g_config->activator.cleanse_taunt->get< bool >( ) ) continue;
                is_buff_cleansable = true;
                break;
            case EBuffType::stun:
                if ( !g_config->activator.cleanse_stun->get< bool >( ) ) continue;
                is_buff_cleansable = true;
                break;
            case EBuffType::fear:
                if ( !g_config->activator.cleanse_fear->get< bool >( ) ) continue;
                is_buff_cleansable = true;
                break;
            case EBuffType::polymorph:
                if ( !g_config->activator.cleanse_polymorph->get< bool >( ) ) continue;
                is_buff_cleansable = true;
                break;
            case EBuffType::asleep:
                if ( !g_config->activator.cleanse_sleep->get< bool >( ) ) continue;
                is_buff_cleansable = true;
                break;
            case EBuffType::drowsy:
                if ( !g_config->activator.cleanse_drowsy->get< bool >( ) ) continue;
                is_buff_cleansable = true;
                break;
            case EBuffType::berserk:
                if ( !g_config->activator.cleanse_berserk->get< bool >( ) ) continue;
                is_buff_cleansable = true;
                break;
            case EBuffType::disarm:
                if ( !g_config->activator.cleanse_disarm->get< bool >( ) ) continue;
                is_buff_cleansable = true;
                break;
            case EBuffType::blind:
                if ( !g_config->activator.cleanse_blind->get< bool >( ) ) continue;
                is_buff_cleansable = true;
                break;
            case EBuffType::knockup:
            case EBuffType::knockback:
            case EBuffType::suppression:
                prevent_cast = true;
                break;
            default:

                switch ( rt_hash( buff->name.c_str() ) ) {
                case ct_hash( "NasusW" ):
                    if ( !g_config->activator.cleanse_nasus_w->get< bool >( ) ) continue;
                    is_buff_cleansable = true;
                    break;
                case ct_hash( "SummonerExhaust" ):
                    if ( !g_config->activator.cleanse_exhaust->get< bool >( ) ) continue;
                    is_buff_cleansable = true;
                    break;
                case ct_hash( "SummonerDot" ):
                    if ( !g_config->activator.cleanse_ignite->get< bool >( ) ) continue;
                    is_buff_cleansable = true;
                    break;
                case ct_hash( "yasuorknockup" ):
                    if ( !g_config->activator.cleanse_yasuo_r->get< bool >( ) ) continue;
                    is_buff_cleansable = true;
                    break;
                default:
                    break;
                }

                break;
            }

            if ( prevent_cast ) break;

            if ( !should_cast && is_buff_cleansable ) {
                const auto duration_left = buff->buff_data->end_time - *g_time;
                const auto duration      = *g_time - buff->buff_data->start_time;

                if ( duration < g_config->activator.cleanse_delay->get< int >( ) / 1000.f
                    || duration_left <= 0.3f )
                    continue;

                should_cast = true;
            }
        }


        if ( !should_cast || prevent_cast ) return false;

        if ( g_input->cast_spell( slot ) ) {
            m_last_cast_time = *g_time;
            if ( g_config->misc.extra_eye_candy->get< bool >( ) ) {
                g_function_caller->floating_text(
                    g_local->network_id,
                    _( "Activator: Cleanse" ),
                    m_text_type
                );
            }
            return true;
        }

        return false;
    }

    auto Activator::run_auto_qss( const ESpellSlot slot ) -> bool{
        if ( !g_config->activator.qss_toggle->get< bool >( ) ||
            g_config->activator.qss_smart->get< bool >( ) && g_features->orbwalker->get_mode( ) !=
            Orbwalker::EOrbwalkerMode::combo )
            return false;

        bool should_cast{ };
        bool prevent_cast{ };

        for ( const auto buff : g_features->buff_cache->get_all_buffs( g_local->index ) ) {
            if ( !buff ) continue;

            const auto type = static_cast< EBuffType >( buff->buff_data->type );
            bool       is_buff_cleansable{ };
            switch ( type ) {
            case EBuffType::snare:
                if ( !g_config->activator.cleanse_root->get< bool >( ) ) continue;

                is_buff_cleansable = true;
                break;
            case EBuffType::charm:
                if ( !g_config->activator.cleanse_charm->get< bool >( ) ) continue;

                is_buff_cleansable = true;
                break;
            case EBuffType::taunt:
                if ( !g_config->activator.cleanse_taunt->get< bool >( ) ) continue;
                is_buff_cleansable = true;
                break;
            case EBuffType::stun:
                if ( !g_config->activator.cleanse_stun->get< bool >( ) ) continue;
                is_buff_cleansable = true;
                break;
            case EBuffType::fear:
                if ( !g_config->activator.cleanse_fear->get< bool >( ) ) continue;
                is_buff_cleansable = true;
                break;
            case EBuffType::polymorph:
                if ( !g_config->activator.cleanse_polymorph->get< bool >( ) ) continue;
                is_buff_cleansable = true;
                break;
            case EBuffType::asleep:
                if ( !g_config->activator.cleanse_sleep->get< bool >( ) ) continue;
                is_buff_cleansable = true;
                break;
            case EBuffType::drowsy:
                if ( !g_config->activator.cleanse_drowsy->get< bool >( ) ) continue;
                is_buff_cleansable = true;
                break;
            case EBuffType::berserk:
                if ( !g_config->activator.cleanse_berserk->get< bool >( ) ) continue;
                is_buff_cleansable = true;
                break;
            case EBuffType::disarm:
                if ( !g_config->activator.cleanse_disarm->get< bool >( ) ) continue;
                is_buff_cleansable = true;
                break;
            case EBuffType::blind:
                if ( !g_config->activator.cleanse_blind->get< bool >( ) ) continue;
                is_buff_cleansable = true;
                break;
            case EBuffType::suppression:
                if ( !g_config->activator.cleanse_suppression->get< bool >( ) ) continue;
                is_buff_cleansable = true;
                break;
            case EBuffType::knockup:
            case EBuffType::knockback:
                prevent_cast = true;
                break;
            default:

                switch ( rt_hash( buff->name.c_str() ) ) {
                case ct_hash( "NasusW" ):
                    if ( !g_config->activator.cleanse_nasus_w->get< bool >( ) ) continue;
                    is_buff_cleansable = true;
                    break;
                case ct_hash( "mordekaiserr_statstealenemy" ):
                    if ( !g_config->activator.cleanse_mordekaiser_r->get< bool >( ) ) continue;
                    is_buff_cleansable = true;
                    break;
                case ct_hash( "SummonerExhaust" ):
                    if ( !g_config->activator.cleanse_exhaust->get< bool >( ) ) continue;
                    is_buff_cleansable = true;
                    break;
                case ct_hash( "SummonerDot" ):
                    if ( !g_config->activator.cleanse_ignite->get< bool >( ) ) continue;
                    is_buff_cleansable = true;
                    break;
                case ct_hash( "yasuorknockup" ):
                    if ( !g_config->activator.cleanse_yasuo_r->get< bool >( ) ) continue;
                    is_buff_cleansable = true;
                    break;
                default:
                    break;
                }

                break;
            }

            if ( prevent_cast ) break;

            if ( !should_cast && is_buff_cleansable ) {
                const auto duration_left = buff->buff_data->end_time - *g_time;
                const auto duration      = *g_time - buff->buff_data->start_time;

                if ( duration < g_config->activator.cleanse_delay->get< int >( ) / 1000.f || duration_left <=
                    0.3f )
                    continue;

                should_cast = true;
            }
        }


        if ( !should_cast || prevent_cast ) return false;

        if ( g_input->cast_spell( slot ) ) {
            m_last_cast_time = *g_time;
            if ( g_config->misc.extra_eye_candy->get< bool >( ) ) {
                g_function_caller->floating_text(
                    g_local->network_id,
                    _( "Activator: QSS" ),
                    m_text_type
                );
            }
            return true;
        }

        return false;
    }

    auto Activator::run_auto_potion( const ESpellSlot slot ) -> bool{
        if ( !g_config->activator.autopotion_toggle->get< bool >( ) ||
            g_config->activator.autopotion_disable_early->get< bool >( ) && *g_time <= 90.f )
            return false;

        // ItemCrystalFlask - refillable potion buff
        // ItemDarkCrystalFlask - corrupting potion buff
        // Item2003 - health potion buff

        const auto heal_threshold = g_config->activator.autopotion_health_threshold->get< int >( ) / 100.f * g_local->
            max_health;

        if ( g_local->health >= heal_threshold ) return false;

        if ( g_config->activator.autopotion_only_combo->get< bool >( ) &&
            g_features->orbwalker->get_mode( ) != Orbwalker::EOrbwalkerMode::combo
        )
            return false;


        // Not sure if this needs to be replaced with the above buff names instead, as it'd be more reliable? Not sure how many buffs give "heal"
        if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "ItemCrystalFlask" ) ) ||
            g_features->buff_cache->get_buff( g_local->index, ct_hash( "ItemDarkCrystalFlask" ) ) ||
            g_features->buff_cache->get_buff( g_local->index, ct_hash( "Item2003" ) )
        )
            return false;

        if ( g_input->cast_spell( slot ) ) {
            m_last_cast_time = *g_time;
            if ( g_config->misc.extra_eye_candy->get< bool >( ) ) {
                g_function_caller->floating_text(
                    g_local->network_id,
                    _( "Activator: Potion" ),
                    m_text_type
                );
            }
            return true;
        }


        return false;
    }

    auto Activator::run_auto_smite( const int buffed, const ESpellSlot slot ) -> bool{
        if ( !g_config->activator.smite_toggle->get< bool >( ) ) return false;

        const Object* target{ };
        bool          found;

        for ( const auto obj : g_entity_list.get_enemy_minions( ) ) {
            if ( !obj ||
                obj->is_dead( ) ||
                obj->dist_to_local( ) > 550.f + g_entity_list.get_bounding_radius( obj->index ) ||
                obj->is_invisible( ) ||
                !obj->is_jungle_monster( )
            )
                continue;

            const auto name_hash = rt_hash( obj->get_name().c_str() );

            found = name_hash == ct_hash( "SRU_RiftHerald" ) ||
                name_hash == ct_hash( "SRU_Baron" ) ||
                name_hash == ct_hash( "SRU_Dragon_Earth" ) ||
                name_hash == ct_hash( "SRU_Dragon_Air" ) ||
                name_hash == ct_hash( "SRU_Dragon_Water" ) ||
                name_hash == ct_hash( "SRU_Dragon_Fire" ) ||
                name_hash == ct_hash( "SRU_Dragon_Hextech" ) ||
                name_hash == ct_hash( "SRU_Dragon_Chemtech" ) ||
                name_hash == ct_hash( "SRU_Dragon_Elder" ) ||
                g_config->activator.smite_buffs->get< bool >( ) && ( name_hash == ct_hash( "SRU_Red" ) || name_hash ==
                    ct_hash( "SRU_Blue" ) ) ||
                g_config->activator.smite_crabs->get< bool >( ) && name_hash == ct_hash( "Sru_Crab" );


            if ( !found ) continue;

            target = obj;
            break;
        }

        if ( !target ) return false;

        float damage = 0;
        switch ( buffed ) {
        case 0:
            damage = 600.f;
            break;
        case 1:
            damage = 900.f;
            break;
        case 2:
            damage = 1200.f;
            break;
        default:
            break;
        }
        const auto health = g_config->activator.smite_predict_health->get< bool >( )
                                ? g_features->prediction->predict_health(
                                    target,
                                    g_features->orbwalker->get_ping( ) * 0.5f
                                )
                                : target->health;
        if ( health > damage || *g_time - m_last_cast_time <= 0.4f ) return false;

        if ( g_input->cast_spell( slot, target->network_id ) ) {
            m_last_cast_time = *g_time;

            //std::cout << "[ " << target->get_name() << " ] Smited HP: " << target->health << std::endl;
            if ( g_config->misc.extra_eye_candy->get< bool >( ) ) {
                g_function_caller->floating_text(
                    g_local->network_id,
                    _( "Activator: Smite" ),
                    m_text_type
                );
            }
            return true;
        }

        //std::cout << "[ " << target->get_name() << " ] cant smite?! " << std::endl;

        return false;
    }

    auto Activator::run_auto_heal( const ESpellSlot slot ) -> bool{
        if ( !g_config->activator.heal_toggle->get< bool >( ) ) return false;

        const auto heal_threshold = g_config->activator.heal_percent_threshold->get< int >( ) / 100.f * g_local->
            max_health;
        const auto health = g_config->activator.heal_predict->get< bool >( )
                                ? g_features->prediction->predict_health( g_local.get( ), 0.25f )
                                : g_local->health;
        if ( health >= heal_threshold ) return false;

        if ( g_config->activator.heal_logic->get< int >( ) > 0 && g_features->orbwalker->get_mode( ) !=
            Orbwalker::EOrbwalkerMode::combo ||
            g_config->activator.heal_logic->get< int >( ) == 2 && !GetAsyncKeyState( VK_CONTROL ) )
            return false;

        if ( g_input->cast_spell( slot ) ) {
            m_last_cast_time = *g_time;
            if ( g_config->misc.extra_eye_candy->get< bool >( ) ) {
                g_function_caller->floating_text(
                    g_local->network_id,
                    _( "Activator: Heal" ),
                    m_text_type
                );
            }

            return true;
        }

        return false;
    }

    auto Activator::run_auto_barrier( const ESpellSlot slot ) -> bool{
        if ( !g_config->activator.barrier_toggle->get< bool >( ) ) return false;

        const auto barrier_threshold = g_config->activator.barrier_percent_threshold->get< int >( ) / 100.f * g_local->
            max_health;
        const auto health = g_config->activator.barrier_predict->get< bool >( )
                                ? g_features->prediction->predict_health( g_local.get( ), 0.25f )
                                : g_local->health;
        if ( health >= barrier_threshold ) return false;

        if ( g_config->activator.barrier_logic->get< int >( ) > 0 && g_features->orbwalker->get_mode( ) !=
            Orbwalker::EOrbwalkerMode::combo ||
            g_config->activator.barrier_logic->get< int >( ) == 2 && !GetAsyncKeyState( VK_CONTROL ) )
            return false;


        if ( g_input->cast_spell( slot ) ) {
            m_last_cast_time = *g_time;

            if ( g_config->misc.extra_eye_candy->get< bool >( ) ) {
                g_function_caller->floating_text(
                    g_local->network_id,
                    _( "Activator: Barrier" ),
                    m_text_type
                );
            }
            return true;
        }

        return false;
    }

    auto Activator::run_auto_ignite( const ESpellSlot slot ) -> bool{
        if ( !g_config->activator.ignite_toggle->get< bool >( ) ||
            g_config->activator.ignite_mode->get< int >( ) > 0 && g_features->orbwalker->get_mode( ) !=
            Orbwalker::EOrbwalkerMode::combo
            || g_config->activator.ignite_mode->get< int >( ) == 2 && !GetAsyncKeyState( VK_CONTROL ) )
            return false;

        const auto target = g_features->target_selector->get_default_target( );
        if ( !target || target->dist_to_local( ) > 600.f ) return false;

        const auto damage = 50.f + 20.f * std::min( g_local->level, 18 );
        const auto health = g_config->activator.ignite_predict->get< bool >( )
                                ? g_features->prediction->predict_health( target, 0.5f, false, false, true )
                                : target->health;

        if ( damage <= target->health || health <= 0 ) return false;

        if ( g_input->cast_spell( slot, target->network_id ) ) {
            m_last_cast_time = *g_time;

            if ( g_config->misc.extra_eye_candy->get< bool >( ) ) {
                g_function_caller->floating_text(
                    g_local->network_id,
                    _( "Activator: Ignite" ),
                    m_text_type
                );
            }
            return true;
        }

        return false;
    }

    auto Activator::run_auto_exhaust( const ESpellSlot slot ) -> bool{
        if ( !g_config->activator.exhaust_toggle->get< bool >( ) || !g_config->activator.exhaust_hotkey->get<
            bool >( ) )
            return false;

        const Object* target{ };
        int           target_priority{ };
        float         target_damage_stat{ };

        for ( const auto enemy : g_entity_list.get_enemies( ) ) {
            if ( !enemy || enemy->dist_to_local( ) >= 650.f || g_features->target_selector->is_bad_target(
                enemy->index
            ) )
                continue;

            const auto priority    = g_features->target_selector->get_target_priority( enemy->champion_name.text );
            const auto damage_stat = enemy->attack_damage( ) > enemy->ability_power( )
                                         ? enemy->attack_damage( )
                                         : enemy->ability_power( );

            if ( priority < target_priority || priority == target_priority && damage_stat <
                target_damage_stat )
                continue;

            target             = enemy;
            target_priority    = priority;
            target_damage_stat = damage_stat;
        }

        if ( !target ) return false;


        if ( g_input->cast_spell( slot, target->network_id ) ) {
            m_last_cast_time = *g_time - 0.45f;

            if ( g_config->misc.extra_eye_candy->get< bool >( ) ) {
                g_function_caller->floating_text(
                    g_local->network_id,
                    _( "Activator: Exhaust" ),
                    m_text_type
                );
            }
            return true;
        }

        return false;
    }


    auto Activator::run_auto_everfrost( const ESpellSlot slot ) -> bool{
        if ( !g_config->activator.everfrost_toggle->get< bool >( ) ||
            g_config->activator.everfrost_only_in_combo->get< bool >( ) &&
            g_features->orbwalker->get_mode( ) != Orbwalker::EOrbwalkerMode::combo
        )
            return false;

        const auto target = g_features->target_selector->get_default_target( );
        if ( !target ) return false;

        const auto pred = g_features->prediction->predict( target->index, 800.f, 2000.f, 60.f, 0.3f );
        if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->activator.
            everfrost_hitchance->get< int >( ) ) )
            return false;

        if ( g_input->cast_spell( slot, pred.position ) ) {
            m_last_cast_time = *g_time;
            g_features->orbwalker->on_cast( );
            if ( g_config->misc.extra_eye_candy->get< bool >( ) ) {
                g_function_caller->floating_text(
                    g_local->network_id,
                    _( "Activator: Everfrost" ),
                    m_text_type
                );
            }
            return true;
        }

        return false;
    }

    auto Activator::run_auto_everfrost_antigapclose( const ESpellSlot slot ) -> bool{
        if ( !g_config->activator.everfrost_toggle->get< bool >( ) ) return false;
        constexpr auto range           = 800.f;
        constexpr auto speed           = 2000.f;
        constexpr auto width           = 60.f;
        constexpr auto delay           = 0.3f;
        const auto     source_position = g_local->position;
        constexpr auto edge_range      = true;
        const Object*  target          = { };

        for ( const auto enemy : g_entity_list.get_enemies( ) ) {
            if ( !enemy || enemy->dist_to_local( ) > ( range * 1.5f > 1000.f ? range * 1.5f : 1000.f ) ||
                g_features->target_selector->is_bad_target( enemy->index ) )
                continue;

            Vec3       cast_position{ };
            const auto bounding_radius =
                edge_range ? g_features->prediction->get_champion_radius( rt_hash( enemy->champion_name.text ) ) : 0.f;

            const auto dash_pred = g_features->prediction->predict_dash(
                enemy->index,
                range,
                speed,
                width + bounding_radius,
                delay,
                source_position
            );

            if ( !dash_pred.valid ) {
                const auto dash_spell_pred = g_features->prediction->predict_dash_from_spell(
                    enemy->index,
                    range,
                    speed,
                    width + bounding_radius,
                    delay,
                    source_position
                );

                if ( !dash_spell_pred.valid ) {
                    const auto blink_pred = g_features->prediction->predict_blink(
                        enemy->index,
                        range,
                        speed,
                        width + bounding_radius,
                        delay,
                        source_position
                    );

                    if ( !blink_pred.valid ) continue;

                    cast_position = blink_pred.position;
                } else cast_position = dash_spell_pred.position;
            } else cast_position = dash_pred.position;

            if ( source_position.dist_to( cast_position ) >= range ) continue;

            target = enemy;
        }

        if ( !target ) return false;

        auto pred = g_features->prediction->predict( target->index, 800.f, 2000.f, 60.f, 0.3f );
        if ( !pred.valid || ( int )pred.hitchance <= 3 ) return false;

        if ( g_input->cast_spell( slot, pred.position ) ) {
            m_last_cast_time = *g_time;
            g_features->orbwalker->on_cast( );
            if ( g_config->misc.extra_eye_candy->get< bool >( ) ) {
                g_function_caller->floating_text(
                    g_local->network_id,
                    _( "Activator: Everfrost" ),
                    m_text_type
                );
            }
            return true;
        }
        return false;
    }

    auto Activator::run_auto_goredrinker( const ESpellSlot slot ) -> bool{
        if ( !g_config->activator.goredrinker_toggle->get< bool >( ) ||
            g_config->activator.goredrinker_only_in_combo->get< bool >( ) &&
            g_features->orbwalker->get_mode( ) != Orbwalker::EOrbwalkerMode::combo )
            return false;

        Object*    target     = { };
        auto       count      = 0;
        const auto percent_hp = ( g_local->health / g_local->max_health ) * 100;

        for ( const auto enemy : g_entity_list.get_enemies( ) ) {
            if ( !enemy || g_features->target_selector->is_bad_target( enemy->index ) || enemy->dist_to_local( ) > 400
                || enemy->is_dead( ) )
                continue;

            count++;
        }

        if ( count < g_config->activator.goredrinker_min_hit->get< int >( ) ) return false;

        if ( !g_config->activator.goredrinker_HP_thresh_toggle->get< bool >( ) ) {
            if ( g_input->cast_spell( slot ) ) {
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }
        } else if ( percent_hp < g_config->activator.goredrinker_HP_thresh->get< int >( ) ) {
            if ( g_input->cast_spell( slot ) ) {
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }
        }
        return false;
    }

    auto Activator::run_auto_mikaels( const ESpellSlot slot ) -> bool{
        if ( !g_config->activator.mikaels_toggle->get< bool >( ) ) return false;

        bool prevent_cast{ };
        bool is_mikaelable{ };
        //unsigned int_32 ally_NID = g_local->network_id;
        unsigned int ally_NID{ };

        for ( const auto ally : g_entity_list.get_allies( ) ) {
            if ( !ally || !ally->is_alive( ) || ally->dist_to_local( ) > 650.f ) continue;


            for ( const auto buff : g_features->buff_cache->get_all_buffs( ally->index ) ) {
                if ( !buff ) continue;

                const auto type = static_cast< EBuffType >( buff->buff_data->type );

                switch ( type ) {
                case EBuffType::snare:
                case EBuffType::charm:
                case EBuffType::taunt:
                case EBuffType::stun:
                case EBuffType::fear:
                case EBuffType::polymorph:
                case EBuffType::asleep:
                case EBuffType::drowsy:
                case EBuffType::berserk:
                    ally_NID = ally->network_id;
                    is_mikaelable = true;
                    break;
                case EBuffType::disarm:
                case EBuffType::blind:
                case EBuffType::suppression:

                    is_mikaelable = false;
                    break;
                case EBuffType::knockup:
                case EBuffType::knockback:
                    prevent_cast = true;
                    break;
                default:

                    switch ( rt_hash( buff->name.c_str() ) ) {
                    case ct_hash( "NasusW" ):
                        is_mikaelable = true;
                        break;
                    case ct_hash( "mordekaiserr_statstealenemy" ):
                        is_mikaelable = true;
                        break;
                    case ct_hash( "SummonerExhaust" ):
                        is_mikaelable = true;
                        break;
                    case ct_hash( "SummonerDot" ): ;
                        is_mikaelable = true;
                        break;
                    case ct_hash( "yasuorknockup" ):
                        is_mikaelable = true;
                        break;
                    default:
                        break;
                    }

                    break;
                }

                if ( prevent_cast ) break;
            }
        }

        if ( prevent_cast || !is_mikaelable ) return false;

        if ( g_input->cast_spell( slot, ally_NID ) ) {
            m_last_cast_time = *g_time;
            g_features->orbwalker->on_cast( );
            return true;
        }
        return { };
    }

    auto Activator::run_auto_redemption( const ESpellSlot slot ) -> bool{
        Prediction::PredictionResult pred{ };
        if ( !g_config->activator.redemption_toggle->get< bool >( ) ) return false;

        for ( const auto ally : g_entity_list.get_allies( ) ) {
            if ( !ally || !ally->is_alive( ) || ally->dist_to_local( ) > 5500 ) continue;

            const float ally_percent_hp = ( ally->health / ally->max_health ) * 100;

            if ( ally_percent_hp > g_config->activator.redemption_ally_hp_threshold->get< int >( ) ) continue;

            pred = g_features->prediction->predict( ally->index, 5500.f, 0.f, 400.f, 0.5f );
        }
        if ( !pred.valid ) return false;

        if ( g_input->cast_spell( slot, pred.position ) ) {
            m_last_cast_time = *g_time;
            g_features->orbwalker->on_cast( );
            return true;
        }

        return true;
    }

    auto Activator::run_auto_ward_specific( const ESpellSlot slot ) -> bool{
        if ( !g_config->activator.wardhelper_toggle->get< bool >( ) ||
            g_features->orbwalker->get_mode( ) != Orbwalker::EOrbwalkerMode::none ||
            m_placing_ward ||
            !g_input->is_key_pressed( utils::EKey::N )
        )
            return false;

        const auto cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;

        bool should_ward{ };
        Vec3 walk_position{ };
        Vec3 cast_position{ };

        for ( auto i = 0; i < static_cast< int >( m_cast_position.size( ) ); i++ ) {
            auto ward_point = m_cast_position[ i ];
            auto walk_point = m_walk_position[ i ];

            if ( cursor.dist_to( ward_point ) > 150.f ||
                g_local->position.dist_to( ward_point ) < 500.f ||
                g_local->position.dist_to( ward_point ) > 1500.f ||
                g_local->position.dist_to( walk_point ) > 800.f
            )
                continue;

            walk_position = walk_point;
            cast_position = ward_point;
            should_ward   = true;
            break;
        }

        if ( !should_ward ) return false;

        if ( g_local->position.dist_to( cast_position ) <= 625.f && g_input->cast_spell( slot, cast_position ) ) {
            m_last_cast_time = *g_time;
            m_last_ward_time = *g_time;
            m_placing_ward   = false;
            return true;
        }

        if ( g_features->orbwalker->send_move_input( walk_position, true ) ) {
            m_placing_ward       = true;
            m_ward_cast_position = cast_position;
            m_ward_walk_position = walk_position;
            ward_slot            = slot;
            m_last_ward_time     = *g_time;
            return true;
        }

        return false;
    }

    auto Activator::run_ward_assist( ) -> void{
        if ( !m_placing_ward || !g_config->activator.wardhelper_toggle->get< bool >( ) ) return;

        if ( g_features->orbwalker->get_mode( ) != Orbwalker::EOrbwalkerMode::none || g_features->evade->
            is_active( ) ) {
            //std::cout << "ward action stopped\n";
            m_placing_ward = false;
            return;
        }

        if ( *g_time - m_last_ward_time < 0.1f ) return;

        auto aimgr = g_local->get_ai_manager( );
        if ( !aimgr ) {
            //std::cout << "bad aimgr, stop placing ward\n";
            m_placing_ward = false;
            return;
        }

        if ( aimgr->is_moving ) {
            const auto path = aimgr->get_path( );

            const auto path_end = path[ path.size( ) - 1 ];
            if ( path_end.dist_to( m_ward_walk_position ) > 25.f ) {
                //std::cout << "path not close to position, stop ward action\n";
                m_placing_ward = false;
                return;
            }

            return;
        }

        if ( g_local->position.dist_to( m_ward_walk_position ) > 5.f ) return;

        if ( g_input->cast_spell( ward_slot, m_ward_cast_position ) ) {
            m_placing_ward   = false;
            m_last_cast_time = *g_time;
            //std::cout << "ward placed\n";
        }
    }

    int get_killcount() {

        auto value = app->memory->read<int>( g_local.get_address( ) + 0x5610);

        if (!value.has_value()) return 0;

        return *value;
    }

    int get_champion_killcount( intptr_t address ){

        auto value = app->memory->read<int>(address + 0x5610);

        if (!value.has_value()) return 0;

        return *value;
    }

    auto Activator::auto_question( ) -> bool{
        if ( !m_initialized_chat ) {
            m_last_kills       = get_killcount();
            m_initialized_chat = true;

            std::cout << "initialized local kills: " << std::dec << m_last_kills << std::endl;
            return false;
        }

        if (get_killcount() == m_last_kills && !m_should_chat) return false;

        if (get_killcount() != m_last_kills)
        {
            if (get_killcount() > m_last_kills)
            {
                m_last_kill_time = *g_time;

                if ( g_config->misc.kill_effect_mode->get< int >( ) == 1 || g_config->misc.kill_effect_mode->get<
                    int >( ) == 3 ) {
                    const auto kill_sound_path = path::join(
                        { directory_manager::get_resources_path( ), "killsound.wav" }
                    );

                    if ( kill_sound_path.has_value( ) ) {
                        sdk::audio::play(
                            *kill_sound_path,
                            g_config->misc.audio_volume->get< float >( )
                        );
                    }
                }
            }

            //std::cout << "Kill found on local\n";

            m_should_chat      = get_killcount() > m_last_kills;
            m_last_change_time = *g_time;
            m_last_kills       = get_killcount();
            return false;
        }

        if ( g_config->misc.chat_taunt_on_kill->get< int >( ) == 0 || *g_time - m_last_chat_time <= 2.f ) return false;

        if ( *g_time - m_last_change_time <= 2.f ) return false;

        if ( *g_time - m_last_change_time > 4.f ) {
            m_should_chat = false;
            return false;
        }

        if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo ) {
            const auto target = g_features->target_selector->get_default_target( );
            if ( target && g_features->orbwalker->is_attackable( target->index ) ) return false;
        }

        const auto autofill_taunt = !m_autofill_quote && *g_time < 400.f;

        if ( autofill_taunt ) {
            g_function_caller->send_chat( _( "autofill ?" ), true );
            m_autofill_quote = true;
        } else {
            if ( g_config->misc.chat_taunt_on_kill->get< int >( ) == 2 ) {
                const int  random = rand( ) % m_taunts.size( );
                const auto msg    = m_taunts[ random ];

                g_function_caller->send_chat( msg.data( ), true );
            } else g_function_caller->send_chat( _( "?" ), true );
        }


        std::cout << "did kill event action\n";
        m_last_kills     = get_killcount();
        m_last_chat_time = *g_time;
        m_should_chat    = false;
        return false;
    }

    auto Activator::auto_encourage_ally( ) -> bool{
        if ( !g_config->misc.chat_encourage_allies->get< bool >( )
            || *g_time - m_last_chat_time <= 3.f || *g_time > 1500.f )
            return false;

        if ( m_allies_data.size( ) != g_entity_list.get_allies( ).size( ) - 1 ) {
            for ( auto ally : g_entity_list.get_allies( ) ) {
                if ( !ally || ally->network_id == g_local->network_id || is_ally_logged( ally->network_id ) ) continue;

                AllyData data;

                auto unit = g_entity_list.get_by_index(ally->index);
                if (!unit) continue;

                data.index       = ally->index;
                data.network_id  = ally->network_id;
                data.kill_count  = get_champion_killcount( unit.get_address( ) );
                data.initialized = true;

                m_allies_data.push_back( data );
            }
        } else {
            if ( !m_allies_initialized )
                debug_log( "[ AUTO-CHAT ]: Allies initialized" );
            m_allies_initialized = true;
        }

        auto can_encourage{ *g_time - m_last_encourage_time > 5.f };
        if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo ) {
            auto target = g_features->target_selector->get_default_target( );
            if ( target && g_features->orbwalker->is_attackable( target->index ) ) can_encourage = false;
        }

        if ( !m_can_chat ) return false;

        for ( auto& inst : m_allies_data ) {
            auto ally = g_entity_list.get_by_index( inst.index );
            if ( !ally ) continue;

            auto unit = g_entity_list.get_by_index(ally->index);
            if (!unit) continue;

            auto kills = get_champion_killcount(unit.get_address());

            if (kills == inst.kill_count && !inst.should_encourage || inst.validation_count >=
                5 )
                continue;

            if (kills != inst.kill_count)
            {
                if ( !inst.should_encourage && !ally->is_dead( ) ) { inst.total_kills = kills - inst.
                        kill_count;
                } else if ( inst.should_encourage && !ally->is_dead( ) ) { inst.total_kills += kills - inst.
                        kill_count;
                }

                int allies_near{ };
                for ( auto hero : g_entity_list.get_allies( ) ) {
                    if ( !hero || hero->is_dead( ) || hero->network_id == inst.network_id ||
                        hero->position.dist_to( ally->position ) > 1500.f )
                        continue;

                    ++allies_near;
                }

                inst.should_encourage = kills > m_last_kills && allies_near <= 2 &&
                    !is_support( rt_hash( ally->champion_name.text ) ) && *g_time - inst.last_encourage_time > 60.f;
                inst.last_kill_time = *g_time;
                inst.kill_count     = kills;
                continue;
            }

            if ( *g_time - inst.last_kill_time <= 2.f ) continue;

            if ( *g_time - inst.last_kill_time > 6.f ) {
                inst.should_encourage = false;
                inst.total_kills      = 0;
                continue;
            }

            if ( !can_encourage ) continue;

            std::string text{ ally->champion_name.text };

            text[ 0 ] = tolower( text[ 0 ] );

            switch ( rt_hash( ally->champion_name.text ) ) {
            case ct_hash( "AurelionSol" ):
                text = _( "asol" );
                break;
            case ct_hash( "Blitzcrank" ):
                text = _( "blitz" );
                break;
            case ct_hash( "Caitlyn" ):
                text = _( "cait" );
                break;
            case ct_hash( "Cassiopeia" ):
                text = _( "cassio" );
                break;
            case ct_hash( "Chogath" ):
                text = _( "cho" );
                break;
            case ct_hash( "DrMundo" ):
                text = _( "mundo" );
                break;
            case ct_hash( "Evelynn" ):
                text = _( "eve" );
                break;
            case ct_hash( "FiddleSticks" ):
                text = _( "fiddle" );
                break;
            case ct_hash( "Gangplank" ):
                text = _( "gp" );
                break;
            case ct_hash( "Hecarim" ):
                text = _( "heca" );
                break;
            case ct_hash( "Heimerdinger" ):
                text = _( "heimer" );
                break;
            case ct_hash( "JarvanIV" ):
                text = _( "j4" );
                break;
            case ct_hash( "Kassadin" ):
                text = _( "kassa" );
                break;
            case ct_hash( "Katarina" ):
                text = _( "kata" );
                break;
            case ct_hash( "KhaZix" ):
                text = _( "kha" );
                break;
            case ct_hash( "KogMaw" ):
                text = _( "kog" );
                break;
            case ct_hash( "KSante" ):
                text = _( "ksante" );
                break;
            case ct_hash( "Leblanc" ):
                text = _( "leb" );
                break;
            case ct_hash( "LeeSin" ):
                text = _( "lee" );
                break;
            case ct_hash( "Lucian" ):
                text = _( "luc" );
                break;
            case ct_hash( "Malphite" ):
                text = _( "malph" );
                break;
            case ct_hash( "Malzahar" ):
                text = _( "malz" );
                break;
            case ct_hash( "MasterYi" ):
                text = _( "yi" );
                break;
            case ct_hash( "MissFortune" ):
                text = _( "mf" );
                break;
            case ct_hash( "MonkeyKing" ):
                text = _( "wuk" );
                break;
            case ct_hash( "Mordekaiser" ):
                text = _( "morde" );
                break;
            case ct_hash( "Morgana" ):
                text = _( "morg" );
                break;
            case ct_hash( "Nautilus" ):
                text = _( "naut" );
                break;
            case ct_hash( "Nidalee" ):
                text = _( "nida" );
                break;
            case ct_hash( "Nocturne" ):
                text = _( "noc" );
                break;
            case ct_hash( "Orianna" ):
                text = _( "ori" );
                break;
            case ct_hash( "Pantheon" ):
                text = _( "panth" );
                break;
            case ct_hash( "RekSai" ):
                text = _( "reksai" );
                break;
            case ct_hash( "Rengar" ):
                text = _( "reng" );
                break;
            case ct_hash( "Renekton" ):
                text = _( "rene" );
                break;
            case ct_hash( "Samira" ):
                text = _( "sami" );
                break;
            case ct_hash( "Sejuani" ):
                text = _( "sej" );
                break;
            case ct_hash( "Seraphine" ):
                text = _( "sera" );
                break;
            case ct_hash( "Shyvana" ):
                text = _( "shyv" );
                break;
            case ct_hash( "TahmKench" ):
                text = _( "tahm" );
                break;
            case ct_hash( "Taliyah" ):
                text = _( "tali" );
                break;
            case ct_hash( "Tristana" ):
                text = _( "tris" );
                break;
            case ct_hash( "Tryndamere" ):
                text = _( "trynd" );
                break;
            case ct_hash( "TwistedFate" ):
                text = _( "tf" );
                break;
            case ct_hash( "VelKoz" ):
                text = _( "vkoz" );
                break;
            case ct_hash( "Viktor" ):
                text = _( "vik" );
                break;
            case ct_hash( "Vladimir" ):
                text = _( "vlad" );
                break;
            case ct_hash( "Volibear" ):
                text = _( "voli" );
                break;
            case ct_hash( "Warwick" ):
                text = _( "ww" );
                break;
            case ct_hash( "Xerath" ):
                text = _( "xer" );
                break;
            case ct_hash( "XinZhao" ):
                text = _( "xin" );
                break;
            case ct_hash( "Yasuo" ):
                text = _( "yas" );
                break;
            case ct_hash( "Zilean" ):
                text = _( "zil" );
                break;
            case ct_hash( "Qiyana" ):
                text = _( "qiya" );
                break;
            default:
                break;
            }

            auto god_random = rand( ) % 21;
            int  random     = rand( ) % m_encouragements.size( );

            std::string message;
            if ( god_random <= 1 ) message = text + " god";
            else message                   = m_encouragements[ random ] + " " + text;

            g_function_caller->send_chat( message.c_str( ), false );
            debug_log(
                "[ AUTO-CHAT ]: Validated {} for {} kills | T: {} | {}",
                text,
                inst.total_kills,
                *g_time,
                message
            );

            inst.kill_count          = kills;
            inst.should_encourage    = false;
            inst.last_encourage_time = *g_time;
            inst.total_kills         = 0;
            inst.validation_count++;

            m_last_chat_time = *g_time;
        }

        return true;
    }

    auto Activator::auto_log_cooldowns( ) -> bool{
        if ( g_config->misc.chat_spell_cooldowns->get< int >( ) == 0 || *g_time > m_max_say_cooldown_time ) {
            return
                false;
        }

        if ( m_enemies_data.size( ) < 5 ) {
            for ( auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || is_enemy_tracked( enemy->network_id ) ) continue;

                EnemySpellData data;

                data.index         = enemy->index;
                data.network_id    = enemy->network_id;
                data.champion_name = enemy->champion_name.text;

                m_enemies_data.push_back( data );
            }
        } else {
            if ( !m_enemy_spells_initialized )
                debug_log( "[ AUTO-CHAT ]: Enemy tracking initialized" );
            m_enemy_spells_initialized = true;
        }

        for ( auto& inst : m_enemies_data ) {
            auto enemy = g_entity_list.get_by_index( inst.index );
            if ( !enemy ) continue;

            if ( !inst.initialized ) {
                for ( auto i = 0; i < 3; i++ ) {
                    SpellInfo info{ };

                    switch ( i ) {
                    case 0:
                    {
                        auto spell = enemy->spell_book.get_spell_slot( ESpellSlot::r );
                        if ( !spell ) continue;

                        info.was_ready      = spell->is_ready( );
                        info.last_cast_time = spell->cooldown_expire;
                        info.initialized    = true;

                        inst.ultimate = info;
                        break;
                    }
                    case 1:
                    {
                        auto spell = enemy->spell_book.get_spell_slot( ESpellSlot::d );
                        if ( !spell ) continue;

                        info.was_ready      = spell->is_ready( );
                        info.last_cast_time = spell->cooldown_expire;
                        info.initialized    = true;

                        inst.summoner1 = info;
                        break;
                    }
                    case 2:
                    {
                        auto spell = enemy->spell_book.get_spell_slot( ESpellSlot::f );
                        if ( !spell ) continue;

                        info.was_ready      = spell->is_ready( );
                        info.last_cast_time = spell->cooldown_expire;
                        info.initialized    = true;

                        inst.summoner2 = info;
                        break;
                    }
                    default:
                        break;
                    }
                }


                inst.initialized = inst.ultimate.initialized && inst.summoner1.initialized && inst.summoner2.
                    initialized;

                if ( !inst.initialized ) continue;
            }

            for ( auto i = 0; i < 3; i++ ) {
                switch ( i ) {
                case 0:
                {
                    auto spell = enemy->spell_book.get_spell_slot( ESpellSlot::r );
                    if ( !spell ) continue;

                    // if ( false && inst.ultimate.was_ready && !spell->is_ready( ) ) {
                    //     SpellCooldownData data{ };
                    //
                    //     data.index         = inst.index;
                    //     data.network_id    = inst.network_id;
                    //     data.champion_name = inst.champion_name;
                    //
                    //     data.distance              = enemy->dist_to_local( );
                    //     data.slot                  = ESpellSlot::r;
                    //     data.last_cast_time        = spell->cooldown_expire - spell->cooldown;
                    //     data.ignore_distance_check = m_is_dead && *g_time - m_death_time >= 5.f;
                    //
                    //
                    //     //std::cout << "[ AUTO-CHAT ]: Logged cooldown " << enemy->champion_name.text << " on ult, time since cast: " << *g_time - data.last_cast_time << std::endl;
                    //     m_cooldowns.push_back( data );
                    // }

                    inst.ultimate.was_ready      = spell->is_ready( );
                    inst.ultimate.last_cast_time = spell->cooldown_expire;
                    break;
                }
                case 1:
                {
                    auto spell = enemy->spell_book.get_spell_slot( ESpellSlot::d );
                    if ( !spell ) continue;

                    if ( inst.summoner1.was_ready && !spell->is_ready( ) ) {
                        SpellCooldownData data{ };

                        data.index         = inst.index;
                        data.network_id    = inst.network_id;
                        data.champion_name = inst.champion_name;

                        data.distance              = enemy->dist_to_local( );
                        data.slot                  = ESpellSlot::d;
                        data.last_cast_time        = spell->cooldown_expire - spell->cooldown;
                        data.ignore_distance_check = m_is_dead && *g_time - m_death_time >= 5.f;

                        m_cooldowns.push_back( data );
                    }

                    inst.summoner1.was_ready      = spell->is_ready( );
                    inst.summoner1.last_cast_time = spell->cooldown_expire;
                    break;
                }
                case 2:
                {
                    auto spell = enemy->spell_book.get_spell_slot( ESpellSlot::f );
                    if ( !spell ) continue;

                    if ( inst.summoner2.was_ready && !spell->is_ready( ) ) {
                        SpellCooldownData data{ };

                        data.index         = inst.index;
                        data.network_id    = inst.network_id;
                        data.champion_name = inst.champion_name;

                        data.distance              = enemy->dist_to_local( );
                        data.slot                  = ESpellSlot::f;
                        data.last_cast_time        = spell->cooldown_expire - spell->cooldown;
                        data.ignore_distance_check = m_is_dead && *g_time - m_death_time >= 5.f;

                        m_cooldowns.push_back( data );
                    }

                    inst.summoner2.was_ready      = spell->is_ready( );
                    inst.summoner2.last_cast_time = spell->cooldown_expire;
                    break;
                }
                default:
                    break;
                }
            }
        }

        return { };
    }

    auto Activator::auto_say_cooldowns( ) -> bool{
        if ( g_config->misc.chat_spell_cooldowns->get< int >( ) == 0 || *g_time - m_last_chat_time <= 5.f
            || *g_time > m_max_say_cooldown_time || !m_can_chat )
            return false;

        for ( auto& inst : m_cooldowns ) {
            if ( *g_time - inst.last_cast_time > 20.f ) {
                debug_log( "[ AUTO-CHAT ]: Removed old cooldown {}", inst.champion_name );
                remove_cooldown( inst.network_id, inst.slot );
                continue;
            }

            if ( *g_time - inst.last_cast_time <= 5.f || inst.distance > 5000.f && !inst.
                ignore_distance_check )
                continue;

            const auto source = g_entity_list.get_by_index( inst.index );
            if ( !source ) continue;

            auto spell = source->spell_book.get_spell_slot( inst.slot );
            if ( !spell ) continue;

            bool should_say;
            switch ( rt_hash( spell->get_name( ).c_str() ) ) {
            case ct_hash( "SummonerFlash" ):
                should_say = true;
                break;
            case ct_hash( "SummonerExhaust" ):
            case ct_hash( "SummonerBarrier" ):
            case ct_hash( "SummonerDot" ):
                should_say = g_config->misc.chat_spell_cooldowns->get< int >( ) == 2;
                break;
            default:
                should_say = false;
                break;
            }

            if ( !should_say ) {
                remove_cooldown( inst.network_id, inst.slot );
                continue;
            }

            if ( g_features->tracker->
                             send_chat_cooldown( inst.champion_name, spell, inst.slot, inst.network_id, false ) ) {
                remove_cooldown( inst.network_id, inst.slot );
                m_last_chat_time = *g_time;
                break;
            }
        }

        return { };
    }

    auto Activator::auto_gank_warn( ) -> bool{
        if ( !m_jungler.initialized ) {
            for ( auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy ) continue;

                bool found_smite{ };
                for ( auto i = 0; i < 2; i++ ) {
                    auto slot = i == 0 ? ESpellSlot::d : ESpellSlot::f;

                    auto spell = enemy->spell_book.get_spell_slot( slot );
                    if ( !spell ) continue;

                    auto name = rt_hash( spell->get_name().c_str() );

                    switch ( name ) {
                    case ct_hash( "SummonerSmite" ):
                    case ct_hash( "S5_SummonerSmiteDuel" ):
                    case ct_hash( "S5_SummonerSmitePlayerGanker" ):
                    case ct_hash( "SummonerSmiteAvatarUtility" ):
                    case ct_hash( "SummonerSmiteAvatarOffensive" ):
                    case ct_hash( "SummonerSmiteAvatarDefensive" ):
                        found_smite = true;
                        break;
                    default:
                        break;
                    }

                    if ( found_smite ) break;
                }

                if ( !found_smite ) continue;

                m_jungler.index          = enemy->index;
                m_jungler.network_id     = enemy->network_id;
                m_jungler.last_seen_side = EMapSide::unknown;

                m_jungler.initialized = true;
                debug_log( "[ AUTO-CHAT ]: Gank warning initialized, tracked enemy: {}", enemy->champion_name.text );
                break;
            }

            if ( !m_jungler.initialized ) return false;
        }

        if ( *g_time < 120.f || g_config->misc.chat_auto_gank_warn->get< int >( ) == 0 || *g_time > 1000.f ) {
            return
                false;
        }

        auto& hero = g_entity_list.get_by_index( m_jungler.index );
        if ( !hero || hero->is_dead( ) ) return false;

        if ( hero->is_visible( ) ) {
            m_jungler.last_seen_time = *g_time;
            m_jungler.last_seen_side = get_mapside( hero->position );
        }

        if ( *g_time - m_last_chat_time <= 5.f || !m_can_chat ) return false;

        auto last_seen_data = g_features->tracker->get_last_seen_data( m_jungler.index );
        if ( !last_seen_data ) return false;

        auto position_data   = last_seen_data.value( );
        auto current_mapside = get_mapside( position_data.last_position );

        auto should_warn{
            g_config->misc.chat_auto_gank_warn->get< int >( ) < 3 && current_mapside != EMapSide::unknown && *g_time -
            position_data.last_seen_time <= 5.f
            && ( current_mapside != m_jungler.last_seen_side || *g_time - m_jungler.last_warn_time > 103.f )
        };

        if ( *g_time - m_jungler.last_warn_time <= 37.f ) return false;

        if ( !should_warn ) { // find jungler position from FOW camps
            current_mapside = get_mapside( m_last_camp_position );
            should_warn = g_config->misc.chat_auto_gank_warn->get< int >( ) == 1 && current_mapside != EMapSide::unknown
                && ( current_mapside != m_jungler.last_seen_side || *g_time - m_jungler.last_warn_time > 60.f ) && *
                g_time - m_last_camp_action_time <= 10.f;

            if ( !should_warn ) return false;
        }

        if ( hero->is_visible( ) || *g_time - m_jungler.last_seen_time <= 10.f ) {
            // find jungler position from last seen position (non FOW)
            auto closest_ally{ 9999999.f };

            for ( auto ally : g_entity_list.get_allies( ) ) {
                if ( !ally ) continue;

                if ( ally->position.dist_to( hero->position ) < closest_ally ) {
                    closest_ally = ally->position.dist_to(
                        hero->position
                    );
                }
            }

            if ( closest_ally <= 2000.f ) return false;
        }

        auto        name    = get_humanized_name( hero->champion_name.text );
        std::string message = _( "jg" );

        switch ( current_mapside ) {
        case EMapSide::top:
            message += " topside careful\n";
            break;
        case EMapSide::bot:
            message += " botside careful";
            break;
        default: ;
        }

        std::string mapside_name = current_mapside == EMapSide::bot ? "botside" : "topside";
        g_function_caller->send_chat( message.c_str( ), false );
        debug_log( "[ GANK ALERT ]: Alerted team on {} about {} | T: {}", mapside_name, name, *g_time );

        m_last_chat_time         = *g_time;
        m_jungler.last_warn_time = *g_time;
        m_jungler.last_seen_side = current_mapside;

        return true;
    }

    auto Activator::update_missing_enemies( ) -> void{
        if ( !g_config->misc.ping_missing->get< bool >( ) || *g_time <= 100.f ) return;

        for ( auto enemy : g_entity_list.get_enemies( ) ) {
            if ( !enemy || is_enemy_saved( enemy->network_id ) ) continue;

            TrackedEnemy instance{ enemy->index, enemy->network_id };

            m_tracked_enemies.push_back( instance );
        }

        for ( auto& inst : m_tracked_enemies ) {
            auto& enemy = g_entity_list.get_by_index( inst.index );
            if ( !enemy || enemy->is_dead( ) || enemy->is_visible( ) ) {
                inst.was_pinged = false;
                continue;
            }

            if ( inst.was_pinged ) continue;

            auto data = g_features->tracker->get_last_seen_data( enemy->index );
            if ( !data ) continue;

            auto seen_data = data.value( );

            auto ping_position = seen_data.path_end;

            if ( *g_time - seen_data.last_visible_time <= 3.5f || *g_time - inst.last_ping_time <= 15.f || *g_time -
                seen_data.last_visible_time > 20.f
                || ping_position.dist_to( m_last_ping_position ) <= 1500.f )
                continue;

            auto mapside = get_mapside( seen_data.last_visible_position );
            if ( mapside == EMapSide::unknown && ( *g_time - seen_data.last_visible_time <= 6.f || *g_time >
                1200.f ) )
                continue;

            if ( mapside != EMapSide::unknown ) add_ping_instance( ping_position, utils::ESendPingType::danger, 0.f );
            else add_ping_instance( ping_position, utils::ESendPingType::enemy_missing, 0.f );

            m_last_ping_position = ping_position;
            inst.was_pinged      = true;
            inst.last_ping_time  = *g_time;
        }
    }

    auto Activator::get_mapside( const Vec3& position ) -> EMapSide{
        if ( !m_initialized_map ) {
            for ( auto i = 0; i < 4; i++ ) {
                sdk::math::Polygon temp{ };

                switch ( i ) {
                case 0:
                    m_blueside_top_poly.points = m_blueside_top;
                    break;
                case 1:
                    m_blueside_bot_poly.points = m_blueside_bot;
                    break;
                case 2:
                    m_redside_top_poly.points = m_redside_top;
                    break;
                case 3:
                    m_redside_bot_poly.points = m_redside_bot;
                    break;
                default:
                    break;
                }
            }

            m_initialized_map = true;
        }


        if ( m_blueside_top_poly.is_inside( position ) || m_redside_top_poly.is_inside( position ) ) {
            return
                EMapSide::top;
        }
        if ( m_blueside_bot_poly.is_inside( position ) || m_redside_bot_poly.is_inside( position ) ) {
            return
                EMapSide::bot;
        }

        return EMapSide::unknown;
    }

    auto Activator::get_lane( const Vec3& position ) -> ELane{
        if ( !m_initialized_lanes ) {
            for ( auto i = 0; i < 3; i++ ) {
                sdk::math::Polygon temp{ };

                switch ( i ) {
                case 0:
                    m_toplane_poly.points = m_toplane;
                    break;
                case 1:
                    m_midlane_poly.points = m_midlane;
                    break;
                case 2:
                    m_botlane_poly.points = m_botlane;
                    break;
                default:
                    break;
                }
            }

            m_initialized_lanes = true;
        }

        if ( m_toplane_poly.is_inside( position ) ) return ELane::top;
        if ( m_midlane_poly.is_inside( position ) ) return ELane::mid;
        if ( m_botlane_poly.is_inside( position ) ) return ELane::bot;

        return ELane::none;
    }


    auto Activator::update_state( ) -> void{
        const auto can_say_chat = !g_features->evade->is_active( ) &&
            !g_local->spell_book.get_spell_cast_info( ) &&
            ( g_local->is_dead( ) ||
                g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::recalling ||
                g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::none
            );

        if ( !can_say_chat ) {
            m_can_chat  = false;
            m_idle_time = 0.f;
            return;
        }

        // check sci

        if ( m_idle_time == 0.f ) m_idle_time = *g_time;

        m_can_chat = *g_time - m_idle_time > 4.f;
    }

    auto Activator::ping_wards( ) -> bool{
        if ( !g_config->misc.ping_wards->get< bool >( ) ) return false;

        for ( auto& inst : m_pingable_wards ) {
            const auto& ward = g_entity_list.get_by_index( inst.index );
            if ( !ward || ward->is_dead( ) ) {
                m_pinged_wards.push_back( inst.network_id );
                remove_pingable_ward( inst.network_id );
                continue;
            }

            if ( inst.was_pinged || *g_time - inst.ping_time > 11.5f ) {
                m_pinged_wards.push_back( inst.network_id );
                remove_pingable_ward( inst.network_id );
                continue;
            }

            if ( !can_ping( ) /* || *g_time < inst.ping_time + inst.random_delay*/ || m_is_manual_pinging_active || !
                GetAsyncKeyState( 0x4D ) )
                continue;

            manual_ping_ward( world_to_minimap( ward->position ) );
            //g_function_caller->send_ping( ward->position, 0, utils::e_send_ping_type::ping );
            g_features->tracker->add_text_popup( g_local->index, _( "WARD: +5g" ), Color( 255, 255, 0 ) );

            m_total_ping_count++;
            // debug_log( "[ PING ] Pinged {} for gold | total income: {}", ward->get_name( ), m_total_ping_count * 5 );

            on_ping( );

            m_pinged_wards.push_back( inst.network_id );
            remove_pingable_ward( inst.network_id );
        }

        return false;
    }

    auto Activator::update_pings( ) -> void{
        for ( auto& inst : m_queued_pings ) {
            if ( *g_time - inst.ping_time > 10.f ) {
                remove_ping_instance( inst );
                continue;
            }

            if ( !can_ping( ) || inst.ping_time > *g_time ) continue;

            auto ping_position = inst.position;

            if ( inst.randomize ) {
                const auto direction_modifier = rand( ) % 70 + 1;
                const auto length_modifier    = rand( ) % 11;

                const auto angle  = -56.2f * static_cast< float >( direction_modifier );
                const auto length = 300.f / 10.f * static_cast< float >( length_modifier );


                ping_position = inst.position.extend( inst.position.rotated( angle ), length );
            }


            g_function_caller->send_ping( ping_position, 0, inst.type );
            on_ping( );

            remove_ping_instance( inst );
        }
    }

    auto Activator::on_ping( ) -> void{
        m_last_ping_time = *g_time;

        if ( !m_is_ping_timer_active ) m_is_ping_timer_active = true;

        m_ping_timer_count++;
    }



    auto Activator::remove_pingable_ward( const unsigned network_id ) -> void{
        const auto to_remove = std::ranges::remove_if(
            m_pingable_wards,
            [&]( const PingableWard& inst ) -> bool{ return inst.network_id == network_id; }
        );

        if ( to_remove.empty( ) ) return;

        m_pingable_wards.erase( to_remove.begin( ), to_remove.end( ) );
    }

    auto Activator::update_ping_timer( ) -> void{
        if ( !m_is_ping_timer_active ) return;

        if ( *g_time - m_last_ping_time > 5.f || *g_time - m_last_ping_time > 3.25f && m_ping_timer_count < 6 ) {
            //std::cout << "[ TIMER ] Ping timer was reset\n";

            m_is_ping_timer_active = false;
            m_ping_timer_count     = 0;
        }
    }

    auto Activator::get_humanized_name( const std::string& name ) const -> std::string{
        auto text{ name };

        text[ 0 ] = tolower( text[ 0 ] );

        switch ( rt_hash( name.data( ) ) ) {
        case ct_hash( "AurelionSol" ):
            text = _( "asol" );
            break;
        case ct_hash( "Blitzcrank" ):
            text = _( "blitz" );
            break;
        case ct_hash( "Caitlyn" ):
            text = _( "cait" );
            break;
        case ct_hash( "Cassiopeia" ):
            text = _( "cassio" );
            break;
        case ct_hash( "Chogath" ):
            text = _( "cho" );
            break;
        case ct_hash( "DrMundo" ):
            text = _( "mundo" );
            break;
        case ct_hash( "Evelynn" ):
            text = _( "eve" );
            break;
        case ct_hash( "Fiddlesticks" ):
            text = _( "fiddle" );
            break;
        case ct_hash( "Gangplank" ):
            text = _( "gp" );
            break;
        case ct_hash( "Heimerdinger" ):
            text = _( "heimer" );
            break;
        case ct_hash( "JarvanIV" ):
            text = _( "j4" );
            break;
        case ct_hash( "Kassadin" ):
            text = _( "kassa" );
            break;
        case ct_hash( "Katarina" ):
            text = _( "kata" );
            break;
        case ct_hash( "KhaZix" ):
            text = _( "kha" );
            break;
        case ct_hash( "KogMaw" ):
            text = _( "kog" );
            break;
        case ct_hash( "KSante" ):
            text = _( "ksante" );
            break;
        case ct_hash( "Leblanc" ):
            text = _( "leb" );
            break;
        case ct_hash( "LeeSin" ):
            text = _( "lee" );
            break;
        case ct_hash( "Malzahar" ):
            text = _( "malz" );
            break;
        case ct_hash( "MasterYi" ):
            text = _( "yi" );
            break;
        case ct_hash( "MissFortune" ):
            text = _( "mf" );
            break;
        case ct_hash( "MonkeyKing" ):
            text = _( "wukong" );
            break;
        case ct_hash( "Mordekaiser" ):
            text = _( "morde" );
            break;
        case ct_hash( "Nautilus" ):
            text = _( "naut" );
            break;
        case ct_hash( "Nidalee" ):
            text = _( "nida" );
            break;
        case ct_hash( "Orianna" ):
            text = _( "ori" );
            break;
        case ct_hash( "RekSai" ):
            text = _( "reksai" );
            break;
        case ct_hash( "Seraphine" ):
            text = _( "sera" );
            break;
        case ct_hash( "TahmKench" ):
            text = _( "tahm" );
            break;
        case ct_hash( "Tryndamere" ):
            text = _( "trynd" );
            break;
        case ct_hash( "TwistedFate" ):
            text = _( "tf" );
            break;
        case ct_hash( "VelKoz" ):
            text = _( "velkoz" );
            break;
        case ct_hash( "Vladimir" ):
            text = _( "vlad" );
            break;
        case ct_hash( "Volibear" ):
            text = _( "voli" );
            break;
        case ct_hash( "Warwick" ):
            text = _( "ww" );
            break;
        case ct_hash( "Xerath" ):
            text = _( "xer" );
            break;
        case ct_hash( "XinZhao" ):
            text = _( "xin" );
            break;
        default:
            break;
        }

        return text;
    }

    auto Activator::auto_cancerping( ) -> void{
        if ( !g_config->misc.ping_spam_ally->get< bool >( ) || !GetAsyncKeyState( 0x30 ) || !can_ping( ) ) return;

        const auto cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;

        unsigned       nid{ };
        Vec3           position{ };
        constexpr auto lowest_distance{ 9999.f };
        std::string    ally_name{ };
        for ( const auto ally : g_entity_list.get_allies( ) ) {
            if ( !ally || ally->is_dead( ) || ally->is_invisible( ) || ally->position.dist_to( cursor ) > 600.f
                || ally->position.dist_to( cursor ) > lowest_distance )
                continue;

            nid       = ally->network_id;
            position  = ally->position;
            ally_name = ally->champion_name.text;

            auto pred = g_features->prediction->predict_default( ally->index, 0.425f );
            if ( pred ) position = pred.value( );
        }

        if ( nid == 0 ) return;

        /*for (auto enemy : g_entity_list->get_enemies()) {
            if (!enemy) continue;

            nid = enemy->network_id;
            position = enemy->position;
            break;
        }*/

        const auto ping_type = g_config->misc.ping_spam_mode->get< int >( ) == 0
                                   ? utils::ESendPingType::enemy_missing
                                   : utils::ESendPingType::bait;

        //std::cout << "sended ping | " << *g_time << std::endl;
        g_function_caller->send_ping( position, nid, ping_type );

        const auto popup_text = "PING: " + ally_name;

        g_features->tracker->add_text_popup( g_local->index, popup_text, Color( 255, 255, 25 ) );

        on_ping( );
    }

    auto Activator::can_ping( ) const -> bool{ return *g_time - m_last_ping_time > 0.6f && m_ping_timer_count <= 4; }

    auto Activator::cancer_chat( ) -> void{
        if ( !GetAsyncKeyState( 0x30 ) && !m_chatting || *g_time - m_last_chat <= 0.05f
            || *g_time - m_last_time <= 1.f )
            return;

        g_function_caller->send_chat( FOURTEEN_WORDS[ m_chat_index ].c_str( ), true );


        ++m_chat_index;
        m_chatting  = true;
        m_last_chat = *g_time;

        if ( m_chat_index > 12 ) // 12
        {
            m_chat_index = 0;
            m_chatting   = false;
            m_last_time  = *g_time;
        }
    }

    auto Activator::is_ally_logged( const unsigned network_id ) const -> bool{
        for ( const auto& inst : m_allies_data ) if ( inst.network_id == network_id ) return true;

        return false;
    }

    auto Activator::is_enemy_tracked( const unsigned network_id ) const -> bool{
        for ( const auto& inst : m_enemies_data ) if ( inst.network_id == network_id ) return true;

        return false;
    }

    auto Activator::is_enemy_saved( const unsigned network_id ) const -> bool{
        for ( const auto& inst : m_tracked_enemies ) if ( inst.network_id == network_id ) return true;

        return false;
    }

    auto Activator::is_cooldown_logged( const unsigned network_id, const ESpellSlot slot ) const -> bool{
        for ( const auto& inst : m_cooldowns ) if ( inst.network_id == network_id && inst.slot == slot ) return true;

        return false;
    }

    auto Activator::remove_cooldown( const unsigned network_id, const ESpellSlot slot ) -> void{
        const auto to_remove = std::ranges::remove_if(
            m_cooldowns,
            [&]( const SpellCooldownData& cooldown ) -> bool{
                return cooldown.network_id == network_id && cooldown.slot == slot;
            }
        );

        if ( to_remove.empty( ) ) return;

        m_cooldowns.erase( to_remove.begin( ), to_remove.end( ) );
    }

    auto Activator::set_last_camp( const Vec3& position ) -> void{
        m_last_camp_action_time = *g_time;
        m_last_camp_position    = position;
    }

    auto Activator::is_support( const hash_t name ) -> bool{
        switch ( name ) {
        case ct_hash( "Yuumi" ):
        case ct_hash( "Soraka" ):
        case ct_hash( "Janna" ):
        case ct_hash( "Bard" ):
        case ct_hash( "Braum" ):
        case ct_hash( "Lulu" ):
        case ct_hash( "Leona" ):
        case ct_hash( "Nami" ):
        case ct_hash( "Rakan" ):
        case ct_hash( "Renata" ):
        case ct_hash( "Rell" ):
            return true;
        default:
            return false;
        }
    }

    auto Activator::add_pingable_ward( const int16_t index, const unsigned network_id ) -> void{
        PingableWard inst{ index, network_id, false, *g_time };

        const auto& ward = g_entity_list.get_by_index( index );
        if ( !ward ) inst.random_delay = static_cast< float >( rand( ) % 10 + 1 ) / 10.f;
        else {
            bool ally_nearby{ };
            for ( const auto ally : g_entity_list.get_allies( ) ) {
                if ( !ally || ally->is_dead( ) || ally->network_id == g_local->network_id || ally->position.dist_to(
                    ward->position
                ) > 1000.f )
                    continue;

                ally_nearby = true;
                break;
            }

            const auto max_delay = ally_nearby ? 10 : 40;
            inst.random_delay    = static_cast< float >( rand( ) % max_delay + 1 ) / 10.f;
        }


        m_pingable_wards.push_back( inst );
    }

    auto Activator::is_ward_pingable( const unsigned network_id ) const -> bool{
        for ( const auto nid : m_pinged_wards ) if ( nid == network_id ) return false;

        return true;
    }

    auto Activator::add_ping_instance(
        const Vec3&                position,
        const utils::ESendPingType type,
        const float                delay,
        const bool                 randomize_position
    ) -> void{
        const PingInstance inst{ position, type, *g_time + delay, randomize_position };

        m_queued_pings.push_back( inst );
    }

    auto Activator::remove_ping_instance( const PingInstance& instance ) -> void{
        const auto to_remove = std::ranges::remove_if(
            m_queued_pings,
            [&]( const PingInstance& inst ) -> bool{
                return inst.ping_time == instance.ping_time && instance.type == inst.type && instance.position.dist_to(
                    inst.position
                ) <= 1.f;
            }
        );

        if ( to_remove.empty( ) ) return;

        m_queued_pings.erase( to_remove.begin( ), to_remove.end( ) );
    }

    auto Activator::remove_ally_ward( const int16_t index ) -> void{
        const auto to_remove = std::ranges::remove_if(
            m_ally_wards,
            [ & ]( const WardOccurrence& inst ) -> bool{ return inst.index == index; }
        );

        if ( to_remove.empty( ) ) return;

        m_ally_wards.erase( to_remove.begin( ), to_remove.end( ) );
    }

    auto Activator::is_ward_tracked( const int16_t index ) const -> bool{
        for ( const auto ward : m_ally_wards ) if ( ward.index == index ) return true;

        return false;
    }



    auto Activator::anti_afk( ) -> void{
        if ( !g_config->misc.antiafk_toggle->get< bool >( ) || g_local->is_recalling( ) ) return;

        if ( g_local->position.dist_to( m_last_position ) >= 1.f ) {
            m_last_position        = g_local->position;
            m_last_position_update = *g_time;
        }

        if ( *g_time - m_last_position_update > m_next_update_delay && *g_time - m_last_path_time > 2.f ) {
            const auto direction_modifier = rand( ) % 70 + 1;
            const auto length_modifier    = rand( ) % 11;

            const auto angle  = -56.2f * static_cast< float >( direction_modifier );
            const auto length = 400.f / 10.f * static_cast< float >( length_modifier );

            m_next_update_delay = 30.f + static_cast< float >( rand( ) % 20 );

            const auto afk_path = g_local->position.extend( g_local->position.rotated( angle ), length );

            g_function_caller->issue_order_move( afk_path );
            m_last_path_time = *g_time;
        }
    }

    auto Activator::draw_ward_spots( ) const -> void{
        if ( !g_config->activator.wardhelper_toggle->get< bool >( ) ||
            g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo )
            return;

        for ( auto i = 0; i < static_cast< int >( m_cast_position.size( ) ); i++ ) {
            auto ward_point = m_cast_position[ i ];
            auto walk_point = m_walk_position[ i ];

            if ( g_local->position.dist_to( ward_point ) > 1500.f || g_local->position.dist_to( ward_point ) < 500.f ||
                g_local->position.dist_to( walk_point ) > 800.f )
                continue;

            g_render->circle_3d(
                ward_point,
                g_features->orbwalker->animate_color( Color( 52, 171, 235 ), EAnimationType::pulse, 4 ).alpha( 25 ),
                150.f,
                Renderer::outline | Renderer::filled,
                35,
                2.f
            );

            Vec2 sp{ };
            if ( !world_to_screen( ward_point, sp ) ) continue;

            g_render->text_shadow(
                { sp.x - 20.f, sp.y - 5.f },
                Color( 255, 255, 255 ),
                g_fonts->get_block( ),
                "WARD SPOT",
                8
            );
            // g_render->text_shadow({ sp.x - 20.f, sp.y + 10.f }, color(255, 255, 255), g_fonts->get_block(),
            // std::to_string(ward_point.dist_to(walk_point)).c_str(), 8);
            //  g_render->circle_3d(walk_point, color(25, 171, 60), 40.f, 2, 40, 2.f);
        }

        if ( !m_placing_ward ) return;

        Vec2 sp{ };
        if ( !world_to_screen( g_local->position, sp ) ) return;

        g_render->text_shadow(
            { sp.x, sp.y - 32.f },
            Color( 255, 255, 255 ),
            g_fonts->get_default_navbar( ),
            _( "PLACING WARD..." ),
            24.f
        );
    }

    auto Activator::draw_kill_efect( ) const -> void{
        constexpr auto duration = 1.5f;

        if ( g_config->misc.kill_effect_mode->get< int >( ) < 2 || *g_time - m_last_kill_time > duration ) return;

        const auto time_since_kill = *g_time - m_last_kill_time;
        auto       modifier        = std::min( time_since_kill / 1.f, 1.f );
        if ( modifier < 0.f ) modifier = 0.f;

        const auto kill_color = Color( 255.f, 255.f, 255.f, 40.f - 40.f * modifier );

        const Vec2 box_start = { 0.f, 0.f };
        const Vec2 box_size  = {
            static_cast< float >( g_render_manager->get_width( ) ),
            static_cast< float >( g_render_manager->get_height( ) )
        };

        g_render->filled_box( box_start, box_size, kill_color, -1 );
    }

    auto Activator::draw_autoward( ) const -> void{
        if ( !m_initialized_autoward || !g_config->activator.warding_toggle->get< bool >( ) ) return;

        const Vec2 texture_size{ 32.f, 32.f };

        Vec2 sp{ };

        const auto cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;

        for ( auto inst : m_ward_points ) {
            if ( inst.is_control_ward && ( !m_can_control_ward || m_control_ward_amount == 0 ) ||
                !inst.is_control_ward && ( !m_can_ward || m_ward_amount == 0 ) )
                continue;

            if ( !world_to_screen( inst.position, sp ) ) continue;

            auto is_point_valid{ true };
            for ( auto ward : m_ally_wards ) {
                if ( inst.position.dist_to( ward.position ) > 1100.f ||
                    inst.vision_area.is_outside( ward.position ) && ward.vision_area.is_outside( inst.position ) )
                    continue;

                is_point_valid = false;
                break;
            }

            if ( !is_point_valid ) continue;

            const auto distance = inst.position.dist_to( cursor );
            //float modifier = std::clamp(1.f - std::max(distance - 200.f, 0.f) / 300.f, 0.f, 1.f );

            static auto bold_control_ward_icon = path::join(
                {
                    directory_manager::get_resources_path( ),
                    "common",
                    "bold_control_ward_icon.png"
                }
            );
            static auto automatic_ward_icon = path::join(
                {
                    directory_manager::get_resources_path( ),
                    "common",
                    "automatic_ward_icon.png"
                }
            );

            static auto bold_ward_highlight = path::join(
                {
                    directory_manager::get_resources_path( ),
                    "common",
                    "bold_ward_highlight.png"
                }
            );

            static auto bold_ward_icon = path::join(
                {
                    directory_manager::get_resources_path( ),
                    "common",
                    "bold_ward_icon.png"
                }
            );

            const auto texture = inst.is_control_ward
                                     ? g_render->load_texture_from_file(
                                         bold_control_ward_icon.has_value( ) ? *bold_control_ward_icon : ""
                                     )
                                     : inst.is_automatic
                                           ? g_render->load_texture_from_file(
                                               automatic_ward_icon.has_value( ) ? *automatic_ward_icon : ""
                                           )
                                           : distance <= 250.f && inst.position.dist_to( g_local->position ) < 625.f &&
                                             g_config->activator.wardhelper_toggle->get< bool >( )
                                                 ? g_render->load_texture_from_file(
                                                     bold_ward_highlight.has_value( ) ? *bold_ward_highlight : ""
                                                 )
                                                 : g_render->load_texture_from_file(
                                                     bold_ward_icon.has_value( ) ? *bold_ward_icon : ""
                                                 );

            const auto fade_modifier = utils::ease::ease_in_out_cubic(
                std::clamp( ( inst.end_time - *g_time ) / 1.f, 0.f, 1.f )
            );

            const Vec2 draw_texture_size = { texture_size.x * fade_modifier, texture_size.y * fade_modifier };

            if ( texture ) {
                const auto texture_position =
                    Vec2{ sp.x - draw_texture_size.x / 2.f, sp.y - draw_texture_size.y / 2.f };
                g_render->image( texture_position, draw_texture_size, texture );
            }

            //if(distance < 500.f) g_render->circle_3d( inst.position, inst.is_bush ? color( 129, 201, 105 ) : color( 255, 255, 255 ),50.f,Renderer::outline,32, 2.f, angle );

            auto       text{ "" + std::to_string( inst.value ) };
            const auto size = g_render->get_text_size( text, g_fonts->get_zabel_16px( ), 16 * fade_modifier );

            g_render->text_shadow(
                { sp.x - size.x / 2.f, sp.y + draw_texture_size.y / 2.f },
                inst.is_control_ward
                    ? Color( 255.f, 0.f, 0.f, 255.f * fade_modifier )
                    : inst.is_automatic
                          ? Color( 25, 255, 25 )
                          : Color( 255.f, 255.f, 255.f, 255.f * fade_modifier ),
                g_fonts->get_zabel_16px( ),
                text.data( ),
                16 * fade_modifier
            );

            if ( *g_time - inst.last_ward_time <= 2.f ) {
                if ( *g_time - inst.last_ward_time <= 0.25f ) {
                    auto modifier = std::clamp( ( *g_time - inst.last_ward_time ) / 0.25f, 0.f, 1.f );
                    modifier      = utils::ease::ease_out_expo( modifier );

                    //g_render->polygon_3d( inst.vision_area, color( 255, 255, 255, 100 * modifier ), c_renderer::filled, 2.f );
                    g_render->polygon_3d( inst.vision_area, Color( 255.f, 255.f, 255.f, 255.f * modifier ), 2, 2.f );

                    continue;
                }

                auto modifier = std::clamp( ( *g_time - inst.last_ward_time ) / 1.75f, 0.f, 1.f );
                modifier      = utils::ease::ease_out_quint( modifier );

                g_render->polygon_3d(
                    inst.vision_area,
                    Color( 255.f, 255.f, 255.f, 255.f - 255.f * modifier ),
                    2,
                    2.f
                );
            }
        }
    }

    auto Activator::draw_controlward_reminder( ) const -> void{
        if ( !g_config->activator.warding_controlward_reminder->get< bool >( ) || g_local->level == 1 || g_local->
            current_gold <= 65 || !m_can_control_ward ||
            m_control_ward_amount > 0 || !g_menugui->is_shop_open( ) )
            return;

        if ( g_local->team == 100 ) {
            const sdk::math::Polygon area{
                {
                    Vec3{ 205.f, 182.f, 237.f },
                    Vec3{ 62.f, 183.f, 596.f },
                    Vec3{ 418.f, 176.f, 882.f },
                    Vec3{ 490.f, 101.f, 1351.f },
                    Vec3{ 835.f, 109.f, 1271.f },
                    Vec3{ 1158.f, 118.f, 988.f },
                    Vec3{ 1328.f, 100.f, 725.f },
                    Vec3{ 1370.f, 105.f, 399.f },
                    Vec3{ 1048.f, 162.f, 342.f },
                    Vec3{ 951.f, 168.f, 120.f },
                    Vec3{ 2488.f, 93.f, 108.f }
                }
            };

            if ( area.is_outside( g_local->position ) ) return;
        } else {
            const sdk::math::Polygon area{
                {
                    Vec3{ 12149.f, 93.f, 14762.f },
                    Vec3{ 14672.f, 171.f, 14402.f },
                    Vec3{ 14556.f, 168.f, 13952.f },
                    Vec3{ 14454.f, 147.f, 13661.f },
                    Vec3{ 14331.f, 106.f, 13389.f },
                    Vec3{ 13823.f, 112.f, 13569.f },
                    Vec3{ 13493.f, 110.f, 13872.f },
                    Vec3{ 13280.f, 98.f, 14212.f },
                    Vec3{ 13180.f, 93.f, 14488.f },
                    Vec3{ 13919.f, 171.f, 14581.f },
                    Vec3{ 12113.f, 93.f, 14730.f }
                }
            };

            if ( area.is_outside( g_local->position ) ) return;
        }

        const auto reminder_start = Vec2{
            g_render_manager->get_width( ) * 0.2f,
            g_render_manager->get_height( ) * 0.3f
        };

        const Vec2 texture_size = { 64.f, 64.f };

        static auto control_ward = path::join(
            {
                directory_manager::get_resources_path( ),
                "common",
                "control_ward.png"
            }
        );
        const auto texture = control_ward.has_value( )
                                 ? g_render->load_texture_from_file(
                                     *control_ward
                                 )
                                 : nullptr;
        if ( !texture ) return;

        const std::string text = " REMINDER ";
        const auto        size = g_render->get_text_size( text, g_fonts->get_zabel( ), 32 );

        const auto box_size = Vec2{ texture_size.x + size.x, texture_size.y };

        g_render->filled_box( reminder_start, box_size, Color( 10, 10, 10, 155 ) );
        g_render->image( reminder_start, texture_size, texture );

        g_render->box( reminder_start, box_size, g_features->orbwalker->get_pulsing_color( ), -1, 5.f );


        const auto text_position =
            Vec2{ reminder_start.x + texture_size.x, reminder_start.y + texture_size.y / 2.f - size.y / 2.f };

        g_render->text_shadow(
            text_position,
            g_features->orbwalker->get_pulsing_color( ),
            g_fonts->get_zabel( ),
            text.data( ),
            32
        );
    }


    // TrinketTotemLvl1 - normal ward
    // TrinketOrbLvl3 - farsight
    // TrinketSweeperLvl3 - sweeper

    auto Activator::run_auto_ward( ) -> void{
        if ( !g_config->activator.warding_toggle->get< bool >( ) ||
            !g_config->activator.warding_fully_automatic->get< bool >( ) || *g_time - m_last_ward_time <= 2.f ||
            !m_initialized_autoward || !m_can_ward && !m_can_control_ward )
            return;

        //std::cout << "CAN WARD: " << m_can_ward << " [ " << m_ward_amount << " ] "
        //          << " | CAN CONTROL: " << m_can_control_ward << " [ " << m_control_ward_amount << " ] \n";

        if ( m_can_ward && m_ward_amount > 0 ) {
            auto ward_spell = g_local->spell_book.get_spell_slot( m_ward_slot );
            if ( ward_spell && ward_spell->is_ready( ) ) {
                Vec3 highest_value_point{ };
                int  highest_value{ };

                for ( auto inst : m_ward_points ) {
                    if ( inst.position.dist_to( g_local->position ) > m_ward_range || !inst.is_automatic ||
                        inst.is_control_ward || inst.value < highest_value )
                        continue;

                    auto is_point_valid{ true };

                    for ( auto ward : m_ally_wards ) {
                        if ( inst.position.dist_to( ward.position ) > 1100.f ||
                            inst.vision_area.is_outside( ward.position ) &&
                            ward.vision_area.is_outside( inst.position ) )
                            continue;

                        is_point_valid = false;
                        break;
                    }

                    if ( !is_point_valid ) continue;

                    highest_value_point = inst.position;
                    highest_value       = inst.value;
                }

                if ( highest_value_point.length( ) > 0.f ) {
                    if ( g_input->cast_spell( m_ward_slot, highest_value_point ) ) {
                        m_last_cast_time = *g_time;
                        m_last_ward_time = *g_time;

                        m_last_autoward_position = highest_value_point;
                        m_last_autoward_time     = *g_time;

                        for ( auto& inst : m_ward_points ) {
                            if ( inst.position.dist_to( highest_value_point ) <= 1.f ) {
                                inst.last_ward_time = *g_time;
                                break;
                            }
                        }
                        return;
                    }
                }
            }

            if ( m_has_secondary_ward ) {
                ward_spell = g_local->spell_book.get_spell_slot( m_secondary_ward_slot );
                if ( ward_spell && ward_spell->is_ready( ) && ward_spell->charges > 0 ) {
                    Vec3 highest_value_point{ };
                    int  highest_value{ };

                    for ( auto inst : m_ward_points ) {
                        if ( inst.position.dist_to( g_local->position ) > m_ward_range || !inst.is_automatic ||
                            inst.is_control_ward || inst.value < highest_value )
                            continue;

                        auto is_point_valid{ true };

                        for ( auto ward : m_ally_wards ) {
                            if ( inst.position.dist_to( ward.position ) > 1100.f ||
                                inst.vision_area.is_outside( ward.position ) &&
                                ward.vision_area.is_outside( inst.position ) )
                                continue;

                            is_point_valid = false;
                            break;
                        }

                        if ( !is_point_valid ) continue;

                        highest_value_point = inst.position;
                        highest_value       = inst.value;
                    }

                    if ( highest_value_point.length( ) > 0.f ) {
                        if ( g_input->cast_spell( m_secondary_ward_slot, highest_value_point ) ) {
                            m_last_cast_time = *g_time;
                            m_last_ward_time = *g_time;

                            m_last_autoward_position = highest_value_point;
                            m_last_autoward_time     = *g_time;

                            for ( auto& inst : m_ward_points ) {
                                if ( inst.position.dist_to( highest_value_point ) <= 1.f ) {
                                    inst.last_ward_time = *g_time;
                                    break;
                                }
                            }

                            std::cout << "[ autoward ] placed ward to point with value: " << highest_value << std::endl;
                            return;
                        }
                    }
                }
            }
        }

        if ( m_can_control_ward && m_control_ward_amount > 0 ) {
            auto ward_spell = g_local->spell_book.get_spell_slot( m_control_slot );
            if ( !ward_spell ) return;

            Vec3 highest_value_point{ };
            int  highest_value{ };

            for ( auto inst : m_ward_points ) {
                if ( inst.position.dist_to( g_local->position ) > m_ward_range || !inst.is_control_ward ||
                    inst.value < highest_value )
                    continue;

                auto is_point_valid{ true };

                for ( auto ward : m_ally_wards ) {
                    if ( inst.position.dist_to( ward.position ) > 1100.f || inst.vision_area.is_outside( ward.position )
                        && ward.vision_area.is_outside( inst.position ) )
                        continue;

                    is_point_valid = false;
                    break;
                }

                if ( !is_point_valid ) continue;

                highest_value_point = inst.position;
                highest_value       = inst.value;
            }

            if ( highest_value_point.length( ) <= 0.f ) return;


            if ( g_input->cast_spell( m_control_slot, highest_value_point ) ) {
                m_last_cast_time = *g_time;
                m_last_ward_time = *g_time;

                m_last_autoward_position = highest_value_point;
                m_last_autoward_time     = *g_time;

                for ( auto& inst : m_ward_points ) {
                    if ( inst.position.dist_to( highest_value_point ) <= 1.f ) {
                        inst.last_ward_time = *g_time;
                        break;
                    }
                }

                std::cout << "[ autoward ] placed CONTROL WARD to point with value: " << highest_value << std::endl;
                return;
            }
        }
    }

    auto Activator::semi_manual_autoward( ) -> void{
        if ( !g_config->activator.warding_toggle->get< bool >( ) ||
            !g_config->activator.wardhelper_toggle->get< bool >( ) || *g_time - m_last_ward_time <= 2.f ||
            !m_initialized_autoward || !m_can_ward )
            return;


        if ( !GetAsyncKeyState( 0x4E ) ) return;

        const auto cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;
        WardPoint  wardpoint{ };
        bool       found_point{ };

        for ( auto inst : m_ward_points ) {
            if ( inst.position.dist_to( cursor ) > 250.f || inst.position.dist_to( g_local->position ) >=
                m_ward_range )
                continue;

            wardpoint   = inst;
            found_point = true;
            break;
        }

        if ( !found_point ) return;

        auto is_point_valid{ true };
        for ( auto ward : m_ally_wards ) {
            if ( wardpoint.position.dist_to( ward.position ) > 1100.f ||
                wardpoint.vision_area.is_outside( ward.position ) &&
                ward.vision_area.is_outside( wardpoint.position ) )
                continue;

            is_point_valid = false;
            break;
        }

        if ( !is_point_valid ) return;


        if ( g_input->cast_spell( m_ward_slot, wardpoint.position ) ) {
            m_last_cast_time = *g_time;
            m_last_ward_time = *g_time;

            m_last_autoward_position = wardpoint.position;
            m_last_autoward_time     = *g_time;

            for ( auto& inst : m_ward_points ) {
                if ( inst.position.dist_to( wardpoint.position ) <= 1.f ) {
                    inst.last_ward_time = *g_time;
                    break;
                }
            }

            std::cout << "[ autoward ] Semi-manual cast " << std::endl;

            return;
        }

        if ( !m_has_secondary_ward ) return;

        if ( g_input->cast_spell( m_secondary_ward_slot, wardpoint.position ) ) {
            m_last_cast_time = *g_time;
            m_last_ward_time = *g_time;

            m_last_autoward_position = wardpoint.position;
            m_last_autoward_time     = *g_time;

            for ( auto& inst : m_ward_points ) {
                if ( inst.position.dist_to( wardpoint.position ) <= 1.f ) {
                    inst.last_ward_time = *g_time;
                    break;
                }
            }

            std::cout << "[ autoward ] Semi-manual cast " << std::endl;
        }
    }

    auto Activator::update_auto_ward( ) -> void{
        if ( !g_config->activator.warding_toggle->get< bool >( ) ) return;

        update_ward_points( );
        if ( !m_initialized_autoward ) return;

        register_ward_points( );

        for ( const auto minion : g_entity_list.get_ally_minions( ) ) {
            if ( !minion || minion->is_dead( ) || !minion->is_ward( ) || is_ward_tracked( minion->index ) ) continue;

            const auto type = minion->get_ward_type( );
            switch ( type ) {
            case Object::EWardType::blue:
            case Object::EWardType::control:
            case Object::EWardType::zombie:
            case Object::EWardType::normal:
            case Object::EWardType::fiddlesticks_effigy:
                break;
            default:
                continue;
            }

            WardOccurrence occurrence{
                minion->index,
                minion->get_owner_index( ),
                type,
                minion->position,
                *g_time - ( minion->max_mana - minion->mana ),
                *g_time + minion->mana,
                type != Object::EWardType::blue && type != Object::EWardType::control
            };

            m_ally_wards.push_back( occurrence );
        }

        auto can_place_control{ true };
        int  wards_placed{ };

        for ( auto& ward : m_ally_wards ) {
            const auto object = g_entity_list.get_by_index( ward.index );
            if ( !object || object->is_dead( ) ) {
                remove_ally_ward( ward.index );
                continue;
            }

            if ( ward.vision_area.points.empty( ) ) {
                ward.vision_area = simulate_ward_vision(
                    ward.position,
                    ward.type == Object::EWardType::blue ? 650.f : 900.f
                );
            }

            if ( ward.owner_index == g_local->index ) {
                switch ( ward.type ) {
                case Object::EWardType::normal:
                    wards_placed++;
                    break;
                case Object::EWardType::control:
                    can_place_control = false;
                    break;
                default:
                    break;
                }
            }
        }

        m_can_ward         = wards_placed < 3;
        m_can_control_ward = can_place_control;

        int ward_count{ };
        int controlward_count{ };

        bool found_wardslot{ };
        bool found_secondary_wardslot{ };

        for ( auto i = 1; i < 8; i++ ) // cuz Inventory Slot starts at 1, so we iterate all, 7 is trinket so no check
        {
            const auto spell_slot_object = static_cast< ESpellSlot >( 5 + i );
            auto       spell_slot        = g_local->spell_book.get_spell_slot( spell_slot_object );

            if ( !spell_slot ) continue;

            auto slot = g_local->inventory.get_inventory_slot( i );
            if ( !slot ) continue;
            auto item_base = slot->get_base_item( );
            if ( !item_base ) continue;
            auto item_data = item_base->get_item_data( );
            if ( !item_data ) continue;

            switch ( static_cast< EItemId >( item_data->id ) ) {
            default:
                break;
            case EItemId::ward_trinket:
            case EItemId::frostfang:
            case EItemId::shard_of_true_ice:
            case EItemId::harrowing_cresent:
            case EItemId::blackmist_scythe:
                if ( spell_slot->charges == 0 && item_base->stacks_left == 0 ) break;

                ward_count += item_base->stacks_left > 0 ? item_base->stacks_left : spell_slot->charges;

                if ( !found_wardslot ) {
                    m_ward_slot    = spell_slot_object;
                    found_wardslot = true;
                } else {
                    m_secondary_ward_slot    = spell_slot_object;
                    found_secondary_wardslot = true;
                }
                break;
            case EItemId::control_ward:
                m_control_slot = spell_slot_object;
                controlward_count++;
                break;
            }
        }

        m_has_secondary_ward = found_secondary_wardslot;

        m_ward_amount         = ward_count;
        m_control_ward_amount = controlward_count;
    }

    auto Activator::update_ward_points( ) -> void{
        if ( !m_initialized_autoward || next_auto_ward_update_time <= *g_time ) {
            if ( m_initialized_autoward ) m_ward_points.clear( );
            else {
                m_ward_positions.push_back( Vec3{ 3135.f, -66.f, 10823.f } );
                m_ward_positions.push_back( Vec3{ 4317.f, -69.f, 9929.f } );
                m_ward_positions.push_back( Vec3{ 7547.f, 51.f, 9934.f } );
                m_ward_positions.push_back( Vec3{ 7828.f, 56.f, 11872.f } );
                m_ward_positions.push_back( Vec3{ 8199.f, 49.f, 10213.f } );
                m_ward_positions.push_back( Vec3{ 8243.f, -71.f, 6290.f } );
                m_ward_positions.push_back( Vec3{ 7977.f, 51.f, 3441.f } );
                m_ward_positions.push_back( Vec3{ 7157.f, 52.f, 3150.f } );
                m_ward_positions.push_back( Vec3{ 6618.f, 48.f, 4573.f } );
                m_ward_positions.push_back( Vec3{ 7362.f, 48.f, 4847.f } );
                m_ward_positions.push_back( Vec3{ 10089.f, 51.f, 7729.f } );
                m_ward_positions.push_back( Vec3{ 11640.f, 51.f, 7079.f } );
                m_ward_positions.push_back( Vec3{ 10712.f, 61.f, 8857.f } );
                m_ward_positions.push_back( Vec3{ 9013.f, 55.f, 11327.f } );
                m_ward_positions.push_back( Vec3{ 5031.f, 51.f, 7838.f } );
                m_ward_positions.push_back( Vec3{ 11956.f, 52.f, 7403.f } );
                m_ward_positions.push_back( Vec3{ 5646.f, 51.f, 3566.f } );
                m_ward_positions.push_back( Vec3{ 2983.f, 51.f, 8259.f } );
                m_ward_positions.push_back( Vec3{ 3199.f, 51.f, 7882.f } );
                m_ward_positions.push_back( Vec3{ 2933.f, 51.f, 7464.f } );
                m_ward_positions.push_back( Vec3{ 4649.f, 50.f, 7256.f } );
                m_ward_positions.push_back( Vec3{ 4041.f, 52.f, 5980.f } );
                m_ward_positions.push_back( Vec3{ 7703.f, 55.f, 11820.f } );
                m_ward_positions.push_back( Vec3{ 8253.f, 50.f, 5395.f } );
                m_ward_positions.push_back( Vec3{ 10492.f, 51.f, 2970.f } );
                m_ward_positions.push_back( Vec3{ 4639.f, 56.f, 11957.f } );
                m_ward_positions.push_back( Vec3{ 8954.f, 50.f, 2232.f } );
                m_ward_positions.push_back( Vec3{ 8194.f, -71.f, 6222.f } );
                m_ward_positions.push_back( Vec3{ 8337.f, 53.f, 3523.f } );
                m_ward_positions.push_back( Vec3{ 6403.f, 56.f, 11334.f } );
                m_ward_positions.push_back( Vec3{ 6722.f, -71.f, 8588.f } );
                m_ward_positions.push_back( Vec3{ 8567.f, -71.f, 6488.f } );
                m_ward_positions.push_back( Vec3{ 6249.f, -69.f, 8270.f } );
                m_ward_positions.push_back( Vec3{ 7993.f, 53.f, 4260.f } );
                m_ward_positions.push_back( Vec3{ 7134.f, 57.f, 5585.f } );
                m_ward_positions.push_back( Vec3{ 4147.f, 50.f, 7995.f } );
                m_ward_positions.push_back( Vec3{ 4045.f, 52.f, 6554.f } );
                m_ward_positions.push_back( Vec3{ 2977.f, 51.f, 8213.f } );
                m_ward_positions.push_back( Vec3{ 10461.f, -62.f, 5087.f } );
                m_ward_positions.push_back( Vec3{ 8072.f, 51.f, 3383.f } );
                m_ward_positions.push_back( Vec3{ 8349.f, 54.f, 3593.f } );
                m_ward_positions.push_back( Vec3{ 9103.f, 54.f, 3801.f } );
                m_ward_positions.push_back( Vec3{ 2833.f, 57.f, 5825.f } );

                int count{ };
                for ( const auto point : m_ward_positions ) {
                    m_developer_points.push_back( { point, count, false, false, false, { }, { }, 0.f, 10000.f } );
                    count++;
                }
            }

            if ( g_local->team == 100 ) {
                if ( *g_time < 180.f ) {
                    // bot river pixel bush
                    m_ward_points.push_back( { Vec3{ 9429.f, -71.f, 5635.f }, 45, true, false, true } );

                    // bot tribush
                    m_ward_points.push_back( { Vec3{ 10152.f, 49.f, 2965.f }, 25, true, false, true } );

                    // top river pixel bush
                    m_ward_points.push_back( { Vec3{ 5275.f, -71.f, 9156.f }, 45, true, false, true } );

                    // top jungle/mid connector bush
                    m_ward_points.push_back( { Vec3{ 8201.f, 49.f, 10366.f }, 70, true, false, true } );

                    // top raptor camp
                    m_ward_points.push_back( { Vec3{ 7715.f, 52.f, 9314.f }, 55, false, false, true } );

                    // top redbuff camp
                    m_ward_points.push_back( { Vec3{ 6896.f, 55.f, 10606.f }, 65, false, false, true } );

                    // bot jungle river entrance
                    m_ward_points.push_back( { Vec3{ 8542.f, 51.f, 4796.f }, 30, true, false, false } );

                    // bot bluebuff camp
                    m_ward_points.push_back( { Vec3{ 10786.f, 51.f, 7002.f }, 65, true, false, true } );

                    // bot river bush
                    m_ward_points.push_back( { Vec3{ 11756.f, -70.f, 4121.f }, 35, true, false, false } );

                    next_auto_ward_update_time = 180.f;
                } // 0 - 3 MINUTES
                else if ( *g_time < 360.f ) {
                    // BOTLANE ALLY TOWER FIRST BUSH
                    m_ward_points.push_back( { Vec3{ 12173.f, 51.f, 1305.f }, 30, true, false, true } );

                    // BOTLANE ENEMY TOWER FIRST BUSH
                    m_ward_points.push_back( { Vec3{ 13405.f, 51.f, 2493.f }, 35, true, false, true } );

                    // BOTLANE RIVER BUSH
                    m_ward_points.push_back( { Vec3{ 11792.f, -70.f, 4129.f }, 50, true, false, false } );

                    // BOT JUNGLE BLUE/GROMP
                    m_ward_points.push_back( { Vec3{ 11839.f, 51.f, 6640.f }, 75, false, false, true } );

                    // BOT JUNGLE GROMP/BLUE CONNECTOR TO RIVER
                    m_ward_points.push_back( { Vec3{ 11662.f, 50.f, 5923.f }, 55, true, false, false } );

                    // BOT TRIBUSH [CONTROL]
                    m_ward_points.push_back( { Vec3{ 10152.f, 49.f, 2965.f }, 35, true, true, true } );

                    // bot jungle entrance mid brush [CONTROL]
                    m_ward_points.push_back( { Vec3{ 8455.f, 52.f, 4922.f }, 50, true, true, true } );

                    // BOTSIDE MID RIVER DRAGON
                    m_ward_points.push_back( { Vec3{ 10450.f, -62.f, 5088.f }, 60, false, false, true } );

                    // REDSIDE JUNGLE RIVER ENTRANCE
                    m_ward_points.push_back( { Vec3{ 6810.f, 53.f, 9253.f }, 60, false, false, true } );

                    // TOPSIDE RIVER MID BUSH [ CONTROL ]
                    m_ward_points.push_back( { Vec3{ 5282.f, -66.f, 8584.f }, 50, true, true, true } );

                    // BOTSIDE RIVER MID BUSH
                    m_ward_points.push_back( { Vec3{ 9779.f, -44.f, 6282.f }, 60, true, false, false } );

                    // BOTSIDE RIVER PIXEL BUSH
                    m_ward_points.push_back( { Vec3{ 9356.f, -71.f, 5679.f }, 55, true, false, false } );

                    // TOPSIDE RIVER PIXEL BUSH
                    m_ward_points.push_back( { Vec3{ 5394.f, -71.f, 9146.f }, 55, true, false, false } );

                    // BOTSIDE MID/WOLVES CONNECTOR
                    m_ward_points.push_back( { Vec3{ 9778.f, 51.f, 7008.f }, 65, false, false, true } );

                    // TOPSIDE JUNGLE GROMP/BLUE CONNECTOR TO RIVER
                    m_ward_points.push_back( { Vec3{ 3062.f, 51.f, 9023.f }, 40, true, false, false } );

                    // TOPSIDE RED BUFF DEEP WARD
                    m_ward_points.push_back( { Vec3{ 6842.f, 53.f, 11520.f }, 80, true, false, true } );

                    // TOPSIDE TRIBUSH
                    m_ward_points.push_back( { Vec3{ 4392.f, 56.f, 11724.f }, 60, true, false, false } );

                    // TOPSIDE KRUG BUSH
                    m_ward_points.push_back( { Vec3{ 5926.f, 52.f, 12743.f }, 70, true, true, true } );

                    // TOPSIDE JUNGLE ENTRACE MID BUSH
                    m_ward_points.push_back( { Vec3{ 6336.f, 54.f, 10044.f }, 60, true, false, false } );

                    next_auto_ward_update_time = 360.f;
                } // 3 - 6 MINUTES
                else if ( *g_time < 600.f ) {
                    // 6 - 10 minutes
                    //  BOTLANE ALLY TOWER FIRST BUSH
                    m_ward_points.push_back( { Vec3{ 12173.f, 51.f, 1305.f }, 30, true, false, true } );

                    // CONTROL
                    m_ward_points.push_back( { m_ward_positions[ 6 ], 60, true, true, true } );
                    m_ward_points.push_back( { m_ward_positions[ 5 ], 50, true, true, true } );
                    m_ward_points.push_back( { m_ward_positions[ 9 ], 50, true, true, true } );
                    m_ward_points.push_back( { m_ward_positions[ 14 ], 60, true, true, true } );
                    m_ward_points.push_back( { m_ward_positions[ 17 ], 70, true, true, true } );

                    // AUTO
                    m_ward_points.push_back( { m_ward_positions[ 3 ], 75, true, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 7 ], 55, true, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 13 ], 65, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 15 ], 75, true, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 16 ], 60, true, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 20 ], 55, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 23 ], 80, true, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 24 ], 40, true, false, true } );

                    // NON AUTO
                    m_ward_points.push_back( { m_ward_positions[ 2 ], 40, true, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 4 ], 55, true, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 11 ], 45, true, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 12 ], 55, true, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 18 ], 65, true, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 19 ], 50, true, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 51 ], 45, true, false, false } );


                    next_auto_ward_update_time = 600.f;
                } else if ( *g_time < 900.f ) {
                    // CONTROL
                    m_ward_points.push_back( { m_ward_positions[ 6 ], 60, true, true, true } );
                    m_ward_points.push_back( { m_ward_positions[ 5 ], 50, true, true, true } );
                    m_ward_points.push_back( { m_ward_positions[ 9 ], 50, true, true, true } );
                    m_ward_points.push_back( { m_ward_positions[ 14 ], 60, true, true, true } );
                    m_ward_points.push_back( { m_ward_positions[ 17 ], 70, true, true, true } );

                    // AUTOMATIC
                    m_ward_points.push_back( { m_ward_positions[ 3 ], 75, true, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 7 ], 65, true, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 18 ], 60, true, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 20 ], 65, true, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 23 ], 85, true, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 29 ], 80, true, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 31 ], 80, true, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 34 ], 90, true, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 41 ], 80, true, false, true } );

                    // MANUAL
                    m_ward_points.push_back( { m_ward_positions[ 4 ], 60, true, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 8 ], 55, true, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 10 ], 55, true, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 12 ], 55, true, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 13 ], 60, true, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 15 ], 75, true, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 16 ], 60, true, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 21 ], 65, true, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 32 ], 70, true, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 46 ], 45, true, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 51 ], 45, true, false, false } );


                    next_auto_ward_update_time = 900.f;
                } else if ( *g_time < 12000.f ) {
                    // CONTROL
                    m_ward_points.push_back( { m_ward_positions[ 6 ], 60, true, true, true } );
                    m_ward_points.push_back( { m_ward_positions[ 5 ], 50, true, true, true } );
                    m_ward_points.push_back( { m_ward_positions[ 9 ], 50, true, true, true } );
                    m_ward_points.push_back( { m_ward_positions[ 17 ], 70, true, true, true } );
                    m_ward_points.push_back( { m_ward_positions[ 30 ], 80, true, true, true } );

                    // AUTOMATIC
                    m_ward_points.push_back( { m_ward_positions[ 7 ], 65, true, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 12 ], 55, true, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 13 ], 65, true, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 15 ], 75, true, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 18 ], 60, true, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 20 ], 65, true, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 23 ], 85, true, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 29 ], 80, true, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 34 ], 90, true, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 41 ], 80, true, false, true } );

                    // MANUAL
                    m_ward_points.push_back( { m_ward_positions[ 3 ], 75, true, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 4 ], 60, true, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 8 ], 55, true, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 10 ], 55, true, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 11 ], 55, true, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 14 ], 75, true, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 16 ], 60, true, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 21 ], 65, true, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 31 ], 80, true, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 32 ], 70, true, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 46 ], 45, true, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 48 ], 70, true, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 51 ], 45, true, false, false } );

                    next_auto_ward_update_time = 12000.f;
                }
            } else {
                if ( *g_time < 180.f ) {
                    // AUTO
                    m_ward_points.push_back( { m_ward_positions[ 25 ], 75, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 27 ], 80, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 52 ], 70, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 53 ], 60, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 54 ], 70, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 55 ], 65, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 56 ], 90, false, false, true } );

                    // MANUAL
                    m_ward_points.push_back( { m_ward_positions[ 11 ], 50, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 12 ], 50, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 18 ], 40, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 19 ], 35, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 20 ], 40, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 28 ], 65, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 43 ], 40, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 44 ], 30, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 45 ], 55, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 53 ], 60, false, false, false } );

                    next_auto_ward_update_time = 180.f;
                } else if ( *g_time < 360.f ) {
                    // CONTROL
                    m_ward_points.push_back( { m_ward_positions[ 4 ], 40, true, true, true } );
                    m_ward_points.push_back( { m_ward_positions[ 10 ], 55, true, true, true } );
                    m_ward_points.push_back( { m_ward_positions[ 18 ], 50, true, true, true } );
                    m_ward_points.push_back( { m_ward_positions[ 39 ], 75, true, true, true } );
                    m_ward_points.push_back( { m_ward_positions[ 44 ], 45, true, true, true } );
                    m_ward_points.push_back( { m_ward_positions[ 45 ], 65, true, true, true } );

                    // AUTO
                    m_ward_points.push_back( { m_ward_positions[ 6 ], 60, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 20 ], 65, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 26 ], 75, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 27 ], 75, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 33 ], 65, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 40 ], 65, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 43 ], 45, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 53 ], 60, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 56 ], 80, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 57 ], 65, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 58 ], 70, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 60 ], 65, false, false, true } );


                    // MANUAL
                    m_ward_points.push_back( { m_ward_positions[ 9 ], 55, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 11 ], 50, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 12 ], 50, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 14 ], 60, true, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 19 ], 45, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 28 ], 65, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 42 ], 55, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 49 ], 45, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 50 ], 45, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 53 ], 60, false, false, false } );

                    next_auto_ward_update_time = 360.f;
                } else if ( *g_time < 600.f ) {
                    // CONTROL
                    m_ward_points.push_back( { m_ward_positions[ 4 ], 40, true, true, true } );
                    m_ward_points.push_back( { m_ward_positions[ 10 ], 55, true, true, true } );
                    m_ward_points.push_back( { m_ward_positions[ 18 ], 60, true, true, true } );
                    m_ward_points.push_back( { m_ward_positions[ 39 ], 75, true, true, true } );
                    m_ward_points.push_back( { m_ward_positions[ 44 ], 45, true, true, true } );
                    m_ward_points.push_back( { m_ward_positions[ 45 ], 65, true, true, true } );

                    // AUTO
                    m_ward_points.push_back( { m_ward_positions[ 6 ], 65, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 14 ], 65, true, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 19 ], 45, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 20 ], 65, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 26 ], 75, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 27 ], 75, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 33 ], 65, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 40 ], 65, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 43 ], 45, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 53 ], 60, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 57 ], 65, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 58 ], 70, false, false, true } );


                    // MANUAL
                    m_ward_points.push_back( { m_ward_positions[ 9 ], 55, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 11 ], 50, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 12 ], 50, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 28 ], 65, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 42 ], 55, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 49 ], 45, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 50 ], 45, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 53 ], 50, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 56 ], 75, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 60 ], 60, false, false, false } );


                    next_auto_ward_update_time = 600.f;
                } else if ( *g_time < 1100.f ) {
                    // CONTROL
                    m_ward_points.push_back( { m_ward_positions[ 4 ], 40, true, true, true } );
                    m_ward_points.push_back( { m_ward_positions[ 10 ], 55, true, true, true } );
                    m_ward_points.push_back( { m_ward_positions[ 18 ], 60, true, true, true } );
                    m_ward_points.push_back( { m_ward_positions[ 37 ], 75, false, true, true } );
                    m_ward_points.push_back( { m_ward_positions[ 44 ], 45, true, true, true } );
                    m_ward_points.push_back( { m_ward_positions[ 45 ], 65, true, true, true } );


                    // AUTO
                    m_ward_points.push_back( { m_ward_positions[ 6 ], 65, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 20 ], 65, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 26 ], 75, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 27 ], 75, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 38 ], 90, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 39 ], 75, true, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 40 ], 65, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 43 ], 45, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 53 ], 60, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 57 ], 65, false, false, true } );


                    // MANUAL
                    m_ward_points.push_back( { m_ward_positions[ 11 ], 50, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 12 ], 50, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 14 ], 65, true, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 19 ], 45, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 28 ], 65, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 33 ], 55, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 35 ], 70, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 42 ], 55, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 49 ], 45, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 50 ], 45, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 53 ], 50, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 56 ], 75, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 58 ], 70, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 60 ], 60, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 61 ], 75, false, false, false } );


                    next_auto_ward_update_time = 1100.f;
                } else {
                    // CONTROL
                    m_ward_points.push_back( { m_ward_positions[ 4 ], 55, true, true, true } );
                    m_ward_points.push_back( { m_ward_positions[ 18 ], 60, true, true, true } );
                    m_ward_points.push_back( { m_ward_positions[ 23 ], 65, true, true, true } );
                    m_ward_points.push_back( { m_ward_positions[ 29 ], 65, true, true, true } );
                    m_ward_points.push_back( { m_ward_positions[ 30 ], 70, true, true, true } );
                    m_ward_points.push_back( { m_ward_positions[ 37 ], 75, false, true, true } );
                    m_ward_points.push_back( { m_ward_positions[ 41 ], 75, false, true, true } );

                    // AUTO
                    m_ward_points.push_back( { m_ward_positions[ 6 ], 75, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 14 ], 70, true, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 18 ], 60, true, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 20 ], 70, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 26 ], 85, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 27 ], 80, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 35 ], 70, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 38 ], 90, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 39 ], 80, true, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 43 ], 65, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 53 ], 60, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 57 ], 70, false, false, true } );
                    m_ward_points.push_back( { m_ward_positions[ 61 ], 75, false, false, true } );

                    // MANUAL
                    m_ward_points.push_back( { m_ward_positions[ 4 ], 50, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 11 ], 60, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 10 ], 55, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 12 ], 65, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 15 ], 55, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 19 ], 45, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 22 ], 50, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 23 ], 55, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 28 ], 65, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 33 ], 55, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 34 ], 60, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 40 ], 75, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 42 ], 55, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 49 ], 45, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 50 ], 45, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 53 ], 50, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 56 ], 75, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 58 ], 70, false, false, false } );
                    m_ward_points.push_back( { m_ward_positions[ 60 ], 60, false, false, false } );


                    next_auto_ward_update_time = 99999.f;
                }
            }

            m_registration_completed = false;

            m_initialized_autoward = true;
        }
    }

    auto Activator::register_ward_points( ) -> void{
        if ( m_registration_completed ) return;

        m_register_start_time = *g_time;

        bool omitted{ };

        for ( auto& inst : m_ward_points ) {
            if ( inst.is_registered ) continue;


            if ( *g_time - m_register_start_time >= 0.033f ) {
                omitted = true;
                break;
            }

            inst.vision_area = simulate_ward_vision( inst.position, 900.f );
            // inst.farsight_vision_area = simulate_ward_vision( inst.position, 650.f );
            inst.end_time      = next_auto_ward_update_time;
            inst.is_registered = true;
        }

        m_registration_completed = !omitted;

        if ( m_registration_completed ) std::cout << "[ Auto-ward ] Registration finished\n";
        else std::cout << "[ Auto-ward ] Registration paused, continuing later\n";
    }


    auto Activator::simulate_ward_vision( const Vec3& position, const float vision_radius ) -> sdk::math::Polygon{
        const auto pos = position;

        auto       bypass_poly = Circle( pos, vision_radius ).to_polygon( 0, -1, 32 );
        const auto in_bush     = static_cast< int >( g_navgrid->get_collision( pos ) ) &
                                 static_cast< int >( ECollisionFlag::grass )
                                     ? true
                                     : false;

        constexpr auto bypass_angle  = 120.f;
        constexpr auto bypass_radius = 70.f;

        for ( auto& point : bypass_poly.points ) {
            auto brushPhase = in_bush ? 1 : 0;
            int  bypass_count{ };

            for ( auto i = 1; i <= 100; i++ ) {
                auto check           = pos.extend( point, static_cast< float >( i ) * ( vision_radius / 100.f ) );
                check.y              = g_navgrid->get_height( check );
                const auto collision = static_cast< int >( g_navgrid->get_collision( check ) );

                if ( collision & static_cast< int >( ECollisionFlag::wall ) ) {
                    auto polygon =
                        Sector( check, check.extend( pos, -50.f ), bypass_angle, bypass_radius ).to_polygon_new( );
                    bool found_bypass{ };

                    for ( auto& p : polygon.points ) {
                        if ( p.dist_to( check ) <= 5.f ) continue;

                        const auto coll = static_cast< int >( g_navgrid->get_collision( p ) );
                        if ( coll & static_cast< int >( ECollisionFlag::wall ) ) continue;

                        bypass_count++;
                        found_bypass = true;
                        break;
                    }

                    if ( found_bypass ) continue;

                    point = check;
                    break;
                }

                if ( collision & static_cast< int >( ECollisionFlag::grass ) ) {
                    if ( brushPhase == 2 || brushPhase == 0 ) {
                        auto polygon =
                            Sector( check, check.extend( pos, -50.f ), bypass_angle, bypass_radius ).to_polygon_new( );
                        bool found_bypass{ };

                        for ( auto& p : polygon.points ) {
                            if ( p.dist_to( check ) <= 5.f ) continue;

                            const auto coll = static_cast< int >( g_navgrid->get_collision( p ) );
                            if ( coll & static_cast< int >( ECollisionFlag::wall ) ||
                                coll & static_cast< int >( ECollisionFlag::grass ) )
                                continue;

                            bypass_count++;
                            found_bypass = true;
                            break;
                        }

                        if ( found_bypass ) continue;

                        point = check;
                        break;
                    }
                } else if ( brushPhase == 1 ) brushPhase = 2;
            }
        }

        return bypass_poly;
    }

    auto Activator::draw_vision_score( ) const -> void{
        if ( !g_config->misc.show_vision_score->get< bool >( ) || *g_time - m_vision_score_change_time > 3.f ) return;

        Vec2 sp;
        if ( !world_to_screen( g_local->position, sp ) ) return;

        auto       base_position = g_local->get_hpbar_position( );
        const auto bar_length    = static_cast< float >( g_render_manager->get_width( ) ) * 0.0546875f;

        base_position.x -= bar_length * 0.75f;
        base_position.y -= static_cast< float >( g_render_manager->get_height( ) ) * 0.008f;

        //g_render->filled_circle( base_position, Color( 255, 255, 255 ), 2.f, 16 );

        std::string text = "+";
        auto        data = std::to_string( m_vision_score_addition );
        data.resize( 4 );
        text += data;

        const auto size          = g_render->get_text_size( text, g_fonts->get_zabel( ), 20 );
        Vec2       text_position = { base_position.x - size.x, base_position.y - size.y };

        auto text_color = Color( 78, 107, 252 );

        if ( *g_time - m_vision_score_change_time < 1.25f ) {
            const auto modifier      = std::clamp( 1.f - ( *g_time - m_vision_score_change_time ) / 1.25f, 0.f, 1.f );
            const auto alt_modifier  = utils::ease::ease_in_quint( modifier );
            const auto fade_modifier = utils::ease::ease_out_quint( modifier );

            text_color.r += 177 * fade_modifier;
            text_color.g += 148 * fade_modifier;
            text_color.b += 3 * fade_modifier;

            text_color.alpha( 255 - 255 * alt_modifier );
            text_position.x -= size.x * alt_modifier;
        } else if ( *g_time - m_vision_score_change_time >= 2.25f ) {
            const auto opacity_modifier = std::clamp(
                ( *g_time - m_vision_score_change_time - 2.25f ) / 0.75f,
                0.f,
                1.f
            );

            const auto opacity = 255.f - 255.f * opacity_modifier;

            text_color.alpha( opacity );
        }

        g_render->text_shadow( text_position, text_color, g_fonts->get_zabel( ), text.data( ), 20 );
    }

    auto Activator::draw_pingable_ward_indicator( ) const -> void{
        if ( !g_config->misc.ping_wards->get< bool >( ) || !g_config->misc.draw_pingable_indicator->get< bool >( ) ||
            m_pingable_wards.empty( ) )
            return;

        const int wards_count = m_pingable_wards.size( );

        const auto scale = g_config->misc.pingable_indicator_scale->get< int >( ) / 100.f;

        Vec2 anchor_position{
            static_cast< float >( g_config->misc.pingable_indicator_x->get< int >( ) ),
            static_cast< float >( g_config->misc.pingable_indicator_y->get< int >( ) )
        };
        const Vec2 instance_size{ 150.f * scale, 33.f * scale };

        g_render->filled_box(
            Vec2( anchor_position.x - instance_size.x / 2.f, anchor_position.y - instance_size.y / 2.f * wards_count ),
            Vec2( instance_size.x, instance_size.y * wards_count ),
            Color( 0, 0, 0, 150 )
        );

        anchor_position =
            Vec2( anchor_position.x - instance_size.x / 2.f, anchor_position.y - instance_size.y / 2.f * wards_count );

        const auto progress_size{ 8.f * scale };
        float      height{ };
        const auto margin_size{ 4.f * scale };

        const auto font_size = 16.f * scale;

        const Vec2                           texture_size{ instance_size.y, instance_size.y };
        std::shared_ptr< Renderer::Texture > texture{ };

        const auto text_font = font_size <= 16.f ? g_fonts->get_zabel_16px( ) : g_fonts->get_zabel( );

        for ( const auto inst : m_pingable_wards ) {
            if ( *g_time - inst.ping_time > 11.5f ) continue;

            const auto instance_age  = *g_time - inst.ping_time;
            const auto duration_left = 11.5f - instance_age;

            auto ward = g_entity_list.get_by_index( inst.index );
            if ( !ward || ward->is_dead( ) ) continue;

            ward.update( );

            switch ( ward->get_ward_type( ) ) {
            case Object::EWardType::control:
            {
                static auto controlward = path::join(
                    { directory_manager::get_resources_path( ), "common", "controlward.png" }
                );
                if ( controlward.has_value( ) ) {
                    texture = g_render->load_texture_from_file(
                        *controlward
                    );
                }
            }
            break;
            case Object::EWardType::blue:
            {
                static auto blueward = path::join(
                    { directory_manager::get_resources_path( ), "common", "blueward.png" }
                );
                if ( blueward.has_value( ) ) {
                    texture = g_render->load_texture_from_file(
                        *blueward
                    );
                }
            }
            break;
            case Object::EWardType::normal:
            {
                static auto trinket_icon = path::join(
                    { directory_manager::get_resources_path( ), "common", "blueward.png" }
                );

                if ( trinket_icon.has_value( ) ) {
                    texture = g_render->load_texture_from_file(
                        *trinket_icon
                    );
                }
            }
            break;
            case Object::EWardType::zombie:
            {
                static auto zombieward = path::join(
                    { directory_manager::get_resources_path( ), "common", "zombieward.png" }
                );
                if ( zombieward.has_value( ) ) {
                    texture = g_render->load_texture_from_file(
                        *zombieward
                    );
                }
            }
            break;
            default:
                continue;
            }


            const Vec2 instance_position{ anchor_position.x, anchor_position.y + height };

            const auto modifier = std::clamp( duration_left / 11.5f, 0.f, 1.f );

            auto timer_color = Color(
                255.f - 255.f * std::clamp( ( modifier - 0.5f ) / 0.5f, 0.f, 1.f ),
                255.f * std::clamp( modifier / 0.5f, 0.f, 1.f ),
                0.f
            );

            std::string text = "Pingable";

            auto timer_text = std::to_string( duration_left );
            timer_text.resize( duration_left >= 10.f ? 4 : 3 );
            timer_text += "s";

            const auto text_size = g_render->get_text_size( text, text_font, font_size );
            const auto size      = g_render->get_text_size( timer_text, text_font, font_size );

            const Vec2 draw_position = {
                instance_position.x + texture_size.x + margin_size,
                anchor_position.y + height + ( texture_size.y - progress_size ) / 2.f - text_size.y / 2.f
            };
            const Vec2 timer_position = { instance_position.x + instance_size.x - size.x * 1.2f, draw_position.y };

            g_render->text_shadow( timer_position, timer_color, text_font, timer_text.data( ), font_size );

            g_render->image( instance_position, texture_size, texture );

            g_render->text_shadow( draw_position, Color( 255, 255, 255 ), text_font, text.data( ), font_size );

            const Vec2 bar_position = {
                instance_position.x + texture_size.x,
                instance_position.y + texture_size.y - progress_size
            };
            const Vec2 bar_size = { ( instance_size.x - texture_size.x ) * modifier, progress_size };

            g_render->filled_box( bar_position, bar_size, timer_color );


            height += instance_size.y;
        }
    }

    auto Activator::manual_ping_ward( const Vec2 position ) -> void{
        m_is_manual_pinging_active    = true;
        m_manual_ping_time            = *g_time;
        m_manual_ping_position        = position;
        m_manual_ping_return_position = g_input->get_cursor_position( );
        m_manual_ping_state           = 0;
        m_manual_ping_iteration       = 0;
    }

    auto Activator::update_manual_ping( ) -> void{
        if ( !m_is_manual_pinging_active || *g_time <= m_manual_ping_next_action_time ) return;

        switch ( m_manual_ping_state ) {
        case 0:
        {
            g_input->set_cursor_position_run( m_manual_ping_position );
            m_manual_ping_next_action_time = *g_time;
            ++m_manual_ping_iteration;


            if ( m_manual_ping_iteration > 2 ) {
                m_manual_ping_iteration = 0;
                m_manual_ping_state     = 1;
            }

            break;
        }
        case 1:
        {
            if ( m_manual_ping_iteration >= 3 ) {
                g_input->send_key_event_run( utils::EKey::O, utils::Input::EKeyState::key_up );
                m_manual_ping_next_action_time = *g_time;
                m_manual_ping_iteration        = 0;
                m_manual_ping_state            = 2;
                break;
            }

            if ( m_manual_ping_iteration == 0 ) {
                g_input->send_key_event_run( utils::EKey::O, utils::Input::EKeyState::key_down );
                m_manual_ping_next_action_time = *g_time;
                ++m_manual_ping_iteration;

                g_input->set_cursor_position_run( m_manual_ping_position );
                break;
            }

            g_input->set_cursor_position_run( m_manual_ping_position );
            if ( m_manual_ping_iteration > 0 ) {
                m_manual_ping_next_action_time = *g_time;
                ++m_manual_ping_iteration;

                //g_input->send_key_event_run( utils::e_key::O, utils::Input::e_key_state::key_up );
            }
            break;
        }
        case 2:
        {
            g_input->set_cursor_position_run( m_manual_ping_return_position );
            m_manual_ping_next_action_time = *g_time;
            ++m_manual_ping_iteration;

            if ( m_manual_ping_iteration >= 2 ) {
                m_manual_ping_iteration    = 0;
                m_manual_ping_state        = 3;
                m_is_manual_pinging_active = false;
            }
            break;
        }
        default:
            m_is_manual_pinging_active = false;
            return;
        }
    }
}
