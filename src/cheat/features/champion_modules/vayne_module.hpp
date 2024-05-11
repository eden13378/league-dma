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
    class vayne_module final : public IModule {
    public:
        virtual ~vayne_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "vayne_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Vayne" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "vayne" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "Q SETTINGS" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->vayne.q_enabled );
            q_settings->checkbox( _( "harass q" ), g_config->vayne.q_harass );
            //q_settings->checkbox( _( "q cursor range check (?)" ), g_config->vayne.q_check_mouse_range ) ->set_tooltip( _( "Will cast Q only if cursor is outside of AA range" ) );
            q_settings->checkbox( _( "q turret range check" ), g_config->vayne.q_check_turret_range );
            //q_settings->select( _( "q direction" ), g_config->vayne.q_direction_mode, { "Automatic", "To cursor" } );

            q_settings->multi_select(
                _( "q conditions " ),
                { g_config->vayne.q_engage, g_config->vayne.q_proc_w, g_config->vayne.q_position_for_e },
                { _( "Engage" ), _( "2x W stack" ), _( "Q->E combo" ) }
            );
            q_settings->checkbox( _( "q require aa reset" ), g_config->vayne.q_require_aa_reset );
            q_settings->slider_int( _( "dont q into X > enemies" ), g_config->vayne.q_max_enemy_count, 2, 5, 1 );

            e_settings->checkbox( _( "enable" ), g_config->vayne.e_enabled );
            e_settings->checkbox( _( "killsteal e" ), g_config->vayne.e_killsteal );
            e_settings->checkbox( _( "auto interrupt (?)" ), g_config->vayne.e_autointerrupt )->set_tooltip(
                _( "Automatically interrupts spells like Katarina R, Malzahar R, etc" )
            );
            e_settings->checkbox( _( "antigapcloser" ), g_config->vayne.e_antigapclose );
            e_settings->select( _( "mode" ), g_config->vayne.e_mode, { "Consistent", "Fast" } );

            drawings->checkbox( _( "draw q range" ), g_config->vayne.q_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->vayne.e_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->vayne.dont_draw_on_cooldown );
            drawings->checkbox( _( "draw e push indicator" ), g_config->vayne.e_draw_push );
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) return;

            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->vayne.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) && !m_q_active || !g_config->vayne.
                    dont_draw_on_cooldown->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 31, 88, 255, 255 ),
                        m_q_range,
                        Renderer::outline,
                        60,
                        2.f
                    );
                }
            }

            if ( g_config->vayne.e_draw_range->get< bool >( ) || g_config->vayne.e_draw_push->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->vayne.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    if ( g_config->vayne.e_draw_range->get< bool >( ) ) {
                        g_render->circle_3d(
                            g_local->position,
                            Color( 255, 220, 0 ),
                            m_e_range,
                            Renderer::outline,
                            60,
                            3.f
                        );
                    }

                    if ( g_config->vayne.e_draw_push->get< bool >( ) ) {
                        const auto target = g_features->target_selector->get_default_target( );
                        if ( target && target->dist_to_local( ) < m_e_range + m_q_range ) {
                            const auto condemn = get_condemn_wall_position(
                                g_local->position,
                                target->position,
                                target->get_bounding_radius( )
                            );
                            const auto push_position = condemn.has_value( )
                                                           ? condemn.value( )
                                                           : target->position.extend( g_local->position, -475.f );

                            const auto draw_color = condemn.has_value( )
                                                        ? g_features->orbwalker->get_pulsing_color( )
                                                        : Color( 255, 255, 255 );

                            g_render->line_3d( target->position, push_position, draw_color, 5.f );

                            const auto cap_start = push_position.extend(
                                push_position + ( target->position - g_local->position ).rotated_raw( 90.f ),
                                50.f
                            );
                            const auto cap_end = cap_start.extend( push_position, 100.f );

                            g_render->line_3d( cap_start, cap_end, draw_color, 5.f );
                        }
                    }
                }
            }

            /*if ( !g_features->orbwalker->should_reset_aa( ) ) return;

            const auto cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;
            Vec2       sp{ };
            if ( !world_to_screen( cursor, sp ) ) return;

            sp.y -= 20.f;

            const std::string text = "-> Reset AA ";

            g_render->text_shadow( sp, Color( 50, 255, 25 ), g_fonts->get_nexa_16px( ), text.data( ), 16 );*/
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            cast_tracking( );

            m_q_active  = !!g_features->buff_cache->get_buff( g_local->index, ct_hash( "vaynetumblebonus" ) );
            m_q_allowed = !g_config->vayne.q_require_aa_reset->get< bool >( ) || g_features->orbwalker->
                should_reset_aa( );

            if ( m_disable_spells_expire_time > *g_time ) return;

            e_cast( );

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return;

            antigapclose_e( );
            autointerrupt_e( );

            if ( g_features->orbwalker->in_attack( ) ) return;

            e_killsteal( );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                combo_q( );
                condemn_q( );
                spell_e( );

            //if ( g_config->vayne.q_enabled->get< bool >( ) ) spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->vayne.q_harass->get< bool >( ) ) spell_q( );
                break;
            default:
                break;
            }

            //VayneSilveredDebuff
            //vaynetumblebonus

            // std::cout << "flag: " << std::hex << m_slot_q.get_address() << std::endl;

            if ( !m_e_print_hit ) return;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return;

            const auto buff = g_features->buff_cache->get_buff( target->index, ct_hash( "VayneSilveredDebuff" ) );
            if ( buff ) {
                std::cout << "[ Hit found ] time: " << buff->buff_data->start_time << std::endl;
                m_e_print_hit = false;
            }
            // else std::cout << "No buff found yet? \n";
        }

    private:
        auto spell_q( ) -> bool override{
            if ( *g_time - m_last_q_time <= 0.4f || !m_q_allowed || m_q_active || !m_slot_q->is_ready( true )
                || !g_features->orbwalker->should_reset_aa( ) )
                return false;

            Vec3 cast_position{ };
            bool allow_q{ };

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:
            case Orbwalker::EOrbwalkerMode::harass:
            {
                if ( g_config->vayne.q_direction_mode->get< int >( ) == 0 ) {
                    const auto pos = get_automatic_dash_position( );
                    if ( pos.has_value( ) ) {
                        cast_position = *pos;
                        allow_q       = true;
                    }
                } else {
                    cast_position = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;
                    allow_q       = true;
                }
                break;
            }
            case Orbwalker::EOrbwalkerMode::laneclear:
                if ( g_features->orbwalker->get_last_target( ) == ETargetType::turret
                    || g_features->orbwalker->get_last_target( ) == ETargetType::minion ) {
                    const auto fast = get_fast_dash_position( );
                    if ( fast.has_value( ) ) {
                        cast_position = fast.value( );
                        allow_q       = true;
                    }
                }
                break;

            default:
                break;
            }

            if ( !allow_q ) return false;

            if ( cast_spell( ESpellSlot::q, cast_position ) ) {
                m_last_q_time = *g_time;
                return true;
            }

            return false;
        }

        auto combo_q( ) -> bool{
            if ( *g_time - m_last_cast_time <= 0.05f || *g_time - m_last_q_time <= 0.4f || !m_q_allowed || m_q_active ||
                !m_slot_q->is_ready( true ) )
                return false;

            for ( auto i = 0; i < 2; i++ ) {
                if ( g_features->target_selector->is_forced( ) && i > 0 ) break;

                const auto target = g_features->target_selector->get_default_target( i > 0 );
                if ( !target ) break;

                if ( engage_q( target ) || proc_w_q( target ) ) return true;
            }

            return false;
        }

        auto engage_q( Object* target ) -> bool{
            if ( !target || !g_config->vayne.q_engage->get< bool >( )
                || g_features->orbwalker->is_attackable( target->index ) )
                return false;

            const auto cursor        = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;
            const auto dash_position = g_local->position.extend( cursor, 300.f );

            const auto pred = g_features->prediction->predict_default( target->index, 0.3f );
            if ( !pred ) return false;

            const auto attack_range = g_local->attack_range + 65.f;

            if ( dash_position.dist_to( target->position ) > attack_range || dash_position.dist_to( *pred ) >
                attack_range
                || g_navgrid->is_wall( dash_position ) || !is_good_dash_position( dash_position ) )
                return false;

            if ( cast_spell( ESpellSlot::q, cursor ) ) {
                m_last_cast_time = *g_time;
                m_last_q_time    = *g_time;

                std::cout << "[ Vayne: Q ] Engaging toward " << target->champion_name.text << std::endl;

                return true;
            }

            return false;
        }

        auto proc_w_q( Object* target ) -> bool{
            if ( !target || !g_config->vayne.q_proc_w->get< bool >( ) ) return false;

            const auto buff = g_features->buff_cache->get_buff( target->index, ct_hash( "VayneSilveredDebuff" ) );
            if ( buff ) {
                auto stacks = buff->stacks( );

                if ( stacks == 1 && g_features->prediction->get_incoming_attack_count( target->index ) > 0 ) ++stacks;

                if ( stacks < 2 ) return false;
            } else return false;

            const auto cursor        = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;
            const auto dash_position = g_local->position.extend( cursor, 300.f );

            const auto pred = g_features->prediction->predict_default( target->index, 0.3f );
            if ( !pred ) return false;

            const auto attack_range = g_local->attack_range + 65.f;

            if ( dash_position.dist_to( target->position ) > attack_range ||
                dash_position.dist_to( *pred ) > attack_range || g_navgrid->is_wall( dash_position ) ||
                !is_good_dash_position( dash_position ) ) {
                std::cout << "Bad dash | " << *g_time << std::endl;
                return false;
            }

            if ( cast_spell( ESpellSlot::q, cursor ) ) {
                m_last_cast_time = *g_time;
                m_last_q_time    = *g_time;

                std::cout << "[ Vayne: Q ] Proc W off " << target->champion_name.text << std::endl;

                return true;
            }

            return false;
        }

        auto condemn_q( ) -> bool{
            if ( !g_config->vayne.q_position_for_e->get< bool >( ) || !m_q_allowed ||
                *g_time - m_last_cast_time <= 0.05f || *g_time - m_last_q_time <= 0.4f ||
                m_q_active || !m_slot_e->is_ready( true ) || !m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto cast_prediction = g_features->prediction->predict_movement( target->index, 0.333f );
            if ( !cast_prediction ) return false;

            const auto target_position = cast_prediction.value( );

            const auto target_bounding = target->get_bounding_radius( );
            const auto local_bounding  = g_local->get_bounding_radius( );

            const auto points = g_render->get_3d_circle_points( g_local->position, 300.f, 16 );

            Vec3 selected_point{ };
            bool found_dash{ };

            for ( auto dash_point : points ) {
                if ( dash_point.dist_to( target_position ) > m_e_range + local_bounding + target_bounding * 0.75f ||
                    g_navgrid->is_wall( dash_point ) )
                    continue;

                auto travel_time = 0.333f + 0.25f + dash_point.dist_to( target->position ) / 2200.f;
                auto pred        = g_features->prediction->predict_movement( target->index, travel_time );
                if ( !pred ) return false;

                travel_time = 0.333f + 0.25f + dash_point.dist_to( *pred ) / 2200.f;
                pred        = g_features->prediction->predict_movement( target->index, travel_time );
                if ( !pred ) return false;

                if ( !get_condemn_wall_position( dash_point, *pred, target->get_bounding_radius( ) ) ) continue;

                selected_point = dash_point;
                found_dash     = true;
                break;
            }

            if ( !found_dash ) return false;

            if ( cast_spell( ESpellSlot::q, selected_point ) ) {
                m_last_cast_time = *g_time;
                m_last_q_time    = *g_time;

                m_waiting_e_cast    = true;
                m_e_cast_network_id = target->network_id;

                m_disable_spells_expire_time = *g_time + 0.25f;

                std::cout << "[ Vayne: Q ] Aligning position for E stun | " << *g_time << std::endl;
                return true;
            }

            return false;
        }

        static auto is_good_dash_position( const Vec3& position ) -> bool{
            if ( g_config->vayne.q_check_turret_range->get< bool >( ) && helper::is_position_under_turret( position ) &&
                !helper::is_position_under_turret( g_local->position ) )
                return false;

            // const auto cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;
            // if ( g_config->vayne.q_check_mouse_range->get< bool >( ) &&
            //      cursor.dist_to( g_local->position ) < g_local->attack_range + 75.f )
            //     return false;

            int enemy_count{ };
            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->dist_to_local( ) > g_local->attack_range + 100.f || enemy->is_dead( ) ) continue;

                ++enemy_count;
            }

            return enemy_count < g_config->vayne.q_max_enemy_count->get< int >( );
        }

        auto e_cast( ) -> bool{
            if ( *g_time - m_last_cast_time <= 0.05f || m_e_active || !m_waiting_e_cast || !m_slot_e->
                is_ready( true ) )
                return false;

            if ( cast_spell( ESpellSlot::e, m_e_cast_network_id ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                m_waiting_e_cast = false;

                std::cout << "[ Vayne: E ] Casting E due to forced order | " << *g_time << std::endl;
            }

            return false;
        }

        auto spell_w( ) -> bool override{ return false; }

        auto spell_e( ) -> bool override{
            if ( !g_config->vayne.e_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f || *g_time -
                m_last_e_time <= 0.4f || !m_slot_e->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || !g_features->orbwalker->is_attackable( target->index, m_e_range ) ) return false;

            bool allow_e{ };

            const auto pred = g_features->prediction->predict( target->index, 1300.f, 2200.f, 0.f, 0.25f );
            if ( !pred.valid || g_config->vayne.e_mode->get< int >( ) == 0 && ( int )pred.hitchance < 2 ) return false;

            allow_e = get_condemn_wall_position(
                pred.position,
                g_local->position.extend( pred.position, 100.f )
            ).has_value( );

            if ( !allow_e ) return false;

            if ( cast_spell( ESpellSlot::e, target->network_id ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                std::cout << "[ Vayne: E ] Condemn cast at target " << target->champion_name.text << std::endl;
                return true;
            }

            return false;
        }

        auto e_killsteal( ) -> bool{
            if ( !g_config->vayne.e_killsteal->get< bool >( ) || *g_time - m_last_q_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.05f || !m_slot_e->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_killsteal_target(
                m_e_range + g_local->get_bounding_radius( ),
                [ this ]( Object* unit ) -> float{ return 0.25f + unit->dist_to_local( ) / 2200.f; },
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::e, unit ); },
                1
            );
            if ( !target || !g_features->orbwalker->is_attackable( target->index, m_e_range ) ) return false;

            if ( cast_spell( ESpellSlot::e, target->network_id ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                std::cout << "[ Vayne: E ] Killsteal cast at " << target->champion_name.text << std::endl;
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{ return false; }

        static auto will_enemy_hit_wall( const Vec3& start_position ) -> bool{
            for ( auto i = 0; i < 35; i++ ) {
                const auto extra_dist = 55.f + 10.f * i;
                auto       nav_position{
                    g_local->position.extend( start_position, g_local->position.dist_to( start_position ) + extra_dist )
                };

                if ( g_navgrid->is_wall( nav_position ) ) return true;
            }

            return false;
        }

        static auto get_condemn_wall_position(
            const Vec3& start_position,
            const Vec3& push_position,
            const float target_bounding_radius = 55.f
        ) -> std::optional< Vec3 >{
            Vec3 simulated_position{ };

            const auto simulation_limit     = 25;
            const auto simulation_step_size = ( 350.f - target_bounding_radius ) / simulation_limit;

            for ( auto i = 0; i <= simulation_limit; i++ ) {
                const auto push_amount = simulation_step_size * i + target_bounding_radius;
                simulated_position     = start_position.extend(
                    push_position,
                    start_position.dist_to( push_position ) + push_amount
                );

                if ( is_stunnable_position( push_position, simulated_position, target_bounding_radius ) ) {
                    return
                        std::make_optional( simulated_position );
                }
            }

            return std::nullopt;
        }

        static auto is_stunnable_position(
            const Vec3& start_position,
            const Vec3& position,
            const float target_bounding_radius = 55.f
        ) -> bool{
            if ( g_navgrid->is_wall( position ) ) return true;

            const auto bounding_position = position.extend( start_position, -target_bounding_radius );

            if ( g_navgrid->is_wall( bounding_position ) ) return true;

            return false;
        }

        auto antigapclose_e( ) -> void{
            if ( !g_config->vayne.e_antigapclose->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->
                is_ready( true ) )
                return;

            const auto target = get_antigapclose_target( 400.f );
            if ( !target || !g_features->orbwalker->is_attackable( target->index, m_e_range ) ) return;

            if ( cast_spell( ESpellSlot::e, target->network_id ) ) m_last_e_time = *g_time;
        }

        auto autointerrupt_e( ) -> void{
            if ( !g_config->vayne.e_autointerrupt->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->
                is_ready( true ) )
                return;

            const auto target = get_interruptable_target( m_e_range + 65.f );
            if ( !target || !g_features->orbwalker->is_attackable( target->index, m_e_range )
                || g_features->prediction->windwall_in_line( g_local->position, target->position ) )
                return;

            if ( cast_spell( ESpellSlot::e, target->network_id ) ) m_last_e_time = *g_time;
        }

        auto get_automatic_dash_position( ) -> std::optional< Vec3 >{
            std::vector< Vec3 > enemy_points{ };
            auto                lowest_distance{ 9999.f };

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return std::nullopt;

            const auto target_pred = g_features->prediction->predict_default( target->index, 0.25f );
            if ( !target_pred ) return std::nullopt;

            const auto target_position = *target_pred;
            auto cursor = g_local->position.extend( g_pw_hud->get_hud_manager( )->cursor_position_unclipped, 300.f );

            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) || enemy->dist_to_local( ) > 1000.f || enemy
                    ->network_id == target->network_id )
                    continue;

                auto pred = g_features->prediction->predict_default( enemy->index, 0.25f );
                if ( !pred ) continue;

                const auto distance = g_local->position.dist_to( *pred );

                enemy_points.push_back( *pred );

                if ( distance < lowest_distance ) lowest_distance = distance;
            }

            if ( target_position.dist_to( g_local->position ) < target->dist_to_local( ) &&
                target_position.dist_to( g_local->position ) > 300.f &&
                get_fast_dash_position( ).has_value( ) )
                return get_fast_dash_position( );

            if ( !is_wall_in_line( g_local->position, cursor ) &&
                g_features->orbwalker->is_attackable( target->index ) &&
                target_position.dist_to( g_local->position ) > target->dist_to_local( ) &&
                target->position.dist_to( cursor ) < g_local->attack_range + 65.f &&
                target_position.dist_to( cursor ) >= 300.f )
                return std::make_optional( cursor );


            // advanced dash code
            std::vector< Vec3 > dodge_points{ };
            for ( auto i = 0; i < 8; i++ ) {
                Vec3 test_position{ };

                switch ( i ) {
                case 0: // EAST
                    test_position = g_local->position + Vec3( 300.f, 0.f, 0.f );
                    break;
                case 1: // WEST
                    test_position = g_local->position + Vec3( -300.f, 0.f, 0.f );
                    break;
                case 2: // NORTH
                    test_position = g_local->position + Vec3( 0.f, 0.f, 300.f );
                    break;
                case 3: // SOUTH
                    test_position = g_local->position + Vec3( 0.f, 0.f, -300.f );
                    break;
                case 4: // NORTH EAST
                    test_position = g_local->position + Vec3( 150.f, 0.f, 150.f );
                    break;
                case 5: // SOUTH EAST
                    test_position = g_local->position + Vec3( 150.f, 0.f, -150.f );
                    break;
                case 6: // SOUTH WEST
                    test_position = g_local->position + Vec3( -150.f, 0.f, -150.f );
                    break;
                case 7: // NORTH WEST
                    test_position = g_local->position + Vec3( -150.f, 0.f, 150.f );
                    break;
                default:
                    break;
                }

                if ( is_wall_in_line( g_local->position, test_position ) ) continue;

                dodge_points.push_back( test_position );
            }

            const auto local_pred = g_features->prediction->predict_default( g_local->index, 0.25f );
            if ( !local_pred ) return std::nullopt;

            Vec3 best_point{ };
            auto highest_distance_to_pred{ 0.f };
            auto lowest_target_distance{ 9999.f };
            bool attack_range_dash{ };

            for ( auto point : dodge_points ) {
                const auto is_in_attack_range = point.dist_to( target_position ) < g_local->attack_range + 20.f;
                if ( attack_range_dash && !is_in_attack_range ||
                    point.dist_to( target_position ) < m_safe_distance ||
                    point.dist_to( target->position ) < m_safe_distance )
                    continue;

                if ( !attack_range_dash && is_in_attack_range ||
                    point.dist_to( target_position ) >= highest_distance_to_pred && point.dist_to( target->position ) <=
                    lowest_target_distance ) {
                    best_point        = point;
                    attack_range_dash = is_in_attack_range;

                    highest_distance_to_pred = point.dist_to( target_position );
                    lowest_target_distance   = point.dist_to( target->position );
                }
            }

            // point.dist_to(*local_pred) >= highest_distance_to_pred && point.dist_to(target_position) <= lowest_target_distance


            if ( best_point.length( ) != 0 ) return std::make_optional( best_point );


            // prefer dash to cursor position
            //if ( !is_wall_in_line(g_local->position, cursor))
            //    return std::make_optional(cursor);

            // prefer fast position if closest is relatively far
            /*if (lowest_distance > m_safe_distance && lowest_distance <= 575.f)
            {
                auto fast = get_fast_dash_position();
                if (fast.has_value()) return std::make_optional(*fast);
            }*/


            return std::nullopt;
        }

        static auto get_fast_dash_position( ) -> std::optional< Vec3 >{
            auto closest_wall_distance{ 300.f };
            Vec3 wall_dash{ };
            bool wall_found{ };

            for ( auto i = 0; i < 8; i++ ) {
                Vec3 test_position{ };

                switch ( i ) {
                case 0: // EAST
                    test_position = g_local->position + Vec3( 300.f, 0.f, 0.f );
                    break;
                case 1: // WEST
                    test_position = g_local->position + Vec3( -300.f, 0.f, 0.f );
                    break;
                case 2: // NORTH
                    test_position = g_local->position + Vec3( 0.f, 0.f, 300.f );
                    break;
                case 3: // SOUTH
                    test_position = g_local->position + Vec3( 0.f, 0.f, -300.f );
                    break;
                case 4: // NORTH EAST
                    test_position = g_local->position + Vec3( 150.f, 0.f, 150.f );
                    break;
                case 5: // SOUTH EAST
                    test_position = g_local->position + Vec3( 150.f, 0.f, -150.f );
                    break;
                case 6: // SOUTH WEST
                    test_position = g_local->position + Vec3( -150.f, 0.f, -150.f );
                    break;
                case 7: // NORTH WEST
                    test_position = g_local->position + Vec3( -150.f, 0.f, 150.f );
                    break;
                default:
                    break;
                }

                for ( auto x = 1; x <= 4; x++ ) {
                    const auto value = 20.f * x;
                    auto       temp  = g_local->position.extend( test_position, value );
                    if ( !g_navgrid->is_wall( temp ) ) continue;

                    if ( value < closest_wall_distance ) {
                        closest_wall_distance = value;
                        wall_dash             = test_position;
                        wall_found            = true;
                    }

                    break;
                }
            }

            if ( !wall_found ) return std::nullopt;

            return std::make_optional( wall_dash );
        }

        static auto is_wall_in_line( const Vec3& start, const Vec3& end ) -> bool{
            const auto loop_count = static_cast< int >( std::ceil( start.dist_to( end ) / 40.f ) );
            for ( auto i = 1; i <= loop_count; i++ ) {
                const auto value = std::min( i * 40.f, start.dist_to( end ) );
                auto       test  = start.extend( end, value );
                if ( g_navgrid->is_wall( test ) ) return true;
            }

            return false;
        }

        auto cast_tracking( ) -> void{
            if ( !m_e_active ) {
                auto sci = g_local->spell_book.get_spell_cast_info( );
                if ( !sci || sci->slot != 2 || sci->server_cast_time < *g_time ) return;

                m_e_active           = true;
                m_e_server_cast_time = sci->server_cast_time;

                const auto target = g_entity_list.get_by_index( sci->get_target_index( ) );
                if ( !target ) {
                    m_e_active = false;
                    return;
                }

                std::cout << "[ Condemn: Found cast ] Hit time: "
                    << sci->server_cast_time + sci->start_position.dist_to( target->position ) / 2200.f
                    << std::endl;
            }

            if ( *g_time > m_e_server_cast_time ) {
                m_e_active    = false;
                m_e_print_hit = true;
            }
        }

        auto get_w_damage( const int16_t index, const bool predict_attack = false ) -> float{
            const auto target = g_entity_list.get_by_index( index );
            if ( !target ) return 0.f;

            const auto buff = g_features->buff_cache->get_buff( target->index, ct_hash( "VayneSilveredDebuff" ) );
            if ( buff ) {
                auto       stacks          = buff->stacks( );
                const auto incoming_attack = g_features->prediction->get_incoming_attack_count( target->index ) > 0;
                if ( stacks == 2 && incoming_attack ) return 0.f;

                if ( incoming_attack ) stacks++;
                if ( predict_attack ) stacks++;

                if ( stacks != 2 ) return 0.f;
            } else return 0.f;

            const auto true_damage = m_w_percent_damage[ m_slot_w->level ] * target->max_health;

            return true_damage;
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
            {
                float total_damage{ };

                total_damage += helper::calculate_damage(
                    m_q_ad_modifier[ m_slot_q->level ] * g_local->attack_damage( ),
                    target->index,
                    true
                );

                return total_damage + helper::get_aa_damage( target->index, true ) +
                    get_w_damage( target->index, true );
            }
            case ESpellSlot::w:
                return target->max_health * m_w_percent_damage[ get_slot_w( )->level ];
            case ESpellSlot::e:
            {
                auto damage = helper::calculate_damage(
                    m_e_damage[ m_slot_e->level ] + g_local->bonus_attack_damage( ) * 0.5f,
                    target->index,
                    true
                );

                damage += get_w_damage( target->index, true );

                return damage;
            }
            default:
                break;
            }


            return 0.f;
        }

    private:
        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_r_time{ };
        float m_last_cast_time{ };

        bool     m_waiting_e_cast{ };
        unsigned m_e_cast_network_id{ };

        // tracking
        bool  m_e_active{ };
        float m_e_server_cast_time{ };
        bool  m_e_print_hit{ };

        bool m_q_active{ };
        bool m_q_allowed{ };

        // disable spells
        float m_disable_spells_expire_time{ };

        float m_safe_distance{ 375.f };

        std::vector< float > m_q_ad_modifier    = { 0.f, 0.75f, 0.85f, 0.95f, 1.05f, 1.15f };
        std::vector< float > m_w_percent_damage = { 0.f, 0.06f, 0.07f, 0.08f, 0.09f, 0.1f };
        std::vector< float > m_e_damage         = { 0.f, 50.f, 85.f, 120.f, 155.f, 190.f };

        float m_q_range{ 300.f };

        float m_w_range{ 1300.f };
        float m_e_range{ 550.f };
    };
}
