#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class darius_module final : public IModule {
    public:
        virtual ~darius_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "darius_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Darius" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation     = g_window->push( _( "darius" ), menu_order::champion_module );
            const auto q_settings     = navigation->add_section( _( "q settings" ) );
            const auto drawings       = navigation->add_section( _( "drawings" ) );
            const auto w_settings     = navigation->add_section( _( "w settings" ) );
            const auto e_settings     = navigation->add_section( _( "e settings" ) );
            const auto r_settings     = navigation->add_section( _( "r settings" ) );
            const auto combo_settings = navigation->add_section( _( "combo settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->darius.q_enabled );
            q_settings->checkbox( _( "use in harass" ), g_config->darius.q_harass );
            q_settings->checkbox( _( "dont cast in aa range" ), g_config->darius.q_aa_range_limit );
            q_settings->checkbox( _( "enable magnet" ), g_config->darius.q_magnet );
            q_settings->checkbox( _( "magnet turret check" ), g_config->darius.q_magnet );

            w_settings->checkbox( _( "enable" ), g_config->darius.w_enabled );
            w_settings->checkbox( _( "only to reset aa" ), g_config->darius.w_aa_reset );

            e_settings->checkbox( _( "enable" ), g_config->darius.e_enabled );
            e_settings->checkbox( _( "dont cast in aa range" ), g_config->darius.e_aa_range_limit );
            e_settings->select(
                _( "hitchance" ),
                g_config->darius.e_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            r_settings->checkbox( _( "enable" ), g_config->darius.r_enabled );

            combo_settings->checkbox( _( "prefer e over q (?)" ), g_config->darius.prefer_e_over_q )->set_tooltip(
                _( "Only in full combo" )
            );

            drawings->checkbox( _( "draw q range" ), g_config->darius.q_draw_range );
            drawings->checkbox( _( "draw w range" ), g_config->darius.e_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->darius.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->darius.q_draw_range->get< bool >( ) &&
                !g_config->darius.e_draw_range->get< bool >( ) &&
                !g_config->darius.q_magnet->get< bool >( ) ||
                g_local->is_dead( ) )
                return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->darius.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->darius.dont_draw_on_cooldown->get
                    < bool >( ) ) ) {
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

            if ( g_config->darius.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->darius.dont_draw_on_cooldown->get
                    < bool >( ) ) ) {
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
                    g_fonts->get_bold( ),
                    "Magnet",
                    16
                );
            }

            // if (m_magnet_active)
            //   for (auto point : m_magnet_points) g_render->circle_3d(point, color::white(), 20.f, 2, 10, 2.f);
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return;

            magnet_q( );

            m_q_active = !!g_features->buff_cache->get_buff( g_local->index, ct_hash( "dariusqcast" ) );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_r( );
                spell_e( );
                spell_q( );
                spell_w( );
                break;

            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->darius.q_harass->get< bool >( ) ) spell_q( );
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->darius.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.5f || *g_time -
                m_last_e_time <= 0.5f || !m_slot_q->is_ready( true )
                || m_q_active
                || g_config->darius.prefer_e_over_q->get< bool >( ) && g_config->darius.e_enabled->get< bool >( )
                && g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo
                && GetAsyncKeyState( VK_CONTROL ) && m_slot_e->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) > 475.f ||
                g_config->darius.q_aa_range_limit->get< bool >( ) && g_features->orbwalker->is_attackable(
                    target->index
                ) )
                return false;

            const auto local_pred = g_features->prediction->predict_default( g_local->index, 0.1f );
            if ( !local_pred ) return false;

            const auto pred = g_features->prediction->predict_default( target->index, 0.1f );
            if ( !pred || g_local->position.dist_to( *pred ) > target->dist_to_local( )
                && local_pred.value( ).dist_to( *pred ) > target->dist_to_local( ) )
                return false;

            if ( cast_spell( ESpellSlot::q ) ) {
                m_last_q_time   = *g_time;
                m_magnet_target = target->index;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->darius.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || m_q_active || !
                m_slot_w->is_ready( true ) )
                return false;

            if ( g_config->darius.w_aa_reset->get< bool >( ) && !g_features->orbwalker->should_reset_aa( ) ) {
                return
                    false;
            }

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || !g_features->orbwalker->
                                         is_attackable( target->index, g_local->attack_range + 25.f ) )
                return false;

            if ( cast_spell( ESpellSlot::w ) ) {
                m_last_w_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->darius.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || m_q_active || !
                m_slot_e->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || g_config->darius.e_aa_range_limit->get< bool >( ) && g_features->orbwalker->is_attackable(
                target->index
            ) )
                return false;

            auto pred = g_features->prediction->predict( target->index, 575.f, 0.f, 0.f, 0.25f );
            if ( !pred.valid ) return false;

            auto cast_position{ pred.position };
            auto allow_e = static_cast< int >( pred.hitchance ) >= g_config->darius.e_hitchance->get< int >( );

            if ( !allow_e && g_config->darius.e_hitchance->get< int >( ) <= 3
                /* && pred.position != target->position*/ ) {
                const auto sect      = sdk::math::Sector( g_local->position, target->position, 50.f, 575.f );
                const auto pred_sect = sdk::math::Sector( g_local->position, pred.position, 50.f, 575.f );

                auto base_poly = sect.to_polygon_new( );
                auto pred_poly = pred_sect.to_polygon_new( );

                allow_e = pred_poly.is_inside( target->position ) && pred_poly.is_inside( pred.position );

                if ( !allow_e && base_poly.is_inside( target->position ) && base_poly.is_inside( pred.position ) ) {
                    allow_e       = true;
                    cast_position = target->position;
                }
            }

            if ( !allow_e ) return false;

            if ( cast_spell( ESpellSlot::e, cast_position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->darius.r_enabled->get< bool >( ) || *g_time - m_last_r_time <= 0.4f || !m_slot_r->is_ready(
                true
            ) )
                return false;

            const Object* target{ };
            auto          target_priority{ -1 };

            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->dist_to_local( ) > m_r_range || g_features->target_selector->is_bad_target(
                    enemy->index
                ) )
                    continue;

                const auto damage = get_spell_damage( ESpellSlot::r, enemy );
                const auto health = enemy->health + enemy->base_health_regen;
                if ( health > damage ) continue;


                const auto priority = g_features->target_selector->get_target_priority( enemy->champion_name.text );
                if ( priority < target_priority ) continue;

                target          = enemy;
                target_priority = priority;
            }

            if ( !target ) return false;


            if ( cast_spell( ESpellSlot::r, target->network_id ) ) {
                g_features->orbwalker->on_cast( );
                m_last_r_time = *g_time;
                return true;
            }

            return false;
        }

        auto magnet_q( ) -> void{
            if ( !g_config->darius.q_magnet->get< bool >( ) ||
                g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::flee ||
                g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::none ||
                g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::recalling ) {
                if ( g_features->orbwalker->is_movement_disabled( ) ) g_features->orbwalker->allow_fast_move( );
                g_features->orbwalker->allow_movement( true );
                g_features->orbwalker->allow_attacks( true );

                m_magnet_active = false;
                return;
            }

            const auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "dariusqcast" ) );
            if ( !buff ) {
                if ( g_features->orbwalker->is_movement_disabled( ) ) g_features->orbwalker->allow_fast_move( );
                g_features->orbwalker->allow_movement( true );
                g_features->orbwalker->allow_attacks( true );

                m_magnet_active = false;
                return;
            }

            Object*     target;
            const auto& obj = g_entity_list.get_by_index( m_magnet_target );
            if ( !obj || obj->is_dead( ) || obj->is_invisible( ) || obj->dist_to_local( ) > 500.f ) {
                target = g_features->target_selector->get_default_target( );
                if ( !target ) return;
            } else target = obj.get( );

            const auto time_left = buff->buff_data->end_time - *g_time;

            const auto pred = g_features->prediction->predict_default( target->index, time_left );
            if ( !pred ) return;

            const auto target_position{ *pred };

            const auto distance = g_local->position.dist_to( target_position );
            const auto prioritize_outer{ distance > 350.f };
            const auto point_distance = prioritize_outer ? 380.f : 250.f;

            const auto candidates = g_features->evade->get_circle_segment_points(
                target_position,
                point_distance,
                60
            ).points;

            m_magnet_points.clear( );
            const auto is_local_under_turret{ is_position_in_turret_range( ) };
            const auto max_distance    = ( time_left > 0.2f ? time_left - 0.1f : time_left ) * g_local->movement_speed;
            const auto cursor          = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;
            const auto cursor_position = g_local->position.extend( cursor, max_distance );

            Vec3 goal_point{ };
            auto lowest_distance{ 999999.f };
            bool found_point{ };

            for ( auto point : candidates ) {
                auto walk_point{ point };

                /*if (point.dist_to(g_local->position) <= 80.f) {
                    auto point_dist = point.dist_to(g_local->position);
                    auto extend_value = 100.f - point_dist;

                    walk_point = target_position.extend(point, target_position.dist_to( point ) + extend_value);
                }*/

                if ( g_navgrid->is_wall( walk_point ) || g_local->position.dist_to( walk_point ) > max_distance
                    || !is_local_under_turret && is_position_in_turret_range( walk_point ) ) {
                    m_magnet_points.push_back( walk_point );
                    continue;
                }

                if ( walk_point.dist_to( cursor_position ) > lowest_distance ) continue;

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


            if ( *g_time - m_last_move_time > 0.05f && !g_features->orbwalker->in_attack( ) ) {
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

    private:
        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_r_time{ };

        bool m_q_active{ };

        // q magnet
        bool                m_magnet_active{ };
        float               m_last_move_time{ };
        int16_t             m_magnet_target{ };
        std::vector< Vec3 > m_magnet_points{ };

        std::array< float, 6 > m_q_damage = { 0.f, 65.f, 95.f, 125.f, 155.f, 185.f };
        std::array< float, 6 > m_e_damage = { 0.f, 80.f, 105.f, 130.f, 155.f, 180.f };
        std::vector< float >   m_r_damage = { 0.f, 125.f, 250.f, 375.f };

        float m_q_range{ 400.f };
        float m_w_range{ 0.f };
        float m_e_range{ 600.f };
        float m_r_range{ 475.f };
    };
}
