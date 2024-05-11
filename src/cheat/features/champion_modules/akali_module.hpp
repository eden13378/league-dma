#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"
#include "../../sdk/game/ai_manager.hpp"
#include "../../sdk/game/hud_manager.hpp"


namespace features::champion_modules {
    class akali_module final : public IModule {
    public:
        virtual ~akali_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "akali_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Akali" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "akali" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto settings   = navigation->add_section( _( "combo settings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto magnet     = navigation->add_section( _( "magnet" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->akali.q_enabled );
            q_settings->checkbox( _( "use to harass" ), g_config->akali.q_harass );
            q_settings->select(
                _( "hitchance" ),
                g_config->akali.q_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            settings->checkbox( _( "passive block cast (?)" ), g_config->akali.passive_block_cast )->set_tooltip(
                _( "Block auto Q/E1 when passive is active, Full combo overrides this option" )
            );
            settings->checkbox( _( "magnet block cast (?)" ), g_config->akali.magnet_block_cast )->set_tooltip(
                _( "Block auto Q/E1 when magnet is active, Full combo overrides this option" )
            );
            settings->checkbox( _( "hold e in full combo (?)" ), g_config->akali.e_hold_in_full_combo )->set_tooltip(
                _( "Will hold E in full combo if R is ready for animation cancel" )
            );

            w_settings->checkbox( _( "enable" ), g_config->akali.w_enabled );
            w_settings->checkbox( _( "anti-gapcloser" ), g_config->akali.w_antigapcloser );
            w_settings->multi_select(
                "use conditions",
                { g_config->akali.w_restore_energy, g_config->akali.w_multiple_enemy, g_config->akali.w_cancel_casts },
                { _( "Restore energy" ), _( "Many enemy nearby" ), _( "Cancel targeted casts" ) }
            );

            e_settings->checkbox( _( "enable" ), g_config->akali.e_enabled );
            e_settings->multi_select(
                "E2 conditions",
                { g_config->akali.e_on_killable, g_config->akali.e_in_full_combo },
                { _( "On killable" ), _( "In full combo" ) }
            );
            e_settings->select(
                _( "E1 hitchance" ),
                g_config->akali.e_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            r_settings->checkbox( _( "enable" ), g_config->akali.r_enabled );
            r_settings->checkbox( _( "cancel animation (?)" ), g_config->akali.r_animation_cancel )->set_tooltip(
                _( "Will cancel R1 animation with E if possible" )
            );
            r_settings->checkbox( _( "R1 in full combo" ), g_config->akali.r_in_full_combo );
            r_settings->checkbox( _( "killsteal(? )" ), g_config->akali.r_killsteal )->set_tooltip(
                _( "Only affects R2" )
            );
            r_settings->select(
                _( "R2 hitchance" ),
                g_config->akali.r_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );


            magnet->checkbox( _( "enable magnet" ), g_config->akali.magnet_orbwalk );
            magnet->checkbox( _( "magnet turret check (?)" ), g_config->akali.magnet_turret_check )->set_tooltip(
                _( "If you are not under enemy turret, will never magnet to under turret position" )
            );
            magnet->slider_int( _( "magnet distance" ), g_config->akali.magnet_distance, 100, 400, 5 );

            drawings->checkbox( _( "draw q range" ), g_config->akali.q_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->akali.e_draw_range );
            drawings->checkbox( _( "draw r range" ), g_config->akali.r_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->akali.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->akali.q_draw_range->get< bool >( ) &&
                !g_config->akali.e_draw_range->get< bool >( ) &&
                !g_config->akali.r_draw_range->get< bool >( ) ||
                g_local->is_dead( ) )
                return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->akali.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->akali.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
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

            if ( g_config->akali.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->akali.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 144, 66, 245 ),
                        m_e_range,
                        Renderer::outline,
                        60,
                        2.f
                    );
                }
            }

            if ( g_config->akali.r_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::r );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->akali.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 173, 47, 68, 255 ),
                        m_r_range,
                        Renderer::outline,
                        60,
                        2.f
                    );
                }
            }


            Vec2 sp{ };
            if ( m_magnet_active && world_to_screen( g_local->position, sp ) ) {
                g_render->text_shadow(
                    { sp.x - 20.f, sp.y - 24.f },
                    Color( 252, 194, 3 ),
                    g_fonts->get_bold( ),
                    "Magnet",
                    16
                );
            }

            return;

            if ( *g_time - m_last_e_time <= 2.5f ) {
                auto& target = g_entity_list.get_by_index( m_target_index );
                if ( !target ) return;

                g_render->circle_3d(
                    m_local_pred,
                    Color( 25, 255, 25, 80 ),
                    25.f,
                    Renderer::outline | Renderer::filled,
                    40,
                    2.f
                );
                g_render->circle_3d(
                    m_target_pred,
                    Color( 255, 50, 25, 80 ),
                    25.f,
                    Renderer::outline | Renderer::filled,
                    40,
                    2.f
                );

                g_render->circle_3d(
                    m_local_cast_pred,
                    Color( 255, 255, 255, 80 ),
                    25.f,
                    Renderer::outline | Renderer::filled,
                    40,
                    2.f
                );

                g_render->circle_3d(
                    m_cast_position,
                    Color( 255, 255, 25, 80 ),
                    25.f,
                    Renderer::outline | Renderer::filled,
                    40,
                    2.f
                );
                g_render->circle_3d(
                    m_start_pred,
                    Color( 50, 139, 168, 80 ),
                    25.f,
                    Renderer::outline | Renderer::filled,
                    40,
                    2.f
                );

                //m_cast_position

                //g_render->line_3d(g_local->position, g_local->position.extend(cast_pos, 200.f), color(25, 255, 25), 4.f);
            }

            auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return;

            Vec2 spp{ };
            if ( !world_to_screen(
                g_local->position.extend( target->position, target->dist_to_local( ) / 2.f ),
                spp
            ) )
                return;

            auto text = std::to_string( static_cast< int >( target->dist_to_local( ) ) );

            g_render->text_shadow( spp, Color::white( ), g_fonts->get_bold( ), text.c_str( ), 16 );

            return;

            // auto pred = g_features->prediction->predict_default(g_local->index, 0.4f);
            // if (pred.has_value()) g_render->circle_3d(pred.value(), color(255, 255, 25), 20.f, 2, 40, 3.f);

            // very nice local playa path drawing

            /*if (m_magnet_active)
            {
                auto aimgr = g_local->get_ai_manager();
                auto path = aimgr->get_path();

                if (aimgr->is_moving && path.size() > 1 && aimgr->next_path_node != 0 && path.size() != aimgr->next_path_node)
                {
                    vec2 sp_next;

                    if (path.size() > 1u)
                    {
                        for (int i = aimgr->next_path_node; i < static_cast<int>(path.size()); i++) {
                            if (i == aimgr->next_path_node) {
                                if (!sdk::math::world_to_screen(g_local->position, sp) || !sdk::math::world_to_screen(path[i], sp_next)) break;
                            }
                            else {
                                if (!sdk::math::world_to_screen(path[i - 1], sp) || !sdk::math::world_to_screen(path[i], sp_next)) break;
                            }

                            //g_render->line({ sp.x + 1, sp.y + 1 }, { sp_next.x + 1, sp_next.y + 1 }, color(10, 10, 10, 255), 1.f);
                            g_render->line(sp, sp_next, color(25, 255, 25), 2.f);
                        }

                        if (sdk::math::world_to_screen(path[path.size() - 1], sp))
                        {
                            g_render->filled_circle(sp, color(255, 255, 255), 3.f, 15);
                            //g_render->text_shadow({ sp.x - 5, sp.y }, color(0, 145, 255, 255), g_fonts->get_bold(), "Local", 16);
                        }
                    }
                }
            }*/

            if ( !m_ring_found ) return;

            for ( auto point : m_magnet_points ) g_render->circle_3d( point, Color::white( ), 15.f, 2, 30, 2.f );
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            m_passive_active = !!g_features->buff_cache->get_buff( g_local->index, ct_hash( "AkaliPWeapon" ) );

            cast_tracker( );

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return;

            autocancel_w( );
            antigapclose_w( );
            passive_magnet( );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_r( );
                spell_w( );
                spell_q( );
                spell_e( );
                break;

            case Orbwalker::EOrbwalkerMode::harass:
            case Orbwalker::EOrbwalkerMode::laneclear:
                if ( g_config->akali.q_harass->get< bool >( ) ) spell_q( );
                break;
            default:
                break;
            }

            if ( !m_ring_found ) {
                for ( const auto object : g_entity_list.get_ally_minions( ) ) {
                    if ( !object || object->is_dead( ) || object->is_invisible( ) || object->dist_to_local( ) > 5000.f
                        || is_ignored_ring( object->network_id ) )
                        continue;

                    std::string name = object->name.text;

                    if ( name.find( _( "TwilightShroud" ) ) == std::string::npos ) continue;

                    if ( object->dist_to_local( ) > 550.f ) {
                        m_ignored_rings.push_back( object->network_id );
                        continue;
                    }

                    m_ring_position   = object->position;
                    m_ring_index      = object->index;
                    m_ring_nid        = object->network_id;
                    m_ring_found      = true;
                    m_ring_start_time = *g_time;
                    break;
                }
            } else {
                const auto& obj = g_entity_list.get_by_index( m_ring_index );
                if ( m_passive_active || !obj || obj->is_dead( ) ) {
                    m_ignored_rings.push_back( m_ring_nid );
                    reset_ring( );
                }
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->akali.q_enabled->get< bool >( ) ||
                m_passive_active && g_config->akali.passive_block_cast->get< bool >( ) ||
                m_magnet_active && g_config->akali.magnet_block_cast->get< bool >( ) ||
                *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto pred = g_features->prediction->predict( target->index, m_q_range, 0.f, 0.f, 0.25f );
            if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->akali.q_hitchance->get<
                int >( ) )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->akali.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w->
                is_ready( ) )
                return false;


            auto       cast_position = g_local->position;
            const auto target        = g_features->target_selector->get_default_target( );
            if ( target && target->dist_to_local( ) <= 500.f ) cast_position = target->position;

            auto allow_w{ g_config->akali.w_restore_energy->get< bool >( ) && g_local->mana <= 50.f };

            if ( !allow_w && g_config->akali.w_multiple_enemy->get< bool >( ) ) {
                int enemy_count{ };
                for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                    if ( !enemy || enemy->dist_to_local( ) > 300.f || g_features->target_selector->is_bad_target(
                        enemy->index
                    ) )
                        continue;

                    ++enemy_count;
                }

                allow_w = enemy_count > 1;
            }


            if ( !allow_w ) return false;

            if ( cast_spell( ESpellSlot::w, cast_position ) ) {
                m_last_w_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->akali.e_enabled->get< bool >( )
                || *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready( ) )
                return false;

            if ( rt_hash( m_slot_e->get_name().c_str() ) == ct_hash( "AkaliE" ) ) {
                if ( !m_slot_e->is_ready( true ) ||
                    m_passive_active && g_config->akali.passive_block_cast->get< bool >( ) ||
                    ( m_magnet_active || *g_time - m_ring_start_time <= 0.2f ) && g_config->akali.magnet_block_cast->get
                    < bool >( ) ||
                    g_config->akali.e_hold_in_full_combo->get< bool >( ) && m_slot_r->is_ready( ) && rt_hash(
                        m_slot_r->get_name().c_str()
                    ) == ct_hash( "AkaliR" ) )
                    return false;

                const auto target = g_features->target_selector->get_default_target( );
                if ( !target || target->dist_to_local( ) > 825.f ) return false;

                auto pred = g_features->prediction->predict( target->index, m_e_range, 1800.f, 60.f, 0.25f );
                if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->akali.e_hitchance->get< int >( )
                    || g_features->prediction->minion_in_line( g_local->position, pred.position, 70.f ) )
                    return false;

                if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                    m_last_e_time = *g_time;
                    return true;
                }

                return false;
            }

            Object* target{ };
            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) ) continue;

                const auto buff = g_features->buff_cache->get_buff( enemy->index, ct_hash( "AkaliEMis" ) );
                if ( !buff ) continue;

                target = enemy;
                break;
            }

            if ( !target ) return false;

            auto allow_e{ g_config->akali.e_in_full_combo->get< bool >( ) && GetAsyncKeyState( VK_CONTROL ) };

            if ( !allow_e && g_config->akali.e_on_killable->get< bool >( ) ) {
                auto damage = get_spell_damage( ESpellSlot::e, target );
                if ( m_slot_q->is_ready( true ) ) damage += get_spell_damage( ESpellSlot::q, target );

                if ( damage + helper::get_aa_damage( target->index, true ) < target->health + target->
                    total_health_regen )
                    return false;

                allow_e = true;
            }

            if ( !allow_e ) return false;

            if ( cast_spell( ESpellSlot::e ) ) {
                m_last_e_time = *g_time;
                return true;
            }


            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->akali.r_enabled->get< bool >( ) || *g_time - m_last_r_time <= 0.4f || !m_slot_r->
                is_ready( ) )
                return false;

            if ( rt_hash( m_slot_r->get_name().c_str() ) == ct_hash( "AkaliR" ) ) {
                const auto target = g_features->target_selector->get_default_target( );
                if ( !target || target->dist_to_local( ) > m_r_range ) return false;

                const auto allow_r{ g_config->akali.r_in_full_combo->get< bool >( ) && GetAsyncKeyState( VK_CONTROL ) };

                if ( !allow_r ) return false;

                if ( cast_spell( ESpellSlot::r, target->network_id ) ) m_last_r_time = *g_time;

                return false;
            }

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) > m_r_range ) return false;

            bool allow_r{ };
            bool is_execute{ };

            if ( g_config->akali.r_killsteal->get< bool >( ) ) {
                const auto damage = get_spell_damage( ESpellSlot::r, target );
                if ( damage > target->health ) {
                    is_execute = true;
                    allow_r    = true;
                }
            }

            if ( !allow_r ) return false;

            auto pred = g_features->prediction->predict( target->index, 775.f, 3000.f, 60.f, 0.f );
            if ( !pred.valid || ( int )pred.hitchance < g_config->akali.r_hitchance->get< int >( ) && !
                is_execute )
                return false;

            if ( cast_spell( ESpellSlot::r, pred.position ) ) {
                m_last_r_time = *g_time;
                return true;
            }

            return false;
        }

        auto antigapclose_w( ) -> void{
            if ( !g_config->akali.w_antigapcloser->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w->
                is_ready( ) )
                return;

            const auto target = get_antigapclose_target( 350.f );
            if ( !target ) return;


            if ( cast_spell( ESpellSlot::w, g_local->position ) ) m_last_w_time = *g_time;
        }

        auto autocancel_w( ) -> void{
            if ( !g_config->akali.w_cancel_casts->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w->
                is_ready( ) )
                return;

            bool can_cancel{ };

            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || g_features->target_selector->is_bad_target( enemy->index ) ) continue;

                auto sci = enemy->spell_book.get_spell_cast_info( );
                if ( !sci || sci->is_autoattack || sci->get_target_index( ) != g_local->index || sci->server_cast_time <
                    *g_time + g_features->orbwalker->get_ping( ) )
                    continue;

                switch ( rt_hash( enemy->champion_name.text ) ) {
                case ct_hash( "Mordekaiser" ):
                case ct_hash( "LeeSin" ):
                case ct_hash( "Garen" ):
                    if ( sci->slot == 3 ) can_cancel = true;
                    break;
                case ct_hash( "Singed" ):
                    if ( sci->slot == 2 ) can_cancel = true;
                    break;
                default:
                    break;
                }

                if ( can_cancel ) break;
            }


            if ( !can_cancel ) return;

            const auto cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;

            if ( cast_spell( ESpellSlot::w, cursor ) ) {
                m_last_w_time = *g_time;
                return;
            }
        }

        auto passive_magnet( ) -> void{
            if ( !m_ring_found || m_passive_active || !g_config->akali.magnet_orbwalk->get< bool >( ) ||
                g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::flee ||
                g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::none ||
                g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::recalling ) {
                if ( g_features->orbwalker->is_movement_disabled( ) ) g_features->orbwalker->allow_fast_move( );

                g_features->orbwalker->allow_movement( true );
                g_features->orbwalker->allow_attacks( true );
                m_magnet_active = false;

                return;
            }

            Vec3       goal_point{ };
            auto       lowest_distance{ 9999.f };
            bool       found_point{ };
            const auto local_under_turret = is_position_in_turret_range( g_local->position );

            m_magnet_points.clear( );
            const auto candidates = get_circle_segment_points( m_ring_position, 500.f, 60 );

            for ( auto& point : candidates.points ) {
                if ( point.dist_to( g_local->position ) > g_config->akali.magnet_distance->get< int >( ) + (
                        m_magnet_active ? 30.f : 0.f ) ||
                    point.dist_to( g_local->position ) > lowest_distance ) {
                    m_magnet_points.push_back( point );
                    continue;
                }

                auto walk_point{ point };

                if ( point.dist_to( g_local->position ) <= 80.f ) {
                    const auto point_dist   = point.dist_to( g_local->position );
                    const auto extend_value = 120.f - point_dist;

                    walk_point = m_ring_position.extend( point, 500.f + extend_value );
                }

                if ( g_navgrid->is_wall( walk_point ) || !local_under_turret && is_position_in_turret_range(
                    walk_point
                ) ) {
                    m_magnet_points.push_back( walk_point );
                    continue;
                }

                goal_point      = walk_point;
                lowest_distance = point.dist_to( g_local->position );
                found_point     = true;

                m_magnet_points.push_back( walk_point );
            }

            if ( !found_point ) {
                // enable orb movement when not magneting
                if ( g_features->orbwalker->is_movement_disabled( ) ) g_features->orbwalker->allow_fast_move( );

                g_features->orbwalker->allow_movement( true );
                g_features->orbwalker->allow_attacks( true );

                m_magnet_active = false;

                return;
            }

            if ( g_local->position.dist_to( m_ring_position ) > 500.f ) {
                // std::cout << "early magnet end\n";
                g_features->orbwalker->allow_fast_move( );
                g_features->orbwalker->allow_movement( true );
                g_features->orbwalker->allow_attacks( true );

                m_magnet_active = false;
                m_ignored_rings.push_back( m_ring_nid );
                reset_ring( );
                return;
            }

            if ( !g_features->orbwalker->is_movement_disabled( ) ) {
                g_features->orbwalker->allow_movement( false );
                g_features->orbwalker->allow_attacks( false );
            }

            if ( *g_time - m_last_move_time > 0.05f && !g_features->orbwalker->in_attack( ) ) {
                auto aimgr = g_local->get_ai_manager( );
                if ( !aimgr ) return;

                const auto path     = aimgr->get_path( );
                const auto path_end = path[ path.size( ) - 1 ];

                if ( path_end.dist_to( goal_point ) < 5.f ) return;

                if ( g_features->orbwalker->send_move_input( goal_point, true ) ) {
                    m_last_move_time = *g_time;
                    m_magnet_active  = true;
                }
            }
        }

        auto cast_tracker( ) -> void{
            if ( !g_config->akali.r_animation_cancel->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->
                is_ready( true ) )
                return;

            if ( !m_active ) {
                auto sci = g_local->spell_book.get_spell_cast_info( );
                if ( !sci || sci->slot != 3 ) return;

                auto info = sci->get_spell_info( );
                if ( !info ) return;

                auto data = info->get_spell_data( );
                if ( !data ) return;

                if ( rt_hash( data->get_name().c_str() ) != ct_hash( "AkaliR" ) ) return;

                // std::cout << "Cast detected: " << data->get_name() << std::endl;

                m_active       = true;
                m_cast_time    = sci->server_cast_time;
                m_target_index = sci->get_target_index( );

                auto& target = g_entity_list.get_by_index( m_target_index );
                if ( !target ) {
                    m_active = false;
                    return;
                }

                target.update( );

                m_target_position = target->position;
                m_local_position  = g_local->position;

                return;
            }

            const auto& target = g_entity_list.get_by_index( m_target_index );
            if ( !target || target->is_dead( ) || target->is_invisible( ) ) {
                // std::cout << "bad target, stop cancel\n";
                m_active = false;
                return;
            }

            float time_to_cast{ };
            auto  sci = g_local->spell_book.get_spell_cast_info( );
            if ( sci ) time_to_cast = sci->server_cast_time - *g_time;
            else {
                //std::cout << "sci null, restart\n";
                m_active = false;
                return;
            }

            //std::cout << "cast time left: " << time_to_cast << std::endl;

            const auto pred       = g_features->prediction->predict_default( g_local->index, 0.25f + time_to_cast );
            const auto cast_pred  = g_features->prediction->predict_default( g_local->index, 0.18f + time_to_cast );
            const auto start_pred = g_features->prediction->predict_default(
                g_local->index,
                time_to_cast + g_features->orbwalker->get_ping( )
            );
            if ( !pred || !cast_pred || !start_pred ) return;

            const auto target_pred = g_features->prediction->predict(
                target->index,
                m_e_range,
                1800.f,
                0.f,
                0.25f + time_to_cast,
                pred.value( )
            );
            if ( !target_pred.valid ) {
                //std::cout << "bad target pred\n";
                return;
            }

            if ( g_features->prediction->minion_in_line( pred.value( ), target_pred.position, 60.f ) ) return;

            m_target_pred     = target_pred.position;
            m_local_pred      = pred.value( );
            m_local_cast_pred = cast_pred.value( );
            m_start_pred      = start_pred.value( );

            const auto diff          = m_target_pred - cast_pred.value( );
            auto       cast_position = start_pred.value( ) + diff;

            if ( time_to_cast > 0.045f ) return;

            if ( m_local_position.dist_to( cast_pred.value( ) ) <= m_target_position.dist_to( m_local_position ) ) {
                cast_position = m_target_pred;
                //std::cout << "\n[ Akali ]: Alternative prediction used\n";
            }

            if ( cast_spell( ESpellSlot::e, cast_position ) ) {
                m_last_e_time   = *g_time;
                m_cast_position = cast_position;
                m_active        = false;

                //std::cout << "casted e with " << time_to_cast << "s left\n";
            }
        }

        static auto get_circle_segment_points(
            const Vec3  center,
            const float radius,
            const int   segments
        ) -> sdk::math::Polygon{
            const auto         angle_step = D3DX_PI * 2.0f / segments;
            sdk::math::Polygon poly{ };

            //if ( safe_distance ) radius += g_config->evade.extra_distance->get< int >( ) + m_safe_distance * 0.3f;

            for ( float angle = 0; angle < ( D3DX_PI * 2.0f ); angle += angle_step ) {
                // D3DXVECTOR3 v_start( radius * cosf( angle ) + center.x, radius * sinf( angle ) + center.z, center.y );
                D3DXVECTOR3 v_end(
                    radius * cosf( angle + angle_step ) + center.x,
                    radius * sinf( angle + angle_step ) + center.z,
                    center.y
                );

                const auto temp_z = v_end.z;
                v_end.z           = v_end.y;
                v_end.y           = temp_z;

                poly.points.push_back( { v_end.x, v_end.y, v_end.z } );
            }

            return poly;
        }

        auto is_ignored_ring( const unsigned network_id ) const -> bool{
            for ( const auto& inst : m_ignored_rings ) if ( inst == network_id ) return true;

            return false;
        }

        auto reset_ring( ) -> void{
            m_ring_found      = false;
            m_ring_index      = 0;
            m_ring_nid        = 0;
            m_ring_position   = Vec3( 0, 0, 0 );
            m_ring_start_time = 0.f;
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->attack_damage( ) * 0.65f + g_local->ability_power( ) *
                    0.6f,
                    target->index,
                    false
                );
            case ESpellSlot::e:
                return helper::calculate_damage(
                    m_e2_damage[ get_slot_e( )->level ] + g_local->attack_damage( ) * 0.595f + g_local->ability_power( )
                    * 0.84f,
                    target->index,
                    false
                );
            case ESpellSlot::r:
            {
                const auto missing_health  = 1.f - target->health / target->max_health;
                const auto damage_modifier = missing_health >= 0.7f ? 2.f : missing_health / 0.01f * 0.0286f;
                auto       base_damage     = m_r2_damage[ get_slot_r( )->level ] + g_local->ability_power( ) * 0.3f;

                base_damage *= 1.f + damage_modifier;

                return helper::calculate_damage( base_damage, target->index, false );
            }
            default:
                break;
            }

            return 0.f;
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
            {
                const auto tt   = 0.25f + g_local->position.dist_to( target->position ) / 1600.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.25f + g_local->position.dist_to( pred.value( ) ) / 1600.f;
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

        // passive magnet
        float m_last_move_time{ };
        bool  m_magnet_active{ };

        // passive tracking
        bool     m_ring_found{ };
        int16_t  m_ring_index{ };
        unsigned m_ring_nid{ };
        Vec3     m_ring_position{ };
        float    m_ring_start_time{ };

        // animation cancel tech
        bool    m_active{ };
        float   m_cast_time{ };
        int16_t m_target_index{ };

        Vec3 m_target_pred{ };
        Vec3 m_local_pred{ };
        Vec3 m_local_cast_pred{ };
        Vec3 m_start_pred{ };

        Vec3 m_cast_position{ };

        Vec3 m_target_position{ };
        Vec3 m_local_position{ };


        bool m_passive_active{ };

        std::vector< unsigned > m_ignored_rings{ };
        std::vector< Vec3 >     m_magnet_points{ };

        std::vector< float > m_q_damage = { 0.f, 30.f, 55.f, 80.f, 105.f, 130.f };
        std::vector< float > m_e_damage = { 0.f, 80.f, 105.f, 130.f, 155.f, 180.f };
        std::vector< float > m_r_damage = { 0.f, 80.f, 100.f, 120.f };

        std::vector< float > m_e2_damage = { 0.f, 70.f, 131.25f, 192.5f, 253.75f, 315.f };
        std::vector< float > m_r2_damage = { 0.f, 60.f, 130.f, 200.f };

        float m_q_range{ 500.f };
        float m_w_range{ 0.f };
        float m_e_range{ 825.f };
        float m_r_range{ 675.f };
    };
}
