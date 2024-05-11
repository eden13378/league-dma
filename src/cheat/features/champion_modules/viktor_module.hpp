#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class viktor_module final : public IModule {
    public:
        virtual ~viktor_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "viktor_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Viktor" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "viktor" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );
            const auto spellclear = navigation->add_section( _( "spellclear" ) );

            q_settings->checkbox( _( "enable" ), g_config->viktor.q_enabled );
            q_settings->checkbox( _( "harass q" ), g_config->viktor.q_harass );
            q_settings->checkbox( _( "flee q (?)" ), g_config->viktor.q_flee )->set_tooltip(
                _( "Ignored until Q is upgraded" )
            );

            w_settings->checkbox( _( "enable" ), g_config->viktor.w_enabled );
            w_settings->checkbox( _( "flee w" ), g_config->viktor.w_flee );
            w_settings->checkbox( _( "autointerrupt w" ), g_config->viktor.w_autointerrupt )->set_tooltip(
                _( "Automatically interrupts spells like Katarina R, Malzahar R, etc" )
            );;
            w_settings->checkbox( _( "antigapclose w" ), g_config->viktor.w_antigapclose );
            w_settings->checkbox( _( "ignore hitchance if nearby" ), g_config->viktor.w_ignore_hitchance );
            w_settings->select(
                _( "hitchance" ),
                g_config->viktor.w_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            e_settings->checkbox( _( "enable" ), g_config->viktor.e_enabled );
            e_settings->checkbox( _( "multihit" ), g_config->viktor.e_multihit );
            e_settings->checkbox( _( "harass e" ), g_config->viktor.e_harass );
            e_settings->checkbox( _( "killsteal e" ), g_config->viktor.e_killsteal );
            e_settings->checkbox( _( "flee e" ), g_config->viktor.e_flee );
            e_settings->select(
                _( "hitchance" ),
                g_config->viktor.e_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );
            e_settings->slider_int( _( "max range %" ), g_config->viktor.e_max_range, 50, 100, 1 );

            r_settings->checkbox( _( "enable" ), g_config->viktor.r_enabled );
            e_settings->checkbox( _( "killsteal r" ), g_config->viktor.r_killsteal );
            r_settings->checkbox( _( "autofollow enemy" ), g_config->viktor.r_autofollow );
            r_settings->checkbox( _( "auto interrupt (?)" ), g_config->viktor.r_autointerrupt )->set_tooltip(
                _( "Automatically interrupts spells like Katarina R, Malzahar R, etc" )
            );
            r_settings->select(
                _( "hitchance" ),
                g_config->viktor.r_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );
            r_settings->slider_int( _( "if x ticks lethal" ), g_config->viktor.r_ticks_lethal, 1, 5, 1 );

            spellclear->select(
                _( "lasthit q" ),
                g_config->viktor.lasthit_q_mode,
                { _( "Off" ), _( "Necessary only" ), _( "Always" ) }
            );
            spellclear->checkbox( _( "spellclear q" ), g_config->viktor.spellclear_q );
            spellclear->checkbox( _( "spellclear e" ), g_config->viktor.spellclear_e );
            spellclear->slider_int( _( "^ e minimum hitcount" ), g_config->viktor.spellclear_e_min_hitcount, 2, 8, 1 );

            drawings->checkbox( _( "draw q range" ), g_config->viktor.q_draw_range );
            drawings->checkbox( _( "draw w range" ), g_config->viktor.w_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->viktor.e_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->viktor.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->viktor.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->viktor.dont_draw_on_cooldown->get
                    < bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 144, 66, 245, 255 ),
                        m_q_range + 65.f,
                        Renderer::outline,
                        60,
                        2.f
                    );
                }
            }

            if ( g_config->viktor.w_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->viktor.dont_draw_on_cooldown->get
                    < bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 132, 245, 66, 255 ),
                        m_w_range,
                        Renderer::outline,
                        60,
                        2.f
                    );
                }
            }

            if ( g_config->viktor.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->viktor.dont_draw_on_cooldown->get
                    < bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 30, 100, 255, 255 ),
                        500.f + 700.f * ( g_config->viktor.e_max_range->get< int >( ) / 100.f ),
                        Renderer::outline,
                        72,
                        3.f
                    );
                }
            }
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            r_autofollow( );

            m_q_upgraded = m_slot_q->get_usable_state( ) == 1;
            m_e_upgraded = m_slot_e->get_usable_state( ) == 1;

            killsteal_e( );

            if ( !g_features->orbwalker->in_action( ) && !g_features->evade->is_active( ) ) {
                antigapclose_w( );
                autointerrupt_w( );
                autointerrupt_r( );
                killsteal_r( );
            }

            if ( g_features->orbwalker->in_attack( ) ) return;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:
                if ( spell_e( ) ) break;

                if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return;

                spell_r( );
                spell_w( );
                spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->viktor.e_harass->get< bool >( ) && spell_e( ) ) break;

                if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return;

                if ( g_config->viktor.q_harass->get< bool >( ) ) spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::laneclear:
                if ( g_config->viktor.e_harass->get< bool >( ) && spell_e( ) || g_features->orbwalker->in_action( ) ||
                    g_features->evade->is_active( ) )
                    break;

                if ( lasthit_q( ) || spellclear_q( ) || spellclear_e( ) ) break;
                break;
            case Orbwalker::EOrbwalkerMode::lasthit:
                if ( g_config->viktor.e_harass->get< bool >( ) && spell_e( ) ||
                    g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) )
                    break;

                lasthit_q( );
                break;
            case Orbwalker::EOrbwalkerMode::flee:
                if ( g_config->viktor.e_flee->get< bool >( ) && spell_e( )
                    || g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) )
                    break;

                if ( g_config->viktor.q_flee->get< bool >( ) && m_q_upgraded && ( spell_q( ) || flee_q( ) ) ) break;

                flee_w( );
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->viktor.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.1f ||
                !m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || !g_features->orbwalker->is_attackable( target->index, m_q_range ) ) return false;

            if ( cast_spell( ESpellSlot::q, target->network_id ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->viktor.w_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.1f ||
                *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            //bool is_nearby = g_config->viktor.w_ignore_hitchance->get< bool >( ) && g_features->orbwalker->is_attackable( target->index );
            if ( !g_features->orbwalker->is_attackable( target->index ) ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_w_range, 0.f, 0.f, 1.f );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->viktor.w_hitchance->
                get< int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->viktor.e_enabled->get< bool >( ) ||
                *g_time - m_last_cast_time <= 0.1f || *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready( true ) )
                return false;

            for ( auto i = 0; i < 2; i++ ) {
                if ( g_features->target_selector->is_forced( ) && i > 0 ) break;

                const auto target = g_features->target_selector->get_default_target( i > 0 );
                if ( !target ) break;

                if ( combo_e( target ) ) return true;
            }

            return false;
        }

        auto combo_e( Object* target ) -> bool{
            if ( !target ) return false;

            const auto start_position{ g_local->position.extend( target->position, 500.f ) };
            const auto in_start_range = target->dist_to_local( ) <= 500.f;

            if ( in_start_range && g_config->viktor.e_multihit->get< bool >( ) ) {
                const auto multi = get_viktor_multihit_position( target->position, 1050.f, 90.f, target->network_id );
                if ( multi.has_value( ) && multi.value( ).hit_count > 1 ) {
                    if ( cast_spell( ESpellSlot::e, target->position, multi->position ) ) {
                        m_last_e_time = *g_time;
                        return true;
                    }
                }
            }

            const auto pred = g_features->prediction->predict(
                target->index,
                700.f * ( g_config->viktor.e_max_range->get< int >( ) / 100.f ),
                1050.f,
                45.f,
                0.f,
                start_position,
                true,
                Prediction::include_ping
            );

            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->viktor.e_hitchance->
                get< int >( ) ) && !in_start_range )
                return false;

            if ( cast_spell( ESpellSlot::e, start_position, pred.position ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->viktor.r_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.1f || *g_time -
                m_last_r_time <= 0.4f ||
                rt_hash( m_slot_r->get_name( ).data( ) ) == ct_hash( "ViktorChaosStormGuide" ) || !m_slot_r->is_ready(
                    true
                ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto total_damage = get_ult_raw_damage( g_config->viktor.r_ticks_lethal->get< int >( ) );
            const auto health       = helper::get_real_health( target->index, EDamageType::magic_damage, 1.f, true );

            if ( helper::calculate_damage( total_damage, target->index, false ) < health ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_r_range, 0.f, 0.f, 0.3f );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->viktor.r_hitchance->
                get< int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::r, pred.position ) ) {
                //std::cout << "[ normal ult ] target health: " << health << " | total damage: " << total_damage
                //          << " from intial + " << g_config->viktor.r_ticks_lethal->get< int >( ) << "ticks \n";

                g_features->orbwalker->on_cast( );
                m_last_r_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto flee_w( ) -> bool{
            if ( !g_config->viktor.w_flee->get< bool >( ) || *g_time - m_last_cast_time <= 0.1f ||
                *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_w_range, 0.f, 0.f, 1.f );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->viktor.w_hitchance->
                get< int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto flee_q( ) -> bool{
            if ( !g_config->viktor.q_flee->get< bool >( ) || *g_time - m_last_q_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.1f || !m_slot_q->is_ready( true ) )
                return false;

            const Object* target{ };
            auto          lowest_distance{ FLT_MAX };
            bool          found_target{ };

            for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
                if ( !minion || minion->is_dead( ) || minion->is_invisible( ) || !minion->is_jungle_monster( ) && !
                    minion->is_lane_minion( )
                    || !g_features->orbwalker->is_attackable( minion->index, m_q_range ) )
                    continue;

                const auto distance = minion->dist_to_local( );
                if ( distance > lowest_distance ) continue;

                target          = minion;
                found_target    = true;
                lowest_distance = distance;
            }

            if ( !found_target || !target ) return false;

            if ( cast_spell( ESpellSlot::q, target->network_id ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto killsteal_e( ) -> bool{
            if ( !g_config->viktor.e_killsteal->get< bool >( ) || *g_time - m_last_e_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.05f || !m_slot_e->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_killsteal_target(
                500.f + 700.f * ( g_config->viktor.e_max_range->get< int >( ) / 100.f ),
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::e, unit ); },
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::e, unit ); },
                2
            );
            if ( !target ) return false;

            auto start_position{ g_local->position.extend( target->position, 500.f ) };
            if ( target->dist_to_local( ) <= 500.f ) start_position = target->position;

            const auto predicted = g_features->prediction->predict(
                target->index,
                700.f,
                1050.f,
                45.f,
                0.f,
                start_position,
                true,
                Prediction::include_ping,
                Prediction::ESpellType::linear
            );

            if ( !predicted.valid ) return false;

            if ( cast_spell( ESpellSlot::e, start_position, predicted.position ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                std::cout << "[ KS E ] Order cast at " << target->champion_name.text << std::endl;
                return true;
            }

            return false;
        }

        auto killsteal_r( ) -> bool{
            if ( !g_config->viktor.r_killsteal->get< bool >( ) || *g_time - m_last_r_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.05f || rt_hash( m_slot_r->get_name( ).data( ) ) == ct_hash(
                    "ViktorChaosStormGuide"
                ) || !m_slot_r->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_killsteal_target(
                m_r_range,
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::r, unit ); },
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::r, unit ); },
                2
            );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_r_range, 0.f, 100.f, 0.25f );
            if ( !pred.valid ) return false;

            if ( cast_spell( ESpellSlot::r, pred.position ) ) {
                m_last_r_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                std::cout << "[ KS R ] Order cast at " << target->champion_name.text << " | " << *g_time << std::endl;
                return true;
            }

            return false;
        }

        auto r_autofollow( ) -> void{
            if ( !g_config->viktor.r_autofollow->get< bool >( ) || *g_time - m_last_r_move_time <= 0.1f
                || rt_hash( m_slot_r->get_name( ).data( ) ) != ct_hash( "ViktorChaosStormGuide" ) )
                return;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return;

            auto       cast_position = target->position;
            const auto pred          = g_features->prediction->predict_default( target->index, 0.2f );
            if ( pred.has_value( ) ) cast_position = pred.value( );

            if ( cast_spell( ESpellSlot::r, cast_position ) ) m_last_r_move_time = *g_time;
        }

        auto antigapclose_w( ) -> void{
            if ( !g_config->viktor.w_antigapclose->get< bool >( ) || *g_time - m_last_cast_time <= 0.1f ||
                *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready( true ) )
                return;

            const auto target = get_advanced_antigapclose_target( m_w_range, 0.f, 150.f, 1.f );
            if ( !target ) return;

            const auto pred = g_features->prediction->predict( target->index, m_w_range, 0.f, 150.f, 1.f );
            if ( !pred.valid || ( int )pred.hitchance < 3 ) return;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
            }
        }

        auto autointerrupt_r( ) -> void{
            if ( !g_config->viktor.r_autointerrupt->get< bool >( ) || *g_time - m_last_cast_time <= 0.1f || *g_time -
                m_last_r_time <= 0.4f ||
                rt_hash( m_slot_r->get_name( ).data( ) ) == ct_hash( "ViktorChaosStormGuide" ) || !m_slot_r->is_ready(
                    true
                ) )
                return;

            const auto target = get_interruptable_target( m_r_range );
            if ( !target ) return;

            const auto pred = g_features->prediction->predict( target->index, m_r_range, 0.f, 150.f, 0.25f );
            if ( !pred.valid || ( int )pred.hitchance <= 3 ) return;

            if ( cast_spell( ESpellSlot::r, pred.position ) ) {
                m_last_r_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                std::cout << "[ Interrupt R ] Order cast at " << target->champion_name.text << " | " << *g_time <<
                    std::endl;
            }
        }

        auto autointerrupt_w( ) -> void{
            if ( !g_config->viktor.w_autointerrupt->get< bool >( ) || *g_time - m_last_cast_time <= 0.1f || *g_time -
                m_last_w_time <= 0.4f ||
                !m_slot_w->is_ready( true ) )
                return;

            const auto target = get_interruptable_target( m_w_range );
            if ( !target ) return;

            const auto pred = g_features->prediction->predict( target->index, m_w_range, 0.f, 150.f, 1.5f );
            if ( !pred.valid || ( int )pred.hitchance < 3 ) return;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
            }
        }

        auto lasthit_q( ) -> bool{
            if ( g_config->viktor.lasthit_q_mode->get< int >( ) == 0 || *g_time - m_last_q_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.1f || !m_slot_q->is_ready( true ) )
                return false;

            const auto            lasthit_data = get_targetable_lasthit_target(
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                m_q_range + 65.f + 20.f,
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); }
            );

            if ( !lasthit_data ) return false;

            const auto target = g_entity_list.get_by_index( lasthit_data->index );
            if ( !target ) return false;

            if ( g_config->viktor.lasthit_q_mode->get< int >( ) == 1 ) {
                if ( g_features->orbwalker->can_attack( ) && g_features->orbwalker->
                                                                         is_attackable( target->index ) )
                    return false;

                const auto next_aa_time   = g_features->orbwalker->get_next_aa_time( );
                const auto next_aa_health = g_features->prediction->predict_minion_health(
                    target->index,
                    next_aa_time > *g_time ? next_aa_time - *g_time : 0.f
                );
                const auto aa_damage = helper::get_aa_damage( target->index, true );

                if ( next_aa_health > aa_damage * 0.7f ) return false;
            }

            if ( cast_spell( ESpellSlot::q, lasthit_data->network_id ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;

                g_features->orbwalker->ignore_minion( lasthit_data->index, lasthit_data->travel_time * 2.f );
                return true;
            }

            return false;
        }

        auto spellclear_q( ) -> bool{
            if ( !g_config->viktor.spellclear_q->get< bool >( ) || !g_input->is_key_pressed( utils::EKey::control ) ||
                *g_time - m_last_q_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.1f || !m_slot_q->is_ready( true ) )
                return false;

            const auto            spellclear = get_line_laneclear_target(
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                m_q_range + 90.f,
                0.f,
                false,
                false,
                true
            );

            if ( !spellclear ) return false;

            const auto target = g_entity_list.get_by_index( spellclear->target_index );
            if ( !target ) return false;

            if ( cast_spell( ESpellSlot::q, target->network_id ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;

                g_features->prediction->add_special_attack(
                    spellclear->target_index,
                    spellclear->damage,
                    spellclear->travel_time,
                    true,
                    ESpellSlot::q
                );
                return true;
            }

            return false;
        }

        auto spellclear_e( ) -> bool{
            if ( !g_config->viktor.spellclear_e->get< bool >( ) || !g_input->is_key_pressed( utils::EKey::control ) ||
                *g_time - m_last_e_time <= 0.4f || *g_time - m_last_cast_time <= 0.1f || !m_slot_e->is_ready( true ) )
                return false;

            const auto spellclear = get_viktor_spellclear_position( );
            if ( !spellclear || spellclear->count < g_config->viktor.spellclear_e_min_hitcount->get< int >( ) ) {
                return
                    false;
            }

            if ( cast_spell( ESpellSlot::e, spellclear->start_position, spellclear->end_position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto get_ult_raw_damage( const int allowed_ticks ) -> float{
            const auto initial_damage = m_r_damage[ m_slot_r->level ] + g_local->ability_power( ) *
                0.5f; // initial cast
            const auto tick_damage = m_r_tick_damage[ m_slot_r->level ] + g_local->ability_power( ) * 0.45f;

            return initial_damage + tick_damage * allowed_ticks;
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ m_slot_q->level ] + g_local->ability_power( ) * 0.4f,
                    target->index,
                    false
                );
            case ESpellSlot::e:
                return helper::calculate_damage(
                    m_e_damage[ m_slot_e->level ] + g_local->ability_power( ) * 0.5f,
                    target->index,
                    false
                );
            case ESpellSlot::r:
                return helper::calculate_damage(
                    m_r_damage[ m_slot_r->level ] + g_local->ability_power( ) * 0.5f,
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
                const auto tt   = 0.25f + g_local->position.dist_to( target->position ) / 2000.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return tt;

                return 0.25f + g_local->position.dist_to( pred.value( ) ) / 2000.f;
            }
            case ESpellSlot::e:
            {
                auto start_position{ g_local->position.extend( target->position, 500.f ) };
                if ( target->dist_to_local( ) <= 500.f ) start_position = target->position;

                const auto tt = start_position.dist_to( target->position ) / 1050.f + g_features->orbwalker->
                    get_ping( );
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return tt;

                return start_position.dist_to( pred.value( ) ) / 1050.f;
            }
            case ESpellSlot::r:
                return 0.25f;
            default:
                return 0.f;
            }

            return std::numeric_limits< float >::max( );
        }

    private:
        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_r_time{ };
        float m_last_cast_time{ };

        float m_last_r_move_time{ };

        bool m_e_upgraded{ };
        bool m_q_upgraded{ };

        std::vector< float > m_q_damage      = { 0.f, 60.f, 75.f, 90.f, 105.f, 120.f };
        std::vector< float > m_e_damage      = { 0.f, 70.f, 110.f, 150.f, 190.f, 230.f };
        std::vector< float > m_r_damage      = { 0.f, 100.f, 175.f, 250.f };
        std::vector< float > m_r_tick_damage = { 0.f, 65.f, 105.f, 145.f };

        float m_q_range{ 600.f };
        float m_w_range{ 800.f };
        float m_e_range{ 1200.f };
        float m_r_range{ 700.f };

        float m_e_start_range{ 550.f };
    };
}
