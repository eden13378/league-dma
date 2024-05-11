#pragma once
#include "module.hpp"
#include "../../sdk/game/spell_details.hpp"

namespace features::champion_modules {
    class karthus_module final : public IModule {
    public:
        virtual ~karthus_module( ) = default;


        auto get_name( ) -> hash_t override{ return ct_hash( "karthus_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Karthus" ); }

        auto initialize( ) -> void override{ m_priority_list = { r_spell, w_spell, q_spell, e_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation  = g_window->push( _( "karthus" ), menu_order::champion_module );
            const auto q_settings  = navigation->add_section( _( "q settings" ) );
            const auto drawings    = navigation->add_section( _( "drawings" ) );
            const auto w_settings  = navigation->add_section( _( "w settings" ) );
            const auto e_settings  = navigation->add_section( _( "e settings" ) );
            const auto r_settings  = navigation->add_section( _( "r settings" ) );
            const auto laneclear   = navigation->add_section( _( "laneclear" ) );
            const auto jungleclear = navigation->add_section( _( "jungleclear" ) );
            const auto misc        = navigation->add_section( _( "miscellaneous" ) );

            q_settings->checkbox( _( "enable" ), g_config->karthus.q_enabled );
            q_settings->checkbox( _( "harass q" ), g_config->karthus.q_harass );
            q_settings->select(
                _( "hitchance" ),
                g_config->karthus.q_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );
            q_settings->checkbox( _( "champion multihit" ), g_config->karthus.q_multihit );
            q_settings->checkbox( _( "prioritize no minions (?)" ), g_config->karthus.q_prioritize_no_minion )
                      ->set_tooltip( _( "Not recommended!!" ) );

            w_settings->checkbox( _( "enable" ), g_config->karthus.w_enabled );
            w_settings->slider_int( _( "max range %" ), g_config->karthus.w_max_range, 50, 100, 1 );
            w_settings->select(
                _( "hitchance" ),
                g_config->karthus.w_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            e_settings->checkbox( _( "enable" ), g_config->karthus.e_enabled );
            e_settings->slider_int( _( "min enemies" ), g_config->karthus.e_min_enemy, 1, 5, 1 );
            e_settings->slider_int( _( "min mana" ), g_config->karthus.e_min_mana, 1, 50, 1 );
            e_settings->checkbox( _( "disable e below min mana" ), g_config->karthus.e_disable_under_min_mana );

            r_settings->checkbox( _( "enable" ), g_config->karthus.r_enabled );
            r_settings->slider_int( _( "min kills" ), g_config->karthus.r_min_kill, 1, 5, 1 );

            drawings->checkbox( _( "draw q range" ), g_config->karthus.q_draw_range );
            drawings->checkbox( _( "draw w range" ), g_config->karthus.w_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->karthus.e_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->karthus.dont_draw_on_cooldown );
            drawings->checkbox( _( "draw r kill indicator" ), g_config->karthus.r_draw_kill_indcator );

            laneclear->checkbox( _( "enable q" ), g_config->karthus.q_laneclear );
            laneclear->checkbox( _( "enable q last hit" ), g_config->karthus.q_lasthit );
            laneclear->checkbox( _( "enable e" ), g_config->karthus.e_laneclear );
            laneclear->slider_int( _( "minimum mana %" ), g_config->karthus.laneclear_min_mana, 1, 90, 1 );

            jungleclear->checkbox( _( "enable q" ), g_config->karthus.q_jungleclear );
            jungleclear->checkbox( _( "enable e" ), g_config->karthus.e_jungleclear );
            jungleclear->slider_int( _( "e min hitcount" ), g_config->karthus.e_min_hitcount, 1, 4, 1 );

            misc->checkbox( _( "auto q in passive" ), g_config->karthus.q_automatic_if_dead );
            misc->checkbox( _( "full combo ignore q hitchance" ), g_config->karthus.q_ignore_hitchance_in_full_combo );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->karthus.q_draw_range->get< bool >( ) &&
                !g_config->karthus.w_draw_range->get< bool >( ) &&
                !g_config->karthus.e_draw_range->get< bool >( ) &&
                !g_config->karthus.r_draw_kill_indcator->get< bool >( ) ||
                g_local->is_dead( )
            )
                return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->karthus.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->karthus.dont_draw_on_cooldown->
                    get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 31, 88, 255, 255 ),
                        m_q_range,
                        Renderer::outline,
                        40,
                        2.f
                    );
                }
            }

            if ( g_config->karthus.w_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->karthus.dont_draw_on_cooldown->
                    get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 132, 245, 66, 255 ),
                        m_w_range,
                        Renderer::outline,
                        40,
                        2.f
                    );
                }
            }

            if ( g_config->karthus.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || m_e_active || !g_config->karthus.
                    dont_draw_on_cooldown->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 173, 47, 68, 255 ),
                        m_e_range,
                        Renderer::outline,
                        40,
                        2.f
                    );
                }
            }

            /* Vec2 screen{ };
             if ( world_to_screen(g_local->position, screen ) ) {
 
                 screen.y -= 30.f;
                 screen.x += 25.f;
 
 
                 auto aimgr = g_local->get_ai_manager( );
                 if ( !aimgr ) return;
 
                 auto path = aimgr->get_path( );
                 int  path_size = path.size( );
                 int  path_node = aimgr->next_path_node;
 
                 std::string path_text = "PATH: " + std::to_string( path_size );
                 std::string node_text = "NODE: " + std::to_string( path_node );
                 std::string moving_text = "MOVING: " + std::to_string( aimgr->is_moving );
 
                 g_render->text_shadow( screen, Color::white( ), g_fonts->get_nexa_16px( ), path_text.data( ), 16 );
                 screen.y += 16.f;
                 g_render->text_shadow( screen, Color::green( ), g_fonts->get_nexa_16px( ), node_text.data( ), 16 );
                 screen.y += 16.f;
                 g_render->text_shadow( screen, Color::white( ), g_fonts->get_nexa_16px( ), moving_text.data( ), 16 );
 
 
             }*/


            if ( !g_config->karthus.r_draw_kill_indcator->get< bool >( ) ) return;

            Vec2 sp{ };
            if ( !world_to_screen( g_local->position, sp ) ) return;

            const auto text            = std::format( ( "R Potential Kills : {}" ), m_r_potential_kills );
            const auto text_size_kills = static_cast< float >( 16 + ( 8 * m_r_potential_kills > 1
                                                                          ? m_r_potential_kills
                                                                          : 0 ) );
            const auto text_size = g_render->get_text_size( text, g_fonts->get_block( ), text_size_kills );

            g_render->text_shadow(
                { sp.x - ( text_size.x / 4.f ), sp.y + text_size.y },
                m_r_potential_kills > 0 ? Color( 25, 255, 25, 220 ) : Color( 255, 25, 25, 220 ),
                g_fonts->get_default_bold( ),
                text.c_str( ),
                text_size_kills
            );
        }

        auto run( ) -> void override{
            initialize_spell_slots( );
            update_r_indicator( );
            update_spell( );

            //cast_handler( );

            /* if ( m_order_movement && *g_time - m_last_q_time >= 0.078f && *g_time - m_last_q_time <= 0.125f ) {
 
                 if ( g_features->orbwalker->send_move_input( Vec3( ), true ) ) {
 
                     int milliseconds = (*g_time - m_last_q_time) * 1000.f;
 
                     std::cout << "[ move order ] time since cast " << milliseconds << "ms" << std::endl;
 
                     m_last_cast_time = *g_time;
                 }
 
                 return;
             }*/

            m_e_active     = !!g_features->buff_cache->get_buff( g_local->index, ct_hash( "KarthusDefile" ) );
            m_in_passive   = !!g_features->buff_cache->get_buff( g_local->index, ct_hash( "KarthusDeathDefiedBuff" ) );
            m_in_fullcombo = g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo &&
                g_input->is_key_pressed( utils::EKey::control );

            if ( m_in_passive ) if ( g_config->karthus.q_automatic_if_dead->get< bool >( ) ) spell_q( );

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return;

            /* if ( GetAsyncKeyState( 0x31 ) && *g_time - m_last_q_time > 0.4f && m_slot_q->charges == 2 ) {

                Vec3 point = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;

                if ( cast_spell( ESpellSlot::q, point ) ) {

                    m_last_q_time = *g_time;
                    m_last_cast_time = *g_time;
                    m_order_movement = true;

                    g_features->orbwalker->allow_fast_move( );

                    m_cast_position = point;

                    std::cout << "Started movement order | " << *g_time << std::endl;
                    return;
                }
            }*/

            //std::cout << g_local->spell_book.get_spell_slot(e_spell_slot::f)->get_name() << "\n";

            //std::cout << "q address: " << std::hex << m_slot_q.get_address() << std::endl;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:
                spell_r( );
                spell_w( );
                spell_e( );
                spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->karthus.q_harass->get< bool >( ) ) spell_q( );

                lashit_q( );
                break;
            case Orbwalker::EOrbwalkerMode::laneclear:
                if ( g_config->karthus.q_harass->get< bool >( ) && !helper::is_position_under_turret(
                    g_local->position
                ) )
                    spell_q( );

                if ( lashit_q( ) ) return;

                if ( g_config->karthus.q_laneclear->get< bool >( ) && laneclear_q( ) ) return;
                if ( g_config->karthus.e_laneclear->get< bool >( ) && laneclear_e( ) ) return;

                if ( g_config->karthus.q_jungleclear->get< bool >( ) && jungleclear_q( ) ) return;
                if ( g_config->karthus.e_jungleclear->get< bool >( ) && jungleclear_e( ) ) return;
                break;
            case Orbwalker::EOrbwalkerMode::lasthit:
                if ( g_config->karthus.q_lasthit->get< bool >( ) && lashit_q( ) ) return;
                break;
            case Orbwalker::EOrbwalkerMode::flee:
                spell_w( );
                break;
            default:
                break;
            }

            return;

            if ( m_slot_q->charges > 1 ) {
                if ( activated ) {
                    std::cout << ">> Q Refreshed at " << *g_time
                        << " | cooldown expire: " << m_slot_q->final_cooldown_expire << std::endl;
                }

                activated = false;
                return;
            }

            if ( activated ) return;

            std::cout << "CooldownExpire: " << m_slot_q->final_cooldown_expire << std::endl;
            std::cout << "Duration: " << m_slot_q->final_cooldown_expire - *g_time << std::endl;

            activated = true;
            //std::cout << "[ Slot Q: Address " << std::hex << m_slot_q.get_address( ) << std::endl;
        }

        // check enemy positions from predicted spot, if their (predicted?) position is not within it, fire
    private:
        auto update_spell( ) -> void{
            auto sci = g_local->spell_book.get_spell_cast_info( );
            if ( sci && sci->slot == 0 && sci->server_cast_time > m_q_server_cast_time ) {
                m_q_server_cast_time = sci->server_cast_time;
            }
        }

        auto spell_q( ) -> bool override{
            if ( !g_config->karthus.q_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.025f || *g_time -
                m_last_q_time <= 0.35f
                || !can_cast_q( ) )
                return false;

            if ( g_config->karthus.q_multihit->get< bool >( ) ) {
                const auto            multihit = get_circle_multihit(
                    [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                    m_q_range,
                    200.f
                );

                if ( multihit.hit_count > 1 ) {
                    if ( cast_spell( ESpellSlot::q, multihit.position ) ) {
                        g_features->orbwalker->on_cast( );
                        m_last_q_time    = *g_time;
                        m_last_cast_time = *g_time;

                        std::cout << "[ Karthus: Q ] Multihit | count: " << multihit.hit_count << " | T: " << *g_time
                            << std::endl;
                        return true;
                    }
                }
            }

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

            const auto hitchance =
                m_in_fullcombo && g_config->karthus.q_ignore_hitchance_in_full_combo->get< bool >( ) ||
                m_in_passive && g_config->karthus.q_automatic_if_dead->get< bool >( )
                    ? 0
                    : g_config->karthus.q_hitchance->get< int >( );

            const auto delay = 1.075f;
            const auto pred  = g_features->prediction->predict(
                target->index,
                875.f,
                0.f,
                200.f,
                delay,
                { },
                false,
                Prediction::include_ping,
                Prediction::ESpellType::circle
            );

            if ( !pred.valid || static_cast< int >( pred.hitchance ) < hitchance ) return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_cast_time = *g_time;
                m_last_q_time    = *g_time;

                if ( m_slot_q->charges < 2 ) {
                    g_features->orbwalker->disable_for( m_slot_q->final_cooldown_expire - *g_time + 0.075f );
                    //std::cout << "Disabled orbwalker for " << m_slot_q->cooldown_expire - *g_time + 0.075f
                    //          << " | T: " << *g_time << std::endl;
                }

                std::cout << "[ Karthus: Q ] Combo | T: " << *g_time << std::endl;
                return true;
            }

            return false;
        }

        auto complex_q( Object* target ) -> bool{
            if ( !g_config->karthus.q_prioritize_no_minion->get< bool >( ) || !target ) return false;

            return false;

            const auto hitchance =
                m_in_fullcombo && g_config->karthus.q_ignore_hitchance_in_full_combo->get< bool >( ) ||
                m_in_passive && g_config->karthus.q_automatic_if_dead->get< bool >( )
                    ? 0
                    : g_config->karthus.q_hitchance->get< int >( );

            const auto predicted = g_features->prediction->predict(
                target->index,
                m_q_range,
                // range
                0.f,
                // speed
                m_q_radius,
                // width
                m_q_cast_time + m_q_dam_delay // delay
            );

            auto failed = false;

            auto default_valid = false;

            if ( predicted.valid &&
                predicted.hitchance >= static_cast< Prediction::EHitchance >( hitchance ) ) {
                default_valid = true;
                for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
                    if ( !minion || minion->is_dead( ) ) continue;

                    if ( predicted.position.dist_to( minion->position ) < m_q_radius + 55.f ) {
                        failed = true;
                        break;
                    }
                }

                if ( !failed || !g_config->karthus.q_prioritize_no_minion->get< bool >( ) ) {
                    if ( cast_spell( ESpellSlot::q, predicted.position ) ) {
                        debug_log( "STANDARD PREDICTION CAST!" );
                        m_last_q_time    = *g_time;
                        m_last_cast_time = *g_time;
                        g_features->orbwalker->on_cast( );
                        return true;
                    }
                }
            }

            if ( !g_config->karthus.q_prioritize_no_minion->get< bool >( ) ) return false;

            const auto predicted_position = g_features->prediction->predict_default(
                target->index,
                m_q_cast_time + 0.759f //( m_q_dam_delay / 1.5f ) // delay, not quite full dam delay as this seems to
                //increase our hit chance? Not too sure tho!
            );

            if ( predicted_position && predicted_position.has_value( ) ) {
                const auto section_degrees = 360.f / m_q_degree_sections;

                for ( auto offset = m_q_offset_step; offset <= m_q_pred_max_offset; offset += m_q_offset_step ) {
                    std::vector< Vec3 > good_positions{ };
                    for ( auto direction_tranpose = 0.f; direction_tranpose < 360.f;
                          direction_tranpose += section_degrees ) {
                        failed = false;
                        // hypotenuse = offset
                        // degrees = direction_transpose
                        const auto radians  = direction_tranpose * m_pi / 180.f;
                        const auto x_offset = sin( radians ) * offset;
                        const auto z_offset = cos( radians ) * offset;

                        Vec3 transposed_position = {
                            predicted_position->x + x_offset,
                            predicted_position->y,
                            predicted_position->z + z_offset
                        };

                        for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
                            if ( !minion || minion->is_dead( ) ) continue;

                            const auto minion_predict =
                                g_features->prediction->predict_default( minion->index, m_q_cast_time + m_q_dam_delay );
                            if ( transposed_position.dist_to( minion->position ) < m_q_radius + 55.f ||
                                minion_predict &&
                                minion_predict->dist_to( transposed_position ) < m_q_radius + 55.f ) {
                                failed = true;
                                break;
                            }
                        }

                        if ( !failed ) {
                            if ( transposed_position.dist_to( g_local->position ) > m_q_range ) continue;
                            if ( transposed_position.dist_to( predicted_position.value( ) ) > m_q_radius ) continue;
                            good_positions.push_back( transposed_position );
                        }
                    }

                    if ( good_positions.empty( ) ) continue; // if no transpositions are minion-less

                    Vec3 best_pos{ };

                    auto distance = std::numeric_limits< float >::max( ); // default high value so first value is
                    // always better than this
                    for ( auto pos : good_positions ) {
                        const auto our_distance =
                            pos.dist_to( predicted.position ); // using predicted rather than predicted_position cuz
                        // it's most likely more accurate as it uses spell data
                        if ( our_distance < distance ) {
                            best_pos = pos;
                            distance = our_distance;
                        }
                    }

                    if ( cast_spell( ESpellSlot::q, best_pos ) ) {
                        debug_log( "MATHS CAST!" );
                        m_last_q_time    = *g_time;
                        m_last_cast_time = *g_time;
                        g_features->orbwalker->on_cast( );
                        return true;
                    }
                }
            }

            if ( default_valid ) {
                if ( cast_spell( ESpellSlot::q, predicted.position ) ) {
                    debug_log( "STANDARD PREDICTION CAST (SECONDARY)!" );
                    m_last_q_time    = *g_time;
                    m_last_cast_time = *g_time;
                    g_features->orbwalker->on_cast( );
                    return true;
                }
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->karthus.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f
                || *g_time - m_last_cast_time <= 0.05f || !m_slot_w->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_w_range * ( g_config->karthus.w_max_range->get< int >( ) / 100.f ),
                0.f,
                10.f,
                0.25f
            );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->karthus.w_hitchance->
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

        //THIS CANNOT RUN WHILE WE ARE IN DEAD-PASSIVE, ADD A CHECK FOR IT
        auto spell_e( ) -> bool override{
            if ( !g_config->karthus.e_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.025f ||
                *g_time - m_last_e_time <= 0.4f || m_in_passive || !m_slot_e->is_ready( true ) )
                return false;

            const auto mana = g_local->mana / g_local->max_mana * 100;

            if ( g_config->karthus.e_min_mana->get< int >( ) > mana ) {
                if ( g_config->karthus.e_disable_under_min_mana->get< bool >( ) && m_e_active ) {
                    if ( cast_spell( ESpellSlot::e ) ) {
                        m_last_e_time    = *g_time;
                        m_last_cast_time = *g_time;
                        return true;
                    }

                    return false;
                }
            }

            const auto min_enemy = g_config->karthus.e_min_enemy->get< int >( );

            auto enemy_count = 0;
            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->is_dead( ) || enemy->dist_to_local( ) > m_e_range ) continue;

                enemy_count++;
            }

            if ( enemy_count >= min_enemy && !m_e_active ) {
                if ( cast_spell( ESpellSlot::e ) ) {
                    m_last_e_time    = *g_time;
                    m_last_cast_time = *g_time;
                    return true;
                }
            }

            if ( enemy_count < min_enemy && m_e_active ) {
                if ( cast_spell( ESpellSlot::e ) ) {
                    m_last_e_time    = *g_time;
                    m_last_cast_time = *g_time;
                    return true;
                }
            }

            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->karthus.r_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_r_time <= 0.4f || !m_slot_r->is_ready( true ) )
                return false;

            const auto in_passive = m_in_passive;

            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->is_dead( ) ) continue;

                if ( !in_passive ) {
                    if ( enemy->dist_to_local( ) < g_config->karthus.r_disable_enemy_within->get< int >( ) )
                        return
                            false;
                }
            }

            if ( m_r_potential_kills >= g_config->karthus.r_min_kill->get< int >( ) ) {
                if ( !in_passive && helper::is_position_under_turret( g_local->position ) ) return false;

                if ( cast_spell( ESpellSlot::r ) ) {
                    m_last_r_time    = *g_time;
                    m_last_cast_time = *g_time;
                    g_features->orbwalker->on_cast( );
                    return true;
                }
            }
            return false;
        }

        auto laneclear_q( ) -> bool{
            if ( m_slot_q->charges < 2 || *g_time - m_last_q_time <= 0.4f || *g_time - m_last_cast_time <= 0.05f ||
                !g_input->is_key_pressed( utils::EKey::control ) ||
                g_local->mana < g_local->max_mana / 100.f * g_config->karthus.laneclear_min_mana->get< int >( ) ||
                !m_slot_q->is_ready( true ) )
                return false;

            const auto farm_pos = get_best_laneclear_position( m_q_range, m_q_radius, true, false, 0.8f );
            if ( farm_pos.value < 3 ) return false;

            if ( cast_spell( ESpellSlot::q, farm_pos.position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto lashit_q( ) -> bool{
            if ( !g_config->karthus.q_lasthit->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || *g_time -
                m_last_cast_time <= 0.05f || !can_cast_q( ) )
                return false;

            const auto data
                = get_line_lasthit_target_advanced(
                    [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                    [ this ]( Object* unit ) -> float{ return 1.025f; },
                    m_q_range,
                    160.f,
                    0.f,
                    0,
                    g_local->mana <= g_local->max_mana / 2.f
                );

            if ( !data ) return false;

            if ( cast_spell( ESpellSlot::q, data->cast_position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                if ( m_slot_q->charges < 2 ) {
                    g_features->orbwalker->disable_for( m_slot_q->final_cooldown_expire - *g_time + 0.075f );
                    // std::cout << "Disabled orbwalker for " << m_slot_q->cooldown_expire - *g_time + 0.075f
                    //           << " | T: " << *g_time << std::endl;
                }

                g_features->orbwalker->ignore_minion( data->index, data->travel_time );
                return true;
            }

            return false;
        }

        auto laneclear_e( ) -> bool{
            if ( *g_time - m_last_e_time <= 0.5f || *g_time - m_last_cast_time <= 0.05f ||
                !m_slot_e->is_ready( true ) )
                return false;

            int hit_count{ };
            for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
                if ( !minion || minion->is_dead( ) || minion->is_invisible( ) || !minion->is_lane_minion( ) && !minion->
                    is_jungle_monster( ) || minion->position.dist_to( g_local->position ) > m_e_range || minion->
                    is_plant( ) || minion->is_untargetable_minion( ) || minion->is_minion_only_autoattackable( ) )
                    continue;

                hit_count++;
            }

            const auto is_fastclearing = g_input->is_key_pressed( utils::EKey::control );
            const auto should_e = m_e_active && ( hit_count <= 2 || !is_fastclearing ) || is_fastclearing && hit_count >
                2 && !m_e_active;
            if ( !should_e ) return false;

            if ( cast_spell( ESpellSlot::e ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto jungleclear_q( ) -> bool{
            if ( m_slot_q->charges < 2 || *g_time - m_last_q_time <= 0.4f || *g_time - m_last_cast_time <= 0.05f ||
                !m_slot_q->is_ready( true ) )
                return false;

            const auto farm_pos = get_best_laneclear_position( m_q_range, m_q_radius, false, true, 0.8f );
            if ( farm_pos.value < 1 ) return false;

            if ( cast_spell( ESpellSlot::q, farm_pos.position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto jungleclear_e( ) -> bool{
            if ( *g_time - m_last_e_time <= 0.5f || *g_time - m_last_cast_time <= 0.05f || !m_slot_e->
                is_ready( true ) )
                return false;

            int hit_count{ };
            for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
                if ( !minion ||
                    minion->is_dead( ) ||
                    minion->is_invisible( ) ||
                    !minion->is_jungle_monster( ) ||
                    minion->position.dist_to( g_local->position ) > m_e_range
                )
                    continue;

                hit_count++;
            }

            if ( hit_count < 1 ) return false;

            const auto should_e = hit_count < g_config->karthus.e_min_hitcount->get< int >( ) && m_e_active || hit_count
                >= g_config->karthus.e_min_hitcount->get< int >( ) && !m_e_active;
            if ( !should_e ) return false;

            if ( cast_spell( ESpellSlot::e ) ) {
                m_last_e_time = *g_time;
                return true;
            }

            return false;
        }

        auto update_r_indicator( ) -> void{
            if ( !g_config->karthus.r_draw_kill_indcator->get< bool >( ) ) return;

            auto kill_counter = 0;
            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->is_dead( ) ) continue;

                const auto damage             = get_spell_damage( ESpellSlot::r, enemy );
                const auto enemy_health       = enemy->health;
                const auto enemy_regen_health = enemy->total_health_regen * 4;
                const auto enemy_new_health   = enemy_health + enemy_regen_health;

                const auto last_seen_index = g_features->tracker->get_last_seen_index( enemy->index );
                if ( !last_seen_index ) continue;
                // fixme: this code makes no fucking sense
                // ^it does as last seen time is checking, making sure not to store an enemy for too long of time where there is potential regen. If waiting too long
                // then regen is possible and can miss R in total calc
                //const auto last_seen = static_cast< float >( g_features->tracker->get_last_seen_time( enemy->index ) );
                //
                // if ( last_seen >= 10.f ) continue;

                // const float last_seen_check = damage - enemy_new_health - enemy->total_health_regen * last_seen;
                //std::cout << "current damage reading on R" << damage << "\n";

                if ( damage < enemy_health + enemy->total_health_regen || !enemy->is_visible( ) || !enemy->
                    is_alive( ) )
                    continue;
                kill_counter++;

                /*if (damage > enemy_health + enemy->total_health_regen && enemy->is_visible() && enemy->is_alive())
                {
                    kill_counter++;
                }*/
            }

            m_r_potential_kills = kill_counter;
        }

        auto cast_handler( ) -> void{
            return;

            if ( !m_q_active ) {
                auto details = m_slot_q->get_details( );
                if ( !details ) return;

                auto end = details->last_end_position;
                if ( end == m_last_cast_end ) return;

                std::cout << "[: DETAILS :] New end position detected | " << *g_time << std::endl;

                m_last_cast_end = end;

                if ( g_features->orbwalker->send_move_input( Vec3( ), true ) ) {
                    std::cout << "[ move order ] abusing details | " << *g_time << std::endl;

                    m_last_cast_time = *g_time;
                    m_last_q_time    = *g_time;
                }

                return;
            }

            if ( *g_time > m_q_server_cast_time ) {
                m_q_active       = false;
                m_order_movement = false;

                std::cout << "========== Q Cast ended | " << *g_time << std::endl << std::endl;
                return;
            }

            if ( m_order_movement && *g_time - m_q_start_time > 0.016f &&
                g_features->orbwalker->send_move_input( Vec3( ), true ) ) {
                std::cout << "Ordered move with " << m_q_server_cast_time - *g_time << "s left of Q cast\n";
                m_order_movement = false;
            }
        }

        auto can_cast_q( ) -> bool{
            return ( m_in_passive || m_slot_q->get_manacost( ) <= g_local->mana ) && ( m_slot_q->charges == 2 ||
                m_q_server_cast_time > 0.f && m_q_server_cast_time + m_slot_q->get_spell_effect_value( ) <= *g_time +
                g_features->orbwalker->get_ping( ) / 2.f + 0.033f );
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage_minions_and_champs[ m_slot_q->level ] + g_local->ability_power( ) * 0.35f,
                    target->index,
                    false
                );
            case ESpellSlot::r:
                return helper::calculate_damage(
                    m_r_damage[ m_slot_r->level ] + g_local->ability_power( ) * m_r_ap_scale,
                    target->index,
                    false
                );
            }

            return 0.f;
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return 1.1f;
            default:
                return 0.f;
            }

            return std::numeric_limits< float >::max( );
        }

    private:
        static auto is_passive_active( ) -> bool{
            return static_cast< bool >( g_features->buff_cache->get_buff(
                g_local->index,
                ct_hash( "KarthusDeathDefiedBuff" )
            ) );
        }

        float m_last_cast_time{ };

        float m_q_pred_max_offset{ 65.f }; // increase to change how far off the original predicted position we can fire
        float m_q_offset_step
            { m_q_pred_max_offset / 5.f }; // increase the 5.f for better performance, decrease to find better position
        float m_q_degree_sections
        {
            16.f
        }; // this goes into 45 degree sections when set to 8, 22.5 when set to 16 etc... can increase value for more accurate transposition but slower code

        float                  m_q_range{ 875.f };
        float                  m_q_radius{ 160.f };
        float                  m_q_cast_time{ 0.25f };
        float                  m_q_dam_delay{ 0.759f };
        float                  m_last_q_time{ };
        std::array< float, 6 > m_q_damage_minions_and_champs = { 0.f, 45.f, 62.5f, 80.f, 97.5f, 115.f };

        float m_e_range{ 550.f };
        float m_last_e_time{ };
        bool  m_e_active{ };

        // passive
        bool m_in_passive{ };
        bool m_in_fullcombo{ };

        bool activated{ };

        // Q Processing
        bool  m_q_active{ };
        float m_q_server_cast_time{ };
        float m_q_start_time{ };
        bool  m_order_movement{ };

        Vec3 m_cast_position{ };

        Vec3 m_last_cast_end{ };

        float m_last_w_time{ };

        float m_w_range{ 1000.f };

        float m_r_cast_time{ 0.25f };
        // float m_r_dam_delay{ 3.0f };
        std::array< float, 6 > m_r_damage{ 0.0f, 250.f, 350.f, 500.f };
        float                  m_r_ap_scale{ 0.75f };
        float                  m_last_r_time{ };

        int m_r_potential_kills = 0;
    };
}
