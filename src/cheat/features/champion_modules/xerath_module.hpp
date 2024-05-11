#pragma once
#include "module.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"
#include "../evade.hpp"
#include "../../sdk/game/pw_hud.hpp"

#include "../../sdk/game/hud_manager.hpp"

namespace features::champion_modules {
    class xerath_module final : public IModule {
    public:
        virtual ~xerath_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "xerath_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Xerath" ); }

        auto initialize( ) -> void override{ m_priority_list = { w_spell, q_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "xerath" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );
            const auto misc       = navigation->add_section( _( "misc" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );

            q_settings->checkbox( _( "enable" ), g_config->xerath.q_enabled );
            q_settings->checkbox( _( "harass q" ), g_config->xerath.q_harass );
            q_settings->checkbox( _( "multihit" ), g_config->xerath.q_multihit );
            q_settings->checkbox( _( "overcharge" ), g_config->xerath.q_overcharge );
            q_settings->checkbox( _( "debug pred" ), g_config->xerath.q_debug_prediction );
            q_settings->select(
                _( "hitchance" ),
                g_config->xerath.q_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );
            q_settings->select(
                _( "targeting type" ),
                g_config->xerath.q_targeting_mode,
                { _( "Single" ), _( "Multi" ) }
            );

            w_settings->checkbox( _( "enable" ), g_config->xerath.w_enabled );
            w_settings->checkbox( _( "harass w" ), g_config->xerath.w_harass );
            w_settings->checkbox( _( "flee w" ), g_config->xerath.w_flee );
            w_settings->checkbox( _( "killsteal w" ), g_config->xerath.w_killsteal );
            w_settings->checkbox( _( "antigapclose w" ), g_config->xerath.w_antigapclose );
            w_settings->checkbox( _( "multihit" ), g_config->xerath.w_multihit );
            w_settings->checkbox( _( "prefer w center" ), g_config->xerath.w_prefer_center );
            w_settings->select(
                _( "hitchance" ),
                g_config->xerath.w_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            e_settings->checkbox( _( "enable" ), g_config->xerath.e_enabled );
            e_settings->checkbox( _( "flee e" ), g_config->xerath.e_flee );
            e_settings->checkbox( _( "auto interrupt (?)" ), g_config->xerath.e_autointerrupt )->set_tooltip(
                _( "Automatically interrupts spells like Katarina R, Malzahar R, etc" )
            );
            e_settings->checkbox( _( "antigapclose e" ), g_config->xerath.e_antigapclose );
            e_settings->select(
                _( "hitchance" ),
                g_config->xerath.e_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            r_settings->checkbox( _( "enable" ), g_config->xerath.r_enabled );
            r_settings->checkbox( _( "multihit" ), g_config->xerath.r_multihit );
            r_settings->checkbox( _( "fast r mode (hotkey)" ), g_config->xerath.r_fast_mode );
            r_settings->select(
                _( "hitchance" ),
                g_config->xerath.r_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            misc->checkbox( _( "delay q/w during e travel" ), g_config->xerath.e_delay_spells_during_travel );
            misc->checkbox( _( "prefer w over q" ), g_config->xerath.prefer_w_over_q );

            drawings->checkbox( _( "draw q range" ), g_config->xerath.q_draw );
            drawings->checkbox( _( "draw w range" ), g_config->xerath.w_draw );
            drawings->checkbox( _( "draw e missile" ), g_config->xerath.e_draw_missile );
            drawings->checkbox( _( "draw r range minimap" ), g_config->xerath.r_draw_range );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->xerath.q_draw->get< bool >( ) &&
                !g_config->xerath.w_draw->get< bool >( ) &&
                !g_config->xerath.r_draw_range->get< bool >( ) &&
                !g_config->xerath.e_draw_missile->get< bool >( ) &&
                !m_in_ult ||
                g_local->is_dead( ) )
                return;

            g_local.update( );

            if ( m_in_ult ) {
                auto hud = g_pw_hud->get_hud_manager( );
                if ( !hud ) return;

                const auto cursor_position{ hud->cursor_position_unclipped };

                g_render->circle_3d(
                    cursor_position,
                    g_config->xerath.r_fast_mode->get< bool >( )
                        ? Color( 148, 33, 33, 50 )
                        : Color( 255, 255, 255, 255 ),
                    600.f,
                    g_config->xerath.r_fast_mode->get< bool >( )
                        ? Renderer::outline | Renderer::filled
                        : Renderer::outline,
                    80,
                    g_config->xerath.r_fast_mode->get< bool >( ) ? 5.f : 3.f
                );
            } else {
                auto q_slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( g_config->xerath.q_draw->get< bool >( ) && q_slot && q_slot->level > 0 ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 31, 88, 255, 255 ),
                        1470.f,
                        Renderer::outline,
                        80,
                        3.f
                    );
                }

                auto w_slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( g_config->xerath.w_draw->get< bool >( ) && w_slot && w_slot->level > 0 ) {
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

            if ( g_config->xerath.r_draw_range->get< bool >( ) ) {
                auto r_slot = g_local->spell_book.get_spell_slot( ESpellSlot::r );
                if ( r_slot && r_slot->level > 0 && ( r_slot->is_ready( false ) || m_in_ult ) ) {
                    g_render->
                        circle_minimap( g_local->position, Color( 255, 255, 255, 255 ), m_r_range, 50, 1.f );
                }
            }


            if ( g_config->xerath.e_draw_missile->get< bool >( ) && ( m_disable_spells || m_guaranteed_hit ) ) {
                if ( m_guaranteed_hit ) {
                    g_render->rectangle_3d(
                        m_missile_position,
                        m_missile_end,
                        m_missile_radius - 2.f,
                        Color( 25, 255, 25, 60 ),
                        Renderer::filled,
                        0.f
                    );
                }

                g_render->rectangle_3d(
                    m_missile_position,
                    m_missile_end,
                    m_missile_radius,
                    m_guaranteed_hit ? Color( 55, 175, 55 ) : Color( 255, 255, 255 ),
                    Renderer::outline,
                    3.f
                );

                if ( !m_guaranteed_hit ) {
                    for ( auto point : m_dodge_points ) {
                        g_render->circle_3d(
                            point,
                            Color( 52, 119, 235, 50 ),
                            20.f,
                            Renderer::outline | Renderer::filled,
                            16,
                            2.f
                        );
                    }
                }

                const auto& target = g_entity_list.get_by_index( m_missile_target );
                if ( target && target->is_visible( ) ) {
                    //g_render->line_3d(target->position, m_predicted_position, color(50, 255, 25), 2.f);
                    g_render->circle_3d(
                        m_predicted_position,
                        Color( 255, 50, 50, 80 ),
                        20.f,
                        Renderer::outline | Renderer::filled,
                        20,
                        2.f
                    );
                }
            }


            if ( !g_config->xerath.q_debug_prediction->get< bool >( ) || *g_time - m_cast_time > 2.f ) return;

            const auto& path = m_cast_path;
            if ( path.size( ) == 1u ) return;

            Color path_color{ };

            switch ( m_slot ) {
            case ESpellSlot::q:
                path_color = Color( 5, 100, 255 );
                break;
            case ESpellSlot::w:
                path_color = Color( 132, 245, 66 );
                break;
            case ESpellSlot::e:
                path_color = Color( 144, 66, 245 );
                break;
            case ESpellSlot::r:
                path_color = Color( 250, 50, 50 );
                break;
            default:
                path_color = Color::white( );
                break;
            }

            for ( auto i = m_next_path_node; i < static_cast< int >( path.size( ) ); i++ ) {
                Vec3 start{ };
                Vec3 end{ };

                if ( i == m_next_path_node ) {
                    start = m_target_current_position;
                    end   = path[ i ];
                } else {
                    start = path[ i - 1 ];
                    end   = path[ i ];
                }

                g_render->line_3d( start, end, path_color, 4.f );
            }

            g_render->circle_3d(
                m_cast_pos,
                Color( 255, 255, 0, 80 ),
                20.f,
                Renderer::E3dCircleFlags::filled | Renderer::E3dCircleFlags::outline,
                30,
                2.f
            );

            g_render->circle_3d(
                default_position,
                Color( 255, 255, 255, 40 ),
                20.f,
                Renderer::E3dCircleFlags::filled | Renderer::E3dCircleFlags::outline,
                30,
                2.f
            );
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            e_flight_tracking( );

            //PrintSpell();

            if ( g_features->orbwalker->in_action( ) || g_features->evade->
                                                                    is_active( ) )
                return;

            const auto ult_cast = g_features->buff_cache->get_buff( g_local->index, ct_hash( "XerathLocusOfPower2" ) );
            m_in_ult            = ult_cast && ult_cast->amount > 0;

            killsteal_w( );
            antigapclose_e( );
            autointerrupt_e( );
            antigapclose_w( );

            if ( g_features->orbwalker->in_attack( ) ) return;

            if ( m_in_ult ) {
                spell_r( );
                return;
            }

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:
                if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "XerathArcanopulseChargeUp" ) ) ) {
                    spell_q( );
                    return;
                }

                if ( *g_time - m_q_cast_begin > 0.2f ) m_channeling_q = false;

                if ( spell_e( ) ) return;
                if ( spell_w( ) ) return;
                if ( spell_q( ) ) return;
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "XerathArcanopulseChargeUp" ) ) ) {
                    spell_q( );
                    return;
                }

                if ( *g_time - m_q_cast_begin > 0.2f ) m_channeling_q = false;

                if ( g_config->xerath.w_harass->get< bool >( ) ) spell_w( );
                if ( g_config->xerath.q_harass->get< bool >( ) ) spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::flee:
                if ( g_config->xerath.e_flee->get< bool >( ) ) spell_e( );
                if ( g_config->xerath.w_flee->get< bool >( ) ) spell_w( );

                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->xerath.q_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.1f || *g_time -
                m_last_q_time <= 1.f )
                return false;

            const auto buff = g_features->buff_cache->get_buff(
                g_local->index,
                ct_hash( "XerathArcanopulseChargeUp" )
            );

            if ( !buff ) {
                if ( *g_time - m_q_cast_begin <= 0.2f || m_tracking_active && !m_guaranteed_hit || !m_slot_q->is_ready(
                    true
                ) )
                    return false;

                const auto target = g_features->target_selector->get_spell_specific_target( 1450.f );
                if ( !target ) return false;

                const auto pred = g_features->prediction->predict( target->index, 1500.f, 0.f, 0.f, 0.5f );

                if ( !pred.valid ) {
                    m_channeling_q = false;
                    return false;
                }

                // prefer w over q
                if ( g_config->xerath.prefer_w_over_q->get< bool >( ) && g_config->xerath.w_enabled->get< bool >( ) &&
                    target->dist_to_local( ) <= m_w_range
                    && g_local->position.dist_to( pred.position ) < m_w_range - 50.f && m_slot_w->is_ready( true ) )
                    return false;


                if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                    g_features->orbwalker->on_cast( );
                    m_q_cast_begin   = *g_time;
                    m_last_cast_time = *g_time;
                    m_channeling_q   = true;
                }

                return false;
            }

            if ( multihit_q( ) ) return true;


            if ( g_config->xerath.q_targeting_mode->get< int >( ) == 1 ) {
                for ( auto i = 0; i < 2; i++ ) {
                    if ( g_features->target_selector->is_forced( ) && i > 0 ) break;

                    const auto target = g_features->target_selector->get_default_target( i > 0 );
                    if ( !target ) break;

                    if ( charge_q( target ) ) return true;
                }
            } else return charge_q( g_features->target_selector->get_default_target( ) );

            return false;
        }

        auto charge_q( Object* target ) -> bool{
            if ( !target || m_missile_target == target->index && m_tracking_active && !m_guaranteed_hit
                || *g_time - m_last_cast_time <= 0.1f || *g_time - m_last_q_time <= 1.f )
                return false;

            const auto buff =
                g_features->buff_cache->get_buff( g_local->index, ct_hash( "XerathArcanopulseChargeUp" ) );

            if ( !buff ) return false;

            m_q_cast_begin = 0.f;
            m_channeling_q = true;

            auto sci = g_local->spell_book.get_spell_cast_info();
            if (!sci) return false;

            const auto time_charged   = *g_time - sci->server_cast_time;
            auto       range_increase = 650.f * std::clamp( time_charged / 1.75f, 0.f, 1.f );

            // static_cast< int32_t >( std::floor( ( *g_time -
            // buff->buff_data->start_time ) / 0.15f ) * 65.f );

            if ( range_increase >= 650.f ) range_increase = 720.f;

            const auto low_hitchance_time = *g_time < 1500.f ? 2.6f : 1.75f;
            const auto hitchance          = *g_time - buff->buff_data->start_time > low_hitchance_time
                                                ? 0
                                                : g_config->xerath.q_hitchance->get< int >( );

            if ( !target || m_missile_target == target->index && m_tracking_active && !m_guaranteed_hit ) return false;

            if ( g_config->xerath.q_overcharge->get< bool >( ) )
                if ( range_increase < 600.f && range_increase >=
                    65.f )
                    range_increase -= 65.f;

            const auto damage    = get_spell_damage( ESpellSlot::q, target );
            const auto will_kill = damage >= helper::get_real_health( target->index, EDamageType::magic_damage );

            auto radius = 70.f;
            auto delay  = 0.533f; // raw delay 0.5

            bool guaranteed{ };
            if ( m_guaranteed_hit && target->index == m_missile_target ) {
                delay      = m_time_to_stun < 0.533f ? m_time_to_stun : 0.533f;
                radius     = 0.f;
                guaranteed = true;
            }

            const auto pred = g_features->prediction->predict(
                target->index,
                m_q_base_range + range_increase,
                0.f,
                radius,
                delay,
                { },
                true,
                Prediction::include_ping,
                Prediction::ESpellType::linear
            );

            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( hitchance ) && !guaranteed &&
                !will_kill && g_local->position.dist_to( target->position ) > m_q_base_range )
                return false;

            if ( release_chargeable( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                m_channeling_q   = false;

                print_hitchance_reason(
                    guaranteed ? static_cast< Prediction::EHitchance >( 4 ) : pred.hitchance,
                    guaranteed ? Prediction::EHitchanceReason::predicted_stun : pred.reason,
                    target->champion_name.text,
                    ESpellSlot::q
                );

                if ( pred.hitchance >= Prediction::EHitchance::low || guaranteed ) {
                    auto aimgr = target->get_ai_manager( );
                    if ( aimgr ) {
                        const auto path = aimgr->get_path( );

                        if ( !aimgr->is_moving || path.size( ) == 1 || path.size( ) == aimgr->next_path_node ) {
                            return
                                true;
                        }

                        m_target_current_position = target->position;
                        m_next_path_node          = aimgr->next_path_node;
                        m_cast_path               = path;
                        m_cast_time               = *g_time;
                        m_cast_pos                = pred.position;
                        default_position          = pred.default_position;
                        m_slot                    = ESpellSlot::q;
                    }
                }

                return true;
            }

            return false;
        }

        auto multihit_q( ) -> bool{
            if ( !g_config->xerath.q_multihit->get< bool >( ) || *g_time - m_last_cast_time <= 0.1f || *g_time -
                m_last_q_time <= 1.f )
                return false;

            const auto buff =
                g_features->buff_cache->get_buff( g_local->index, ct_hash( "XerathArcanopulseChargeUp" ) );

            if ( !buff ) return false;

            m_q_cast_begin = 0.f;
            m_channeling_q = true;

            auto bonus_range =
                static_cast< int32_t >( std::floor( ( *g_time - buff->buff_data->start_time ) / 0.15f ) * 65.f );

            if ( bonus_range >= 650 ) bonus_range = 720;

            const auto multihit = get_multihit_position( m_q_base_range + bonus_range, 0.f, 120.f, 0.54f, true );

            if ( multihit.hit_count > 1 ) {
                if ( release_chargeable( ESpellSlot::q, multihit.position ) ) {
                    debug_log( "[ XERATH ] Q Multihit count {} | T: {}", multihit.hit_count, *g_time );

                    g_features->orbwalker->on_cast( );
                    m_last_q_time    = *g_time;
                    m_last_cast_time = *g_time;
                    m_channeling_q   = false;
                    return true;
                }
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->xerath.w_enabled->get< bool >( ) || m_channeling_q || !m_slot_w->is_ready( true ) ) {
                return
                    false;
            }


            if ( multihit_w( ) ) return true;

            for ( auto i = 0; i < 2; i++ ) {
                if ( g_features->target_selector->is_forced( ) && i > 0 ) break;

                const auto target = g_features->target_selector->get_default_target( i > 0 );
                if ( !target ) break;

                if ( combo_w( target ) ) return true;
            }

            return false;
        }

        auto combo_w( Object* target ) -> bool{
            if ( !target || m_missile_target == target->index && m_tracking_active && !m_guaranteed_hit ||
                *g_time - m_last_cast_time <= 0.1f || *g_time - m_last_w_time <= 0.4f )
                return false;

            auto radius = g_config->xerath.w_prefer_center->get< bool >( ) ? 125.f : 275.f;
            auto delay  = 0.8f;

            bool guaranteed{ };
            if ( m_guaranteed_hit && target->index == m_missile_target ) {
                delay      = m_time_to_stun < 0.8f ? m_time_to_stun : 0.8f;
                radius     = 0.f;
                guaranteed = true;
            }

            const auto damage    = get_spell_damage( ESpellSlot::w, target );
            const auto will_kill = damage > helper::get_real_health(
                target->index,
                EDamageType::magic_damage,
                0.75f
            );
            const auto is_fleeing = g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::flee;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_w_range,
                0.f,
                radius,
                delay,
                { },
                false,
                Prediction::include_ping,
                Prediction::ESpellType::circle
            );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->xerath.w_hitchance->
                get< int >( ) ) && !is_fleeing && !guaranteed && !will_kill )
                return false;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->set_last_cast( );
                g_features->orbwalker->on_cast( );

                print_hitchance_reason(
                    guaranteed ? static_cast< Prediction::EHitchance >( 4 ) : pred.hitchance,
                    guaranteed ? Prediction::EHitchanceReason::predicted_stun : pred.reason,
                    target->champion_name.text,
                    ESpellSlot::w
                );

                if ( pred.hitchance > Prediction::EHitchance::low || guaranteed ) {
                    auto aimgr = target->get_ai_manager( );
                    if ( aimgr ) {
                        const auto path = aimgr->get_path( );

                        if ( !aimgr->is_moving || path.size( ) == 1 || path.size( ) == aimgr->next_path_node ) {
                            return
                                true;
                        }

                        m_target_current_position = target->position;
                        m_next_path_node          = aimgr->next_path_node;
                        m_cast_path               = path;
                        m_cast_time               = *g_time;
                        m_cast_pos                = pred.position;
                        m_slot                    = ESpellSlot::w;
                    }
                }

                return true;
            }

            return false;
        }

        auto multihit_w( ) -> bool{
            if ( !g_config->xerath.w_multihit->get< bool >( ) || *g_time - m_last_cast_time <= 0.1f || *g_time -
                m_last_w_time <= 0.4f )
                return false;


            const auto center_multihit = get_multihit_position( m_w_range, 0.f, 100.f, 0.85f, false );

            if ( center_multihit.hit_count > 1 ) {
                if ( cast_spell( ESpellSlot::w, center_multihit.position ) ) {
                    debug_log(
                        "[ XERATH ] W hotspot Multihit count {} | DIST: {} | T: {}",
                        center_multihit.hit_count,
                        g_local->position.dist_to( center_multihit.position ),
                        *g_time
                    );

                    m_last_w_time    = *g_time;
                    m_last_cast_time = *g_time;
                    g_features->orbwalker->on_cast( );
                    return true;
                }
            }

            const auto outer_multihit = get_multihit_position( m_w_range, 0.f, 250.f, 0.85f, false );

            if ( outer_multihit.hit_count > 3 ) {
                if ( cast_spell( ESpellSlot::w, outer_multihit.position ) ) {
                    debug_log(
                        "[ XERATH ] W outer Multihit count {} | DIST: {} | T: {}",
                        outer_multihit.hit_count,
                        g_local->position.dist_to( outer_multihit.position ),
                        *g_time
                    );

                    m_last_w_time    = *g_time;
                    m_last_cast_time = *g_time;
                    g_features->orbwalker->on_cast( );
                    return true;
                }
            }

            return false;
        }

        auto killsteal_w( ) -> bool{
            if ( !g_config->xerath.w_killsteal->get< bool >( ) || m_channeling_q || *g_time - m_last_w_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.05f || !m_slot_w->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_killsteal_target(
                m_w_range,
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::w, unit ); },
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::w, unit ); },
                2
            );
            if ( !target ) return false;
            const auto pred = g_features->prediction->predict(
                target->index,
                m_w_range,
                0.f,
                80.f,
                0.8f,
                { },
                false,
                Prediction::include_ping,
                Prediction::ESpellType::circle
            );
            if ( !pred.valid ) return false;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                std::cout << "[ KS W ] Order cast at " << target->champion_name.text << std::endl;
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->xerath.e_enabled->get< bool >( ) || m_channeling_q || *g_time - m_last_e_time <= 0.4f || *
                g_time - m_last_cast_time <= 0.1f )
                return false;
            if ( !m_slot_e->is_ready( true ) ) return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto hitchance = target->dist_to_local( ) < 550.f ? 0 : g_config->xerath.e_hitchance->get< int >( );
            //if ( m_slot_q->is_ready( true ) ) hitchance = 0;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_e_range,
                1400.f,
                60.f,
                0.25f,
                { },
                true,
                Prediction::include_ping,
                Prediction::ESpellType::linear
            );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( hitchance ) ) return false;

            if ( g_features->prediction->minion_in_line_predicted(
                g_local->position,
                pred.position,
                60.f,
                0.25f,
                1400.f
            ) )
                return false;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                g_features->orbwalker->set_last_cast( );
                m_missile_target = target->index;

                print_hitchance_reason( pred.hitchance, pred.reason, target->champion_name.text, ESpellSlot::e );

                if ( pred.hitchance > Prediction::EHitchance::low ) {
                    auto aimgr = target->get_ai_manager( );
                    if ( aimgr ) {
                        const auto path = aimgr->get_path( );

                        if ( !aimgr->is_moving || path.size( ) == 1 || path.size( ) == aimgr->next_path_node ) {
                            return
                                true;
                        }

                        m_target_current_position = target->position;
                        m_next_path_node          = aimgr->next_path_node;
                        m_cast_path               = path;
                        m_cast_time               = *g_time;
                        m_cast_pos                = pred.position;
                        m_slot                    = ESpellSlot::e;
                    }
                }

                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->xerath.r_enabled->get< bool >( ) || *g_time - m_last_r_time <= 0.1f || !m_slot_r->
                is_ready( ) || *g_time - m_last_cast_time <= 0.1f ||
                !g_config->xerath.r_fast_mode->get< bool >( ) && *g_time - m_last_r_time <= 0.8f )
                return false;


            const Object* selected_target{ };
            auto          hits_to_kill_target{ std::numeric_limits< int >::max( ) };
            Vec2          sp{ };

            auto hud = g_pw_hud->get_hud_manager( );
            if ( !hud ) return false;

            if ( g_config->xerath.r_multihit->get< bool >( ) ) {
                const auto multihit = get_multihit_position(
                    m_r_range,
                    0.f,
                    200.f,
                    0.8f + g_features->orbwalker->get_ping( ),
                    false
                );

                if ( multihit.hit_count > 3 ) {
                    if ( cast_spell( ESpellSlot::r, multihit.position ) ) {
                        debug_log( "[ XERATH ] R multihit count: {} | T: {}", multihit.hit_count, *g_time );

                        m_last_r_time    = *g_time;
                        m_last_cast_time = *g_time;
                        return true;
                    }
                }
            }

            auto hitchance = g_config->xerath.r_fast_mode->get< bool >( )
                                 ? 0
                                 : g_config->xerath.r_hitchance->get< int >( );
            const auto cursor_position{ hud->cursor_position_unclipped };
            int        target_priority{ };
            int        target_kills{ };

            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || g_local->position.dist_to( enemy->position ) > 5000.f ||
                    cursor_position.dist_to( enemy->position ) > 600.f ||
                    !sdk::math::world_to_screen( enemy->position, sp ) ||
                    g_features->target_selector->is_bad_target( enemy->index ) )
                    continue;

                const auto health = helper::get_real_health( enemy->index, EDamageType::magic_damage, 0.625f, true );
                const auto hits_to_kill = static_cast< int >( std::ceil(
                    health / get_spell_damage( ESpellSlot::r, enemy )
                ) );
                const auto priority = g_features->target_selector->get_target_priority( enemy->champion_name.text );
                const auto kill_priority = false; /* todo: use kill_count again enemy->kill_count > 2*/
                ;

                if ( hits_to_kill > hits_to_kill_target || hits_to_kill == hits_to_kill_target && priority <
                    target_priority
                    || hits_to_kill == hits_to_kill_target && priority == target_priority && kill_priority
                        /* todo: use kill_count again &&
                                           target_kills >= enemy->kill_count*/ )
                    continue;

                selected_target     = enemy;
                target_priority     = priority;
                hits_to_kill_target = hits_to_kill;
            }

            if ( !selected_target ) return false;

            const auto circle_radius = 200.f;
            const auto delay         = 0.675f;

            const auto pred = g_features->prediction->predict(
                selected_target->index,
                m_r_range,
                0.f,
                circle_radius,
                delay,
                { },
                false,
                Prediction::include_ping,
                Prediction::ESpellType::circle
            );

            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( hitchance ) ) return false;

            if ( cast_spell( ESpellSlot::r, pred.position ) ) {
                m_last_r_time    = *g_time;
                m_last_cast_time = *g_time;

                debug_log( "[ XERATH ] R Cast, ping: {}", g_features->orbwalker->get_ping( ) );
                print_hitchance_reason(
                    pred.hitchance,
                    pred.reason,
                    selected_target->champion_name.text,
                    ESpellSlot::r
                );

                if ( pred.hitchance > Prediction::EHitchance::low ) {
                    auto aimgr = selected_target->get_ai_manager( );
                    if ( aimgr ) {
                        const auto path = aimgr->get_path( );

                        if ( !aimgr->is_moving || path.size( ) == 1 || path.size( ) == aimgr->next_path_node ) {
                            return
                                true;
                        }

                        m_target_current_position = selected_target->position;
                        m_next_path_node          = aimgr->next_path_node;
                        m_cast_path               = path;
                        m_cast_time               = *g_time;
                        m_cast_pos                = pred.position;
                        m_slot                    = ESpellSlot::r;
                    }
                }
            }

            return false;
        }

        auto antigapclose_w( ) -> void{
            if ( !g_config->xerath.w_antigapclose->get< bool >( ) || m_channeling_q ||
                *g_time - m_last_cast_time <= 0.1f || *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready( true ) )
                return;

            const auto target = get_advanced_antigapclose_target( m_in_ult ? 500.f : m_w_range, 0.f, 80.f, 0.85f );
            if ( !target ) return;

            const auto pred =
                g_features->prediction->predict( target->index, m_w_range, 0.f, 120.f, 0.85f );
            if ( !pred.valid || ( int )pred.hitchance < 3 ) return;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                debug_log(
                    "[ XERATH ] Antigapclose W on {} | hitchance: {}",
                    target->champion_name.text,
                    ( int )pred.hitchance
                );
            }
        }

        auto antigapclose_e( ) -> void{
            if ( !g_config->xerath.e_antigapclose->get< bool >( ) || m_channeling_q || *g_time - m_last_cast_time <=
                0.1f || *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready( true ) )
                return;

            const auto target = get_advanced_antigapclose_target(
                m_in_ult ? 500.f : m_e_range,
                1400.f,
                60.f,
                0.25f,
                true
            );
            if ( !target ) return;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_e_range,
                1400.f,
                60.f,
                0.25f,
                { },
                true
            );
            if ( !pred.valid || ( int )pred.hitchance < 3 || g_features->prediction->minion_in_line(
                g_local->position,
                pred.position,
                60.f
            ) )
                return;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                m_missile_target = target->index;
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                debug_log(
                    "[ XERATH ] Antigapclose E on {} | hitchance: {}",
                    target->champion_name.text,
                    (int)pred.hitchance
                );
            }
        }

        auto autointerrupt_e( ) -> void{
            if ( !g_config->xerath.e_autointerrupt->get< bool >( ) || m_channeling_q || m_in_ult ||
                *g_time - m_last_e_time <= 0.4f || *g_time - m_last_cast_time <= 0.1f || !m_slot_e->is_ready( true ) )
                return;

            const auto target = get_interruptable_target( m_e_range );
            if ( !target ) return;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_e_range,
                1400.f,
                60.f,
                0.25f,
                { },
                true
            );
            if ( !pred.valid || ( int )pred.hitchance < 2 || g_features->prediction->minion_in_line(
                g_local->position,
                pred.position,
                60.f
            ) )
                return;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                m_missile_target = target->index;
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                debug_log(
                    "[ XERATH ] Autointerrupt E on {} | hitchance: {}",
                    target->champion_name.text,
                    (int)pred.hitchance
                );
            }
        }

        void PrintSpell() {

            auto sci = g_local->spell_book.get_spell_cast_info();
            if (!sci) return;

            std::cout << "Spell [ " << std::dec << sci->slot << " ] " << sci->start_time
                      << " | Windup: " << sci->windup_time << " | sci->sct: " << sci->server_cast_time << std::endl;

        }

        static auto print_hitchance_reason(
            Prediction::EHitchance       hitchance,
            Prediction::EHitchanceReason reason,
            std::string                  champion_name,
            ESpellSlot                   slot
        ) -> void{
            return;

            std::string message{ "[ CAST " };

            switch ( slot ) {
            case ESpellSlot::q:
                message += "Q";
                break;
            case ESpellSlot::w:
                message += "W";
                break;
            case ESpellSlot::e:
                message += "E";
                break;
            case ESpellSlot::r:
                message += "R";
                break;
            default:
                message += "error";
                break;
            }

            message += " ] At " + champion_name + " with HC: " + std::to_string( static_cast< int >( hitchance ) ) +
                " | Reason: ";

            std::string reason_text{ };

            switch ( reason ) {
            case Prediction::EHitchanceReason::idle:
                reason_text = "idle";
                break;
            case Prediction::EHitchanceReason::recent_path:
                reason_text = "new path";
                break;
            case Prediction::EHitchanceReason::crowd_control:
                reason_text = "crowdcontrol";
                break;
            case Prediction::EHitchanceReason::dash:
                reason_text = "dashing";
                break;
            case Prediction::EHitchanceReason::guaranteed_hit:
                reason_text = "guaranteed";
                break;
            case Prediction::EHitchanceReason::similar_path_angle:
                reason_text = "similiar angle";
                break;
            case Prediction::EHitchanceReason::fresh_path_angle:
                reason_text = "fresh angle";
                break;
            case Prediction::EHitchanceReason::abnormal_path_length:
                reason_text = "abnormal path length";
                break;
            case Prediction::EHitchanceReason::post_spell:
                reason_text = "post spellcast";
                break;
            case Prediction::EHitchanceReason::reaction_time:
                reason_text = "reaction";
                break;
            case Prediction::EHitchanceReason::predicted_stun:
                reason_text = "pred stun";
                break;
            case Prediction::EHitchanceReason::spellcast:
                reason_text = "spellcast";
                break;
            case Prediction::EHitchanceReason::bad_duration:
                reason_text = "bad duration";
                break;
            case Prediction::EHitchanceReason::slowed:
                reason_text = "slowed";
                break;
            default:
                reason_text = "Unknown";
                break;
            }

            message += reason_text;

            debug_log( "{}", message );
        }

        auto e_flight_tracking( ) -> void{
            if ( !g_config->xerath.e_delay_spells_during_travel->get< bool >( ) || m_missile_target == 0 ) return;

            if ( !m_tracking_active ) {
                auto sci = g_local->spell_book.get_spell_cast_info( );
                if ( !sci || sci->slot != 2 ) return;

                m_missile_start = sci->start_position;
                m_missile_end   = sci->end_position;

                m_missile_nid        = sci->missile_nid;
                m_missile_spawn_time = sci->server_cast_time;
                m_missile_end_time   = sci->end_time;
                m_tracking_active    = true;
                m_found_missile      = false;
                m_guaranteed_hit     = false;
                // debug_log( "[ E TRACKING ] Tracking started" );
            }

            if ( *g_time > m_missile_spawn_time && *g_time - m_missile_spawn_time > 0.5f && !m_found_missile ) {
                // debug_log( "[ E TRACKING ]: No missile found, stopping" );
                m_tracking_active = false;
                m_disable_spells  = false;
                m_missile_target  = 0;
                m_guaranteed_hit  = false;
                return;
            }

            if ( *g_time >= m_missile_end_time ) {
                // debug_log( "[ E TRACKING ] Missile end time, stopping" );

                m_tracking_active = false;
                m_disable_spells  = false;
                m_missile_target  = 0;
                m_guaranteed_hit  = false;
                m_time_to_stun    = 0;
                return;
            }

            if ( *g_time < m_missile_spawn_time ) return;

            if ( !m_found_missile ) {
                for ( auto missile : g_entity_list.get_ally_missiles( ) ) {
                    if ( !missile || missile->dist_to_local( ) > 1000.f ) continue;

                    auto spell_info = missile->missile_spell_info( );
                    if ( !spell_info ) continue;

                    auto data = spell_info->get_spell_data( );
                    if ( !data || rt_hash( data->get_name( ).c_str( ) ) !=
                        ct_hash( "XerathMageSpearMissile" ) )
                        continue;

                    m_found_missile = true;
                    m_missile_index = missile->index;
                    debug_log( ">> Found  {} | index: {}", data->get_name( ), missile->index );
                    break;
                }

                return;
            }

            auto missile = g_entity_list.get_by_index( m_missile_index );
            if ( !missile ) {
                // debug_log( "[ E TRACKING ]: Invalid missile, stopping" );
                m_tracking_active = false;
                m_disable_spells  = false;
                m_missile_target  = 0;
                m_guaranteed_hit  = false;
                m_time_to_stun    = 0;
                return;
            }
            auto target = g_entity_list.get_by_index( m_missile_target );
            if ( !target || target->is_dead( ) || target->is_invisible( ) ) {
                // debug_log( "[ E TRACKING ]: Invalid target, stopping" );
                m_tracking_active = false;
                m_disable_spells  = false;
                m_missile_target  = 0;
                m_guaranteed_hit  = false;
                m_time_to_stun    = 0;
                return;
            }

            missile.update( );
            //auto  missile        = obj.get( );
            auto missile_radius = 60.f;

            auto modifier        = missile->position.dist_to( missile->missile_start_position ) / 1125.f;
            auto extend_distance = 1125.f * modifier;
            //if (extend_distance <= 1065.f) extend_distance += missile_radius;

            m_missile_position = missile->missile_start_position.extend(
                missile->missile_end_position,
                extend_distance
            );

            m_missile_end   = missile->missile_end_position;
            m_missile_start = missile->missile_start_position;

            auto closest_point = g_features->evade->get_closest_line_point(
                m_missile_position,
                m_missile_end,
                target->position
            );

            const auto tt      = m_missile_position.dist_to( closest_point ) / 1400.f;
            const auto predict = g_features->prediction->predict_default( target->index, tt );
            if ( !predict ) return;

            closest_point =
                g_features->evade->get_closest_line_point( m_missile_position, m_missile_end, predict.value( ) );

            auto target_bounding = target->get_bounding_radius( );
            auto time_until_hit  = ( m_missile_position.dist_to( closest_point ) - missile_radius ) / 1400.f;

            auto rect = sdk::math::Rectangle(
                m_missile_position,
                m_missile_end,
                target_bounding
            ); // other 60.f is target bounding radius
            auto poly = rect.to_polygon( 60 );

            m_missile_radius = 60.f + target_bounding / 2.f;

            auto pred = g_features->prediction->predict_movement( target->index, time_until_hit );

            if ( !pred ) {
                debug_log( "[ E TRACKING ]: Invalid prediction, stopping. " );
                m_tracking_active = false;
                m_disable_spells  = false;
                m_missile_target  = 0;
                m_guaranteed_hit  = false;
                m_time_to_stun    = 0;
                return;
            }

            m_predicted_position = *pred;

            if ( !poly.is_inside( *pred ) ) {
                debug_log( "[ E TRACKING ]: Target not inside E hitbox" );
                m_tracking_active = false;
                m_disable_spells  = false;
                m_guaranteed_hit  = false;
                m_missile_target  = 0;
                m_time_to_stun    = 0;
                return;
            }

            if ( g_features->prediction->minion_in_line(
                m_missile_position,
                g_features->evade->get_closest_line_point( m_missile_position, m_missile_position, *pred ),
                60.f
            ) ) {
                debug_log( "[ E TRACKING ]: Missile will collide with minion" );
                m_tracking_active = false;
                m_disable_spells  = false;
                m_missile_target  = 0;
                m_guaranteed_hit  = false;
                m_time_to_stun    = 0;
                return;
            }

            if ( m_guaranteed_hit ) {
                m_time_to_stun = time_until_hit;

                if ( time_until_hit <= 0.f ) {
                    m_tracking_active = false;
                    m_disable_spells  = false;
                    m_missile_target  = 0;
                    m_guaranteed_hit  = false;
                    m_time_to_stun    = 0;
                }

                return;
            }

            auto dodge_possibilities = g_features->evade->get_line_segment_points(
                m_missile_position,
                m_missile_end,
                60.f + target_bounding,
                true
            ).points;


            auto lowest_distance{ 99999999.f };
            bool found_safe{ };

            std::vector< Vec3 > dodge_points{ };
            for ( auto point : dodge_possibilities ) {
                if ( g_navgrid->is_wall( point ) ) continue;

                auto time_to_point = target->position.dist_to( point ) / target->movement_speed;
                if ( time_to_point >= time_until_hit ) continue;

                dodge_points.push_back( point );

                if ( target->position.dist_to( point ) < lowest_distance ) {
                    lowest_distance = target->position.dist_to( point );
                    found_safe      = true;
                }
            }

            m_dodge_points = dodge_points;

            if ( !m_guaranteed_hit && !found_safe ) {
                debug_log( "Hit guaranteed, time left: {}s", time_until_hit );
                m_guaranteed_hit = true;
            } else if ( m_guaranteed_hit ) {
                m_time_to_stun =
                    time_until_hit; // add one extra game tick to make sure pred is correct
            }

            m_disable_spells = !m_guaranteed_hit;
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->ability_power( ) * 0.75f,
                    target->index,
                    false
                );
            case ESpellSlot::w:
                return helper::calculate_damage(
                    m_w_damage[ get_slot_w( )->level ] + g_local->ability_power( ),
                    target->index,
                    false
                );
            case ESpellSlot::e:
                return helper::calculate_damage(
                    m_e_damage[ get_slot_e( )->level ] + g_local->ability_power( ) * 0.45f,
                    target->index,
                    false
                );
            case ESpellSlot::r:
                return helper::calculate_damage(
                    m_r_damage[ get_slot_r( )->level ] + g_local->ability_power( ) * 0.45f,
                    target->index,
                    false
                );
            }

            return 0.f;
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                if ( m_guaranteed_hit && m_missile_target == target->index ) return m_time_to_stun;

                return 0.5f;
            case ESpellSlot::w:
                if ( m_guaranteed_hit && m_missile_target == target->index ) return m_time_to_stun;

                return 0.75f;
            case ESpellSlot::e:
            {
                const auto tt   = 0.25f + g_local->position.dist_to( target->position ) / 1400.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.25f + g_local->position.dist_to( pred.value( ) ) / 1400.f;
            }
            }

            return IModule::get_spell_travel_time( slot, target );
        }

    private:
        // q specific stuff
        bool  m_channeling_q{ };
        float m_q_cast_begin{ };
        float m_q_base_range{ 750.f };

        // e tracking
        int16_t  m_missile_target{ };
        bool     m_found_missile{ };
        unsigned m_missile_nid{ };
        int16_t  m_missile_index{ };
        float    m_missile_spawn_time{ };
        float    m_missile_end_time{ };
        bool     m_tracking_active{ };
        Vec3     m_predicted_position{ };

        bool  m_disable_spells{ };
        bool  m_guaranteed_hit{ };
        float m_time_to_stun{ };

        std::vector< Vec3 > m_dodge_points{ };

        Vec3    m_cast_position{ };
        int16_t m_cast_index{ };
        float   m_cast_delay{ };

        Vec3  m_missile_position{ };
        Vec3  m_missile_start{ };
        Vec3  m_missile_end{ };
        float m_missile_radius{ };

        Vec3 default_position{ };

        std::array< float, 6 > m_q_damage = { 0.f, 70.f, 110.f, 150.f, 190.f, 230.f };
        std::array< float, 6 > m_w_damage = { 0.f, 100.f, 158.f, 216.f, 275.f, 333.3f };
        std::array< float, 6 > m_e_damage = { 0.f, 80.f, 110.f, 140.f, 170.f, 200.f };
        std::array< float, 6 > m_r_damage = { 0.f, 200.f, 250.f, 300.f };

        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_r_time{ };
        float m_last_cast_time{ };

        bool m_in_ult{ };

        float m_q_range{ 1400.f };
        float m_w_range{ 1000.f };
        float m_e_range{ 1000.f };
        float m_r_range{ 5000.f };

        // q debug pred
        Vec3                m_target_current_position{ };
        std::vector< Vec3 > m_cast_path{ };
        int                 m_next_path_node{ };
        Vec3                m_cast_pos{ };
        float               m_cast_time{ };
        ESpellSlot          m_slot{ };
    };
}
