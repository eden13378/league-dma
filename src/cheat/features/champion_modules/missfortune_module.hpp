#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"
#include "../../sdk/game/ai_manager.hpp"

namespace features::champion_modules {
    class missfortune_module final : public IModule {
    public:
        virtual ~missfortune_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "missfortune_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "MissFortune" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "missfortune" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->missfortune.q_enabled );
            q_settings->checkbox( _( "harass q" ), g_config->missfortune.q_harass );
            q_settings->checkbox( _( "dont q moving minion" ), g_config->missfortune.q_ignore_moving_minion );
            q_settings->select(
                _( "bounce hitchance" ),
                g_config->missfortune.q_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );
            q_settings->slider_int( _( "max bounce % range" ), g_config->missfortune.q_max_range, 20, 100, 1 );

            e_settings->checkbox( _( "enable" ), g_config->missfortune.e_enabled );
            e_settings->checkbox( _( "harass e" ), g_config->missfortune.e_harass );
            e_settings->checkbox( _( "only out of aa range" ), g_config->missfortune.e_only_out_of_range );
            e_settings->checkbox( _( "antigapcloser" ), g_config->missfortune.e_antigapcloser );
            e_settings->select(
                _( "hitchance" ),
                g_config->missfortune.e_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            w_settings->checkbox( _( "enable" ), g_config->missfortune.w_enabled );
            w_settings->checkbox( _( "harass w" ), g_config->missfortune.w_harass );


            drawings->checkbox( _( "draw q range" ), g_config->missfortune.q_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->missfortune.e_draw_range );
            drawings->checkbox( _( "draw r range" ), g_config->missfortune.r_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->missfortune.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->missfortune.q_draw_range->get< bool >( ) &&
                !g_config->missfortune.e_draw_range->get< bool >( ) &&
                !g_config->missfortune.r_draw_range->get< bool >( ) ||
                g_local->is_dead( ) )
                return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };


