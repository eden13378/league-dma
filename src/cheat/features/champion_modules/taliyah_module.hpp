#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class taliyah_module final : public IModule {
    public:
        virtual ~taliyah_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "taliyah_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Taliyah" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "taliyah" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->taliyah.q_enabled );
            q_settings->checkbox( _( "use in harass" ), g_config->taliyah.q_harass );
            q_settings->select(
                _( "hitchance" ),
                g_config->taliyah.q_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            w_settings->checkbox( _( "enable" ), g_config->taliyah.w_enabled );
            w_settings->checkbox( _( "flee w" ), g_config->taliyah.w_flee );
            w_settings->checkbox( _( "auto interrupt (?)" ), g_config->taliyah.w_autointerrupt )->set_tooltip(
                _( "Automatically interrupts spells like Shen R, Fiddlesticks R, etc" )
            );
            w_settings->select(
                _( "hitchance" ),
                g_config->taliyah.w_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            e_settings->checkbox( _( "enable" ), g_config->taliyah.e_enabled );
            e_settings->checkbox( _( "antigapcloser" ), g_config->taliyah.e_antigapcloser );

            drawings->checkbox( _( "draw q range" ), g_config->taliyah.q_draw_range );
            drawings->checkbox( _( "draw w range" ), g_config->taliyah.w_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->taliyah.e_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->taliyah.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->taliyah.q_draw_range->get< bool >( ) &&
                !g_config->taliyah.w_draw_range->get< bool >( ) &&
                !g_config->taliyah.e_draw_range->get< bool >( ) &&
                !g_config->taliyah.q_magnet->get< bool >( ) ||
                g_local->is_dead( ) )
                return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->taliyah.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->taliyah.dont_draw_on_cooldown->
                    get< bool >( ) ) ) {
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

            if ( g_config->taliyah.w_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->taliyah.dont_draw_on_cooldown->
                    get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 132, 245, 66, 255 ),
                        m_w_range,
                        Renderer::outline,
                        50,
                        2.f
                    );
                }
            }

            if ( g_config->taliyah.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->taliyah.dont_draw_on_cooldown->
                    get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 144, 66, 245, 255 ),
                        m_e_range,
                        Renderer::outline,
                        50,
                        2.f
                    );
                }
            }

            Vec2 sp{ };
            if ( m_magnet_active && world_to_screen( g_local->position, sp ) ) {
                g_render->text_shadow(
                    { sp.x - 20.f, sp.y - 24.f },
                    Color( 252, 194, 3 ),
                    g_fonts->get_zabel( ),
                    "Magnet",
                    16
                );
            }

            return;

            if ( m_magnet_active ) {
                for ( auto point : m_magnet_points ) {
                    g_render->circle_3d(
                        point,
                        Color::white( ),
                        20.f,
                        2,
                        10,
                        2.f
                    );
                }
            }


            if ( m_q_active ) {
                auto extended   = g_local->position + m_q_direction;
                auto hitbox_end = g_local->position.extend( extended, 1000.f );

                auto rect = sdk::math::Rectangle( g_local->position, hitbox_end, 60.f );
                g_render->rectangle_3d( rect.r_start, rect.r_end, rect.width, Color( 255, 255, 255 ), 2, 2.f );
            }

            if ( m_cast_active ) {
                auto hitboxes = get_mine_hitboxes( m_cast_start, m_cast_direction, *g_time - m_cast_time );
                for ( auto hitbox : hitboxes ) {
                    g_render->rectangle_3d(
                        hitbox.r_start,
                        hitbox.r_end,
                        hitbox.width,
                        Color( 255, 255, 255 ),
                        2,
                        2.f
                    );
                }

                return;
                std::string text = "time: ";
                auto        time = std::to_string( *g_time - m_cast_time );
                time.resize( 4 );

                text += time;

                g_render->text_3d(
                    g_local->position,
                    Color( 25, 255, 25, 255 ),
                    g_fonts->get_zabel( ),
                    text.c_str( ),
                    16,
                    true
                );
            }

            return;

            auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return;

            //vec3 extended_point = target->position.extend(g_local->position, 100.f);

            auto points = g_render->get_3d_circle_points( target->position, 200.f, 16 );

            for ( auto i = 0; i < points.size( ); i++ ) {
                //vec3 direction = extended_point - target->position;

                g_render->circle_3d(
                    points[ i ],
                    Color::green( ).alpha( 70 ),
                    25.f,
                    Renderer::outline | Renderer::filled,
                    30,
                    2.f
                );
            }
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            //e_tracking();
            q_tracking( );
            w_tracking( );

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return;

            antigapclose_e( );
            autointerrupt_w( );
            magnet_q( );

            if ( g_features->orbwalker->in_attack( ) ) return;


            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_w( );
                spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->taliyah.q_harass->get< bool >( ) ) spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::flee:
                flee_w( );
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->taliyah.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.5f || *g_time -
                m_last_w_time <= 1.4f || !m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            Vec3 cast_position;

            if ( m_slot_q->get_usable_state( ) == 1 ) {
                auto pred = g_features->prediction->predict( target->index, 1000.f, 1950.f, 100.f, 0.25f );
                if ( !pred.valid || ( int )pred.hitchance < g_config->taliyah.q_hitchance->get< int >( )
                    || g_features->prediction->minion_in_line( g_local->position, pred.position, 100.f ) )
                    return false;

                cast_position = pred.position;
            } else {
                auto pred = g_features->prediction->predict( target->index, 1000.f, 3600.f, 40.f, 0.25f );
                if ( !pred.valid || ( int )pred.hitchance < g_config->taliyah.q_hitchance->get< int >( )
                    || g_features->prediction->minion_in_line( g_local->position, pred.position, 70.f ) )
                    return false;

                cast_position = pred.position;
            }

            if ( cast_spell( ESpellSlot::q, cast_position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                m_empowered_q = m_slot_q->get_usable_state( ) == 1;
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->taliyah.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 1.f || !m_slot_w->is_ready(
                    true
                ) ||
                !m_slot_e->is_ready( true ) || !g_config->taliyah.e_enabled->get< bool >( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto pred = g_features->prediction->predict( target->index, 900.f, 0.f, 200.f, 1.1f );
            if ( !pred.valid || ( int )pred.hitchance < g_config->taliyah.w_hitchance->get< int >( ) ) return false;

            const auto hit_pred = g_features->prediction->predict( target->index, 900.f, 0.f, 0.f, 1.1f );
            if ( !hit_pred.valid ) return false;

            auto cast_direction{ g_local->position };

            int        highest_hitcount{ };
            const auto points = g_render->get_3d_circle_points( hit_pred.position, 400.f, 28 );

            for ( auto position : points ) {
                if ( !is_valid_push( hit_pred.position, position ) ) continue;

                auto rects = get_mine_hitboxes( g_local->position, g_local->position.extend( position, 1000.f ), 1.0f );
                std::vector< sdk::math::Polygon > hitboxes{ };
                for ( auto rect : rects ) hitboxes.push_back( rect.to_polygon( ) );

                auto hitcount{ is_position_in_turret_range( position, true ) ? 2 : 0 };
                for ( auto hitbox : hitboxes ) {
                    bool found_hit{ };
                    for ( auto i = 0; i <= 5; i++ ) {
                        const auto time  = 0.2f * i;
                        auto       point = get_push_position( hit_pred.position, position, time );

                        if ( hitbox.is_inside( point ) ) {
                            found_hit = true;
                            break;
                        }
                    }

                    if ( found_hit ) ++hitcount;
                }

                if ( hitcount > highest_hitcount ) {
                    cast_direction   = pred.position + ( position - hit_pred.position );
                    highest_hitcount = hitcount;
                }
            }

            if ( highest_hitcount <= 2 ) return false;

            if ( cast_spell( ESpellSlot::w, pred.position, cast_direction ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;

                m_push_start     = pred.position;
                m_push_direction = cast_direction;
                m_should_combo   = true;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->taliyah.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || *g_time -
                m_last_cast_time <= 0.1f
                || *g_time - m_last_w_time >= 0.5f || *g_time - m_last_w_time <= 0.25f || !m_slot_e->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;


            if ( cast_spell( ESpellSlot::e, m_push_direction ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time = *g_time;
                debug_log( "[ TALIYAH ] Cast E {}s after W", *g_time - m_last_w_time );
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{ return false; }

        auto flee_w( ) -> bool{
            if ( !g_config->taliyah.w_flee->get< bool >( ) || *g_time - m_last_w_time <= 0.5f || !m_slot_w->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto pred = g_features->prediction->predict( target->index, 900.f, 0.f, 200.f, 1.1f );
            if ( !pred.valid || ( int )pred.hitchance < g_config->taliyah.w_hitchance->get< int >( ) ) return false;

            const auto cast_direction{ g_local->position.extend( pred.position, 1000.f ) };

            if ( cast_spell( ESpellSlot::w, pred.position, cast_direction ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto antigapclose_e( ) -> void{
            if ( !g_config->taliyah.e_antigapcloser->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->
                is_ready( true ) )
                return;

            const auto target = get_antigapclose_target( );
            if ( !target ) return;

            auto aimgr = target->get_ai_manager( );
            if ( !aimgr ) return;

            const auto path     = aimgr->get_path( );
            const auto path_end = path[ 1 ];

            if ( path.size( ) != 2 || g_features->prediction->windwall_in_line( g_local->position, path_end ) ) return;

            if ( cast_spell( ESpellSlot::e, path_end ) ) m_last_e_time = *g_time;
        }

        auto autointerrupt_w( ) -> void{
            if ( !g_config->taliyah.w_autointerrupt->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w->
                is_ready( true ) )
                return;

            const auto target = get_interruptable_target( m_w_range );
            if ( !target ) return;

            auto pred = g_features->prediction->predict( target->index, 900.f, 0.f, 200.f, 1.f );
            if ( !pred.valid || ( int )pred.hitchance <= 3 ) return;

            const auto cast_direction{ g_local->position };

            if ( cast_spell( ESpellSlot::w, pred.position, cast_direction ) ) m_last_e_time = *g_time;
        }

        auto e_tracking( ) -> void{
            if ( !m_cast_active ) {
                auto sci = g_local->spell_book.get_spell_cast_info( );
                if ( !sci || sci->slot != 2 ) return;

                m_cast_active    = true;
                m_cast_start     = sci->start_position;
                m_cast_direction = sci->start_position.extend( sci->end_position, 1000.f );
                m_cast_time      = sci->start_time;
            } else if ( *g_time - m_cast_time > 1.f ) m_cast_active = false;
        }

        auto w_tracking( ) -> void{
            if ( *g_time - m_last_e_time <= 0.25f ) return;

            if ( !m_w_cast_active ) {
                auto sci = g_local->spell_book.get_spell_cast_info( );
                if ( !sci || sci->slot != 1 ) return;

                m_w_cast_active      = true;
                m_w_server_cast_time = sci->server_cast_time;
            } else {
                if ( m_w_server_cast_time > *g_time + g_features->orbwalker->get_ping( ) ) return;

                animation_cancel_e( );

                if ( *g_time > m_w_server_cast_time + 0.05f ) {
                    m_should_combo  = false;
                    m_w_cast_active = false;
                }
            }
        }

        auto q_tracking( ) -> void{
            if ( !m_q_active ) {
                auto sci = g_local->spell_book.get_spell_cast_info( );
                if ( !sci || sci->is_autoattack || sci->slot != 0 ) return;

                m_q_active           = true;
                m_q_server_cast_time = sci->server_cast_time;
                m_q_direction        = sci->end_position - sci->start_position;

                if ( !m_empowered_q ) m_empowered_q = m_slot_q->get_usable_state( ) == 1;

                if ( !m_empowered_q ) g_features->orbwalker->allow_attacks( false );
            } else {
                if ( !m_empowered_q && !m_magnet_active && *g_time > m_q_server_cast_time + 1.f ) {
                    g_features->orbwalker
                              ->allow_attacks( true );
                }

                if ( *g_time > m_q_server_cast_time + 1.75f || *g_time > m_q_server_cast_time && m_empowered_q ) {
                    m_empowered_q = false;
                    m_q_active    = false;
                    g_features->orbwalker->allow_attacks( true );
                }
            }
        }

        auto magnet_q( ) -> void{
            if ( !g_config->taliyah.q_magnet->get< bool >( ) || !m_q_active || m_empowered_q ||
                g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::flee ||
                g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::none ||
                g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::recalling ) {
                if ( g_features->orbwalker->is_movement_disabled( ) ) g_features->orbwalker->allow_fast_move( );
                g_features->orbwalker->allow_movement( true );
                g_features->orbwalker->allow_attacks( true );

                m_magnet_active = false;
                return;
            }


            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return;

            const auto pred = g_features->prediction->predict_default( target->index, 0.3f );
            if ( !pred ) return;

            const auto target_position{ *pred };
            const auto extended   = target_position - m_q_direction;
            const auto hitbox_end = target_position.extend( extended, 1000.f );
            auto       rect       = sdk::math::Rectangle( *pred, hitbox_end, 40.f ).to_polygon( );

            auto candidates = g_features->evade->get_line_segment_points(
                target_position,
                hitbox_end,
                30.f,
                true
            ).points;
            for ( auto point : g_features->evade->get_line_segment_points( target_position, hitbox_end, 70.f, true ).
                                           points )
                candidates.push_back( point );

            m_magnet_points.clear( );
            const auto is_local_under_turret{ is_position_in_turret_range( ) };
            const auto max_distance    = 0.6f * g_local->movement_speed;
            const auto cursor          = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;
            const auto cursor_position = g_local->position.extend( cursor, max_distance );

            Vec3 goal_point{ };
            auto lowest_distance{ 999999.f };
            bool found_point{ };

            for ( auto point : candidates ) {
                auto walk_point{ point };

                if ( point.dist_to( g_local->position ) <= 80.f ) {
                    const auto point_dist   = point.dist_to( g_local->position );
                    const auto extend_value = 100.f - point_dist;

                    walk_point = target_position.extend( point, target_position.dist_to( point ) + extend_value );
                }

                if ( g_navgrid->is_wall( walk_point ) || g_local->position.dist_to( walk_point ) > max_distance
                    || !is_local_under_turret && is_position_in_turret_range( walk_point ) ) {
                    m_magnet_points.push_back( walk_point );
                    continue;
                }

                if ( walk_point.dist_to( cursor_position ) > lowest_distance ) {
                    m_magnet_points.push_back( walk_point );
                    continue;
                }

                goal_point      = walk_point;
                lowest_distance = point.dist_to( cursor_position );
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


            if ( *g_time - m_last_move_time >= 0.05f && !g_features->orbwalker->in_attack( ) ) {
                g_features->orbwalker->allow_movement( false );
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

        auto animation_cancel_e( ) -> void{
            if ( !g_config->taliyah.e_enabled->get< bool >( ) || *g_time - m_last_w_time > 0.75f || *g_time -
                m_last_e_time <= 0.4f || !m_slot_e->is_ready( true ) )
                return;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return;


            if ( cast_spell( ESpellSlot::e, m_push_direction ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time   = *g_time;
                m_w_cast_active = false;
                m_should_combo  = false;
                debug_log( "[ TALIYAH ] animcancel E, delay: {}", *g_time - m_last_w_time );
                return;
            }

            return;
        }

        static auto get_mine_hitboxes(
            const Vec3& start,
            const Vec3& direction,
            const float time
        ) -> std::vector< sdk::math::Rectangle >{
            if ( time <= 0.45f ) return { };

            std::vector< sdk::math::Rectangle > hitboxes{ };
            for ( auto i = 1; i <= 6; i++ ) {
                if ( i == 1 ) {
                    auto mine_start = start.extend( direction, 70.f );
                    auto mine_end   = mine_start.extend( direction, 110.f );

                    hitboxes.push_back( sdk::math::Rectangle( mine_start, mine_end, 110.f ) );
                    continue;
                }

                const auto spawn_time = 0.475f + 0.15f * ( i - 1 );
                if ( time < spawn_time ) break;

                auto mine_start = start.extend( direction, 210.f + 160.f * ( i - 2 ) );
                auto mine_end   = mine_start.extend( direction, i == 6 ? 140.f : 160.f );


                auto poly = sdk::math::Rectangle( mine_start, mine_end, 240.f ).to_polygon( );

                Vec3 start_direction{ };
                auto lowest_distance{ 99999.f };

                for ( auto point : poly.points ) {
                    if ( point.dist_to( mine_start ) < lowest_distance ) {
                        start_direction = point;
                        lowest_distance = point.dist_to( mine_start );
                    }
                }

                auto dir = mine_end - mine_start;

                for ( auto x = 0; x < 4; x++ ) {
                    auto hitbox_start = start_direction.extend( mine_start, 60.f + 120.f * x );
                    auto end_point    = hitbox_start + dir;
                    auto hitbox_end   = hitbox_start.extend( end_point, 160.f );

                    auto rect = sdk::math::Rectangle( hitbox_start, hitbox_end, 60.f );
                    hitboxes.push_back( rect );
                }
            }

            return hitboxes;
        }

        static auto is_valid_push( const Vec3& start, const Vec3& end ) -> bool{
            bool detected_wall{ };
            for ( auto i = 1; i <= 10; i++ ) {
                const auto extend_distance = start.dist_to( end ) / 10.f;

                auto temp = start.extend( end, extend_distance * i );
                if ( !detected_wall && g_navgrid->is_wall( temp ) ) {
                    detected_wall = true;
                    continue;
                }

                if ( !detected_wall ) continue;

                if ( !g_navgrid->is_wall( temp ) ) {
                    if ( i < 9 ) detected_wall = false;
                    else return false;
                }
            }

            return !detected_wall;
        }

        static auto get_push_position( const Vec3& start, const Vec3& end, float time ) -> Vec3{
            if ( time > 1.f ) time = 1.f;

            return start.extend( end, start.dist_to( end ) * time );
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->ability_power( ) * 0.7f,
                    target->index,
                    false
                );
            case ESpellSlot::e:
                return helper::calculate_damage(
                    m_e_damage[ get_slot_e( )->level ] + g_local->ability_power( ) * 0.8f,
                    target->index,
                    false
                );
            case ESpellSlot::r:
            {
                auto damage = m_r_damage[ m_slot_r->level ] + g_local->bonus_attack_damage( ) * 0.75f;

                const auto buff = g_features->buff_cache->get_buff( target->index, ct_hash( "DariusHemo" ) );
                if ( buff && buff->buff_data->end_time - *g_time >= 0.375f ) damage *= 1.f + 0.2f * buff->stacks( );

                return damage;
            }
            }

            return 0.f;
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::e:
                return 0.25f;
            }

            return std::numeric_limits< float >::max( );
        }

        auto is_mine_saved( const int16_t index ) const -> bool{
            for ( const auto idx : m_mines ) if ( idx == index ) return true;

            return false;
        }

    private:
        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_r_time{ };
        float m_last_cast_time{ };

        // EW logic
        int16_t m_target_index{ };
        Vec3    m_push_start{ };
        Vec3    m_push_direction{ };
        bool    m_should_combo{ };

        // e mines
        std::vector< int16_t > m_mines{ };

        // e cast track
        bool  m_cast_active{ };
        Vec3  m_cast_direction{ };
        Vec3  m_cast_start{ };
        float m_cast_time{ };

        // w cast track
        bool  m_w_cast_active{ };
        float m_w_server_cast_time{ };

        // q tracking
        bool  m_q_active{ };
        bool  m_empowered_q{ };
        float m_q_server_cast_time{ };
        Vec3  m_q_direction{ };

        // q magnet
        bool                m_magnet_active{ };
        float               m_last_move_time{ };
        std::vector< Vec3 > m_magnet_points{ };


        std::array< float, 6 > m_q_damage = { 0.f, 65.f, 95.f, 125.f, 155.f, 185.f };
        std::array< float, 6 > m_e_damage = { 0.f, 80.f, 105.f, 130.f, 155.f, 180.f };
        std::vector< float >   m_r_damage = { 0.f, 125.f, 250.f, 375.f };

        float m_q_range{ 1000.f };
        float m_w_range{ 900.f };
        float m_e_range{ 800.f };
        float m_r_range{ 2500.f };
    };
}
