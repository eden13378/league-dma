#pragma once
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"
#include "module.hpp"

namespace features::champion_modules {
    class anivia_module final : public IModule {
    public:
        virtual ~anivia_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "anivia_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Anivia" ); }

        auto initialize( ) -> void override{ m_priority_list = { r_spell, w_spell, q_spell, e_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "anivia" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );
            const auto spellclear = navigation->add_section( _( "spellclear" ) );

            q_settings->checkbox( _( "enable" ), g_config->anivia.q_enabled );
            q_settings->checkbox( _( "q harass" ), g_config->anivia.q_harass );
            q_settings->select(
                _( "hitchance" ),
                g_config->anivia.q_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            w_settings->checkbox( _( "enable" ), g_config->anivia.w_enabled );
            w_settings->checkbox( _( "auto interrupt (?)" ), g_config->anivia.w_autointerrupt )->set_tooltip(
                _( "Automatically interrupts spells like Katarina R, Malzahar R, etc" )
            );
            w_settings->checkbox( _( "antigapclose" ), g_config->anivia.w_antigapclose );

            e_settings->checkbox( _( "enable" ), g_config->syndra.e_enabled );
            e_settings->checkbox( _( "only if frosted" ), g_config->anivia.e_only_if_frosted );

            r_settings->checkbox( _( "enable" ), g_config->anivia.r_enabled );
            r_settings->slider_int( _( "distance to close cast r" ), g_config->anivia.r_recast, 400, 800, 10 );
            spellclear->checkbox( _( "laneclear r" ), g_config->anivia.r_laneclear );

            spellclear->slider_int( _( "min mana %" ), g_config->anivia.r_laneclear_min_mana, 1, 70, 5 );

            drawings->checkbox( _( "draw q range" ), g_config->anivia.q_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->anivia.e_draw_range );
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->anivia.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->anivia.dont_draw_on_cooldown->get
                    < bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 31, 88, 255, 255 ),
                        m_q_range,
                        Renderer::outline,
                        64,
                        3.f
                    );
                }
            }

            if ( g_config->anivia.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->anivia.dont_draw_on_cooldown->get
                    < bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 144, 66, 245, 255 ),
                        m_e_range + 65.f,
                        Renderer::outline,
                        64,
                        3.f
                    );
                }
            }

            /*std::vector< Vec3 > minion_locations;
            for (const auto minion : g_entity_list->get_enemy_minions(  ) ) {
                if ( !minion || !minion->is_alive( ) || minion->dist_to_local( ) > 1200.f ||
                     minion->is_jungle_monster( ) )
                    continue;

                minion_locations.push_back( minion->position );
            }
            if ( minion_locations.empty(  ) ) return;
             g_render->circle_3d( most_central_point( minion_locations ), color( 255, 0, 0, 255 ), 90.f, c_renderer::outline, 64, 3.f );*/

            /*
            auto cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;
            for( auto block : simulate_iceblocks( cursor )) {
                g_render->circle_3d( block,
                                     color( 50, 140, 255, 50 ),
                                     115.f,
                                     c_renderer::outline | c_renderer::filled,
                                     32,
                                     1.f );


                auto points = g_render->get_3d_circle_points( block, 115.f, 16 );


                for(auto point : points) {

                    if ( g_navgrid->is_wall( point )) {
                        g_render->circle_3d(
                            point, color( 255, 25, 25, 50 ), 5.f, c_renderer::outline | c_renderer::filled, 16, 1.f );   
                    }
                }
            }
            //m_nearest_escape

             g_render->circle_3d( m_push_point, color( 255, 25, 25, 50 ), 20.f, c_renderer::outline | c_renderer::filled, 32, 2.f );
            g_render->circle_3d(m_cast_point, color( 25, 255, 25, 50 ), 20.f, c_renderer::outline | c_renderer::filled, 32, 2.f );


            for ( auto minion : g_entity_list->get_ally_minions(  )) {

                if ( minion->dist_to_local( ) > 1000.f || minion->is_dead(  ) || rt_hash( minion->name.text ) != ct_hash( "IceBlock" ) )
                    continue;

                 g_render->circle_3d( minion->position, color( 255, 255, 255, 50 ), 80.f, c_renderer::outline | c_renderer::filled, 32, 2.f );

            }

            for ( auto point : m_wall_points ) {
                g_render->circle_3d( point,
                                                                                color( 255, 25, 25 ),
                                     5.f,
                                     c_renderer::outline,
                                     16,
                                     2.f );
            }

            for ( auto point : m_push_candidates ) {
                g_render->circle_3d( point, color( 255, 255, 25 ),
                                     10.f,
                                     c_renderer::outline,
                                     16,
                                     2.f );
            }


            if ( m_r_active && m_r_position_updated ) {
                g_render->circle_3d( m_r_position, color( 255, 255, 25 ), 400.f, c_renderer::outline, 72, 3.f );
            }*/
        }

        auto run( ) -> void override{
            initialize_spell_slots( );


            m_w_wall_size = 300.f + 100.f * m_slot_w->level;
            m_r_active    = !!g_features->buff_cache->get_buff( g_local->index, ct_hash( "GlacialStorm" ) );

            track_q_missile( );
            update_cast( );
            r_recast_combo( );

            spell_q_recast( );

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return;

            antigapclose_w( );
            autointerrupt_w( );


            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                wallstuck_w( );
                block_escape_w( );
            //spell_w( );
                spell_r( );
                spell_e( );
                spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_features->orbwalker->in_attack( ) ) return;

                if ( g_config->anivia.q_harass->get< bool >( ) ) spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::laneclear:
                if ( g_features->orbwalker->in_attack( ) ) return;

                if ( g_config->anivia.r_laneclear->get< bool >( ) && ( g_local->mana / g_local->max_mana ) * 100 >
                    g_config->anivia.r_laneclear_min_mana->get< int >( ) ) {
                    laneclear_r( );
                    spell_hold_r_laneclear( );
                }

                break;
            default:
                break;
            }
        }

    private:
        //FlashFrostSpell
        //this is how to find name of the missile
        //FlashFrost
        auto spell_q( ) -> bool override{
            //debug_log( "{}", m_slot_q );
            if ( !g_config->anivia.q_enabled->get< bool >( ) ||
                *g_time - m_last_q_time <= 0.4f ||
                *g_time - m_last_cast_time < 0.1f || m_q_active ||
                !m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_q_range,
                950.f,
                220.f,
                0.25f,
                { },
                true
            );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->anivia.q_hitchance->
                get< int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->anivia.w_enabled->get< bool >( ) ||
                *g_time - m_last_w_time <= 0.4f ||
                *g_time - m_last_cast_time < 0.1f || m_q_active ||
                !m_slot_w->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;
            const auto wall_position = g_local->position.extend( target->position, target->dist_to_local( ) + 10.f );
            if ( cast_spell( ESpellSlot::w, wall_position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto block_escape_w( ) -> bool{
            if ( !g_config->anivia.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f ||
                *g_time - m_last_cast_time < 0.1f || !m_r_active || !m_slot_w->is_ready( true ) )
                return false;

            std::vector< Object* > enemy_list{ };
            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || !enemy->is_alive( ) || enemy->position.dist_to( m_r_position ) > 400.f ) continue;

                enemy_list.push_back( enemy );
            }

            Vec3 cast_position{ };
            bool can_block{ };

            for ( const auto enemy : enemy_list ) {
                auto pred = g_features->prediction->predict_default(
                    enemy->index,
                    0.4f + 100.f / enemy->movement_speed
                );
                if ( !pred || pred.value( ).dist_to( m_r_position ) > 400.f || pred.value( ).dist_to(
                    g_local->position
                ) > m_w_range )
                    continue;


                auto base_position = pred.value( );
                auto delta = ( base_position - g_local->position ).normalize( );
                auto start_point = base_position.extend( base_position + delta.rotated( 1.58f ), m_w_wall_size / 2.f );
                auto end_point = start_point.extend( base_position, m_w_wall_size );

                const auto poly = simulate_wall( pred.value( ) );
                auto wall_points = g_features->evade->get_line_segment_points( start_point, end_point, 110.f, true );
                m_wall_points = wall_points.points;

                auto lowest_distance{ FLT_MAX };
                Vec3 nearest{ };

                for ( auto point : wall_points.points ) {
                    if ( g_navgrid->is_wall( point ) || helper::is_wall_in_line( enemy->position, point )
                        || point.dist_to( m_r_position ) <= 400.f ||
                        lowest_distance < 1000.f && is_hitbox_in_line( enemy->position, point, poly ) )
                        continue;

                    const auto distance = enemy->position.dist_to( point );
                    if ( distance > lowest_distance ) continue;

                    lowest_distance = distance;
                    nearest         = point;
                }

                m_push_point = nearest;

                if ( lowest_distance <= 280.f ) continue;

                can_block     = true;
                cast_position = pred.value( );
                break;
            }

            if ( !can_block ) return false;

            if ( cast_spell( ESpellSlot::w, cast_position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;

                return true;
            }

            return false;
        }

        auto wallstuck_w( ) -> bool{
            if ( !g_config->anivia.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f ||
                *g_time - m_last_cast_time < 0.1f || !m_slot_w->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_w_range, 0.f, 0.f, 0.25f );
            if ( !pred.valid ) return false;

            const auto candidates = g_render->get_3d_circle_points(
                pred.position,
                50.f,
                11,
                360,
                ( target->position - g_local->position ).rotated( 0.9f )
            );
            m_push_candidates = candidates;
            Vec3 cast_position{ };
            bool found_stuck{ };

            for ( auto point : candidates ) {
                if ( point.dist_to( g_local->position ) > m_w_range ) continue;

                auto base_position = point;
                auto delta = ( base_position - g_local->position ).normalize( );
                auto start_point = base_position.extend( base_position + delta.rotated( 1.58f ), m_w_wall_size / 2.f );
                auto end_point = start_point.extend( base_position, m_w_wall_size );

                auto wall_poly = g_features->evade->get_line_segment_points( start_point, end_point, 75.f, true );
                auto can_stuck{ true };

                auto push_position = get_push_position( pred.position, simulate_wall( point ) );

                /* auto iceblocks = simulate_iceblocks( point );
  
                  std::vector< Vec3 > block_wall_points{ };
  
                  for ( auto block : iceblocks ) {
  
                       auto points = g_render->get_3d_circle_points( block, 100.f, 32 );
                       float nearest_wall{ 0.f };
                       Vec3  nearest_wall_point{ };
  
                      for ( auto block_point : points ) {
  
                          if ( g_navgrid->is_wall( block_point ) ) {
  
                              if ( block_point.dist_to( push_position ) > nearest_wall ) {
                                  nearest_wall       = block_point.dist_to( push_position );
                                  nearest_wall_point = block_point;
                                  break;
                              }
                          }
                      }
  
                      if ( nearest_wall == 0.f ) continue;
  
                      block_wall_points.push_back( nearest_wall_point );
                  }
  
                  if ( block_wall_points.size( ) < 2 ) continue;
  
                  can_stuck = true;
  
                  Vec3 start = block_wall_points[ 0 ];
                  Vec3 end   = block_wall_points[ 1 ];
  
                  auto closest = g_features->evade->get_closest_line_point( start, end, push_position );
                  auto sim_area    = sdk::math::Rectangle( start, end, 40.f ).to_polygon(  );
                  float min_range = start.dist_to( end ) * 0.1f;
  
                  if (sim_area.is_outside( push_position ) )
                      continue;*/


                for ( auto wall_point : wall_poly.points ) {
                    if ( g_navgrid->is_wall( wall_point ) || helper::is_wall_in_line( push_position, wall_point ) ||
                        is_hitbox_in_line( push_position, wall_point, wall_poly ) )
                        continue;

                    can_stuck = false;
                    break;
                }

                if ( !can_stuck ) continue;

                m_wall_points = wall_poly.points;
                m_push_point  = push_position;
                m_cast_point  = point;

                found_stuck   = true;
                cast_position = point;
                break;
            }

            if ( !found_stuck ) return false;

            if ( cast_spell( ESpellSlot::w, cast_position ) ) {
                std::cout << "[ WALLSTUCK W ] Casted against " << target->champion_name.text << std::endl;

                g_features->orbwalker->on_cast( );
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;

                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->anivia.e_enabled->get< bool >( ) ||
                *g_time - m_last_e_time <= 0.4f ||
                *g_time - m_last_cast_time < 0.1f || m_q_active ||
                !m_slot_e->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || !g_features->orbwalker->is_attackable( target->index, m_e_range, true ) ) return false;
            if ( !g_features->buff_cache->get_buff( target->index, ct_hash( "aniviachilled" ) ) ||
                !g_config->anivia.e_only_if_frosted->get< bool >( ) )
                return false;


            if ( cast_spell( ESpellSlot::e, target->network_id ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }


            return false;
        }

        //GlacialStorm

        auto spell_r( ) -> bool override{
            if ( !g_config->anivia.r_enabled->get< bool >( ) || *g_time - m_last_cast_time < 0.1f ||
                *g_time - m_last_r_time <= 0.5f || m_r_active || !m_slot_r->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict( target->index, 750.f, 2000.f, 100.f, 0.f, { }, true );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( 2 ) ) return false;

            if ( cast_spell( ESpellSlot::r, pred.position ) ) {
                //g_features->orbwalker->on_cast( );
                m_last_r_time        = *g_time;
                m_last_cast_time     = *g_time;
                m_r_position         = pred.position;
                m_r_position_updated = false;
                return true;
            }

            return false;
        }

        auto r_recast_combo( ) -> bool{
            if ( !m_r_active ) return false;

            auto hitcount = 0;

            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || !enemy->is_alive( ) || enemy->position.dist_to( m_r_position ) > g_config->anivia.
                    r_recast->get< int >( ) )
                    continue;

                ++hitcount;
            }

            if ( hitcount > 0 ) {
                if ( !m_was_enemies_inside_r ) m_was_enemies_inside_r = true;
                return false;
            }

            if ( !m_was_enemies_inside_r ) return false;

            if ( cast_spell( ESpellSlot::r ) ) {
                m_last_r_time    = *g_time;
                m_last_cast_time = *g_time;
                m_r_position     = { };
                return true;
            }

            return false;
        }

        static auto is_hitbox_in_line( const Vec3& start, const Vec3& end, sdk::math::Polygon hitbox ) -> bool{
            for ( auto i = 1; i < 10; i++ ) {
                auto simulated = start.extend( end, start.dist_to( end ) / 10.f * i );
                if ( hitbox.is_inside( simulated ) ) return true;
            }

            return false;
        }

        auto simulate_wall( const Vec3& target_position ) const -> sdk::math::Polygon{
            const auto base_position = target_position;
            const auto delta         = ( base_position - g_local->position ).normalize( );
            const auto start_point   = base_position.extend(
                base_position + delta.rotated( 1.58f ),
                m_w_wall_size / 2.f
            );
            const auto end_point = start_point.extend( target_position, m_w_wall_size );

            const auto rect = sdk::math::Rectangle( start_point, end_point, 100.f );
            return rect.to_polygon( );
        }

        static auto simulate_iceblocks( const Vec3& target_position ) -> std::vector< Vec3 >{
            const auto base_position = target_position;
            const auto delta         = ( base_position - g_local->position ).normalize( );
            auto       start_point   = base_position.extend( base_position + delta.rotated( 1.58f ), 80.f );
            auto       end_point     = start_point.extend( target_position, 160.f );

            return { start_point, end_point };
        }

        static auto get_push_position( const Vec3& current_position, const sdk::math::Polygon& wall_polygon ) -> Vec3{
            bool should_break{ };
            auto lowest_distance{ FLT_MAX };
            Vec3 nearest_position{ };

            for ( auto i = 1; i <= 20; i++ ) {
                auto points = g_render->get_3d_circle_points( current_position, 10.f * static_cast< float >( i ), 16 );

                for ( auto point : points ) {
                    if ( wall_polygon.is_outside( point ) && point.dist_to( current_position ) < lowest_distance ) {
                        should_break     = true;
                        nearest_position = point;
                        lowest_distance  = point.dist_to( current_position );
                    }
                }

                if ( should_break ) break;
            }

            if ( nearest_position.length( ) > 0.f ) return nearest_position;

            return current_position;
        }

        auto laneclear_r( ) -> bool{
            if ( !g_config->anivia.r_enabled->get< bool >( ) || *g_time - m_last_cast_time < 0.1f ||
                *g_time - m_last_r_time <= 0.5f || m_r_active || !m_slot_r->is_ready( true ) )
                return false;

            std::vector< Vec3 > minion_locations;
            for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
                if ( !minion || !minion->is_alive( ) || minion->dist_to_local( ) > 1000.f ) continue;

                minion_locations.push_back( minion->position );
            }

            if ( minion_locations.empty( ) ) return false;

            if ( cast_spell( ESpellSlot::r, most_central_point( minion_locations ) ) ) {
                m_last_r_time        = *g_time;
                m_last_cast_time     = *g_time;
                m_r_position         = most_central_point( minion_locations );
                m_r_position_updated = false;
                return true;
            }
            return false;
        }

        auto spell_hold_r_laneclear( ) -> bool{
            if ( !m_r_active ) return false;

            auto hitcount = 0;

            for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
                if ( !minion || !minion->is_alive( ) || minion->position.dist_to( m_r_position ) > 400.f ) continue;

                ++hitcount;
            }

            if ( hitcount > 1 ) {
                if ( !m_was_enemies_inside_r ) m_was_enemies_inside_r = true;
                return false;
            }

            if ( !m_was_enemies_inside_r ) return false;

            if ( cast_spell( ESpellSlot::r ) ) {
                m_last_r_time    = *g_time;
                m_last_cast_time = *g_time;
                m_r_position     = { };
                return true;
            }

            return false;
        }

        auto spell_q_recast( ) -> bool{
            const auto missile_pos = get_q_position( );
            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || g_features->target_selector->is_bad_target( enemy->index ) || !enemy->is_alive( ) || !
                    m_q_active || enemy->position.dist_to( missile_pos ) > 215.f )
                    continue;


                if ( cast_spell( ESpellSlot::q ) ) {
                    //g_features->orbwalker->on_cast( );
                    m_last_q_time    = *g_time;
                    m_last_cast_time = *g_time;

                    //std::cout << "[Anivia: Q] Found enemy in range: " << enemy->position.dist_to( missile_pos ) << std::endl;
                    return true;
                }
            }
            return false;
        }

        auto antigapclose_w( ) -> void{
            if ( !g_config->anivia.w_antigapclose->get< bool >( ) || *g_time - m_last_w_time <= 0.4f ||
                !m_slot_w->is_ready( true ) )
                return;

            const auto target = get_antigapclose_target( );
            if ( !target ) return;

            const auto pred =
                g_features->prediction->predict(
                    target->index,
                    1000.f,
                    2000.f,
                    m_w_width[ get_slot_w( )->level ] / 2,
                    0.25f
                );
            if ( !pred.valid || ( int )pred.hitchance < 3 ) return;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) m_last_w_time = *g_time;
        }

        auto autointerrupt_w( ) -> void{
            if ( !g_config->anivia.w_autointerrupt->get< bool >( ) || *g_time - m_last_w_time <= 0.4f ||
                !m_slot_w->is_ready( true ) )
                return;

            const auto target = get_interruptable_target( m_w_range );
            if ( !target ) return;

            const auto pred = g_features->prediction->predict(
                target->index,
                1000.f,
                2000.f,
                m_w_width[ get_slot_w( )->level ] / 2,
                0.25f
            );
            if ( !pred.valid || ( int )pred.hitchance < 3 ) return;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) m_last_w_time = *g_time;
        }

        auto update_cast( ) -> void{
            if ( m_r_active ) {
                if ( !m_r_position_updated ) {
                    for ( const auto particle : g_entity_list.get_enemy_uncategorized( ) ) {
                        if ( !particle || particle->dist_to_local( ) > 1000.f ) continue;

                        auto name = particle->get_alternative_name( );
                        if ( name.find( "Anivia" ) == std::string::npos ||
                            name.find( "R_indicator_ring" ) == std::string::npos )
                            continue;

                        m_r_position         = particle->position;
                        m_r_position_updated = true;
                        break;
                    }
                }
            } else {
                m_r_position_updated   = false;
                m_was_enemies_inside_r = false;
            }
        }

        auto track_q_missile( ) -> void{
            if ( !m_q_active ) {
                auto sci = g_local->spell_book.get_spell_cast_info( );
                if ( !sci || rt_hash( sci->get_spell_name( ).data( ) ) == ct_hash( "FlashFrostSpell" ) || sci->
                    server_cast_time < *g_time )
                    return;

                m_q_active         = true;
                m_server_cast_time = sci->server_cast_time;
                m_start_position   = sci->start_position;
                m_end_position     = sci->start_position.extend( sci->end_position, 1155.f );
                m_spell_detected   = false;
                m_did_print        = false;

                m_calculated_split = false;

                if ( m_q_target_index == 0 ) {
                    const auto target = g_features->target_selector->get_default_target( );
                    if ( !target ) return;

                    m_q_target_index = target->index;
                }

                return;
            }

            if ( *g_time - m_server_cast_time > 1.f ) {
                m_q_active            = false;
                m_last_split_position = get_q_position( );
                m_last_tracking_time  = *g_time;
                m_q_target_index      = 0;

                return;
            }

            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "FlashFrost" ) ) ) {
                if ( !m_spell_detected ) m_spell_detected = true;

                m_last_split_position = get_q_position( );

                return;
            }

            if ( m_spell_detected ) {
                m_last_tracking_time = *g_time;
                m_q_active           = false;
                m_last_freeze_time   = *g_time;
                m_q_target_index     = 0;
            }
        }

        auto get_q_position( const bool next_tick = false, const bool include_ping = false ) -> Vec3{
            if ( !m_q_active ) return m_simulated_split;

            const auto time_traveled     = std::max( *g_time - m_server_cast_time, 0.f );
            auto       distance_traveled =
                time_traveled * 950.f + ( next_tick && *g_time > m_server_cast_time ? 55.f : 0.f );

            if ( include_ping && *g_time > m_server_cast_time ) {
                distance_traveled += g_features->orbwalker->get_ping( )
                    * 950.f;
            }

            if ( distance_traveled > 1155.f ) distance_traveled = 1155.f;

            if ( next_tick ) return m_start_position.extend( m_end_position, distance_traveled );

            m_simulated_split = m_start_position.extend( m_end_position, distance_traveled );
            if ( !m_slot_q->is_ready( ) ) m_simulated_split = { };

            return m_simulated_split;
        }

        auto stop_tracking_q( ) -> void{ m_q_active = false; }

        auto most_central_point( const std::vector< Vec3 >& Points ) -> Vec3{
            auto  min_X              = Points.front( ).x;
            auto  max_X              = Points.front( ).x;
            auto  min_Y              = Points.front( ).y;
            auto  max_Y              = Points.front( ).y;
            Vec3  center_point       = { };
            float min_total_distance = { };

            for ( const auto point : Points ) {
                min_X = std::min( min_X, point.x );
                max_X = std::max( max_X, point.x );
                min_Y = std::min( min_Y, point.y );
                max_Y = std::max( max_Y, point.y );
            }

            min_total_distance = total_distance( Points.front( ), Points );

            Vec3 check_point{ };
            check_point.x = Points.front( ).x;
            check_point.y = Points.front( ).y;
            check_point.z = Points.front( ).z;
            float total_distance_to_points;
            for ( auto x = min_X; x <= max_X; x += 1.0f ) {
                for ( auto y = min_Y; y <= max_Y; y += 1.0f ) {
                    check_point.x            = x;
                    check_point.y            = y;
                    total_distance_to_points = total_distance( check_point, Points );
                    if ( total_distance_to_points < min_total_distance ) {
                        center_point       = check_point;
                        min_total_distance = total_distance_to_points;
                    }
                }
            }

            return center_point;
        }

        static auto total_distance( const Vec3& pos, const std::vector< Vec3 >& Points ) -> float{
            float sum = { };

            for ( const auto& point : Points ) sum += pos.dist_to( point );

            return sum;
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->ability_power( ) * 0.25f,
                    target->index,
                    false
                );
            case ESpellSlot::r:
                return helper::calculate_damage(
                    300,
                    target->index,
                    false
                );
            }


            return 0.f;
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return 0.6f;
            }

            return IModule::get_spell_travel_time( slot, target );
        }

    private:
        std::vector< float > m_q_damage = { 0.f, 50.f, 70.f, 90.f, 110.f, 130.f };
        std::vector< float > m_w_width  = { 0.f, 600.f, 700.f, 9800.f, 900.f, 1000.f };

        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_r_time{ };
        float m_last_cast_time{ };

        // debug
        std::vector< Vec3 > m_push_candidates{ };
        std::vector< Vec3 > m_wall_points{ };
        Vec3                m_push_point{ };
        Vec3                m_cast_point{ };

        float m_q_range{ 1100.f };
        float m_w_range{ 1000.f };
        float m_e_range{ 600.f };
        float m_r_range{ 750.f };

        float m_w_wall_size{ 400.f };

        bool m_r_active{ };
        Vec3 m_r_position{ };
        bool m_r_position_updated{ };
        bool m_was_enemies_inside_r{ };

        // q tracking
        bool  m_q_active{ };
        float m_server_cast_time{ };

        Vec3 m_start_position{ };
        Vec3 m_end_position{ };

        Vec3 m_simulated_split{ };
        Vec3 m_simulated_hit{ };

        Vec3 m_predicted_point{ };
        int  m_good_hitchance{ };

        Vec3 m_nearest_raw_point{ };
        Vec3 m_nearest_raw_end_point{ };

        Vec3 m_split_end{ };
        Vec3 m_split_start{ };

        bool m_calculated_split{ };

        float m_previous_distance{ };
        bool  m_found_point{ };

        Vec3 m_cast_missile_position{ };

        bool m_did_print{ };

        std::vector< Vec3 > m_near_points{ };

        float m_last_tracking_time{ };
        Vec3  m_last_split_position{ };

        Vec3 m_last_cast_direction{ };
        Vec3 m_last_cast_position{ };

        bool m_spell_detected{ };

        int16_t m_q_target_index{ };

        bool m_last_freeze_time{ };
    };
}
