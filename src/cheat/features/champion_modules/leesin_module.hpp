#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"
#include "../../sdk/game/spell_cast_info.hpp"

namespace features::champion_modules {
    class leesin_module final : public IModule {
    public:
        virtual ~leesin_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "leesin_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "LeeSin" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation  = g_window->push( _( "lee sin" ), menu_order::champion_module );
            const auto q_settings  = navigation->add_section( _( "q settings" ) );
            const auto drawings    = navigation->add_section( _( "drawings" ) );
            const auto w_settings  = navigation->add_section( _( "w settings" ) );
            const auto e_settings  = navigation->add_section( _( "e settings" ) );
            const auto jungleclear = navigation->add_section( _( "jungleclear" ) );
            const auto combo       = navigation->add_section( _( "combo settings" ) );

            q_settings->checkbox( _( "combo q" ), g_config->leesin.q_enabled );
            q_settings->checkbox( _( "killsteal q" ), g_config->leesin.q_killsteal );
            q_settings->select(
                _( "hitchance" ),
                g_config->leesin.q_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            w_settings->checkbox( _( "combo w" ), g_config->leesin.w_enabled );
            w_settings->checkbox( _( "semi manual w (?)" ), g_config->leesin.w_semi_manual )->set_tooltip(
                _( "Press W to dash to closest ally/minion/ward to cursor, Hold W to place ward and dash to it." )
            );

            e_settings->checkbox( _( "combo e" ), g_config->leesin.e_enabled );

            drawings->checkbox( _( "draw q range" ), g_config->leesin.q_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->leesin.e_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->leesin.dont_draw_on_cooldown );

            combo->slider_int(
                _( "delay q/e if passive stacks > X" ),
                g_config->leesin.combo_delay_spells_if_passive,
                0,
                2,
                1
            );

            jungleclear->checkbox( _( "jungleclear w" ), g_config->leesin.w_jungleclear );
            jungleclear->checkbox( _( "jungleclear e" ), g_config->leesin.e_jungleclear );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->leesin.q_draw_range->get< bool >( ) &&
                !g_config->leesin.e_draw_range->get< bool >( ) ||
                g_local->is_dead( )
            )
                return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->leesin.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->leesin.dont_draw_on_cooldown->get
                    < bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 31, 88, 255, 255 ),
                        m_q_range,
                        Renderer::outline,
                        50,
                        2.f
                    );
                }
            }

            if ( g_config->leesin.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->leesin.dont_draw_on_cooldown->get
                    < bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 144, 66, 245, 255 ),
                        m_e_range,
                        Renderer::outline,
                        50,
                        2.f
                    );
                }
            }

            if ( m_q_active && m_target_found ) {
                auto target = g_entity_list.get_by_index( m_q_target_index );
                if ( !target ) return;

                target.update( );
                const auto cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;

                const auto circle_color = cursor.dist_to( target->position ) <= 300.f
                                              ? Color( 255, 50, 75 )
                                              : Color( 15, 106, 255 );
                g_render->circle_3d(
                    target->position,
                    g_features->orbwalker->animate_color( circle_color, EAnimationType::pulse, 8 ).alpha( 15 ),
                    300.f,
                    Renderer::outline | Renderer::filled,
                    48,
                    2.f
                );
            }

            // //ward debug
            // const auto target = g_features->target_selector->get_spell_specific_target(
            //     g_config->leesin.q_gapclose_minimum->get< int >( ),
            //     [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
            //     [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); }
            // );
            // if ( !target ) return;
            //
            // if ( target->dist_to_local( ) > g_config->leesin.q_gapclose_minimum->get< int >( ) || !g_config->leesin.
            //     ward_hop_target_draw->get< bool >( ) )
            //     return;
            //
            // g_render->line_3d( g_local->position, target->position, Color( 144, 66, 245, 255 ), 2 );
            /*for ( const auto ward : g_entity_list.get_ally_minions( ) ) {
                if ( ward->is_ward( ) || ward->is_alive(  )) {
                    g_render->line_3d( g_local->position, ward->position, color( 144, 66, 245, 255 ), 2 );
                }
                if ( !ward || ward->is_minion( ) ) continue;

                
            }*/
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            update_spellstates( );

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return;

            if ( semi_manual_w( ) ) return;

            killsteal_q( );

            const auto buff = g_features->buff_cache->get_buff(
                g_local->index,
                ct_hash( "blindmonkpassive_cosmetic" )
            );
            if ( buff && buff->stacks( ) > 0 ) m_passive_stacks = buff->stacks( );
            else m_passive_stacks                               = 0;

            spell_q( );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:
                spell_e( );
                spell_w( );
                break;
            case Orbwalker::EOrbwalkerMode::laneclear:
                jungleclear_w( );
                jungleclear_e( );
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->leesin.q_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_q_time <= 0.5f || !m_slot_q->is_ready( true ) )
                return false;

            if ( m_q_active ) {
                if ( !m_target_found ) {
                    for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                        if ( !enemy || enemy->dist_to_local( ) > 1500.f ||
                            g_features->target_selector->is_bad_target( enemy->index ) )
                            continue;

                        const auto buff = g_features->buff_cache->get_buff( enemy->index, ct_hash( "BlindMonkQOne" ) );
                        if ( !buff ) continue;

                        m_target_found   = true;
                        m_q_target_index = enemy->index;
                        break;
                    }

                    if ( !m_target_found ) return false;
                }

                if ( g_features->orbwalker->get_mode( ) != Orbwalker::EOrbwalkerMode::combo ) return false;

                const auto target = g_entity_list.get_by_index( m_q_target_index );
                if ( !target || target->is_dead( ) ) return false;

                const auto in_aa_range = g_features->orbwalker->is_attackable( target->index );
                const auto cursor      = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;

                if ( !in_aa_range && cursor.dist_to( target->position ) > 300.f ||
                    in_aa_range && m_passive_stacks > g_config->leesin.combo_delay_spells_if_passive->get< int >( ) &&
                    m_q_cast_expire - *g_time > 0.75f )
                    return false;


                if ( cast_spell( ESpellSlot::q ) ) {
                    m_last_q_time    = *g_time;
                    m_last_cast_time = *g_time;

                    m_target_found   = false;
                    m_q_target_index = 0;

                    std::cout << "[ Shaolee: Combo Q2 ] Target: " << target->champion_name.text << *g_time << std::endl;
                    return true;
                }

                return false;
            }

            if ( g_features->orbwalker->get_mode( ) != Orbwalker::EOrbwalkerMode::combo ) return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || m_passive_stacks > g_config->leesin.combo_delay_spells_if_passive->get< int >( ) &&
                g_features->orbwalker->is_attackable( target->index ) )
                return false;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_q_range,
                1800.f,
                60.f,
                0.25f,
                { },
                true
            );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->leesin.q_hitchance->
                    get< int >( ) ) ||
                g_features->prediction->minion_in_line_predicted(
                    g_local->position,
                    pred.position,
                    60.f,
                    0.25f,
                    1800.f
                ) )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;

                m_target_found   = false;
                m_q_target_index = 0;

                std::cout << "[ Shaolee: Combo Q1 ] Target: " << target->champion_name.text << *g_time << std::endl;
                return true;
            }


            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->leesin.w_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_w_time <= 0.4f || m_passive_stacks > 0 ||
                !m_slot_w->is_ready( true ) )
                return false;

            if ( m_w_active ) {
                if ( !m_slot_w->is_ready( true ) ) return false;

                const auto target = g_features->target_selector->get_default_target( );
                if ( !target || !g_features->orbwalker->is_attackable( target->index ) || m_passive_stacks > 0 ) {
                    return
                        false;
                }

                if ( cast_spell( ESpellSlot::w ) ) {
                    std::cout << "[ Shaolee: Combo W2 ] Stacking passive | " << *g_time << std::endl;
                    m_last_w_time    = *g_time;
                    m_last_cast_time = *g_time;
                    return true;
                }
            } else {
                if ( !m_slot_w->is_ready( true ) ) return false;
                bool allow_cast{ };

                for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                    if ( !enemy ) continue;
                    if ( g_features->target_selector->is_bad_target( enemy->index ) ) continue;

                    auto sci = enemy->spell_book.get_spell_cast_info( );
                    if ( !sci || !sci->is_autoattack || sci->get_target_index( ) != g_local->index ) continue;

                    allow_cast = true;
                    break;
                }

                if ( !allow_cast ) return false;

                if ( cast_spell( ESpellSlot::w, g_local->network_id ) ) {
                    m_last_w_time    = *g_time;
                    m_last_cast_time = *g_time;

                    std::cout << "[ Shaolee: Combo W ] Shilelding local from damage |" << *g_time << std::endl;
                    return true;
                }
            }

            return false;
        }

        auto semi_manual_w( ) -> bool{
            if ( !g_config->leesin.w_semi_manual->get< bool >( ) || !g_input->is_key_pressed( utils::EKey::W ) || *
                g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_w_time <= 0.4f || m_w_active || !m_slot_w->is_ready( true ) ) {
                if ( m_is_holding && !g_input->is_key_pressed( utils::EKey::W ) ) m_is_holding = false;

                return false;
            }

            if ( m_w_active ) return false;

            if ( !m_is_holding ) {
                m_hold_start_time = *g_time;
                m_is_holding      = true;
            }

            bool          allow_cast{ };
            const Object* target{ };
            auto          lowest_distance{ FLT_MAX };

            const auto cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;
            for ( const auto ally : g_entity_list.get_allies( ) ) {
                if ( !ally || ally->dist_to_local( ) > m_w_range || ally->network_id == g_local->network_id ||
                    g_features->target_selector->is_bad_target( ally->index ) || ally->position.dist_to( cursor ) >
                    300.f )
                    continue;

                const auto distance = ally->position.dist_to( cursor );
                if ( distance > lowest_distance ) continue;

                lowest_distance = distance;
                target          = ally;
                allow_cast      = true;
            }

            if ( !target ) {
                lowest_distance = FLT_MAX;

                for ( const auto minion : g_entity_list.get_ally_minions( ) ) {
                    if ( !minion || minion->dist_to_local( ) > m_w_range || minion->dist_to_local( ) < g_local->
                        attack_range ||
                        minion->is_dead( ) || ( *g_time - m_last_trinket_time > 0.5f || minion->position.dist_to(
                            m_ward_position
                        ) > 100.f ) && minion->position.dist_to( cursor ) > 375.f )
                        continue;

                    bool       skip{ };
                    const auto type = minion->get_minion_type( );

                    switch ( type ) {
                    case Object::EMinionType::super:
                    case Object::EMinionType::melee:
                    case Object::EMinionType::siege:
                    case Object::EMinionType::ranged:
                        break;
                    case Object::EMinionType::misc:
                        skip = minion->get_ward_type( ) == Object::EWardType::unknown || minion->get_ward_type( ) ==
                            Object::EWardType::blue;
                        break;
                    default:
                        skip = true;
                        break;
                    }

                    if ( skip ) continue;

                    const auto special = *g_time - m_last_trinket_time <= 0.5f && minion->position.dist_to(
                            m_ward_position
                        )
                        <= 100.f;
                    const auto distance = minion->position.dist_to( cursor );
                    if ( distance > lowest_distance && !special ) continue;

                    lowest_distance = distance;
                    target          = minion;
                    allow_cast      = true;

                    if ( special ) break;
                }
            }

            if ( !target || !allow_cast ) {
                w_place_ward_and_dash( );
                return false;
            }

            if ( cast_spell( ESpellSlot::w, target->network_id ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Shaolee: semi manual w ] Cast targeting " << target->get_name( ) << " | " << *g_time
                    << std::endl;

                m_is_holding      = false;
                m_hold_start_time = 0.f;
                return true;
            }

            return false;
        }

        auto w_place_ward_and_dash( ) -> bool{
            if ( *g_time - m_hold_start_time <= 0.2f || *g_time - m_last_trinket_time <= 1.f ) return false;


            auto cast_slot = ESpellSlot::q;
            bool found_ward{ };

            for ( auto i = 1; i < 8; i
                  ++ ) // cuz Inventory Slot starts at 1, so we iterate all, 7 is trinket so no check
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

                // if (i == 7) std::cout << "name: " << spell_slot->get_spell_info()->get_spell_data()->get_name() <<
                // "\n" << "id: " << item_data->id << std::endl;
                //  tear 3070
                //  3004 manamune

                switch ( static_cast< EItemId >( item_data->id ) ) {
                default:
                    // if ( i == 1 ) std::cout << "id: " << std::dec << item_data->id << " | ready: " <<
                    // spell_slot->cooldown_expire << " | charges: " << spell_slot->charges << std::endl;
                    break;
                case EItemId::ward_trinket:
                case EItemId::control_ward:
                case EItemId::frostfang:
                case EItemId::shard_of_true_ice:
                case EItemId::harrowing_cresent:
                case EItemId::blackmist_scythe:
                    if ( spell_slot->charges == 0 && item_base->stacks_left == 0 &&
                        static_cast< EItemId >( item_data->id ) != EItemId::control_ward )
                        break;

                    cast_slot  = spell_slot_object;
                    found_ward = true;
                    break;
                }

                if ( found_ward ) break;
            }

            if ( !found_ward ) return false;

            const auto cursor        = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;
            const auto cast_position = g_local->position.extend( cursor, 615.f );

            if ( cast_spell( cast_slot, cast_position ) ) {
                m_last_trinket_time = *g_time;
                m_ward_position     = cast_position;

                std::cout << "[ Shaolee: semi-manual w ] Placed ward for dash | " << *g_time << std::endl;
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->leesin.e_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_e_time <= 0.4f ||
                m_passive_stacks > g_config->leesin.combo_delay_spells_if_passive->get< int >( ) ||
                !m_slot_e->is_ready( true ) )
                return false;


            if ( m_e_active ) {
                const auto target = g_features->target_selector->get_default_target( );
                if ( !target || m_passive_stacks > 0 || !g_features->orbwalker->is_attackable( target->index ) ) {
                    return
                        false;
                }

                if ( cast_spell( ESpellSlot::e ) ) {
                    m_last_e_time    = *g_time;
                    m_last_cast_time = *g_time;
                    g_features->orbwalker->on_cast( );

                    std::cout << "[ Shaolee: Combo E2 ] " << *g_time << std::endl;
                    return true;
                }

                return false;
            }

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict_default( target->index, 0.25f, true );
            if ( !pred || g_local->position.dist_to( pred.value( ) ) >= m_e_range ) return false;

            if ( cast_spell( ESpellSlot::e ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                std::cout << "[ Shaolee: Combo E1 ] " << *g_time << std::endl;
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{ return false; }

        auto jungleclear_w( ) -> bool{
            if ( !g_config->leesin.w_jungleclear->get< bool >( ) || *g_time - m_last_cast_time <= 0.15f || *g_time -
                m_last_w_time <= 0.4f ||
                !m_slot_w->is_ready( true ) )
                return false;

            if ( m_w_active ) {
                if ( m_passive_stacks > 0 && m_w_cast_expire > *g_time + 0.3f ) return false;

                if ( cast_spell( ESpellSlot::w ) ) {
                    m_last_w_time    = *g_time;
                    m_last_cast_time = *g_time;

                    std::cout << "[ Shaolee: Jungleclear W2 ] Passive stacks: " << m_passive_stacks << std::endl;
                    return true;
                }

                return false;
            }

            if ( !m_slot_w->is_ready( true ) || m_passive_stacks > 0 || m_e_active ) return false;
            bool allow_cast{ };

            for ( const auto enemy : g_entity_list.get_enemy_minions( ) ) {
                if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) || !enemy->is_jungle_monster( ) ) continue;

                auto sci = enemy->spell_book.get_spell_cast_info( );
                if ( !sci || sci->server_cast_time < *g_time || sci->get_target_index( ) != g_local->index ) continue;

                allow_cast = true;
                break;
            }

            if ( !allow_cast ) return false;

            if ( cast_spell( ESpellSlot::w, g_local->network_id ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Shaolee: Jungleclear W ] Passive stacks: " << m_passive_stacks << std::endl;
                return true;
            }


            return false;
        }

        auto jungleclear_e( ) -> bool{
            if ( !g_config->leesin.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || *g_time -
                m_last_cast_time <= 0.15f || !m_slot_e->is_ready( true ) )
                return false;

            if ( m_e_active ) {
                if ( m_passive_stacks > 0 ) return false;

                if ( cast_spell( ESpellSlot::e ) ) {
                    m_last_e_time    = *g_time;
                    m_last_cast_time = *g_time;
                    g_features->orbwalker->on_cast( );

                    std::cout << "[ Shaolee: Jungleclear E2 ] Passive stacks: " << m_passive_stacks << std::endl;
                    return true;
                }

                return false;
            }

            if ( m_passive_stacks > 0 || m_w_active ) return false;

            int hit_count{ };
            for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
                if ( !minion ||
                    minion->is_dead( ) ||
                    minion->is_invisible( ) ||
                    minion->dist_to_local( ) > 550.f ||
                    !minion->is_jungle_monster( )
                )
                    continue;

                auto pred = g_features->prediction->predict_default( minion->index, 0.25f );
                if ( !pred || g_local->position.dist_to( pred.value( ) ) > m_e_range ) continue;

                hit_count++;
            }

            if ( hit_count == 0 ) return false;

            int  possible_hitcount{ };
            Vec3 possible_pos{ };

            const auto pred = g_features->prediction->predict_default( g_local->index, 0.5f );
            if ( !pred ) return false;

            possible_pos = pred.value( );
            for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
                if ( !minion ||
                    minion->is_dead( ) ||
                    minion->is_invisible( ) ||
                    possible_pos.dist_to( minion->position ) > 450.f ||
                    !minion->is_jungle_monster( )
                )
                    continue;

                auto pred = g_features->prediction->predict_default( minion->index, 0.25f );
                if ( !pred || possible_pos.dist_to( pred.value( ) ) > m_e_range ) continue;

                possible_hitcount++;
            }

            if ( possible_hitcount > hit_count ) return false;

            if ( cast_spell( ESpellSlot::e ) ) {
                m_last_e_time = *g_time;
                g_features->orbwalker->on_cast( );

                std::cout << "[ Shaolee: Jungleclear E ] Passive stacks: " << m_passive_stacks << std::endl;
                return true;
            }

            return false;
        }

        auto ward_hop_set_logic( ) -> void{
            for ( auto i = 1; i < 8; i
                  ++ ) // cuz Inventory Slot starts at 1, so we iterate all, 7 is trinket so no check
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

                // if (i == 7) std::cout << "name: " << spell_slot->get_spell_info()->get_spell_data()->get_name() <<
                // "\n" << "id: " << item_data->id << std::endl;
                //  tear 3070
                //  3004 manamune

                switch ( static_cast< EItemId >( item_data->id ) ) {
                default:
                    // if ( i == 1 ) std::cout << "id: " << std::dec << item_data->id << " | ready: " <<
                    // spell_slot->cooldown_expire << " | charges: " << spell_slot->charges << std::endl;
                    break;
                case EItemId::ward_trinket:
                case EItemId::control_ward:
                case EItemId::frostfang:
                case EItemId::shard_of_true_ice:
                case EItemId::harrowing_cresent:
                case EItemId::blackmist_scythe:
                    if ( spell_slot->charges == 0 && item_base->stacks_left == 0 &&
                        static_cast< EItemId >( item_data->id ) != EItemId::control_ward )
                        break;

                    if ( !GetAsyncKeyState( VK_CONTROL ) ) gap_close( spell_slot_object );
                    else return;
                //insec( spell_slot_object );
                    break;
                }
            }
        }

        auto gap_close( const ESpellSlot slot ) -> void{
            if ( !g_config->leesin.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || *g_time -
                m_last_q_time <= 0.4f ||
                !m_slot_w->is_ready( true ) )
                return;
            const auto target = g_features->target_selector->get_spell_specific_target(
                g_config->leesin.q_gapclose_minimum->get< int >( ),
                [this]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); }
            );
            if ( !target ) return;

            if ( target->dist_to_local( ) > g_config->leesin.q_gapclose_minimum->get< int >( ) ) return;

            const auto vec_to_cast = g_local->position.extend( target->position, 600.f );
            if ( vec_to_cast.dist_to( g_local->position ) > 600.f ) return;

            if ( g_input->cast_spell( slot, vec_to_cast ) ) {
                m_last_ward_time = *g_time;
                m_last_ward_pos  = vec_to_cast;
                return;
            }

            return;
        }

        //add cast to ward
        auto gap_close_w( ) -> void{
            if ( !g_config->leesin.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || *g_time -
                m_last_q_time <= 0.4f ||
                !m_slot_w->is_ready( ) )
                return;

            const auto target = g_features->target_selector->get_spell_specific_target(
                g_config->leesin.q_gapclose_minimum->get< int >( ),
                [this]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); }
            );
            if ( !target ) return;

            //if ( target->dist_to_local( ) > g_config->leesin.q_gapclose_minimum->get< int >( ) ) return;

            for ( const auto ward : g_entity_list.get_ally_minions( ) ) {
                if ( !ward ) continue;
                if ( ward->is_ward( ) || ward->is_alive( ) ) {
                    if ( ward->dist_to_local( ) > 700 || ward->position.dist_to( target->position ) > target->
                        dist_to_local( ) )
                        continue;

                    if ( g_input->cast_spell( ESpellSlot::w, ward->network_id ) ) {
                        m_last_w_time = *g_time;
                        return;
                        // break;
                    }
                }
            }
            return;
        }

        auto killsteal_q( ) -> bool{
            if ( !g_config->leesin.q_killsteal->get< bool >( ) || *g_time - m_last_q_time <= 0.4f ||
                !m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_killsteal_target(
                m_q_range,
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                1
            );
            if ( !target ) return false;

            const auto predicted =
                g_features->prediction->predict( target->index, 1200.f, 2000.f, 60.f, 0.25f, { }, true );

            if ( !predicted.valid ||
                g_features->prediction->minion_in_line( g_local->position, predicted.position, 60.f ) )
                return false;

            if ( cast_spell( ESpellSlot::q, predicted.position ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->on_cast( );

                std::cout << "[ KS Q ] Order cast at " << target->champion_name.text << std::endl;
                return true;
            }

            return false;
        }

        auto update_spellstates( ) -> void{
            const auto is_second_q = rt_hash( m_slot_q->get_name( ).data( ) ) == ct_hash( "BlindMonkQTwo" );

            if ( !m_q_active && *g_time > m_q_cast_expire && is_second_q ) {
                m_q_active      = true;
                m_q_cast_expire = *g_time + 3.25f;
            } else if ( !is_second_q ) {
                m_q_active       = false;
                m_q_cast_expire  = 0.f;
                m_target_found   = false;
                m_q_target_index = 0;
            }

            //if ( m_q_active ) std::cout << "Q2 Expires in " << m_q_cast_expire - *g_time << std::endl;

            const auto is_second_w = rt_hash( m_slot_w->get_name( ).data( ) ) == ct_hash( "BlindMonkWTwo" );
            if ( !m_w_active && *g_time > m_w_cast_expire && is_second_w ) {
                m_w_active      = true;
                m_w_cast_expire = *g_time + 3.25f;
            } else if ( !is_second_w ) {
                m_w_active      = false;
                m_w_cast_expire = 0.f;
            }

            //if ( m_w_active ) std::cout << "W2 Expires in " << m_w_cast_expire - *g_time << std::endl;

            const auto is_second_e = rt_hash( m_slot_e->get_name( ).data( ) ) == ct_hash( "BlindMonkETwo" );
            if ( !m_e_active && *g_time > m_e_cast_expire && is_second_e ) {
                m_e_active      = true;
                m_e_cast_expire = *g_time + 3.25f;
            } else if ( !is_second_e ) {
                m_e_active      = false;
                m_e_cast_expire = 0.f;
            }

            //if ( m_e_active ) std::cout << "E2 Expires in " << m_e_cast_expire - *g_time << std::endl;
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->bonus_attack_damage( ),
                    target->index,
                    true
                );
            }

            return 0.f;
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
            {
                const auto tt   = 0.25f + g_local->position.dist_to( target->position ) / 1800.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.25f + g_local->position.dist_to( pred.value( ) ) / 1800.f;
            }
            }

            return std::numeric_limits< float >::max( );
        }

    private:
        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_cast_time{ };

        bool  m_q_active{ };
        float m_q_cast_expire{ };

        bool  m_w_active{ };
        float m_w_cast_expire{ };

        bool  m_e_active{ };
        float m_e_cast_expire{ };

        // Q2 target
        int16_t m_q_target_index{ };
        bool    m_target_found{ };

        // passive
        int m_passive_stacks{ };

        // semi manual w
        bool  m_is_holding{ };
        float m_hold_start_time{ };

        float m_last_trinket_time{ };
        Vec3  m_ward_position{ };

        std::array< float, 6 > m_q_damage = { 0.f, 55.f, 80.f, 105.f, 130.f, 155.f };

        float m_q_range{ 1200.f };
        float m_w_range{ 700.f };
        float m_e_range{ 450.f };

        ESpellSlot ward_slot{ };
        float      m_last_ward_time{ };
        Vec3       m_last_ward_pos{ };
    };
}
