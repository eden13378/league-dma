#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"
#include "../../sdk/game/pw_hud.hpp"
#include "../../sdk/game/hud_manager.hpp"

namespace features::champion_modules {
    class kindred_module final : public IModule {
    public:
        virtual ~kindred_module( ) = default;

        struct jump_point_t {
            Vec3 source{ };
            Vec3 cast{ };

            Vec3 reverse_source{ };
            Vec3 reverse_cast{ };
        };

        auto get_name( ) -> hash_t override{ return ct_hash( "kindred_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Kindred" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "kindred" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->kindred.q_enabled );
            q_settings->checkbox( _( "harass q" ), g_config->kindred.q_harass );
            q_settings->checkbox( _( "spellclear q" ), g_config->kindred.q_laneclear );
            q_settings->checkbox( _( "machinegun q" ), g_config->kindred.q_machinegun );
            q_settings->checkbox( _( "only if can reset aa" ), g_config->kindred.q_aa_reset );
            q_settings->checkbox( _( "auto wallhop q" ), g_config->kindred.q_wallhop );

            w_settings->checkbox( _( "enable" ), g_config->kindred.w_enabled );

            e_settings->checkbox( _( "enable" ), g_config->kindred.e_enabled );
            e_settings->checkbox( _( "jungleclear e" ), g_config->kindred.e_jungleclear );
            e_settings->checkbox( _( "only if can reset aa" ), g_config->kindred.e_aa_reset );
            e_settings->checkbox(
                _( "only cast to target if below 50% hp" ),
                g_config->kindred.e_execute_champ_threshold
            );
            e_settings->checkbox(
                _( "only cast to monster if below 50% hp" ),
                g_config->kindred.e_execute_camp_threshold
            );

            r_settings->checkbox( _( "enable" ), g_config->kindred.r_enabled );
            r_settings->checkbox( _( "only in full combo" ), g_config->kindred.r_only_full_combo );
            r_settings->slider_int( _( "hp% threshold to cast" ), g_config->kindred.r_health_threshold, 0, 25 );

            drawings->checkbox( _( "draw q range" ), g_config->kindred.q_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->kindred.dont_draw_on_cooldown );
            drawings->select(
                _( "stack tracker" ),
                g_config->kindred.draw_stack_tracker,
                { _( "Off" ), _( "On cooldown" ), _( "Always" ) }
            );
            drawings->checkbox( _( "^ show possible camps" ), g_config->kindred.stack_tracker_show_possible_camps );
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };


