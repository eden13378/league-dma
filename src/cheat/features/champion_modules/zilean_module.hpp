#pragma once
#include "../../sdk/game/spell_cast_info.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"
#include "module.hpp"

namespace features::champion_modules {
    class ZileanModule final : public IModule {
    public:
        virtual ~ZileanModule( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "zilean_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Zilean" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell, e_spell, r_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "zilean" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );
            const auto laneclear  = navigation->add_section( _( "laneclear" ) );

            q_settings->checkbox( _( "enable" ), g_config->zilean.q_enabled );
            q_settings->select(
                _( "hitchance" ),
                g_config->zilean.q_hitchance,
                { ( "Fast" ), ( "Medium" ), ( "High" ), ( "Very high" ), ( "Immobile" ) }
            );
            q_settings->checkbox( _( "enable killsteal" ), g_config->zilean.q_killsteal );
            q_settings->checkbox( _( "enable antigapclose" ), g_config->zilean.q_antigapclose );
            q_settings->checkbox( _( "enable autointerrupt" ), g_config->zilean.q_autointerrupt );

            w_settings->checkbox( _( "enable" ), g_config->zilean.w_enabled );

            e_settings->checkbox( _( "enable" ), g_config->zilean.e_enabled );
            e_settings->checkbox( _( "anti-melee" ), g_config->zilean.e_antimelee );
            e_settings->checkbox( _( "gap close" ), g_config->zilean.e_gapclose );
            e_settings->checkbox( _( "e flee" ), g_config->zilean.e_flee );

            r_settings->checkbox( _( "enable (?)" ), g_config->zilean.r_enabled )->set_tooltip(
                _( "must be in full combo, R self manually" )
            );
            r_settings->checkbox( _( "only R priority allies" ), g_config->zilean.only_buff_priority_ally );
            r_settings->slider_int( _( "ally % hp to cast" ), g_config->zilean.r_min_hp, 5, 50 );

            drawings->checkbox( _( "draw q range" ), g_config->zilean.q_draw_range );
            drawings->checkbox( _( "draw r range" ), g_config->zilean.q_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->zilean.dont_draw_on_cooldown );

