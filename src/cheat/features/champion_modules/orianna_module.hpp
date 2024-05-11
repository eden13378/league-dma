#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class orianna_module final : public IModule {
    public:
        virtual ~orianna_module( ) = default;

        enum class EBallForm {
            unknown = 0,
            minion,
            missile,
            on_unit
        };

        auto get_name( ) -> hash_t override{ return ct_hash( "orianna_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Orianna" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell, e_spell, r_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation    = g_window->push( _( "orianna" ), menu_order::champion_module );
            const auto q_settings    = navigation->add_section( _( "q settings" ) );
            const auto drawings      = navigation->add_section( _( "drawings" ) );
            const auto w_settings    = navigation->add_section( _( "w settings" ) );
            const auto e_settings    = navigation->add_section( _( "e settings" ) );
            const auto r_settings    = navigation->add_section( _( "r settings" ) );
            const auto priority_ally = navigation->add_section( _( "priority ally logic" ) );

            q_settings->checkbox( _( "enable q" ), g_config->orianna.q_enabled );
            q_settings->checkbox( _( "killsteal q" ), g_config->orianna.q_killsteal );
            q_settings->checkbox( _( "harass q" ), g_config->orianna.q_harass );
            q_settings->checkbox( _( "flee q" ), g_config->orianna.q_flee );
            q_settings->checkbox( _( "increase range (?)" ), g_config->orianna.q_increase_range )->set_tooltip(
                _( "Will use spell hitbox to increase max range" )
            );;
            q_settings->checkbox( _( "position for multihit r" ), g_config->orianna.q_position_multihit_r );
            q_settings->select(
                _( "hitchance" ),
                g_config->orianna.q_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );
            q_settings->slider_int(
                _( "max ball travel distance to cast" ),
                g_config->orianna.q_max_ball_travel_distance,
                1000,
                2100,
                1
            );

            w_settings->checkbox( _( "enable w" ), g_config->orianna.w_enabled );
            w_settings->checkbox( _( "auto harass w" ), g_config->orianna.w_autoharass );
            w_settings->select(
                _( "hitchance" ),
                g_config->orianna.w_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            e_settings->checkbox( _( "enable e" ), g_config->orianna.e_enabled );

            e_settings->multi_select(
                _( "e logic " ),
                {
                    g_config->orianna.e_shield_self,
                    g_config->orianna.e_shield_allies,
                    g_config->orianna.e_damage
                },
                {
                    _( "Shield self" ),
                    _( "Shield allies" ),
                    _( "Damage (combo)" )
                }
            );

            e_settings->checkbox( _( "autoshield (?)" ), g_config->orianna.e_autoshield )->set_tooltip(
                _( "Will shield even if not in combo" )
            );

            e_settings->slider_int( _( "min damage to shield" ), g_config->orianna.e_shield_threshold, 20, 300, 1 );
            e_settings->checkbox( _( "semi automatic e (?) " ), g_config->orianna.e_semi_automatic )->set_tooltip(
                _( "Shields closest to cursor when pressing E, if no one close will shield self" )
            );


            priority_ally->checkbox( _( "enable" ), g_config->orianna.ally_priority_enabled )->set_tooltip(
                _( "Press MB5 to select a priority ally to buff with E/W" )
            );

            priority_ally->multi_select(
                _( "shield ally if " ),
                {
                    g_config->orianna.ally_shield_on_damage,
                    g_config->orianna.ally_shield_on_chase,
                    g_config->orianna.ally_shield_on_flee,
                    g_config->orianna.ally_shield_in_combat
                },
                { _( "Recieving damage" ), _( "Chasing enemy" ), _( "Fleeing" ), _( "In combat" ) }
            );

            priority_ally->multi_select(
                _( "w ally if " ),
                {
                    g_config->orianna.ally_buff_on_chase,
                    g_config->orianna.ally_buff_on_flee,
                    g_config->orianna.ally_buff_in_combat
                },
                { _( "Chasing enemy" ), _( "Fleeing" ), _( "In combat" ) }
            );

            r_settings->checkbox( _( "enable r" ), g_config->orianna.r_enabled );
            r_settings->checkbox( _( "solo target only in full combo" ), g_config->orianna.r_single_target_full_combo );
            r_settings->select(
                _( "solo target hitchance" ),
                g_config->orianna.r_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );
            r_settings->checkbox( _( "auto-interrupt (?)" ), g_config->orianna.r_autointerrupt )->set_tooltip(
                _( "Automatically interrupt spells such as Katarina R, Malzahar R, etc" )
            );
            r_settings->checkbox( _( "multihit r" ), g_config->orianna.r_multihit );
            r_settings->slider_int( _( "min multihit" ), g_config->orianna.r_min_multihit, 2, 5, 1 );

            drawings->checkbox( _( "draw q range" ), g_config->orianna.q_draw_range );
            drawings->checkbox( _( "draw w hitbox" ), g_config->orianna.w_draw_hitbox );
            drawings->checkbox( _( "draw e range" ), g_config->orianna.e_draw_range );
            //drawings->checkbox(_("draw r hitbox"), g_config->orianna.r_draw_hitbox);
            drawings->checkbox( _( "only draw off cooldown" ), g_config->orianna.dont_draw_on_cooldown );
            drawings->checkbox( _( "draw ball" ), g_config->orianna.draw_ball_effect );
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            //draw q range
            if ( g_config->orianna.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) || !g_config->orianna.dont_draw_on_cooldown->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 31, 88, 255, 255 ),
                        m_q_range + 100.f,
                        Renderer::outline,
                        72,
                        2.f
                    );
                }
            }

            if ( g_config->orianna.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) || !g_config->orianna.dont_draw_on_cooldown->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 255, 213, 0, 200 ),
                        m_e_range,
                        Renderer::outline,
                        72,
                        2.f
                    );
                }
            }

            const auto ball_position     = get_ball_position( );
            const auto ball_end_position = get_ball_end_position( );

            if ( g_config->orianna.draw_ball_effect->get< bool >( ) ) {
                if ( m_is_ball_traveling ) {
                    g_render->rectangle_3d(
                        ball_position,
                        ball_end_position,
                        80.f,
                        Color( 255, 255, 255 ),
                        Renderer::outline,
                        2.f
                    );

                    /* std::vector< sdk::math::Polygon > poly_list{
                        { {
                              sdk::math::Rectangle( ball_position, ball_end_position, 80.f ).to_polygon( ) },
                          { g_render->get_3d_circle_points( ball_end_position, 170.f, 50 ) } }
                    };

                     /* poly_list.push_back( sdk::math::Polygon{
                         sdk::math::Rectangle(
                               ball_position, ball_end_position,
                             80.f )
                             .to_polygon( ) } );

                    
                     poly_list.push_back( sdk::math::Polygon{
                         sdk::math::Circle( ball_position, 130.f).to_polygon( ) } );


                      auto draw_poly = Geometry::to_polygons( Geometry::clip_polygons( poly_list ) );
                     for ( const auto poly : draw_poly ) {

                         g_render->polygon_3d(
                             poly, color( 255, 255, 255 ),
                             c_renderer::outline,
                             3.f );
                     }*/
                } else if ( m_ball_form == EBallForm::minion ) {
                    g_render->circle_3d(
                        ball_position,
                        Color( 255, 255, 255, 30 ),
                        75.f,
                        Renderer::outline | Renderer::filled,
                        50,
                        2.f
                    );

                    //g_render->line_3d( g_local->position, ball_position, color( 255, 255, 255 ), 3.f );
                }
            }

            if ( m_ally_selected ) {
                auto ally = g_entity_list.get_by_index( m_ally_index );
                if ( ally ) {
                    ally.update( );

                    auto draw_color =
                        g_features->orbwalker->animate_color( { 66, 155, 245 }, EAnimationType::pulse, 4 );

                    if ( *g_time - m_ally_selection_time > 1.f ) {
                        const auto start_extend = g_local->position.extend( ally->position, 80.f );
                        const auto extended = g_local->position.extend( ally->position, ally->dist_to_local( ) - 80.f );


                        const auto max_thickness = 8.f;
                        const auto thickness     = max_thickness - max_thickness * ( ally->dist_to_local( ) / 1500.f );

                        g_render->line_3d( start_extend, extended, draw_color, thickness );
                    } else {
                        const auto modifier       = std::clamp( ( *g_time - m_ally_selection_time ) / 1.f, 0.f, 1.f );
                        const auto eased_modifier = utils::ease::ease_out_quart( modifier );


                        const auto extended = g_local->position.extend(
                            ally->position,
                            ( ally->dist_to_local( ) - 80.f ) * eased_modifier
                        );
                        const auto start_extend = g_local->position.extend( ally->position, 80.f );

                        const auto max_thickness = 8.f;
                        const auto thickness     = max_thickness - max_thickness * ( ally->dist_to_local( ) / 1500.f );

                        g_render->line_3d(
                            start_extend,
                            extended,
                            draw_color.alpha( 255 * eased_modifier ),
                            thickness
                        );
                    }
                }
            }

            if ( g_config->orianna.w_draw_hitbox->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) || !g_config->orianna.dont_draw_on_cooldown->get< bool >( ) ) ) {
                    const auto cycle_duration     = 1.5f;
                    const auto shortened_duration = 1.25f;
                    const auto cycle_time         = *g_time - m_animation_start_time - shortened_duration * std::floor(
                        ( *g_time - m_animation_start_time ) / shortened_duration
                    );
                    const auto modifier       = std::clamp( cycle_time / cycle_duration, 0.f, 1.f );
                    const auto eased_modifier = utils::ease::ease_out_quart( modifier );

                    g_render->circle_3d(
                        ball_end_position,
                        Color( 50, 92, 168, m_is_ball_traveling ? 60 : 90 ),
                        225.f,
                        m_is_ball_traveling ? Renderer::outline | Renderer::filled : Renderer::outline,
                        72,
                        3.f
                    );

                    if ( m_ball_form == EBallForm::on_unit ) return;

                    g_render->circle_3d(
                        ball_end_position,
                        Color( 255, 255, 255 ),
                        225.f,
                        Renderer::outline,
                        72,
                        4.f,
                        33.f,
                        ( ball_end_position - Vec3( ) ).rotated( 6.33f * eased_modifier )
                    );

                    g_render->circle_3d(
                        ball_end_position,
                        Color( 255, 255, 255 ),
                        225.f,
                        Renderer::outline,
                        72,
                        4.f,
                        33.f,
                        ( ball_end_position - Vec3( ) ).rotated( 3.165f + 6.33f * eased_modifier )
                    );
                }
            }
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            update_ball( );
            select_priority_ally( );

            if ( m_animation_start_time == 0.f ) m_animation_start_time = *g_time;

            if ( g_features->orbwalker->in_action( ) ) return;

            killsteal_q( );
            semi_automatic_e( );

            shield_priority_ally( );
            buff_priority_ally( );

            harass_w( );
            shielding_e( );
            position_ball_for_ult( );

            if ( g_features->evade->is_active( ) ) return;

            autointerrupt_r( );
            multihit_r( );

            if ( g_features->orbwalker->in_attack( ) ) return;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_q( );
                spell_w( );
                spell_e( );
                spell_r( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
            case Orbwalker::EOrbwalkerMode::laneclear:
                if ( g_config->orianna.q_harass->get< bool >( ) ) spell_q( );
            case Orbwalker::EOrbwalkerMode::flee:
                if ( g_config->orianna.q_flee->get< bool >( ) ) spell_q( );
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->orianna.q_enabled->get< bool >( ) || m_is_ball_traveling || *g_time - m_last_q_time <= 0.4f
                || *g_time - m_last_cast_time <= 0.05f || !m_slot_q->is_ready( true ) )
                return false;

            for ( auto i = 0; i < 2; i++ ) {
                if ( g_features->target_selector->is_forced( ) && i > 0 ) break;

                const auto target = g_features->target_selector->get_default_target( i > 0 );
                if ( !target ) break;

                if ( combo_q( target ) ) return true;
            }

            return false;
        }

        auto combo_q( Object* target ) -> bool{
            if ( !target ) return false;

            const auto ball_point = get_ball_position( );

            auto pred = g_features->prediction->predict(
                target->index,
                m_q_range,
                1400.f,
                170.f,
                0.f,
                ball_point,
                false,
                Prediction::include_ping | Prediction::extend_range_with_hitbox |
                Prediction::check_range_from_local_position
            );

            if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->orianna.q_hitchance->get< int >( ) ||
                pred.position.dist_to( ball_point ) > g_config->orianna.q_max_ball_travel_distance->get< int >( ) )
                return false;

            const auto cast_position = pred.position;

            if ( cast_spell( ESpellSlot::q, cast_position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Orianna: Q ] distance to cast: " << g_local->position.dist_to( cast_position )
                    << std::endl;

                return true;
            }

            return false;
        }

        auto killsteal_q( ) -> bool{
            if ( !g_config->orianna.q_killsteal->get< bool >( ) || m_is_ball_traveling || *g_time - m_last_q_time <=
                0.4f ||
                *g_time - m_last_cast_time <= 0.05f || !m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_killsteal_target(
                m_q_range + 170.f,
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                2
            );
            if ( !target ) return false;

            const auto ball_point = get_ball_position( );

            const auto pred =
                g_features->prediction->predict(
                    target->index,
                    m_q_range,
                    1400.f,
                    170.f,
                    0.f,
                    ball_point,
                    false,
                    Prediction::include_ping | Prediction::extend_range_with_hitbox |
                    Prediction::check_range_from_local_position
                );
            if ( !pred.valid ) return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ orianna ] Killsteal Q | " << *g_time
                    << std::endl;
                return true;
            }

            return false;
        }

        auto position_ball_for_ult( ) -> bool{
            if ( !g_config->orianna.q_enabled->get< bool >( ) ||
                !g_config->orianna.q_position_multihit_r->get< bool >( ) ||
                !g_config->orianna.r_multihit->get< bool >( ) || m_is_ball_traveling || !m_slot_r->is_ready( true ) ||
                *g_time - m_last_q_time <= 0.4f || *g_time - m_last_cast_time <= 0.05f || !m_slot_q->is_ready( true ) )
                return false;

            const auto ball_point = get_ball_position( );

            int  hitcount{ };
            Vec3 cast_position{ };

            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->dist_to_local( ) > 1000.f || g_features->target_selector->is_bad_target(
                    enemy->index
                ) )
                    continue;

                auto pred =
                    g_features->prediction->predict(
                        enemy->index,
                        m_q_range,
                        1400.f,
                        170.f,
                        0.5f,
                        ball_point,
                        false,
                        Prediction::include_ping | Prediction::extend_range_with_hitbox |
                        Prediction::check_range_from_local_position
                    );
                if ( !pred.valid ) return false;

                const auto travel_time = ball_point.dist_to( pred.position ) / 1400.f;
                auto       count{ pred.default_position.dist_to( pred.position ) <= 400.f ? 1 : 0 };

                const auto multihit_point{ pred.position };

                for ( const auto hero : g_entity_list.get_enemies( ) ) {
                    if ( !hero || hero->network_id == enemy->network_id || hero->dist_to_local( ) > 1000.f ||
                        g_features->target_selector->is_bad_target( hero->index ) )
                        continue;

                    const auto predict = g_features->prediction->predict(
                        hero->index,
                        415.f,
                        0.f,
                        0.f,
                        travel_time + 0.5f,
                        multihit_point
                    );
                    if ( !predict.valid ) continue;


                    ++count;
                }

                if ( count < hitcount ) continue;

                hitcount      = count;
                cast_position = multihit_point;
            }

            if ( hitcount < g_config->orianna.r_min_multihit->get< int >( ) ) return false;

            if ( cast_spell( ESpellSlot::q, cast_position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ orianna ] position ball with q for multihit, hitcount " << hitcount << " | " << *g_time
                    << std::endl;
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->orianna.w_enabled->get< bool >( ) || m_is_ball_traveling || *g_time - m_last_w_time <= 0.4f
                || *g_time - m_last_cast_time <= 0.05f || !m_slot_w->is_ready( true ) )
                return false;

            if ( harass_w( ) ) return true;

            return false;
        }

        auto harass_w( ) -> bool{
            if ( !g_config->orianna.w_enabled->get< bool >( ) || !g_config->orianna.w_autoharass->get< bool >( ) ||
                m_is_ball_traveling || *g_time - m_last_w_time <= 0.4f || *g_time - m_last_cast_time <= 0.05f || !
                m_slot_w->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto ball_position = get_ball_position( );

            const auto pred = g_features->prediction->predict(
                target->index,
                215.f,
                0.f,
                0.f,
                g_features->orbwalker->get_ping( ),
                ball_position,
                false,
                0
            );
            if ( !pred.valid ) return false;

            if ( cast_spell( ESpellSlot::w ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;

                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->orianna.e_enabled->get< bool >( ) || g_features->orbwalker->in_attack( ) ||
                m_is_ball_traveling || *g_time - m_last_e_time <= 0.4f || *g_time - m_last_cast_time <= 0.05f || !
                m_slot_e->is_ready( true ) )
                return false;

            if ( combo_e( ) || shielding_self_e( ) || shielding_allies_e( ) ) return true;

            return false;
        }

        auto shielding_e( ) -> bool{
            if ( !g_config->orianna.e_autoshield->get< bool >( ) || g_features->orbwalker->in_attack( ) ||
                m_is_ball_traveling || *g_time - m_last_e_time <= 0.4f || *g_time - m_last_cast_time <= 0.05f ||
                !m_slot_e->is_ready( true ) )
                return false;

            if ( shielding_self_e( ) || shielding_allies_e( ) ) return true;

            return false;
        }

        auto semi_automatic_e( ) -> bool{
            if ( !g_config->orianna.e_semi_automatic->get< bool >( ) || !g_input->is_key_pressed( utils::EKey::E ) ||
                m_is_ball_traveling || *g_time - m_last_e_time <= 0.4f || *g_time - m_last_cast_time <= 0.05f ||
                !m_slot_e->is_ready( true ) )
                return false;

            const auto cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;
            auto       lowest_distance{ 325.f };
            unsigned   target_nid{ };

            for ( const auto ally : g_entity_list.get_allies( ) ) {
                if ( !ally || ally->dist_to_local( ) > m_e_range || ally->is_dead( ) || ally->is_invisible( ) ||
                    ally->get_selectable_flag( ) != 1 )
                    continue;

                const auto distance = ally->position.dist_to( cursor );
                if ( distance > lowest_distance ) continue;

                target_nid      = ally->network_id;
                lowest_distance = distance;
            }

            if ( target_nid == 0 ) return false;

            if ( cast_spell( ESpellSlot::e, target_nid ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ orianna ] semi auto E | " << *g_time << std::endl;

                return true;
            }

            return false;
        }

        auto combo_e( ) -> bool{
            if ( !g_config->orianna.e_damage->get< bool >( ) ) return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto ball = get_ball_position( );

            const auto rect = sdk::math::Rectangle(
                g_local->position,
                ball,
                80.f + g_features->prediction->get_champion_radius( rt_hash( target->champion_name.text ) ) / 2.f
            );
            const auto polygon = rect.to_polygon( );

            const auto nearest_point = g_features->evade->get_closest_line_point(
                g_local->position,
                ball,
                target->position
            );

            const auto travel_time = ball.dist_to( nearest_point ) / 1850.f;
            auto       pred        = g_features->prediction->predict( target->index, 1300.f, 0.f, 80.f, travel_time );
            if ( !pred.valid || ( int )pred.hitchance < g_config->orianna.e_hitchance->get< int >( ) ||
                polygon.is_outside( pred.position ) )
                return false;

            if ( cast_spell( ESpellSlot::e, g_local->network_id ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto shielding_self_e( ) -> bool{
            if ( !g_config->orianna.e_shield_self->get< bool >( ) ) return false;

            auto ball_position = get_ball_position( );

            const auto incoming_damage = helper::get_incoming_damage( g_local->index, 0.5f );
            if ( incoming_damage <= static_cast< float >( g_config->orianna.e_shield_threshold->get< int >( ) ) ) {
                return
                    false;
            }

            if ( cast_spell( ESpellSlot::e, g_local->network_id ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto shielding_allies_e( ) -> bool{
            if ( !g_config->orianna.e_shield_allies->get< bool >( ) ) return false;

            bool     allow_cast{ };
            unsigned target_network_id{ };

            for ( const auto ally : g_entity_list.get_allies( ) ) {
                if ( !ally || ally->network_id == g_local->network_id || ally->is_dead( ) || ally->is_invisible( ) ||
                    ally->get_selectable_flag( ) != 1 || ally->dist_to_local( ) > m_e_range )
                    continue;

                const auto incoming_damage = helper::get_incoming_damage( g_local->index, 0.5f );
                if ( incoming_damage < static_cast< float >( g_config->orianna.e_shield_threshold->get<
                    int >( ) ) )
                    continue;

                allow_cast        = true;
                target_network_id = ally->network_id;
            }

            if ( !allow_cast ) return false;

            if ( cast_spell( ESpellSlot::e, target_network_id ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto shield_priority_ally( ) -> bool{
            if ( !m_ally_selected || m_is_ball_traveling || *g_time - m_last_e_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.05f
                || !m_slot_e->is_ready( true ) )
                return false;

            auto ally = g_entity_list.get_by_index( m_ally_index );
            if ( !ally ) return false;

            ally.update( );

            if ( ally->is_dead( ) || ally->is_invisible( ) || ally->get_selectable_flag( ) != 1 || ally->
                dist_to_local( ) > m_e_range )
                return false;

            int cast_reason{ };

            auto allow_cast{
                g_config->orianna.ally_shield_on_damage->get< bool >( ) &&
                helper::get_incoming_damage( m_ally_index, 0.5f ) >
                g_config->orianna.e_shield_threshold->get< int >( )
            };
            if ( allow_cast ) cast_reason = 1;

            if ( !allow_cast && g_config->orianna.ally_shield_in_combat->get< bool >( ) ) {
                auto sci = ally->spell_book.get_spell_cast_info( );
                if ( sci && sci->server_cast_time > *g_time ) {
                    const auto target_index = sci->get_target_index( );
                    const auto ally_target  = g_entity_list.get_by_index( target_index );

                    if ( ally_target && ally_target->is_hero( ) && ally_target->position.dist_to( ally->position ) <=
                        750.f )
                        allow_cast = true;
                }
            }

            if ( allow_cast ) cast_reason = 2;

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
                        ally_direction.dist_to( enemy->position ) > ally->position.dist_to( enemy->position ) +
                        chase_threshold &&
                        enemy_direction.dist_to( ally->position ) + chase_threshold <
                        enemy->position.dist_to( ally->position ) &&
                        enemy_direction_angle <= 35.f
                    };

                    const auto is_ally_chasing{
                        ally_direction.dist_to( enemy->position ) + chase_threshold < ally->position.dist_to(
                            enemy->position
                        ) &&
                        enemy_direction.dist_to( ally->position ) > enemy->position.dist_to( ally->position ) +
                        chase_threshold &&
                        ally_direction_angle <= 35.f
                    };

                    if ( is_ally_chasing && g_config->orianna.ally_shield_on_flee->get< bool >( ) ) {
                        allow_cast  = true;
                        cast_reason = 3;
                        break;
                    }

                    if ( is_chasing_ally && g_config->orianna.ally_shield_on_flee->get< bool >( ) ) {
                        allow_cast  = true;
                        cast_reason = 4;
                        break;
                    }
                }
            }

            if ( !allow_cast ) return false;

            std::string reason_text{ };

            switch ( cast_reason ) {
            case 1:
                reason_text = "Incoming damage";
                break;
            case 2:
                reason_text = "In combat";
                break;
            case 3:
                reason_text = "Chasing enemy";
                break;
            case 4:
                reason_text = "Fleeing enemy";
                break;
            default:
                std::cout << "unknown ally shield reason: " << cast_reason << std::endl;
                return false;
            }

            if ( cast_spell( ESpellSlot::e, ally->network_id ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ ORI ] Shielded priority ally due to " << reason_text << std::endl;
                return true;
            }

            return false;
        }

        auto buff_priority_ally( ) -> bool{
            if ( !m_ally_selected || m_is_ball_traveling || *g_time - m_last_w_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.05f ||
                m_ball_form != EBallForm::on_unit || m_ball_parent_unit_index != m_ally_index ||
                !m_slot_w->is_ready( true ) )
                return false;

            auto ally = g_entity_list.get_by_index( m_ally_index );
            if ( !ally ) return false;

            ally.update( );

            if ( ally->is_dead( ) || ally->is_invisible( ) || ally->get_selectable_flag( ) != 1 ) return false;

            int cast_reason{ };

            bool allow_cast{ };

            if ( !allow_cast && g_config->orianna.ally_buff_in_combat->get< bool >( ) ) {
                auto sci = ally->spell_book.get_spell_cast_info( );
                if ( sci && sci->server_cast_time > *g_time ) {
                    const auto target_index = sci->get_target_index( );
                    const auto ally_target  = g_entity_list.get_by_index( target_index );

                    if ( ally_target && ally_target->is_hero( ) &&
                        ally_target->position.dist_to( ally->position ) <= 750.f )
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

                    if ( is_ally_chasing && g_config->orianna.ally_buff_on_chase->get< bool >( ) ) {
                        allow_cast  = true;
                        cast_reason = 2;
                        break;
                    }

                    if ( is_chasing_ally && g_config->orianna.ally_buff_on_flee->get< bool >( ) ) {
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

            if ( cast_spell( ESpellSlot::w ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ ORI ] Shielded priority ally due to " << reason_text << std::endl;
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->orianna.r_enabled->get< bool >( ) ||
                g_config->orianna.r_single_target_full_combo->get< bool >( ) && !g_input->is_key_pressed(
                    utils::EKey::control
                ) || m_is_ball_traveling ||
                *g_time - m_last_r_time <= 0.4f || *g_time - m_last_cast_time <= 0.05f || !m_slot_r->is_ready( true ) )
                return false;

            const auto ball_position = get_ball_position( );

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto pred = g_features->prediction->predict( target->index, 500.f, 0.f, 415.f, 0.5f, ball_position );
            if ( !pred.valid || pred.default_position.dist_to( ball_position ) >= 400.f || ( int )pred.hitchance <
                g_config->orianna.r_hitchance->get< int >( ) )
                return false;

            if ( cast_spell( ESpellSlot::r ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                std::cout << "[ orianna ] single target ult | hitchance " << ( int )pred.hitchance << " | " << *g_time
                    << std::endl;

                return true;
            }

            return false;
        }

        auto multihit_r( ) -> bool{
            if ( !g_config->orianna.r_multihit->get< bool >( ) || m_is_ball_traveling ||
                *g_time - m_last_r_time <= 0.4f || *g_time - m_last_cast_time <= 0.05f || !m_slot_r->is_ready( true ) )
                return false;

            const auto ball_pos = get_ball_position( 0.5f );

            auto hitcount = 0;

            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->position.dist_to( ball_pos ) > 600.f ||
                    g_features->target_selector->is_bad_target( enemy->index ) )
                    continue;

                auto pred = g_features->prediction->predict_default( enemy->index, 0.5f );
                if ( !pred || ball_pos.dist_to( *pred ) > 400.f ) continue;

                hitcount++;
            }

            if ( hitcount < g_config->orianna.r_min_multihit->get< int >( ) ) return false;

            if ( cast_spell( ESpellSlot::r ) ) {
                m_last_r_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                std::cout << "[ orianna ] r multihit : hitcount " << hitcount << " | " << *g_time << std::endl;
                return true;
            }

            return false;
        }

        auto autointerrupt_r( ) -> void{
            if ( !g_config->orianna.r_autointerrupt->get< bool >( ) || m_is_ball_traveling || *g_time - m_last_r_time <=
                0.4f ||
                *g_time - m_last_cast_time <= 0.1f || !m_slot_r->is_ready( true ) )
                return;

            const auto target = get_interruptable_target( 1250.f );
            if ( !target ) return;

            const auto ball = get_ball_position( 0.5f );

            const auto pred = g_features->prediction->predict( target->index, 415.f, 0.f, 400.f, 0.5f, ball );
            if ( !pred.valid || ( int )pred.hitchance < 3 ) return;

            if ( cast_spell( ESpellSlot::r ) ) {
                m_last_r_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                debug_log(
                    "[ orianna ] Autointerrupt R against {} | hitchance: {}",
                    target->champion_name.text,
                    ( int )pred.hitchance
                );
            }
        }

        auto select_priority_ally( ) -> bool{
            if ( !g_config->orianna.ally_priority_enabled->get< bool >( ) ) {
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
                    if ( !ally || ally->dist_to_local( ) > 1350.f || ally->network_id == g_local->network_id || ally->
                        is_dead( ) ||
                        ally->is_invisible( ) || ally->get_selectable_flag( ) != 1 )
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
                        4
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
            if ( ally->is_dead( ) || ally->is_invisible( ) || ally->dist_to_local( ) > 1500.f ) {
                unselect_ally( );
                return false;
            }

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

        auto update_ball( ) -> void{
            if ( m_ball_form == EBallForm::missile ) {
                auto missile = g_entity_list.get_by_index( m_ball_missile_index );
                if ( missile ) {
                    missile.update( );

                    m_ball_missile_position    = missile->position;
                    m_ball_missile_last_update = *g_time;

                    const auto time_traveled     = *g_time - m_ball_missile_spawn_time;
                    const auto distance_traveled = time_traveled * ( m_ball_is_q_missile ? 1400.f : 1850.f );

                    m_ball_position = get_ball_position( );

                    if ( m_ball_is_q_missile && distance_traveled >= m_ball_missile_start_point.dist_to(
                        m_ball_missile_end_point
                    ) ) {
                        m_was_ball_missile   = false;
                        m_ball_missile_index = 0;
                        m_ball_form          = EBallForm::unknown;
                        m_is_ball_traveling  = false;
                    } else if ( !m_ball_is_q_missile ) {
                        if ( m_ball_missile_parent_index == g_local->index ) {
                            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "orianaghostself" ) ) ) {
                                m_was_ball_missile   = false;
                                m_ball_missile_index = 0;
                                m_ball_form          = EBallForm::unknown;
                                m_is_ball_traveling  = false;
                            }
                        } else {
                            for ( const auto ally : g_entity_list.get_allies( ) ) {
                                if ( !ally || ally->is_dead( ) || ally->dist_to_local( ) > 2500.f ) continue;

                                if ( g_features->buff_cache->get_buff( ally->index, ct_hash( "orianaghost" ) ) ) {
                                    m_was_ball_missile   = false;
                                    m_ball_missile_index = 0;
                                    m_ball_form          = EBallForm::unknown;
                                    m_is_ball_traveling  = false;
                                    break;
                                }
                            }
                        }
                    }

                    return;
                }

                m_was_ball_missile   = false;
                m_ball_missile_index = 0;
                m_ball_form          = EBallForm::unknown;
                m_is_ball_traveling  = false;
            }

            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "orianaghostself" ) ) ) {
                if ( m_ball_form == EBallForm::minion ) {
                    m_ball_ignored_minion_network_id =
                        m_ball_stationary_network_id;
                }

                m_ball_position          = g_local->position;
                m_ball_parent_unit_index = g_local->index;
                m_ball_form              = EBallForm::on_unit;
                m_is_ball_traveling      = false;

                return;
            }

            for ( const auto missile : g_entity_list.get_ally_missiles( ) ) {
                if ( !missile ) continue;

                auto info = missile->missile_spell_info( );
                if ( !info ) continue;

                auto data = info->get_spell_data( );
                if ( !data ) continue;

                auto       name      = data->get_name( );
                const auto name_hash = rt_hash( name.data( ) );
                if ( name_hash != ct_hash( "OrianaIzuna" ) && name_hash != ct_hash( "OrianaRedact" ) ) continue;

                m_ball_position      = missile->position;
                m_ball_missile_index = missile->index;
                m_was_ball_missile   = true;

                if ( m_ball_form == EBallForm::minion ) {
                    m_ball_ignored_minion_network_id =
                        m_ball_stationary_network_id;
                }

                m_ball_is_q_missile = name_hash == ct_hash( "OrianaIzuna" );

                if ( !m_ball_is_q_missile ) {
                    auto    lowest_distance{ 1000.f };
                    int16_t parent_index{ };

                    for ( const auto ally : g_entity_list.get_allies( ) ) {
                        if ( !ally || ally->is_dead( ) || ally->position.dist_to( missile->missile_end_position ) >
                            lowest_distance )
                            continue;

                        parent_index    = ally->index;
                        lowest_distance = ally->position.dist_to( missile->missile_end_position );
                    }

                    m_ball_missile_parent_index = parent_index;
                }

                m_ball_missile_position    = missile->position;
                m_ball_missile_last_update = *g_time;
                m_is_ball_traveling        = true;

                m_ball_missile_end_point   = missile->missile_end_position;
                m_ball_missile_start_point = missile->missile_start_position;
                m_ball_missile_spawn_time  = missile->missile_spawn_time( );
                m_ball_form                = EBallForm::missile;

                return;
            }

            for ( const auto minion : g_entity_list.get_ally_minions( ) ) {
                if ( !minion || rt_hash( minion->name.text ) != ct_hash( "TheDoomBall" ) || minion->network_id ==
                    m_ball_ignored_minion_network_id || minion->is_dead( ) )
                    continue;

                if ( m_ball_form == EBallForm::minion && m_ball_stationary_network_id != minion->
                    network_id )
                    m_ball_ignored_minion_network_id = m_ball_stationary_network_id;

                m_ball_position              = minion->position;
                m_ball_stationary_network_id = minion->network_id;
                m_ball_form                  = EBallForm::minion;

                m_is_ball_traveling = false;

                /* if ( g_function_caller->is_glow_queueable( ) ) {

                    g_function_caller->enable_glow(
                        minion->network_id, D3DCOLOR_ARGB( 255, 0, 0, 5 ), 5, 0, false );
                }*/

                return;
            }

            for ( const auto ally : g_entity_list.get_allies( ) ) {
                if ( !ally || ally->is_dead( ) || ally->dist_to_local( ) > 1375.f ) continue;

                const auto buff = g_features->buff_cache->get_buff( ally->index, ct_hash( "orianaghost" ) );
                if ( !buff ) continue;

                if ( m_ball_form == EBallForm::minion ) {
                    m_ball_ignored_minion_network_id =
                        m_ball_stationary_network_id;
                }

                m_ball_position          = ally->position;
                m_ball_parent_unit_index = ally->index;
                m_ball_form              = EBallForm::on_unit;

                m_is_ball_traveling = false;

                return;
            }
        }

        auto get_ball_position( const float predict_amount = 0.f ) -> Vec3{
            switch ( m_ball_form ) {
            case EBallForm::on_unit:
            {
                if ( m_ball_parent_unit_index == g_local->index ) {
                    m_ball_position = g_local->position;
                    break;
                }

                auto ally = g_entity_list.get_by_index( m_ball_parent_unit_index );
                if ( !ally ) {
                    m_ball_form = EBallForm::unknown;
                    break;
                }

                ally.update( );

                if ( ally->is_dead( ) || ally->is_invisible( ) ) {
                    m_ball_form = EBallForm::unknown;
                    break;
                }

                if ( predict_amount > 0.f ) {
                    const auto pred = g_features->prediction->predict_default( ally->index, predict_amount, false );
                    if ( !pred ) return ally->position;

                    return *pred;
                }

                m_ball_position = ally->position;
            }
            case EBallForm::missile:
            {
                auto time_traveled = *g_time - m_ball_missile_spawn_time + g_features->orbwalker->get_ping( );

                if ( !m_ball_is_q_missile ) {
                    time_traveled = *g_time - m_ball_missile_last_update + g_features->orbwalker->get_ping( );

                    if ( m_ball_missile_parent_index == g_local->index ) {
                        const auto simulated_position = m_ball_missile_position.extend(
                            g_local->position,
                            std::min( time_traveled * 1850.f, m_ball_missile_position.dist_to( g_local->position ) )
                        );


                        m_ball_position = simulated_position;
                        return m_ball_position;
                    }

                    auto unit = g_entity_list.get_by_index( m_ball_missile_parent_index );
                    if ( unit ) {
                        unit.update( );
                        if ( unit->is_alive( ) && unit->is_visible( ) ) {
                            const auto simulated_position = m_ball_missile_position.extend(
                                unit->position,
                                std::min( time_traveled * 1850.f, m_ball_missile_position.dist_to( unit->position ) )
                            );


                            m_ball_position = simulated_position;
                            return m_ball_position;
                        }
                    }
                }

                const auto simulated_position = m_ball_missile_start_point.extend(
                    m_ball_missile_end_point,
                    std::min(
                        time_traveled * 1400.f,
                        m_ball_missile_start_point.dist_to( m_ball_missile_end_point )
                    )
                );

                m_ball_position = simulated_position;
                break;
            }
            default:
                break;
            }

            return m_ball_position;
        }

        auto get_ball_end_position( ) -> Vec3{
            switch ( m_ball_form ) {
            case EBallForm::on_unit:
            {
                if ( m_ball_parent_unit_index == g_local->index ) return g_local->position;

                auto ally = g_entity_list.get_by_index( m_ball_parent_unit_index );
                if ( !ally ) {
                    m_ball_form = EBallForm::unknown;
                    break;
                }

                ally.update( );

                if ( ally->is_dead( ) || ally->is_invisible( ) ) {
                    m_ball_form = EBallForm::unknown;
                    break;
                }

                return ally->position;
            }
            case EBallForm::missile:
            {
                if ( !m_ball_is_q_missile ) {
                    if ( m_ball_missile_parent_index == g_local->index ) return g_local->position;


                    auto unit = g_entity_list.get_by_index( m_ball_missile_parent_index );
                    if ( unit ) {
                        unit.update( );
                        if ( unit->is_alive( ) && unit->is_visible( ) ) return unit->position;
                    }
                }

                return m_ball_missile_end_point;
            }
            default:
                break;
            }

            return m_ball_position;
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ m_slot_q->level ] + g_local->ability_power( ) * 0.5f,
                    target->index,
                    false
                );
            case ESpellSlot::r:
                return helper::calculate_damage(
                    m_r_damage[ get_slot_r( )->level ] + g_local->bonus_attack_damage( ) +
                    g_local->ability_power( ) * 0.9f,
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
                const auto ball_position = get_ball_position( );

                const auto tt   = ball_position.dist_to( target->position ) / 1400.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                const auto compensation = 80.f / target->movement_speed;

                return ball_position.dist_to( pred.value( ) ) / 1400.f - compensation;
            }
            case ESpellSlot::r:
            {
                const auto tt   = 1.f + g_local->position.dist_to( target->position ) / 2000.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 1.f + g_local->position.dist_to( pred.value( ) ) / 2000.f;
            }
            }

            return std::numeric_limits< float >::max( );
        }

    private:
        float                m_q_range{ 825.f };
        float                m_q_radius{ 175.f / 2 };
        float                m_q_cast_time{ 0.f };
        float                m_q_speed{ 1400.f };
        float                m_last_q_time{ };
        std::vector< float > m_q_damage = { 0.f, 60.f, 90.f, 120.f, 150.f, 180.f };

        float                m_w_range{ 0.f };
        float                m_w_radius{ 225.f / 2 };
        float                m_w_cast_time{ 0.f };
        float                m_last_w_time{ };
        std::vector< float > m_w_damage = { 0.f, 60.f, 105.f, 150.f, 195.f, 240.f };

        float                m_e_range{ 1120.f };
        float                m_e_cast_time{ 0.f };
        float                m_e_speed{ 1850.f };
        float                m_last_e_time{ };
        std::vector< float > m_e_damage = { 0.f, 60.f, 90.f, 120.f, 150.f, 180.f };

        float                m_r_range{ 0.f };
        float                m_r_radius{ 415.f };
        float                m_r_cast_time{ 0.5f };
        float                m_last_r_time{ };
        std::vector< float > m_r_damage = { 0.f, 200.f, 275.f, 350.f };

        float m_last_cast_time{ };

        // priority ally logic
        bool     m_ally_selected{ };
        int16_t  m_ally_index{ };
        unsigned m_ally_nid{ };

        bool m_holding_key{ };

        bool  m_ally_glowing{ };
        float m_ally_last_glow_time{ };

        float m_ally_selection_time{ };


        // animation
        float m_animation_start_time{ };

        // ball tracking
        Vec3 m_ball_position{ };

        bool m_is_ball_traveling{ };

        bool    m_was_ball_missile{ };
        int16_t m_ball_missile_index{ };
        bool    m_ball_is_q_missile{ };
        int16_t m_ball_missile_parent_index{ };

        Vec3  m_ball_missile_position{ };
        float m_ball_missile_last_update{ };

        unsigned m_ball_ignored_minion_network_id{ };
        unsigned m_ball_stationary_network_id{ };

        int16_t m_ball_parent_unit_index{ };


        EBallForm m_ball_form{ };

        Vec3 m_ball_missile_end_point{ };
        Vec3 m_ball_missile_start_point{ };

        float m_ball_missile_spawn_time{ };

        bool ball_is_minion{ };
        bool ball_is_on_champ{ };
        bool ball_is_on_local{ };
    };
}
