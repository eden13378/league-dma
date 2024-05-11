#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class corki_module final : public IModule {
    public:
        virtual ~corki_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "corki_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Corki" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "corki" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );
            const auto spellclear = navigation->add_section( _( "spellclear" ) );

            q_settings->checkbox( _( "enable" ), g_config->corki.q_enabled );
            q_settings->checkbox( _( "harass q" ), g_config->corki.q_harass );
            q_settings->checkbox( _( "killsteal q" ), g_config->corki.q_killsteal );
            q_settings->checkbox( _( "multihit q" ), g_config->corki.q_multihit );
            q_settings->checkbox( _( "immobile q" ), g_config->corki.q_immobile );
            q_settings->checkbox( _( "increase range (?)" ), g_config->corki.q_increase_range )->set_tooltip(
                _( "Range will be increased by using part of spell hitbox as extra range" )
            );
            q_settings->select(
                _( "hitchance" ),
                g_config->corki.q_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            w_settings->checkbox( _( "antigapclose w" ), g_config->corki.w_antigapclose );
            w_settings->checkbox( _( "flee w" ), g_config->corki.w_flee );

            e_settings->checkbox( _( "enable" ), g_config->corki.e_enabled );

            r_settings->checkbox( _( "enable" ), g_config->corki.r_enabled );
            r_settings->checkbox( _( "harass r" ), g_config->corki.r_harass );
            r_settings->checkbox( _( "killsteal r" ), g_config->corki.r_killsteal );
            r_settings->select(
                _( "hitchance" ),
                g_config->corki.r_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );
            r_settings->slider_int( _( "max range %" ), g_config->corki.r_max_range, 75, 100, 1 );

            spellclear->checkbox( _( "spellclear q (hold ctrl)" ), g_config->corki.q_spellclear );

            drawings->checkbox( _( "draw q range" ), g_config->corki.q_draw_range );
            drawings->checkbox( _( "draw w range" ), g_config->corki.w_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->corki.e_draw_range );
            drawings->checkbox( _( "draw r range" ), g_config->corki.r_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->corki.dont_draw_on_cooldown );
            //drawings->checkbox( _( "draw r damage" ), g_config->corki.r_draw_damage );
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };


            if ( g_config->corki.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->corki.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 31, 88, 255, 255 ),
                        m_q_range + m_extra_q_range,
                        Renderer::outline,
                        64,
                        2.f
                    );
                }
            }

            if ( g_config->corki.w_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->corki.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 132, 245, 66, 255 ),
                        m_w_range,
                        Renderer::outline,
                        64,
                        2.f
                    );
                }
            }

            if ( g_config->corki.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->corki.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 144, 66, 245, 255 ),
                        m_e_range,
                        Renderer::outline,
                        64,
                        2.f
                    );
                }
            }

            if ( g_config->corki.r_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::r );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->corki.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    auto r_range = m_special_missile ? m_special_r_range : m_r_range;
                    r_range *= g_config->corki.r_max_range->get< int >( ) / 100.f;

                    g_render->circle_3d(
                        g_local->position,
                        Color( 255, 50, 50, 255 ),
                        r_range,
                        Renderer::outline,
                        -1,
                        m_special_missile ? 5.f : 3.f
                    );
                }
            }

            /*if ( m_point_saved ) {

                g_render->circle_3d(
                    m_flee_point, color( 255, 255, 25, 80 ), 25.f, c_renderer::outline | c_renderer::filled, 32, 2.f );

            }*/

            return; // disabled r damage draw due to causing crashes

            if ( !g_config->corki.r_draw_damage->get< bool >( ) || !m_slot_r->is_ready( ) ) return;

            for ( const auto index : g_features->tracker->get_enemies( ) ) {
                auto enemy = g_entity_list.get_by_index( index );
                if ( !enemy ) continue;

                enemy.update( );

                if ( enemy->is_dead( ) || enemy->is_invisible( ) || enemy->dist_to_local( ) > 1500.f ) continue;

                Vec2 sp{ };
                if ( !world_to_screen( enemy->position, sp ) ) continue;

                auto base_x = 1920.f;
                auto base_y = 1080.f;

                auto width_ratio  = base_x / static_cast< float >( g_render_manager->get_width( ) );
                auto height_ratio = base_y / static_cast< float >( g_render_manager->get_height( ) );

                auto width_offset  = width_ratio * 0.055f;
                auto height_offset = height_ratio * 0.0222f;

                auto bar_length = width_offset * static_cast< float >( g_render_manager->get_width( ) );
                auto bar_height = height_offset * static_cast< float >( g_render_manager->get_height( ) );

                auto base_position = enemy->get_hpbar_position( );

                auto buffer = 1.f;

                base_position.x -= bar_length * 0.43f;
                base_position.y -= bar_height;

                auto damage      = get_spell_damage( ESpellSlot::r, enemy.get( ) );
                auto modifier    = enemy->health / enemy->max_health;
                auto damage_mod  = damage / enemy->max_health;
                auto is_killable = damage > enemy->health;

                Vec2 box_start{ base_position.x + bar_length * modifier + buffer, base_position.y - buffer };
                Vec2 box_size{
                    damage_mod * bar_length > box_start.x - base_position.x
                        ? base_position.x - box_start.x - buffer * 1.f
                        : -( bar_length * damage_mod ) - buffer * 1.f,
                    bar_height * 0.5f + buffer * 2.f
                };

                g_render->filled_box(
                    box_start,
                    box_size,
                    is_killable ? g_features->orbwalker->get_pulsing_color( ).alpha( 175 ) : Color( 40, 150, 255, 180 )
                );
            }
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            //flee_w( );

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return;

            m_special_missile = m_slot_r->get_usable_state( ) == 1;

            m_extra_q_range = g_config->corki.q_increase_range->get< bool >( ) ? 125.f : 0.f;

            antigapclose_w( );

            auto aimgr = g_local->get_ai_manager( );
            if ( aimgr ) m_in_dash = aimgr->is_moving && aimgr->is_dashing;

            killsteal_r( );
            killsteal_q( );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_e( );

                if ( g_features->orbwalker->in_attack( ) ) return;

                spell_r( );
                spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::laneclear:
                if ( g_features->orbwalker->in_attack( ) ) return;

                if ( g_config->corki.q_harass->get< bool >( ) && !is_position_in_turret_range( ) ) spell_q( );
                if ( g_config->corki.r_harass->get< bool >( ) && !is_position_in_turret_range( ) ) spell_r( );

                spellclear_q( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_features->orbwalker->in_attack( ) ) return;

                if ( g_config->corki.q_harass->get< bool >( ) && !is_position_in_turret_range( ) ) spell_q( );
                if ( g_config->corki.r_harass->get< bool >( ) && !is_position_in_turret_range( ) ) spell_r( );
                break;
            default:
                break;
            }

            //std::cout << "name: " << m_slot_w->get_name( ) << " | state: " << ( int )m_slot_w->get_usable_state( )
            //          << std::endl;
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->corki.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.3f || *g_time -
                m_last_cast_time <= 0.05f || !can_cast_q( ) )
                return false;

            if ( g_config->corki.q_multihit->get< bool >( ) ) {
                const auto multihit = get_multihit_position( m_q_range, 1000.f, 250.f, 0.25f, false );

                if ( multihit.hit_count > 1 ) {
                    if ( cast_spell( ESpellSlot::q, multihit.position ) ) {
                        m_last_q_time    = *g_time;
                        m_last_cast_time = *g_time;
                        g_features->orbwalker->on_cast( );
                        return true;
                    }
                }
            }

            for ( auto i = 0; i < 2; i++ ) {
                if ( g_features->target_selector->is_forced( ) && i > 0 ) break;

                const auto target = g_features->target_selector->get_default_target( i > 0 );
                if ( !target ) break;

                if ( combo_q( target ) || immobile_q( target ) ) return true;
            }

            return false;
        }

        auto combo_q( Object* target ) -> bool{
            if ( !target ||
                g_features->orbwalker->can_attack( target->index ) && g_features->orbwalker->is_attackable(
                    target->index
                ) )
                return false;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_q_range + m_extra_q_range,
                1000.f,
                275.f,
                0.25f
            );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->corki.q_hitchance->
                    get< int >( ) )
                && ( g_features->orbwalker->get_last_target( ) != ETargetType::hero && g_features->orbwalker->
                    get_last_target_index( ) != target->index ) )
                return false;

            auto cast_position{ pred.position };
            if ( cast_position.dist_to( g_local->position ) > m_q_range ||
                cast_position.dist_to( g_features->orbwalker->get_local_server_position( ) ) > m_q_range ) {
                auto temp1 = g_local->position.extend( pred.position, 820.f );
                auto temp2 = g_features->orbwalker->get_local_server_position( ).extend( pred.position, 820.f );

                temp1.y = g_navgrid->get_height( temp1 );
                temp2.y = g_navgrid->get_height( temp2 );

                if ( temp1.dist_to( g_local->position ) +
                    temp1.dist_to( g_features->orbwalker->get_local_server_position( ) ) <
                    temp2.dist_to( g_local->position ) +
                    temp2.dist_to( g_features->orbwalker->get_local_server_position( ) ) )
                    cast_position  = temp1;
                else cast_position = temp2;

                cast_position.y = g_navgrid->get_height( cast_position );


                if ( cast_position.dist_to( g_local->position ) > 825.f ||
                    cast_position.dist_to( g_features->orbwalker->get_local_server_position( ) ) > 825.f )
                    return false;
            }

            if ( cast_spell( ESpellSlot::q, cast_position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;

                if ( get_buffer_delay( ) > 0.f ) {
                    g_features->orbwalker->disable_for(
                        m_slot_q->cooldown_expire - *g_time + 0.1f
                    );
                }

                return true;
            }

            return false;
        }

        auto immobile_q( Object* target ) -> bool{
            if ( !target ) return false;

            const auto pred =
                g_features->prediction->predict( target->index, m_q_range + 270.f, 1000.f, 0.f, 0.25f );
            if ( !pred.valid || pred.hitchance < Prediction::EHitchance::very_high ) return false;

            auto cast_position{ pred.position };
            if ( cast_position.dist_to( g_local->position ) > m_q_range ||
                cast_position.dist_to( g_features->orbwalker->get_local_server_position( ) ) > m_q_range ) {
                auto temp1 = g_local->position.extend( pred.position, 820.f );
                auto temp2 = g_features->orbwalker->get_local_server_position( ).extend( pred.position, 820.f );

                temp1.y = g_navgrid->get_height( temp1 );
                temp2.y = g_navgrid->get_height( temp2 );

                if ( temp1.dist_to( g_local->position ) +
                    temp1.dist_to( g_features->orbwalker->get_local_server_position( ) ) <
                    temp2.dist_to( g_local->position ) +
                    temp2.dist_to( g_features->orbwalker->get_local_server_position( ) ) )
                    cast_position  = temp1;
                else cast_position = temp2;

                cast_position.y = g_navgrid->get_height( cast_position );


                if ( cast_position.dist_to( g_local->position ) > 825.f ||
                    cast_position.dist_to( g_features->orbwalker->get_local_server_position( ) ) > 825.f )
                    return false;
            }

            if ( cast_spell( ESpellSlot::q, cast_position ) ) {
                g_features->orbwalker->on_cast( );

                std::cout << "[ Immobile Q ] Spellcast | " << *g_time << std::endl;
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;

                if ( get_buffer_delay( ) > 0.f ) {
                    g_features->orbwalker->disable_for(
                        m_slot_q->cooldown_expire - *g_time + 0.1f
                    );
                }
                return true;
            }

            return false;
        }

        auto killsteal_q( ) -> bool{
            if ( !g_config->corki.q_killsteal->get< bool >( ) || g_features->orbwalker->in_attack( ) ||
                *g_time - m_last_q_time <= 0.2f || *g_time - m_last_cast_time <= 0.025f || !can_cast_q( ) )
                return false;

            const auto target = g_features->target_selector->get_killsteal_target(
                m_q_range,
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                2
            );
            if ( !target ) return false;

            const auto pred =
                g_features->prediction->predict(
                    target->index,
                    m_q_range + m_extra_q_range,
                    1000.f,
                    275.f,
                    0.25f,
                    { },
                    false,
                    { },
                    Prediction::ESpellType::circle
                );
            if ( !pred.valid ) return false;

            auto cast_position{ pred.position };

            if ( cast_position.dist_to( g_local->position ) > m_q_range ||
                cast_position.dist_to( g_features->orbwalker->get_local_server_position( ) ) > m_q_range ) {
                auto temp1 = g_local->position.extend( pred.position, 820.f );
                auto temp2 = g_features->orbwalker->get_local_server_position( ).extend( pred.position, 820.f );

                temp1.y = g_navgrid->get_height( temp1 );
                temp2.y = g_navgrid->get_height( temp2 );

                if ( temp1.dist_to( g_local->position ) +
                    temp1.dist_to( g_features->orbwalker->get_local_server_position( ) ) <
                    temp2.dist_to( g_local->position ) +
                    temp2.dist_to( g_features->orbwalker->get_local_server_position( ) ) )
                    cast_position  = temp1;
                else cast_position = temp2;

                cast_position.y = g_navgrid->get_height( cast_position );


                if ( cast_position.dist_to( g_local->position ) > 825.f ||
                    cast_position.dist_to( g_features->orbwalker->get_local_server_position( ) ) > 825.f )
                    return false;
            }

            if ( cast_spell( ESpellSlot::q, cast_position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                if ( get_buffer_delay( ) > 0.f ) {
                    g_features->orbwalker->disable_for(
                        m_slot_q->cooldown_expire - *g_time + 0.1f
                    );
                }

                std::cout << "[ KS Q ] Order cast at " << target->champion_name.text << std::endl;
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{ return false; }

        auto spell_e( ) -> bool override{
            if ( !g_config->corki.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f
                || !m_slot_e->is_ready( true ) )
                return false;

            auto sci      = g_local->spell_book.get_spell_cast_info( );
            auto should_e = sci && sci->is_autoattack &&
                *g_time < sci->server_cast_time; //&& *g_time > sci->start_time + g_features->orbwalker->get_ping(  );

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto cast_position = target->position;

            if ( !should_e ) {
                if ( g_features->orbwalker->can_attack( target->index ) &&
                    g_features->orbwalker->is_attackable( target->index ) )
                    return false;


                const auto pred = g_features->prediction->predict( target->index, 650.f, 0.f, 0.f, 0.1f );
                if ( !pred.valid ) return false;

                cast_position = pred.position;
                should_e      = true;
            }

            if ( !should_e ) return false;

            if ( cast_spell( ESpellSlot::e ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->corki.r_enabled->get< bool >( ) || *g_time - m_last_r_time <= 0.3f || *g_time -
                m_last_cast_time <= 0.05f || !can_cast_r( ) )
                return false;


            auto r_range = m_special_missile ? m_special_r_range : m_r_range;
            r_range *= g_config->corki.r_max_range->get< int >( ) / 100.f;

            const auto target = g_features->target_selector->get_spell_specific_target(
                r_range,
                [this]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::r, unit ); },
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::r, unit ); },
                2
            );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict(
                target->index,
                r_range,
                2000.f,
                40.f,
                0.175f,
                { },
                true,
                Prediction::include_ping,
                Prediction::ESpellType::linear
            );

            if ( !pred.valid ||
                pred.hitchance < static_cast< Prediction::EHitchance >( g_config->corki.r_hitchance->get< int >( ) ) ||
                g_features->prediction->minion_in_line_predicted(
                    g_local->position,
                    pred.position,
                    40.f,
                    0.175f,
                    2000.f
                ) )
                return false;

            if ( cast_spell( ESpellSlot::r, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_r_time    = *g_time;
                m_last_cast_time = *g_time;

                if ( get_buffer_delay_r( ) > 0.f ) {
                    g_features->orbwalker->disable_for(
                        m_slot_r->cooldown_expire - *g_time + 0.1f
                    );
                }
                return true;
            }

            return false;
        }

        auto killsteal_r( ) -> bool{
            if ( !g_config->corki.r_killsteal->get< bool >( ) ||
                *g_time - m_last_r_time <= 0.3f || *g_time - m_last_cast_time <= 0.025f || !can_cast_r( ) )
                return false;

            auto r_range = m_special_missile ? m_special_r_range : m_r_range;
            r_range *= g_config->corki.r_max_range->get< int >( ) / 100.f;

            const auto target = g_features->target_selector->get_killsteal_target(
                r_range,
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::r, unit ); },
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::r, unit ); },
                2
            );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_special_missile ? m_special_r_range : m_r_range,
                2000.f,
                40.f,
                0.175f,
                { },
                true,
                Prediction::include_ping,
                Prediction::ESpellType::linear
            );

            if ( !pred.valid || g_features->prediction->minion_in_line_predicted(
                g_local->position,
                pred.position,
                40.f,
                0.175f,
                2000.f
            ) )
                return false;

            if ( cast_spell( ESpellSlot::r, pred.position ) ) {
                m_last_r_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                if ( get_buffer_delay_r( ) > 0.f ) {
                    g_features->orbwalker->disable_for(
                        m_slot_q->cooldown_expire - *g_time + 0.1f
                    );
                }

                std::cout << "[ KS R ] Order cast at " << target->champion_name.text << std::endl;
                return true;
            }

            return false;
        }

        auto spellclear_q( ) -> bool{
            if ( !g_config->corki.q_spellclear->get< bool >( ) || !g_input->is_key_pressed( utils::EKey::control ) || *
                g_time - m_last_q_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.1f || !m_slot_q->is_ready( true ) )
                return false;

            const auto            spellclear = get_circle_laneclear_target(
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                m_q_range,
                1000.f,
                0.25f,
                275.f
            );

            if ( spellclear.value < 6 ) return false;

            if ( cast_spell( ESpellSlot::q, spellclear.position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                return true;
            }

            return false;
        }

        auto antigapclose_w( ) -> bool{
            if ( !g_config->corki.w_antigapclose->get< bool >( ) || *g_time - m_last_cast_time <= 0.1f ||
                *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready( true ) || rt_hash(
                    m_slot_w->get_name( ).data( )
                ) == ct_hash( "CarpetBombMega" ) )
                return false;

            const auto dash_time = 650.f + g_local->movement_speed / 600.f;

            const auto target = get_advanced_antigapclose_target( 500.f, 0.f, 0.f, dash_time + 0.25f );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict( target->index, 400.f, 0.f, 0.f, dash_time + 0.5f );
            if ( !pred.valid ) return false;

            const auto threat_position{ pred.position };

            Vec3  best_position{ };
            float best_weight{ };

            const auto points = g_render->get_3d_circle_points( g_local->position, 600.f, 32 );
            for ( const auto point : points ) {
                if ( g_navgrid->is_wall( point ) ) continue;

                const auto weight = get_antigapclose_position_weight( point, threat_position );
                if ( weight < best_weight ) continue;

                best_position = point;
                best_weight   = weight;
            }

            if ( best_position.length( ) <= 0.f ) return false;

            if ( cast_spell( ESpellSlot::w, best_position ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                debug_log(
                    "[ CORKI: Antigapclose W ] against {} | position weight: {}",
                    target->champion_name.text,
                    best_weight
                );

                return true;
            }

            return false;
        }

        auto flee_w( ) -> bool{
            return false;


            if ( !g_config->corki.w_flee->get< bool >( ) || g_features->orbwalker->get_mode( ) !=
                Orbwalker::EOrbwalkerMode::flee
                || g_features->evade->is_active( ) || *g_time - m_last_cast_time <= 0.1f || *g_time - m_last_w_time <=
                0.5f
                || !m_slot_w->is_ready( true ) || g_features->orbwalker->in_action( ) ||
                rt_hash( m_slot_w->get_name( ).data( ) ) == ct_hash( "CarpetBombMega" ) ) {
                if ( m_point_saved ) reset_flee_w( );

                return false;
            }

            if ( !m_point_saved ) {
                if ( GetAsyncKeyState( 0x01 ) ) {
                    auto cursor   = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;
                    m_flee_point  = cursor;
                    m_point_saved = true;
                } else return false;
            }

            if ( !m_path_calculated ) {
                Vec3 best_point{ };
                auto lowest_distance{ FLT_MAX };

                auto points = g_render->get_3d_circle_points( m_flee_point, 600.f, 32 );
                for ( auto point : points ) {
                    if ( g_navgrid->is_wall( point ) ) continue;

                    auto distance = g_local->position.dist_to( point );
                    if ( distance > lowest_distance ) continue;

                    if ( !helper::is_wall_in_line( point, m_flee_point ) ||
                        helper::is_wall_in_line( g_local->position, point ) )
                        continue;

                    best_point      = point;
                    lowest_distance = distance;
                }

                if ( best_point.length( ) <= 0.f ) {
                    reset_flee_w( );
                    return false;
                }

                m_path_goal       = best_point;
                m_path_calculated = true;

                m_path_goal.y = g_navgrid->get_height( m_path_goal );

                g_features->orbwalker->allow_movement( false );
                g_features->orbwalker->allow_attacks( false );
            }

            if ( !m_pathing_active ) {
                if ( g_features->orbwalker->send_move_input( m_path_goal, true ) ) {
                    m_pathing_active = true;
                    m_last_move_time = *g_time;
                } else return false;
            }

            if ( *g_time - m_last_move_time <= 0.25f ) return false;

            if ( g_local->position.dist_to( m_path_goal ) > 50.f ) {
                auto aimgr = g_local->get_ai_manager( );
                if ( !aimgr || !aimgr->is_moving ) {
                    std::cout << "Reset flee, aimgr not moving\n";

                    reset_flee_w( );
                    return false;
                }

                auto path = aimgr->get_path( );
                if ( path.size( ) <= 1 ) {
                    std::cout << "Reset flee, path 1\n";
                    reset_flee_w( );
                    return false;
                }

                auto path_end = path[ path.size( ) - 1 ];

                m_path_confirmed = path_end.dist_to( m_path_goal ) <= 100.f;

                if ( !m_path_confirmed ) {
                    if ( *g_time - m_last_move_time > 0.5f ) {
                        std::cout << "Reset flee, path wait too long | path end dist " << path_end.dist_to(
                            m_path_goal
                        ) << std::endl;
                        reset_flee_w( );
                    }
                    return false;
                }


                return false;
            }

            if ( cast_spell( ESpellSlot::w, m_flee_point ) ) {
                m_last_cast_time = *g_time;
                m_last_w_time    = *g_time;
                g_features->orbwalker->on_cast( );
                reset_flee_w( );
                return true;
            }


            return false;
        }

        auto reset_flee_w( ) -> void{
            m_point_saved     = false;
            m_path_calculated = false;
            m_pathing_active  = false;
            m_path_confirmed  = false;

            g_features->orbwalker->allow_movement( true );
            g_features->orbwalker->allow_attacks( true );
        }

        auto can_cast_q( ) -> bool{
            return !m_in_dash && g_local->mana >= m_slot_q->get_manacost( ) && ( m_slot_q->is_ready( ) ||
                m_slot_q->cooldown_expire - *g_time <= g_features->orbwalker->get_ping( ) * 1.75f );
        }

        auto get_buffer_delay( ) -> float{
            if ( m_slot_q->is_ready( ) || g_local->mana < m_slot_q->get_manacost( ) || m_in_dash ) return 0.f;

            return std::max( m_slot_q->cooldown_expire - *g_time - g_features->orbwalker->get_ping( ), 0.f );
        }

        auto can_cast_r( ) -> bool{
            return !m_in_dash && ( m_slot_r->is_ready( ) && m_slot_r->charges > 0 ||
                m_slot_r->cooldown_expire - *g_time <= g_features->orbwalker->get_ping( ) * 1.75f );
        }

        auto get_buffer_delay_r( ) -> float{
            if ( m_slot_r->is_ready( ) && m_slot_r->charges > 0 || g_local->mana < m_slot_r->get_manacost( ) ||
                m_in_dash )
                return 0.f;

            return std::max( m_slot_r->cooldown_expire - *g_time - g_features->orbwalker->get_ping( ), 0.f );
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            if ( !target ) return 0.f;

            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ m_slot_q->level ] + g_local->ability_power( ) * 0.5f + g_local->bonus_attack_damage( ) *
                    0.7f,
                    target->index,
                    false
                );
            case ESpellSlot::r:
                if ( m_special_missile ) {
                    return helper::calculate_damage(
                        m_r_special_damage[ m_slot_r->level ] + m_r_special_ad_modfier[ m_slot_r->level ] * g_local->
                        attack_damage( ) + g_local->ability_power( ) * 0.24f,
                        target->index,
                        false
                    );
                }

                return helper::calculate_damage(
                    m_r_damage[ m_slot_r->level ] + m_r_ad_modifier[ m_slot_r->level ] * g_local->attack_damage( ) +
                    g_local->ability_power( ) * 0.12f,
                    target->index,
                    false
                );
            default:
                break;
            }
            return 0.f;
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
            {
                const auto tt   = 0.25f + g_local->position.dist_to( target->position ) / 1000.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.25f + g_local->position.dist_to( pred.value( ) ) / 1000.f;
            }
            case ESpellSlot::r:
            {
                const auto tt   = 0.175f + g_local->position.dist_to( target->position ) / 2000.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                const auto compensation =
                    ( g_features->prediction->get_champion_radius( rt_hash( target->champion_name.text ) ) + 40.f ) /
                    target->movement_speed;

                return 0.175f + g_local->position.dist_to( pred.value( ) ) / 2000.f - compensation;
            }
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

        // w flee logic
        Vec3 m_flee_point{ };
        bool m_point_saved{ };

        bool m_path_calculated{ };
        Vec3 m_path_goal{ };

        float m_last_move_time{ };
        bool  m_pathing_active{ };
        bool  m_path_confirmed{ };


        std::vector< float > m_q_damage = { 0.f, 75.f, 120.f, 165.f, 210.f, 255.f };
        std::vector< float > m_e_damage = { 0.f, 80.f, 110.f, 140.f, 170.f, 200.f };

        std::vector< float > m_r_damage      = { 0.f, 80.f, 115.f, 150.f };
        std::vector< float > m_r_ad_modifier = { 0.f, 0.15f, 0.45f, 0.75f };

        std::vector< float > m_r_special_damage     = { 0.f, 160.f, 230.f, 300.f };
        std::vector< float > m_r_special_ad_modfier = { 0.f, 0.3f, 0.9f, 1.5f };

        float m_extra_q_range{ 135.f };
        float m_q_range{ 825.f };
        float m_w_range{ 600.f };
        float m_e_range{ 690.f };
        float m_r_range{ 1300.f };
        float m_special_r_range{ 1500.f };

        bool m_in_dash{ };

        bool m_special_missile{ };
    };
}