            if ( g_config->missfortune.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->mundo.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 31, 88, 255, 255 ),
                        g_local->attack_range + 100.f,
                        Renderer::outline,
                        60,
                        2.f
                    );
                }
            }

            if ( g_config->missfortune.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->missfortune.dont_draw_on_cooldown
                    ->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 144, 66, 245, 255 ),
                        m_e_range,
                        Renderer::outline,
                        60,
                        2.f
                    );
                }
            }

            if ( g_config->missfortune.r_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::r );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->missfortune.dont_draw_on_cooldown
                    ->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 173, 47, 68, 255 ),
                        m_r_range,
                        Renderer::outline,
                        50,
                        2.f
                    );
                }
            }

            return;

            // q debug drawing
            auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return;

            auto sect = sdk::math::Sector(
                target->position,
                target->position.extend( g_local->position, -50.f ),
                75.f,
                500.f * ( static_cast< float >( g_config->missfortune.q_max_range->get< int >( ) ) / 100.f )
            );
            auto polygon = sect.to_polygon_new( );

            for ( size_t i = 0u; i < polygon.points.size( ); i++ ) {
                const auto start_point = polygon.points[ i ];
                const auto end_point = i == polygon.points.size( ) - 1 ? polygon.points[ 0 ] : polygon.points[ i + 1 ];

                Vec2 sp_start;
                Vec2 sp_end;

                if ( !sdk::math::world_to_screen( start_point, sp_start ) || !sdk::math::world_to_screen(
                    end_point,
                    sp_end
                ) )
                    continue;

                g_render->line(
                    sp_start,
                    sp_end,
                    Color( 255, 255, 255 ),
                    1.f
                );
            }
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return;

            if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo
                || g_config->missfortune.w_harass->get< bool >( ) && g_features->orbwalker->get_mode( ) ==
                Orbwalker::EOrbwalkerMode::harass )
                spell_w( );

            if ( g_features->orbwalker->in_attack( ) ) return;

            antigapclose_e( );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_q( );
                spell_e( );
                break;

            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->missfortune.q_harass->get< bool >( ) ) spell_q( );
                if ( g_config->missfortune.e_harass->get< bool >( ) ) spell_e( );

                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->missfortune.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->
                is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto allow_q{
                g_features->orbwalker->should_reset_aa( ) && g_features->orbwalker->is_attackable( target->index )
            };
            auto target_nid{
                allow_q ? target->network_id : 0
            };

            if ( !allow_q ) {
                for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
                    if ( !minion ||
                        minion->is_dead( ) ||
                        minion->is_invisible( ) ||
                        !minion->is_lane_minion( ) &&
                        !minion->is_jungle_monster( ) ||
                        !g_features->orbwalker->is_attackable( minion->index )
                    )
                        continue;

                    auto aimgr = minion->get_ai_manager( );
                    if ( !aimgr || aimgr->is_moving && g_config->missfortune.q_ignore_moving_minion->get<
                        bool >( ) )
                        continue;

                    auto sect = sdk::math::Sector(
                        minion->position,
                        minion->position.extend( g_local->position, -50.f ),
                        75.f,
                        500.f * ( static_cast< float >( g_config->missfortune.q_max_range->get< int >( ) ) / 100.f )
                    );
                    auto polygon = sect.to_polygon_new( );

                    const auto travel_time = g_features->orbwalker->get_attack_cast_delay( ) + minion->dist_to_local( )
                        /
                        1400.f;
                    auto pred = g_features->prediction->predict( target->index, 1000.f, 0.f, 0.f, travel_time );
                    if ( !pred.valid || ( int )pred.hitchance < g_config->missfortune.q_hitchance->get< int >( ) || !
                        polygon.is_inside( pred.position ) )
                        continue;

                    allow_q    = true;
                    target_nid = minion->network_id;
                    break;
                }
            }

            if ( !allow_q ) return false;

            if ( cast_spell( ESpellSlot::q, target_nid ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->set_last_attack_time( *g_time );
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->missfortune.w_enabled->get< bool >( ) || *g_time - m_last_w_time < 0.5f || !m_slot_w->
                is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto allow_w{
                g_features->orbwalker->should_reset_aa( ) && g_features->orbwalker->get_last_target( ) ==
                ETargetType::hero
                && g_features->orbwalker->is_attackable( target->index )
            };

            if ( !allow_w && g_features->orbwalker->in_attack( ) ) {
                auto sci = g_local->spell_book.get_spell_cast_info( );
                if ( !sci || !sci->is_autoattack ) return false;

                const auto& obj = g_entity_list.get_by_index( sci->get_target_index( ) );
                if ( !obj || obj->is_dead( ) || obj->is_invisible( ) ) return false;

                allow_w = obj->is_hero( );
            }

            if ( !allow_w ) return false;

            if ( cast_spell( ESpellSlot::w ) ) m_last_w_time = *g_time;

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->missfortune.e_enabled->get< bool >( ) || *g_time - m_last_e_time < 0.5f || !m_slot_e->
                is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || g_config->missfortune.e_only_out_of_range->get< bool >( ) && g_features->orbwalker->
                is_attackable( target->index ) )
                return false;

            if ( g_features->orbwalker->is_attackable( target->index )
                && !g_features->orbwalker->should_reset_aa( ) && g_features->orbwalker->get_next_possible_aa_time( ) <=
                *g_time + 0.25f )
                return false;

            auto pred = g_features->prediction->predict( target->index, m_e_range, 0.f, 0.f, 0.275f );
            if ( !pred.valid || ( int )pred.hitchance < g_config->missfortune.e_hitchance->get< int >( ) ) return false;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                m_last_e_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{ return false; }

        auto antigapclose_e( ) -> void{
            if ( !g_config->missfortune.e_antigapcloser->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e
                ->is_ready( true ) )
                return;

            const auto target = get_antigapclose_target( );
            if ( !target ) return;

            auto aimgr = target->get_ai_manager( );
            if ( !aimgr ) return;

            const auto path = aimgr->get_path( );
            if ( path.size( ) != 2 ) return;

            const auto path_end = path[ 1 ];

            if ( cast_spell( ESpellSlot::e, path_end ) ) m_last_e_time = *g_time;
        }

    protected:
        auto get_spell_damage( ESpellSlot slot, Object* target ) -> float override{
            // switch ( slot ) {
            // default:
            //     return 0.f;
            // }
            return 0.f;
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
            {
                const auto tt   = 0.25f + g_local->position.dist_to( target->position ) / 2000.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.25f + g_local->position.dist_to( pred.value( ) ) / 2000.f;
            }
            case ESpellSlot::w:
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


        float m_q_range{ 0.f };
        float m_w_range{ 0.f };
        float m_e_range{ 1000.f };
        float m_r_range{ 1450.f };
    };
}