            laneclear->checkbox( _( "enable q" ), g_config->zilean.q_laneclear )->set_tooltip(
                ( "must be in fast clear to laneclear" )
            );
            laneclear->checkbox( _( "enable w" ), g_config->zilean.w_laneclear );
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->zilean.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) || !g_config->zilean.dont_draw_on_cooldown->get< bool >( ) ) ) {
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

            if ( g_config->zilean.r_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::r );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) || !g_config->zilean.dont_draw_on_cooldown->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 173, 47, 68, 255 ),
                        m_r_range,
                        Renderer::outline,
                        50,
                        2.f
                    );
                }
            }

            if ( m_ally_selected ) {
                auto ally = g_entity_list.get_by_index( m_ally_index );
                if ( ally ) {
                    ally.update( );

                    auto draw_color = g_features->orbwalker->animate_color(
                        ally->dist_to_local( ) <= m_r_range ? Color( 255, 255, 50 ) : Color( 66, 155, 245 ),
                        EAnimationType::pulse,
                        2
                    );

                    if ( *g_time - m_ally_selection_time <= 1.7f && *g_time - m_ally_selection_time >= 0.75f ) {
                        auto mod = std::clamp( ( *g_time - m_ally_selection_time - 0.75f ) / 1.f, 0.f, 1.f );
                        mod      = utils::ease::ease_out_quint( mod );

                        auto circle_color = draw_color;
                        circle_color.alpha( 255 - 255 * mod );

                        g_render->circle_3d(
                            ally->position,
                            circle_color,
                            80.f + 150.f * mod,
                            Renderer::outline,
                            50,
                            2.f,
                            360.f,
                            ( g_local->position - ally->position ).normalize( )
                        );
                    }

                    if ( *g_time - m_ally_selection_time > 0.75f ) {
                        const auto start_extend = g_local->position.extend( ally->position, 80.f );
                        const auto extended = g_local->position.extend( ally->position, ally->dist_to_local( ) - 80.f );


                        const auto max_thickness = 10.f;
                        const auto thickness     = max_thickness - max_thickness * ( ally->dist_to_local( ) / 2500.f );

                        /* g_render->circle_3d( ally->position,
                                             draw_color,
                                             80.f,
                                             c_renderer::outline,
                                             32,
                                             3.f,
                                             360.f,
                                             ( g_local->position - ally->position ) );*/

                        if ( ally->dist_to_local( ) > 160.f ) {
                            g_render->line_3d(
                                start_extend,
                                extended,
                                draw_color,
                                thickness
                            );
                        }
                    } else {
                        const auto modifier       = std::clamp( ( *g_time - m_ally_selection_time ) / 1.f, 0.f, 1.f );
                        const auto eased_modifier = utils::ease::ease_out_quart( modifier );


                        const auto extended = g_local->position.extend(
                            ally->position,
                            ( ally->dist_to_local( ) - 80.f ) * eased_modifier
                        );
                        const auto start_extend = g_local->position.extend( ally->position, 80.f );

                        const auto max_thickness = 10.f;
                        const auto thickness     = max_thickness - max_thickness * ( ally->dist_to_local( ) / 2500.f );

                        if ( ally->dist_to_local( ) > 160.f ) {
                            g_render->line_3d(
                                start_extend,
                                extended,
                                draw_color.alpha( 255 * eased_modifier ),
                                thickness
                            );
                        }

                        g_render->circle_3d(
                            ally->position,
                            draw_color,
                            80.f,
                            Renderer::outline,
                            32,
                            3.f,
                            360.f * eased_modifier,
                            ( g_local->position - ally->position )
                        );
                    }
                }
            }
        }

        auto run( ) -> void override{
            initialize_spell_slots( );
            select_priority_ally( );

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) ||
                g_features->evade->is_active( ) )
                return;

            killsteal_q( );
            antigapclose_q( );
            find_bomber( );
            autointerrupt_q( );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:
                spell_q( );
                if ( !m_bomber ) spell_e( );
                else chase_bomber_e( );
                combo_w( );
                spell_r( );
                break;
            case Orbwalker::EOrbwalkerMode::laneclear:
                if ( g_config->zilean.q_laneclear->get< bool >( ) && laneclear_q( ) ) return;
                if ( g_config->zilean.w_laneclear->get< bool >( ) && combo_w( ) ) return;
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                harass_q( );
            case Orbwalker::EOrbwalkerMode::flee:
                flee_e( );
                flee_w( );
            default: ;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->zilean.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.3f || !m_slot_q->is_ready( true ) )
                return false;

            /*if ( g_config->zilean.q_hold_to_stun->get< bool >( ) ) {
                if ( !m_slot_w->is_ready( ) ) return false;
            }*/
            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_q_range, 0.f, 140.f, 0.75f, { }, true );

            if ( !pred.valid ||
                pred.hitchance < static_cast< Prediction::EHitchance >( g_config->zilean.q_hitchance->get< int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_cast_time = *g_time;
                m_last_q_time    = *g_time;

                std::cout << "[ Zilean: Q ] Cast target: " << target->champion_name.text << std::endl;
                return true;
            }


            return false;
        }

        auto combo_w( ) -> bool{
            if ( !g_config->zilean.w_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.1f ||
                *g_time - m_last_cast_time <= 0.2f || m_slot_q->is_ready( true ) || !m_slot_w->is_ready( true ) )
                return false;

            if ( cast_spell( ESpellSlot::w ) ) {
                m_last_w_time = *g_time;
                return true;
            }
            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->zilean.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.3f || !m_slot_e->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_spell_specific_target( 1700.f );
            if ( !target ) return false;

            if ( target->dist_to_local( ) <= 550.f ) {
                if ( cast_spell( ESpellSlot::e, target->network_id ) ) {
                    m_last_e_time = *g_time;
                    return true;
                }
            } else {
                if ( cast_spell( ESpellSlot::e, g_local->network_id ) ) {
                    m_last_e_time = *g_time;
                    return true;
                }
            }

            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->zilean.r_enabled->get< bool >( ) || *g_time - m_last_r_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.3f || !m_slot_r->is_ready( true ) || !GetAsyncKeyState( VK_CONTROL ) )
                return false;

            if ( buff_priority_ally( ) || m_ally_selected && g_config->zilean.only_buff_priority_ally->get< bool >( ) &&
                m_ally_distance <= m_nearby_threshold )
                return true;

            const Object* target{ };
            auto          target_priority{ -1 };

            for ( const auto ally : g_entity_list.get_allies( ) ) {
                if ( !ally || ally->is_dead( ) || ally->network_id == g_local->network_id || ally->is_invisible( ) ||
                    ally->get_selectable_flag( ) != 1 || ally->dist_to_local( ) > m_r_range )
                    continue;

                const auto priority = g_features->target_selector->get_target_priority( ally->champion_name.text );
                if ( priority < target_priority ) continue;

                target_priority = priority;
                target          = ally;
            }

            if ( !target ) return false;

            if ( ( target->health / target->max_health * 100 ) > g_config->zilean.r_min_hp->get< int >( ) ) {
                return
                    false;
            }

            if ( cast_spell( ESpellSlot::r, target->network_id ) ) {
                g_features->orbwalker->on_cast( );
                m_last_r_time    = *g_time;
                m_last_cast_time = *g_time;

                return true;
            }
            return false;
        }

        auto laneclear_q( ) -> bool{
            if ( !g_config->zilean.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.3f || !m_slot_q->is_ready( true ) || !GetAsyncKeyState( VK_CONTROL ) )
                return false;

            const auto min_count = 3;

            const auto farm_pos = get_best_laneclear_position( m_q_range, 350.f, true, true, 0.25f );
            if ( farm_pos.value < min_count ) return false;

            if ( cast_spell( ESpellSlot::q, farm_pos.position ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->on_cast( );

                return true;
            }
            return false;
        }

        auto harass_q( ) -> bool{
            if ( !g_config->zilean.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.3f || !m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_spell_specific_target(
                m_q_range,
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); }
            );

            if ( !target ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_q_range, 0.9f, 80.f, 0.25f, { }, true );

            if ( !pred.valid ||
                pred.hitchance < static_cast< Prediction::EHitchance >( g_config->zilean.q_hitchance->get< int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->set_cast_time( 0.25f );
                m_last_cast_time = *g_time;
                m_last_q_time    = *g_time;
                return true;
            }
            return false;
        }

        auto killsteal_q( ) -> bool{
            if ( !g_config->zilean.q_killsteal->get< bool >( ) || *g_time - m_last_q_time <= 0.3f ||
                !m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_killsteal_target(
                m_q_range,
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                2
            );
            if ( !target ) return false;

            const auto predicted =
                g_features->prediction->predict( target->index, m_q_range, 1100.f, 140.f, 3.25f, { }, true );

            if ( !predicted.valid ) return false;

            if ( cast_spell( ESpellSlot::q, predicted.position ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->on_cast( );

                return true;
            }

            return false;
        }

        auto antigapclose_q( ) -> void{
            if ( !g_config->zilean.q_antigapclose->get< bool >( ) || *g_time - m_last_cast_time <= 0.2f ||
                *g_time - m_last_q_time <= 0.3f || !m_slot_q->is_ready( true ) )
                return;

            const auto target = get_advanced_antigapclose_target( m_q_range, 1100.f, 140.f, 0.25f, true );
            if ( !target ) return;

            const auto pred =
                g_features->prediction->predict( target->index, m_q_range, 2000.f, 80.f, 0.25f, { }, true );
            if ( !pred.valid || ( int )pred.hitchance < 3 ) return;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.25f );
            }
        }

        auto autointerrupt_q( ) -> void{
            if ( !g_config->zilean.q_autointerrupt->get< bool >( ) || *g_time - m_last_q_time <= 0.4f ||
                !m_slot_q->is_ready( true ) )
                return;

            const auto target = get_interruptable_target( m_q_range );
            if ( !target ) return;

            const auto pred =
                g_features->prediction->predict( target->index, m_q_range, 1100.f, 140.f, 0.25f, { }, true );
            if ( !pred.valid || ( int )pred.hitchance <= 3 ) return;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.25f );
            }
        }

        auto chase_bomber_e( ) -> void{
            if ( !g_config->zilean.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.3f || !m_slot_e->is_ready( true ) ||
                !g_config->zilean.e_gapclose->get< bool >( ) )
                return;
            if ( m_bomber->dist_to_local( ) > m_e_range ) {
                if ( cast_spell( ESpellSlot::e, g_local->network_id ) ) {
                    m_last_e_time = *g_time;
                    return;
                }
            } else {
                const auto target = g_features->target_selector->get_spell_specific_target( m_e_range );
                if ( !target ) return;

                if ( cast_spell( ESpellSlot::e, m_bomber->network_id ) ) {
                    m_last_e_time = *g_time;
                    return;
                }
            }
            return;
        }

        auto flee_e( ) -> void{
            if ( !g_config->zilean.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.3f || !m_slot_e->is_ready( true ) ||
                !g_config->zilean.e_flee->get< bool >( ) )
                return;

            auto closest_enemy{ 9999.f };
            bool was_moving_towards{ };
            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) || enemy->dist_to_local( ) >
                    1500.f )
                    continue;

                if ( !was_moving_towards ) {
                    auto pred = g_features->prediction->predict_default( enemy->index, 0.25f, false );
                    if ( pred && pred.value( ).dist_to( g_local->position ) < enemy->
                        dist_to_local( ) )
                        was_moving_towards = true;
                }

                if ( enemy->dist_to_local( ) < closest_enemy ) closest_enemy = enemy->dist_to_local( );
            }

            if ( closest_enemy > 950.f || !was_moving_towards ) return;

            if ( cast_spell( ESpellSlot::e, g_local->network_id ) ) {
                m_last_e_time = *g_time;
                return;
            }
        }

        auto flee_w( ) -> void{
            if ( !g_config->zilean.w_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.1f ||
                *g_time - m_last_cast_time <= 0.2f || m_slot_e->is_ready( true ) || !m_slot_w->is_ready( true ) )
                return;

            if ( cast_spell( ESpellSlot::w ) ) {
                m_last_w_time = *g_time;
                return;
            }
        }

        auto find_bomber( ) -> void{
            auto has_buff = false;
            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || !enemy->is_targetable( ) || enemy->dist_to_local( ) > 2000.f ) continue;

                has_buff = !!g_features->buff_cache->get_buff( enemy->index, ct_hash( "ZileanQEnemyBomb" ) );

                if ( has_buff ) {
                    if ( g_features->buff_cache->get_buff( enemy->index, ct_hash( "ZileanQEnemyBomb" ) )->stacks( ) >
                        0.f )
                        m_bomber = enemy;
                }

                if ( !has_buff ) m_bomber = { };
            }
        }

        auto select_priority_ally( ) -> bool{
            if ( !g_config->lulu.ally_priority_enabled->get< bool >( ) ) {
                if ( m_ally_selected ) unselect_ally( );

                return false;
            }

            if ( !m_ally_selected ) {
                if ( !GetAsyncKeyState( 0x06 ) ) return false;

                const auto cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;

                int16_t  ally_index{ };
                unsigned ally_nid{ };
                auto     ally_distance_to_cursor{ 200.f };

                for ( const auto ally : g_entity_list.get_allies( ) ) {
                    if ( !ally || ally->dist_to_local( ) > 2250.f || ally->network_id == g_local->network_id ||
                        ally->is_dead( ) || ally->is_invisible( ) || ally->get_selectable_flag( ) != 1 )
                        continue;

                    const auto distance = ally->position.dist_to( cursor );
                    if ( distance > ally_distance_to_cursor ) continue;

                    ally_index              = ally->index;
                    ally_nid                = ally->network_id;
                    ally_distance_to_cursor = distance;
                }

                if ( ally_index == 0 || ally_nid == 0 ) return false;

                m_ally_selected = true;
                m_ally_index    = ally_index;
                m_ally_nid      = ally_nid;

                m_holding_key = true;

                m_ally_selection_time = *g_time;

                if ( g_function_caller->is_glow_queueable( ) ) {
                    const auto color = g_features->orbwalker->animate_color(
                        { 66, 155, 245 },
                        EAnimationType::pulse,
                        1
                    );

                    g_function_caller->enable_glow(
                        m_ally_nid,
                        D3DCOLOR_ARGB( 255, color.r, color.g, color.b ),
                        5,
                        2,
                        false
                    );

                    m_ally_glowing        = true;
                    m_ally_last_glow_time = *g_time;
                }

                return false;
            }

            auto ally = g_entity_list.get_by_index( m_ally_index );
            if ( !ally ) {
                unselect_ally( );
                return false;
            }

            ally.update( );
            if ( ally->is_dead( ) || ally->is_invisible( ) || ally->dist_to_local( ) > 2500.f ) {
                unselect_ally( );
                return false;
            }

            m_ally_distance = ally->dist_to_local( );

            if ( *g_time - m_ally_last_glow_time > 0.05f && g_function_caller->is_glow_queueable( ) ) {
                const auto color = g_features->orbwalker->animate_color( { 66, 155, 245 }, EAnimationType::pulse, 4 );

                const auto modifier       = std::clamp( ( *g_time - m_ally_selection_time ) / 1.f, 0.f, 1.f );
                auto       eased_modifier = utils::ease::ease_out_quart( modifier );

                g_function_caller->enable_glow(
                    m_ally_nid,
                    D3DCOLOR_ARGB( 255, color.r, color.g, color.b ),
                    5,
                    2,
                    false
                );

                m_ally_glowing        = true;
                m_ally_last_glow_time = *g_time;
            }

            if ( !GetAsyncKeyState( 0x06 ) ) m_holding_key = false;

            if ( m_holding_key ) return false;

            if ( GetAsyncKeyState( 0x06 ) ) {
                unselect_ally( );
                return false;
            }

            return false;
        }

        auto unselect_ally( ) -> void{
            if ( m_ally_glowing ) {
                if ( g_function_caller->is_glow_queueable( ) ) {
                    g_function_caller->enable_glow( m_ally_nid, D3DCOLOR_ARGB( 255, 255, 255, 25 ), 3, 3, true );

                    m_ally_glowing = false;
                } else return;
            }

            m_ally_selected       = false;
            m_ally_last_glow_time = 0.f;
            m_ally_glowing        = false;
        }

        auto buff_priority_ally( ) -> bool{
            if ( !m_ally_selected || *g_time - m_last_w_time <= 0.4f || *g_time - m_last_cast_time <= 0.05f ) {
                return
                    false;
            }

            auto ally = g_entity_list.get_by_index( m_ally_index );
            if ( !ally ) return false;

            ally.update( );

            if ( ally->is_dead( ) || ally->is_invisible( ) || ally->get_selectable_flag( ) != 1 ) return false;

            int cast_reason{ };

            bool allow_cast{ };

            if ( !allow_cast && g_config->lulu.ally_buff_in_combat->get< bool >( ) ) {
                auto sci = ally->spell_book.get_spell_cast_info( );
                if ( sci && sci->server_cast_time > *g_time ) {
                    const auto target_index = sci->get_target_index( );
                    const auto ally_target  = g_entity_list.get_by_index( target_index );

                    if ( ally_target && ally_target->is_hero( ) &&
                        ally_target->position.dist_to( ally->position ) <= 850.f )
                        allow_cast = true;
                }
            }

            if ( allow_cast ) cast_reason = 1;

            if ( !allow_cast ) {
                const auto pred = g_features->prediction->predict_default( ally->index, 0.5f, false );
                if ( !pred ) return false;

                const auto ally_direction{ *pred };
                const auto chase_threshold{ 50.f };

                for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                    if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) ||
                        enemy->position.dist_to( ally->position ) > 1000.f )
                        continue;

                    auto enemy_pred = g_features->prediction->predict_default( enemy->index, 0.5f, false );
                    if ( !enemy_pred ) return false;

                    auto enemy_direction = *enemy_pred;

                    auto       v1                   = ally_direction - ally->position;
                    auto       v2                   = enemy->position - ally->position;
                    auto       dot                  = v1.normalize( ).dot_product( v2.normalize( ) );
                    const auto ally_direction_angle = acos( dot ) * 180.f / 3.14159265358979323846f;

                    v1                               = enemy_direction - enemy->position;
                    v2                               = ally->position - enemy->position;
                    dot                              = v1.normalize( ).dot_product( v2.normalize( ) );
                    const auto enemy_direction_angle = acos( dot ) * 180.f / 3.14159265358979323846f;

                    const auto is_chasing_ally{
                        ally_direction.dist_to( enemy->position ) >
                        ally->position.dist_to( enemy->position ) + chase_threshold &&
                        enemy_direction.dist_to( ally->position ) + chase_threshold <
                        enemy->position.dist_to( ally->position ) &&
                        enemy_direction_angle <= 35.f
                    };

                    const auto is_ally_chasing{
                        ally_direction.dist_to( enemy->position ) + chase_threshold <
                        ally->position.dist_to( enemy->position ) &&
                        enemy_direction.dist_to( ally->position ) >
                        enemy->position.dist_to( ally->position ) + chase_threshold &&
                        ally_direction_angle <= 35.f
                    };

                    if ( is_ally_chasing && g_config->lulu.ally_buff_on_chase->get< bool >( ) ) {
                        allow_cast  = true;
                        cast_reason = 2;
                        break;
                    }

                    if ( is_chasing_ally && g_config->lulu.ally_buff_on_flee->get< bool >( ) ) {
                        allow_cast  = true;
                        cast_reason = 3;
                        break;
                    }
                }
            }

            if ( !allow_cast ) return false;

            std::string reason_text{ };

            switch ( cast_reason ) {
            case 1:
                reason_text = "In combat";
                break;
            case 2:
                reason_text = "Chasing enemy";
                break;
            case 3:
                reason_text = "Fleeing enemy";
                break;
            default:
                std::cout << "unknown ally shield reason: " << cast_reason << std::endl;
                return false;
            }

            if ( cast_spell( ESpellSlot::w, ally->network_id ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Lulu: W ] Buffed priority ally due to " << reason_text << std::endl;
                return true;
            }

            return false;
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->ability_power( ) * 0.90f,
                    target->index,
                    false
                );
            }

            return 0.f;
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
            {
                const auto tt   = 0.25f + 1100.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.25f + g_local->position.dist_to( pred.value( ) ) / 1100.f;
            }
            }

            return std::numeric_limits< float >::max( );
        }

    private:
        float m_last_q_time{ };
        bool  m_q_double_bomb{ };

        float m_last_w_time{ };

        float m_last_e_time{ };


        float m_last_r_time{ };

        float m_last_cast_time{ };

        std::array< float, 6 > m_q_damage = { 0.f, 75.f, 115.f, 165.f, 230.f, 300.f };

        float m_q_range{ 900.f };
        float m_w_range{ };
        float m_e_range{ 550.f };
        float m_r_range{ 900.f };

        Object* m_bomber{ };

        // priority ally logic
        bool     m_ally_selected{ };
        int16_t  m_ally_index{ };
        unsigned m_ally_nid{ };

        bool m_holding_key{ };

        bool  m_ally_glowing{ };
        float m_ally_last_glow_time{ };

        float m_ally_selection_time{ };
        float m_ally_distance{ };

        float m_nearby_threshold{ 750.f };
    };
} // namespace features::champion_modules