            if ( g_config->kindred.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->kindred.dont_draw_on_cooldown->
                    get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 31, 88, 255, 255 ),
                        m_q_range,
                        Renderer::outline,
                        75,
                        3.f
                    );
                }
            }

            if ( m_dash_end_time > *g_time ) g_render->line_3d( m_dash_start, m_dash_end, Color( 255, 255, 255 ), 5.f );


            if ( g_config->kindred.q_wallhop->get< bool >( ) ) {
                int i{ };
                for ( auto inst : m_jumps ) {
                    Vec3 start, end;

                    if ( inst.source.dist_to( g_local->position ) < inst.reverse_source.dist_to( g_local->position ) ) {
                        start = inst.source;
                        end   = inst.reverse_source;
                    } else {
                        start = inst.reverse_source;
                        end   = inst.source;
                    }

                    if ( start.dist_to( g_local->position ) > 2000.f ) continue;

                    auto arrow_point = end;
                    auto point_color = m_is_jumping && m_jump_index == i ? Color( 50, 255, 50 ) : Color( 255, 255, 0 );

                    g_render->line_3d( start, arrow_point, point_color, 3.f );

                    auto arrow_angle = 42.5f;

                    auto temp = ( start - arrow_point ).rotated_raw( arrow_angle );

                    auto arrow_side_left = arrow_point.extend( arrow_point + temp, 30.f );

                    temp                  = ( start - arrow_point ).rotated_raw( -arrow_angle );
                    auto arrow_side_right = arrow_point.extend( arrow_point + temp, 30.f );

                    auto arrow_color = point_color;

                    g_render->line_3d( arrow_point, arrow_side_left, arrow_color, 4.f );
                    g_render->line_3d( arrow_point, arrow_side_right, arrow_color, 4.f );
                    g_render->line_3d(
                        start.extend( arrow_point, arrow_point.dist_to( start ) - 1.f ),
                        arrow_point.extend( start, -1.f ),
                        arrow_color,
                        4.f
                    );

                    ++i;

                    continue;
                }
            }


            Vec2 sp{ };
            if ( ( g_config->kindred.draw_stack_tracker->get< int >( ) == 2 && m_passive_active ||
                    !m_passive_active && m_passive_renew_time - *g_time < 60.f &&
                    g_config->kindred.draw_stack_tracker->get< int >( ) != 0 ) &&
                world_to_screen( g_local->position, sp ) ) {
                sp.x += 30.f;
                sp.y -= 20.f;

                // sp.y += 16.f;

                std::vector< std::shared_ptr< Renderer::Texture > > textures{ };


                if ( g_config->kindred.stack_tracker_show_possible_camps->get< bool >( ) ) {
                    switch ( m_passive_stacks ) {
                    case 0:
                    {
                        static auto crab_square = path::join(
                            { directory_manager::get_resources_path( ), "jungle", "crab_square.png" }
                        );
                        textures.push_back(
                            g_render->load_texture_from_file(
                                crab_square.has_value( ) ? *crab_square : ""
                            )
                        );
                    }

                        break;
                    case 1:
                    case 2:
                    case 3:
                    {
                        static auto crab_square = path::join( { directory_manager::get_resources_path( ), "jungle", "crab_square.png" } );
                        textures.push_back(
                           g_render->load_texture_from_file(
                               crab_square.has_value(  ) ? *crab_square : ""
                           )
                       );

                        static auto razorbeakmini_square = path::join( { directory_manager::get_resources_path( ), "jungle", "razorbeakmini_square.png" } );
                        textures.push_back(
                            g_render->load_texture_from_file(
                                razorbeakmini_square.has_value(  ) ? *razorbeakmini_square : ""
                            )
                        );

                        static auto gromp_square = path::join( { directory_manager::get_resources_path( ), "jungle", "gromp_square.png" } );
                        textures.push_back(
                            g_render->load_texture_from_file(
                                gromp_square.has_value(  ) ? *gromp_square : ""
                            )
                        );
                    }
                        break;
                    case 4:
                    case 5:
                    case 6:
                    case 7:
                    {
                        static auto krug_square = path::join( { directory_manager::get_resources_path( ), "jungle", "krug_square.png" } );
                        textures.push_back(
                           g_render->load_texture_from_file(
                               krug_square.has_value(  ) ? *krug_square : ""
                           )
                       );

                        static auto brambleback_square = path::join( { directory_manager::get_resources_path( ), "jungle", "brambleback_square.png" } );
                        textures.push_back(
                            g_render->load_texture_from_file(
                                brambleback_square.has_value(  ) ? *brambleback_square : ""
                            )
                        );

                        static auto murkwolf_square = path::join( { directory_manager::get_resources_path( ), "jungle", "murkwolf_square.png" } );
                        textures.push_back(
                            g_render->load_texture_from_file(
                                murkwolf_square.has_value(  ) ? *murkwolf_square : ""
                            )
                        );

                        static auto bluesentinel_square = path::join( { directory_manager::get_resources_path( ), "jungle", "bluesentinel_square.png" } );
                        textures.push_back(
                            g_render->load_texture_from_file(
                                bluesentinel_square.has_value(  ) ? *bluesentinel_square : ""
                            )
                        );
                    }
                        break;
                    default:
                        break;
                    }
                }

                auto font      = g_fonts->get_zabel_16px( );
                auto font_size = 16;

                std::string shield_text{ _( "STACK: " ) };
                auto        duration_text{ std::to_string( m_passive_renew_time - *g_time ) };
                if ( m_passive_renew_time - *g_time >= 10.f ) duration_text.resize( 4 );
                else duration_text.resize( 3 );

                if ( m_passive_active ) duration_text = _( "READY" );

                Vec2 shield_text_position{ sp.x, sp.y };

                auto size          = g_render->get_text_size( shield_text, font, font_size );
                auto duration_size = g_render->get_text_size( duration_text, font, font_size );

                auto duration_color{
                    m_passive_active
                        ? Color( 50, 255, 50 )
                        : m_passive_renew_time - *g_time < 10.f
                              ? g_features->orbwalker->get_pulsing_color( )
                              : Color( 255, 255, 0 )
                };


                //g_render->filled_box( shield_text_position, { duration_size.x + size.x, size.y + 20.f }, Color( 0, 0, 0, 175 ) );

                g_render->text_shadow(
                    shield_text_position,
                    Color( 255, 255, 255 ),
                    font,
                    shield_text.c_str( ),
                    font_size
                );

                g_render->text_shadow(
                    { shield_text_position.x + size.x, shield_text_position.y },
                    duration_color,
                    font,
                    duration_text.c_str( ),
                    font_size
                );

                Vec2 texture_position = { shield_text_position.x, shield_text_position.y + size.y };
                Vec2 texture_size{ 22.f, 22.f };

                float offset{ };

                for ( auto texture : textures ) {
                    Vec2 draw_position = texture_position;
                    draw_position.x += offset;

                    g_render->image( draw_position, texture_size, texture );
                    offset += texture_size.x;
                }
            }
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            track_q( );
            update_data( );

            if ( g_features->orbwalker->in_action( ) ) return;

            autojump_q( );

            m_q_range = 300.f + g_local->attack_range + 65.f;

            if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo ) spell_r( );

            machinegun_q( );

            if ( g_features->orbwalker->in_attack( ) || g_features->evade->is_active( ) ) return;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_w( );
                spell_e( );
                spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->kindred.q_harass->get< bool >( ) ) spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::laneclear:
                laneclear_q( );
                laneclear_e( );
                break;
            default:
                break;
            }

            const auto buff = g_features->buff_cache->get_buff(
                g_local->index,
                ct_hash( "kindredmarkofthekindredstackcounter" )
            );
            if ( buff ) m_passive_stacks = buff->buff_data->count;
        }

    private:
        auto laneclear_q( ) -> bool{
            if ( !g_config->kindred.q_laneclear->get< bool >( ) ||
                *g_time - m_last_q_time <= 0.4f || *g_time - m_last_cast_time <= 0.016f || is_dashing( ) || !g_features
                ->orbwalker->should_reset_aa( ) ||
                !m_slot_q->is_ready( true ) )
                return false;

            const auto index  = g_features->orbwalker->get_last_target_index( );
            const auto target = g_entity_list.get_by_index( index );
            if ( !target || target->is_dead( ) || target->is_plant( ) ) return false;

            auto hud = g_pw_hud->get_hud_manager( );
            if ( !hud ) return false;

            const auto cursor_position{ hud->cursor_position_unclipped };
            const auto after_dash_position{
                g_local->position.extend(
                    cursor_position,
                    std::min( 300.f, g_local->position.dist_to( cursor_position ) )
                )
            };

            if ( cast_spell( ESpellSlot::q, cursor_position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Kindred ] Laneclear Q | " << *g_time << std::endl;
                return true;
            }

            return false;
        }

        auto laneclear_e( ) -> bool{
            if ( !g_config->kindred.e_jungleclear->get< bool >( ) || *g_time - m_last_e_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.01f || m_slot_q->is_ready( ) && !is_dashing( ) ||
                !m_slot_e->is_ready( true ) )
                return false;

            const auto hud = g_pw_hud->get_hud_manager( );
            if ( !hud ) return false;

            const Object* target{ };
            int           priority{ };

            for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
                if ( !minion ||
                    minion->is_dead( ) ||
                    minion->is_invisible( ) ||
                    !minion->is_jungle_monster( ) ||
                    minion->dist_to_local( ) > m_e_range ||
                    minion->get_monster_priority( ) < 2
                )
                    continue;
                //good for jungle clear, want to ignore for OBJs and use HP threshold
                if ( minion->get_monster_priority( ) > priority ) {
                    target   = minion;
                    priority = minion->get_monster_priority( );
                }
            }

            if ( !target ) return false;

            if ( target->health / target->max_health * 100 > 60.f && g_config->kindred.e_execute_camp_threshold->get<
                bool >( ) )
                return false;

            if ( cast_spell( ESpellSlot::e, target->network_id ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                return true;
            }

            return false;
        }

        auto spell_q( ) -> bool override{
            if ( !g_config->kindred.q_enabled->get< bool >( ) ||
                *g_time - m_last_q_time <= 0.3f ||
                !m_slot_q->is_ready( ) && m_slot_q->cooldown_expire - *g_time > g_features->orbwalker->get_ping( ) ||
                g_config->kindred.q_aa_reset->get< bool >( ) && !g_features->orbwalker->should_reset_aa( )
            )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || g_features->orbwalker->is_attackable( target->index ) && !g_features->orbwalker->
                should_reset_aa( ) )
                return false;

            auto hud = g_pw_hud->get_hud_manager( );
            if ( !hud ) return false;

            const auto pred = g_features->prediction->predict_default( target->index, 0.1f );
            if ( !pred ) return false;

            const auto cursor_position{ hud->cursor_position_unclipped };
            auto       after_dash_position{
                g_local->position.extend(
                    cursor_position,
                    std::min( 300.f, g_local->position.dist_to( cursor_position ) )
                )
            };

            if ( g_features->orbwalker->is_autospacing( ) ) {
                const auto spaced_position = g_features->orbwalker->get_sticky_position( );

                if ( spaced_position.has_value( ) ) {
                    const auto dash_goal{ *spaced_position };
                    after_dash_position = g_local->position.dist_to( dash_goal ) > 300.f
                                              ? g_local->position.extend( dash_goal, 300.f )
                                              : dash_goal;
                }
            }

            if ( after_dash_position.dist_to( *pred ) > g_local->attack_range + 125.f || !g_features->evade->
                is_position_safe( after_dash_position ) )
                return false;

            if ( cast_spell( ESpellSlot::q, g_local->position.extend( cursor_position, 500.f ) ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Kindred: Q ] Cast | " << *g_time << std::endl;
                return true;
            }

            return false;
        }

        auto machinegun_q( ) -> bool{
            if ( !g_config->kindred.q_machinegun->get< bool >( ) || *g_time - m_last_q_time <= 0.25f || g_features->
                evade->is_active( ) ||
                !m_slot_q->is_ready( ) && m_slot_q->cooldown_expire - *g_time > g_features->orbwalker->get_ping( ) ||
                g_features->orbwalker->is_autospacing( ) ||
                g_config->kindred.q_aa_reset->get< bool >( ) && !g_features->orbwalker->in_attack( ) )
                return false;

            auto sci = g_local->spell_book.get_spell_cast_info( );
            if ( !sci || sci->server_cast_time > *g_time + g_features->orbwalker->get_ping( ) / 2.f ) return false;

            const auto target = g_entity_list.get_by_index( sci->get_target_index( ) );
            if ( !target || target->is_jungle_monster( ) && target->health / target->max_health <= 0.5f && m_slot_e->
                is_ready( true )
                || target->is_hero( ) && helper::get_real_health(
                    target->index,
                    EDamageType::physical_damage,
                    0.3f,
                    true
                ) > helper::get_aa_damage( target->index, true ) * 2.5f )
                return false;

            if ( cast_spell( ESpellSlot::q ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                std::cout << "[ Kindred: Q ] Machinegun cast | " << *g_time << std::endl;
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->kindred.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.35f || *g_time -
                m_last_cast_time <= 0.016f ||
                !is_dashing( ) || !m_slot_w->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->position.dist_to( m_dash_end ) > 750.f ) return false;


            if ( cast_spell( ESpellSlot::w, m_dash_end ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Kinder: W ] Combo | " << *g_time << std::endl;
                return true;
            }


            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->kindred.e_enabled->get< bool >( ) ||
                *g_time - m_last_e_time <= 0.25f ||
                m_slot_q->is_ready( ) && !is_dashing( ) ||
                *g_time - m_last_cast_time <= 0.01f ||
                !m_slot_e->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || !g_features->orbwalker->is_attackable( target->index ) ||
                g_features->buff_cache->get_buff( target->index, ct_hash( "kindredecharge" ) ) )
                return false;

            const auto target_hp_percent = helper::get_real_health(
                target->index,
                EDamageType::physical_damage,
                0.25f,
                true
            ) / target->max_health;
            if ( target_hp_percent > 0.5f && g_config->kindred.e_execute_champ_threshold->get< bool >( ) ) return false;

            if ( cast_spell( ESpellSlot::e, target->network_id ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                std::cout << "[ Kinder: E ] Combo | " << *g_time << std::endl;
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->kindred.r_enabled->get< bool >( ) || g_config->kindred.r_only_full_combo->get< bool >( ) &&
                !GetAsyncKeyState( VK_CONTROL )
                || *g_time - m_last_r_time <= 0.4f || !m_slot_r->is_ready( true ) )
                return false;

            const auto predicted_health = g_features->prediction->predict_health( g_local.get( ), 0.3f );
            const auto hp_threshold     = g_local->max_health * ( g_config->kindred.r_health_threshold->get< int >( ) *
                0.01f );
            if ( predicted_health > hp_threshold ) return false;

            if ( cast_spell( ESpellSlot::r ) ) {
                m_last_r_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto autojump_q( ) -> void{
            if ( !g_config->kindred.q_wallhop->get< bool >( ) || *g_time - m_last_q_time < 0.3f ) return;

            if ( !m_is_jumping ) {
                if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::none ||
                    g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::recalling ||
                    !m_slot_q->is_ready( ) && m_slot_q->cooldown_expire - *g_time > g_features->orbwalker->get_ping( ) *
                    1.25f ||
                    m_slot_q->get_manacost( ) > g_local->mana )
                    return;


                const auto cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;

                bool did_select{ };
                Vec3 selected_source{ };
                Vec3 selected_cast{ };
                int  selected_index{ };

                int index{ };

                for ( auto inst : m_jumps ) {
                    if ( cursor.dist_to( inst.source ) < 150.f &&
                        g_local->position.dist_to( inst.source ) < 180.f ) {
                        selected_source = inst.source;
                        selected_cast   = inst.cast;
                        selected_index  = index;
                        did_select      = true;
                        break;
                    }

                    if ( cursor.dist_to( inst.reverse_source ) < 150.f &&
                        g_local->position.dist_to( inst.reverse_source ) < 180.f ) {
                        selected_source = inst.reverse_source;
                        selected_cast   = inst.reverse_cast;
                        selected_index  = index;
                        did_select      = true;
                        break;
                    }

                    ++index;
                }

                if ( !did_select || !g_features->orbwalker->send_move_input( selected_source, true ) ) return;

                g_features->orbwalker->disable_for( 1.3f );

                m_is_jumping      = true;
                m_jump_source     = selected_source;
                m_jump_direction  = selected_cast;
                m_jump_start_time = *g_time;
                m_jump_index      = selected_index;
                return;
            }

            if ( g_features->evade->is_active( ) ) {
                stop_jumping( );
                return;
            }

            auto aimgr = g_local->get_ai_manager( );
            if ( !aimgr ) return;

            const auto path_end = aimgr->path_end;
            if ( path_end.dist_to( m_jump_source ) > 20.f ) {
                stop_jumping( );
                return;
            }

            if ( g_local->position.dist_to( path_end ) > 15.f ) return;


            if ( cast_spell( ESpellSlot::q, m_jump_direction ) ) {
                m_last_q_time = *g_time;
                std::cout << "[ Kindred ] Wallhop Q | " << *g_time << std::endl;

                g_features->orbwalker->disable_for( 0.1f );
                m_is_jumping = false;
            }
        }

        auto stop_jumping( ) -> void{
            m_is_jumping = false;
            g_features->orbwalker->disable_for( 0.f );
        }

        auto track_q( ) -> void{
            if ( m_was_q_ready ) {
                if ( m_slot_q->is_ready( ) ) return;

                m_was_q_ready       = false;
                m_q_cooldown_expire = m_slot_q->cooldown_expire;

                const auto last_cast_time{ m_slot_q->cooldown_expire - m_slot_q->cooldown };

                if ( last_cast_time < *g_time + 0.3f && last_cast_time > *g_time - 0.3f ) {
                    //std::cout << "> AA Reset detected | " << *g_time << std::endl;

                    //m_dash_start = m_slot_q->get_details( )->last_start_position;
                    //m_dash_end   = m_slot_q->get_details( )->last_end_position;
                    //if ( m_dash_end.dist_to( m_dash_start ) > 300.f )
                    //m_dash_end = m_dash_start.extend( m_dash_end, 300.f );

                    m_real_dash_found = false;

                    //m_dash_end_time =
                    //    last_cast_time + m_dash_start.dist_to( m_dash_end ) / ( 500.f + g_local->movement_speed );
                }
            }

            if ( !m_was_q_ready && !m_real_dash_found ) {
                auto aimgr = g_local->get_ai_manager( );
                if ( aimgr && aimgr->is_moving && aimgr->is_dashing ) {
                    m_dash_start = aimgr->path_start;
                    m_dash_end   = aimgr->path_end;

                    const auto total_duration = m_dash_start.dist_to( m_dash_end ) / aimgr->dash_speed;
                    const auto duration_left  = g_local->position.dist_to( m_dash_end ) / aimgr->dash_speed;

                    //std::cout << "[ Found Q DASH ] Length: " << aimgr->path_start.dist_to( aimgr->path_end ) << " | speed: " << aimgr->dash_speed << "Dash duration: " << total_duration
                    //          << std::endl;

                    m_dash_end_time   = *g_time + g_local->position.dist_to( m_dash_end ) / aimgr->dash_speed;
                    m_real_dash_found = true;

                    g_features->orbwalker->disable_autoattack_until(
                        m_dash_end_time - g_features->orbwalker->get_ping( )
                    );
                    g_features->orbwalker->reset_aa_timer( );
                }
            }

            if ( !m_was_q_ready && m_slot_q->is_ready( ) ) m_was_q_ready = true;
        }

        auto is_dashing( ) const -> bool{
            return m_real_dash_found && m_dash_end_time > *g_time + g_features->orbwalker->get_ping( );
        }

        auto update_data( ) -> void{
            if ( *g_time < 209.f ) {
                m_passive_active     = false;
                m_passive_end_time   = 0.f;
                m_passive_renew_time = 210.f;
            }

            if ( m_passive_active ) {
                const auto buff = g_features->buff_cache->get_buff(
                    g_local->index,
                    ct_hash( "kindredhitlistmonsteractivetracker" )
                );
                if ( !buff ) {
                    m_passive_active   = false;
                    m_passive_end_time = *g_time;

                    m_passive_renew_time = m_passive_end_time + 45.5f;
                    return;
                }
            } else {
                const auto buff = g_features->buff_cache->get_buff(
                    g_local->index,
                    ct_hash( "kindredhitlistmonsteractivetracker" )
                );
                if ( !buff ) return;

                m_passive_active   = true;
                m_passive_end_time = buff->buff_data->end_time;
            }
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->bonus_attack_damage( ) * 0.4f,
                    target->index,
                    true
                );
            case ESpellSlot::w:
                return helper::calculate_damage(
                    m_w_damage[ get_slot_w( )->level ] + g_local->bonus_attack_damage( ) * 0.7f,
                    target->index,
                    true
                );
            }
            return 0.f;
        }

    private:
        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_r_time{ };

        float m_last_cast_time{ };
        bool  m_passive_logged{ };

        // passive tracker
        bool  m_passive_active{ };
        float m_passive_end_time{ };
        float m_passive_renew_time{ };

        int m_passive_stacks{ };

        // autojumping
        bool m_is_jumping{ };
        Vec3 m_jump_source{ };
        Vec3 m_jump_direction{ };
        int  m_jump_index{ };

        float m_jump_start_time{ };

        // q cd track
        bool  m_was_q_ready{ };
        float m_q_cooldown_expire{ };

        Vec3  m_dash_start{ };
        Vec3  m_dash_end{ };
        float m_dash_end_time{ };

        bool m_real_dash_found{ };

        std::vector< jump_point_t > m_jumps = {

            //topside krug corridor

            {
                Vec3{ 5574.f, 56.f, 12102.f },
                Vec3{ 5491.f, -71.f, 10772.f },
                Vec3{ 5548.f, 56.f, 11702.f },
                Vec3{ 5800.f, 52.f, 14643.f }
            },

            // redbuff topside
            {
                Vec3{ 7224.f, 51.f, 10206.f },
                Vec3{ 6649.f, 53.f, 11860.f },
                Vec3{ 7078.f, 56.f, 10598.f },
                Vec3{ 7358.f, 52.f, 9771.f },
            },

            // baron pit
            {
                Vec3{ 5324.f, 56.f, 11306.f },
                Vec3{ 5170.f, -71.f, 10709.f },
                Vec3{ 5223.f, -71.f, 10908.f },
                Vec3{ 5393.f, 56.f, 11673.f }
            },

            //topside redbuff alt
            {
                Vec3{ 6446.f, 56.f, 11168.f },
                Vec3{ 7138.f, 56.f, 10840.f },
                Vec3{ 6808.f, 55.f, 10994.f },
                Vec3{ 4054.f, 56.f, 12528.f }
            },

            // topside bluebuff
            {
                Vec3{ 4370.f, 48.f, 8166.f },
                Vec3{ 2993.f, 51.f, 7465.f },
                Vec3{ 4014.f, 51.f, 7984.f },
                Vec3{ 6562.f, 27.f, 9237.f }
            },

            // topside bluebuff alt
            {
                Vec3{ 4366.f, 50.f, 7444.f },
                Vec3{ 3601.f, 51.f, 8237.f },
                Vec3{ 4088.f, 51.f, 7730.f },
                Vec3{ 4599.f, 50.f, 7283.f }
            },

            // topside gromp
            {
                Vec3{ 2046.f, 50.f, 7874.f },
                Vec3{ 2633.f, 51.f, 8682.f },
                Vec3{ 2274.f, 51.f, 8212.f },
                Vec3{ 1679.f, 52.f, 7382.f }
            },

            // topside gromp to pixelbush
            {
                Vec3{ 2946.f, 50.f, 9040.f },
                Vec3{ 660.f, 52.f, 8911.f },
                Vec3{ 2548.f, 51.f, 9016.f },
                Vec3{ 5286.f, -71.f, 9046.f }
            },

            // topside blue tribush to toplane riverbush
            {
                Vec3{ 2894.f, 43.f, 10324.f },
                Vec3{ 3762.f, 56.f, 12204.f },
                Vec3{ 3062.f, -70.f, 10686.f },
                Vec3{ 2559.f, 54.f, 9671.f }
            },

            // baronpit to redside jungle
            {
                Vec3{ 5466.f, -72.f, 10418.f },
                Vec3{ 6854.f, 53.f, 9928.f },
                Vec3{ 5844.f, 54.f, 10284.f },
                Vec3{ 4594.f, -71.f, 10645.f }
            },

            // topside chickens to mid
            {
                Vec3{ 7675.f, 52.f, 8923.f },
                Vec3{ 7918.f, 50.f, 10561.f },
                Vec3{ 7727.f, 52.f, 9283.f },
                Vec3{ 7613.f, 52.f, 8255.f },
            },

            // topside blue to pixelbrush
            {
                Vec3{ 3032.f, 51.f, 8380.f },
                Vec3{ 3718.f, -69.f, 10547.f },
                Vec3{ 3154.f, 50.f, 8760.f },
                Vec3{ 2958.f, 51.f, 8007.f },
            },

            // topside red to blastcone
            {
                Vec3{ 8172.f, 50.f, 11106.f },
                Vec3{ 9534.f, 52.f, 11801.f },
                Vec3{ 8526.f, 52.f, 11286.f },
                Vec3{ 7453.f, 55.f, 10638.f },
            },

            // topside red to krugs
            {
                Vec3{ 6774.f, 53.f, 11759.f },
                Vec3{ 5971.f, 52.f, 13133.f },
                Vec3{ 6585.f, 56.f, 12058.f },
                Vec3{ 7206.f, 56.f, 11036.f },
            },

            // topside redside to chickens
            {
                Vec3{ 8572.f, 50.f, 10656.f },
                Vec3{ 7265.f, 54.f, 10480.f },
                Vec3{ 8174.f, 49.f, 10602.f },
                Vec3{ 9977.f, 52.f, 10510.f },
            },

            // blueside botlane jungle entrance
            {
                Vec3{ 5598.f, 50.f, 1768.f },
                Vec3{ 7055.f, 52.f, 3184.f },
                Vec3{ 5886.f, 52.f, 2048.f },
                Vec3{ 4984.f, 107.f, 1051.f },
            },

            // blueside red to krugs
            {
                Vec3{ 8282.f, 51.f, 2856.f },
                Vec3{ 6885.f, 48.f, 4409.f },
                Vec3{ 8012.f, 51.f, 3154.f },
                Vec3{ 8934.f, 62.f, 2006.f },
            },

            //blueside jungle to krugs #2
            {
                Vec3{ 8560.f, 51.f, 2918.f },
                Vec3{ 8819.f, 53.f, 4417.f },
                Vec3{ 8626.f, 54.f, 3312.f },
                Vec3{ 8450.f, 51.f, 2201.f },
            },

            // dragonpit to blueside jungle
            {
                Vec3{ 9794.f, 51.f, 3454.f },
                Vec3{ 10316.f, -62.f, 5389.f },
                Vec3{ 9896.f, -71.f, 3840.f },
                Vec3{ 9614.f, 49.f, 2712.f },
            },

            // botriver bush to redside jungle
            {
                Vec3{ 12100.f, 51.f, 4548.f },
                Vec3{ 11438.f, -70.f, 3463.f },
                Vec3{ 11890.f, -71.f, 4208.f },
                Vec3{ 13084.f, 55.f, 5944.f },
            },

            // redside gromp to jungle pixelbrush
            {
                Vec3{ 11972.f, 50.f, 5848.f },
                Vec3{ 13684.f, 53.f, 6353.f },
                Vec3{ 12374.f, 60.f, 5957.f },
                Vec3{ 10942.f, -67.f, 5524.f },
            },

            // redside gromp to botlane
            {
                Vec3{ 12689.f, 51.f, 6610.f },
                Vec3{ 14014.f, 52.f, 8240.f },
                Vec3{ 12938.f, 51.f, 6887.f },
                Vec3{ 12153.f, 53.f, 5759.f },
            },

            // redside gromp to botlane #2
            {
                Vec3{ 12766.f, 51.f, 6176.f },
                Vec3{ 14184.f, 52.f, 5834.f },
                Vec3{ 13172.f, 55.f, 6107.f },
                Vec3{ 11201.f, 51.f, 6722.f },
            },

            // redside tier1 bot turret to tribush
            {
                Vec3{ 12734.f, 52.f, 4934.f },
                Vec3{ 13631.f, 52.f, 4243.f },
                Vec3{ 13050.f, 51.f, 4690.f },
                Vec3{ 11966.f, 53.f, 5554.f },
            },

            // redside river cornerhop
            {
                Vec3{ 10680.f, -67.f, 5666.f },
                Vec3{ 12060.f, 48.f, 6008.f },
                Vec3{ 11066.f, 10.f, 5762.f },
                Vec3{ 9959.f, -71.f, 5637.f },
            },

            // redside jungle to river
            {
                Vec3{ 11420.f, -71.f, 4858.f },
                Vec3{ 12672.f, 51.f, 5118.f },
                Vec3{ 11810.f, 51.f, 4942.f },
                Vec3{ 10580.f, -72.f, 4584.f },
            },

            // dragonpit to river
            {
                Vec3{ 10466.f, -71.f, 4320.f },
                Vec3{ 12341.f, 51.f, 4449.f },
                Vec3{ 10864.f, -71.f, 4348.f },
                Vec3{ 9491.f, -71.f, 4313.f },
            },

            // blueside botjungle to general jungle
            {
                Vec3{ 9254.f, 54.f, 3194.f },
                Vec3{ 9381.f, 54.f, 2251.f },
                Vec3{ 9306.f, 49.f, 2798.f },
                Vec3{ 9180.f, 53.f, 4225.f },
            },

            // blueside botlane to krugs
            {
                Vec3{ 8562.f, 49.f, 1712.f },
                Vec3{ 8447.f, 53.f, 3451.f },
                Vec3{ 8534.f, 51.f, 2112.f },
                Vec3{ 8615.f, 50.f, 1183.f },
            },

            // blueside botlane near-turret wall
            {
                Vec3{ 11422.f, 49.f, 1958.f },
                Vec3{ 10855.f, 49.f, 2404.f },
                Vec3{ 11122.f, 49.f, 2066.f },
                Vec3{ 12217.f, 52.f, 1726.f },
            },

            // botside alcove
            {
                Vec3{ 13220.f, 51.f, 2064.f },
                Vec3{ 13435.f, 39.f, 1057.f },
                Vec3{ 13302.f, 35.f, 1672.f },
                Vec3{ 12807.f, 51.f, 3534.f },
            },

            // botside alcove #2
            {
                Vec3{ 12896.f, 51.f, 1698.f },
                Vec3{ 13908.f, 51.f, 1302.f },
                Vec3{ 13268.f, 31.f, 1552.f },
                Vec3{ 11902.f, 51.f, 2192.f },
            },

            // blueside bot tribush to bot jungle
            {
                Vec3{ 10104.f, 49.f, 2966.f },
                Vec3{ 9447.f, 61.f, 2191.f },
                Vec3{ 9844.f, 49.f, 2660.f },
                Vec3{ 10717.f, 24.f, 3566.f },
            },

            // botside red to general jungle
            {
                Vec3{ 8360.f, 54.f, 3656.f },
                Vec3{ 6325.f, 48.f, 5238.f },
                Vec3{ 8023.f, 53.f, 3907.f },
                Vec3{ 8950.f, 54.f, 3280.f },
            },

            // dragonpit to blueside jungle
            {
                Vec3{ 9404.f, -71.f, 4716.f },
                Vec3{ 8233.f, 51.f, 5033.f },
                Vec3{ 9016.f, 51.f, 4822.f },
                Vec3{ 10570.f, -71.f, 4222.f },
            },

            // blueside chickens to mid
            {
                Vec3{ 7038.f, 58.f, 5622.f },
                Vec3{ 7937.f, 53.f, 7594.f },
                Vec3{ 7204.f, 52.f, 5986.f },
                Vec3{ 6822.f, 48.f, 5090.f },
            },

            // midlane to redside jungle entrance
            {
                Vec3{ 7326.f, 52.f, 8506.f },
                Vec3{ 7196.f, 54.f, 7552.f },
                Vec3{ 7272.f, 16.f, 8110.f },
                Vec3{ 7453.f, 53.f, 9428.f },
            },

            // midlane topside bush to redside jungle
            {
                Vec3{ 6660.f, -71.f, 8604.f },
                Vec3{ 6936.f, 54.f, 9806.f },
                Vec3{ 6750.f, 51.f, 8992.f },
                Vec3{ 6592.f, -67.f, 8175.f },
            },

            // blueside red to deep entrance
            {
                Vec3{ 6660.f, 48.f, 3802.f },
                Vec3{ 5645.f, 52.f, 3070.f },
                Vec3{ 6336.f, 49.f, 3568.f },
                Vec3{ 7620.f, 50.f, 4526.f },
            },

            // blueside red to deep entrance #2
            {
                Vec3{ 6654.f, 48.f, 4262.f },
                Vec3{ 5450.f, 49.f, 4333.f },
                Vec3{ 6256.f, 48.f, 4286.f },
                Vec3{ 7820.f, 53.f, 4287.f },
            },

            // blueside red to mid river entrance
            {
                Vec3{ 7774.f, 53.f, 4294.f },
                Vec3{ 7474.f, 52.f, 5719.f },
                Vec3{ 7690.f, 49.f, 4686.f },
                Vec3{ 8018.f, 51.f, 3553.f },
            },

            // dragonpit to river #2
            {
                Vec3{ 9670.f, -71.f, 4908.f },
                Vec3{ 9238.f, -71.f, 6122.f },
                Vec3{ 9534.f, -71.f, 5286.f },
                Vec3{ 10299.f, 52.f, 3734.f },
            },

            // blueside mid jungle entrance to midlane
            {
                Vec3{ 7462.f, 52.f, 6238.f },
                Vec3{ 7789.f, 54.f, 7704.f },
                Vec3{ 7550.f, 43.f, 6628.f },
                Vec3{ 7436.f, 52.f, 6043.f },
            },

            // river to redside blue
            {
                Vec3{ 10388.f, 51.f, 6812.f },
                Vec3{ 12098.f, 52.f, 7594.f },
                Vec3{ 10750.f, 51.f, 6978.f },
                Vec3{ 9106.f, -71.f, 6441.f },
            },

            // central redside bot jungle to blue
            {
                Vec3{ 10686.f, 51.f, 7536.f },
                Vec3{ 11067.f, 51.f, 6482.f },
                Vec3{ 10814.f, 51.f, 7176.f },
                Vec3{ 10188.f, 49.f, 8876.f },
            },

            // redside blue to jungle pixelbush
            {
                Vec3{ 11778.f, 51.f, 6550.f },
                Vec3{ 11470.f, 23.f, 5490.f },
                Vec3{ 11666.f, 51.f, 6166.f },
                Vec3{ 12037.f, 52.f, 7543.f },
            },
        };



        std::vector< float > m_q_damage = { 0.f, 40.f, 80.f, 100.f, 130.f, 160.f };
        std::vector< float > m_w_damage = { 0.f, 70.f, 115.f, 160.f, 205.f, 250.f };
        std::vector< float > m_r_damage = { 0.f, 175.f, 250.f, 325.f };

        float m_q_range{ };
        float m_q_dash_range{ 300.f };

        float m_w_range{ 1300.f };
        float m_e_range{ 425.f };
    };
}
