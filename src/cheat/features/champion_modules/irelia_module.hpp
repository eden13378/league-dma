#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"
#include "../../sdk/game/ai_manager.hpp"

namespace features::champion_modules {
    class irelia_module final : public IModule {
    public:
        virtual ~irelia_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "irelia_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Irelia" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "irelia" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            //const auto w_settings = navigation->add_section(_("w settings"));
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );
            const auto special    = navigation->add_section( _( "special" ) );

            q_settings->checkbox( _( "enable" ), g_config->irelia.q_enabled );
            q_settings->checkbox( _( "use to gapclose" ), g_config->irelia.q_gapclose );
            q_settings->checkbox( _( "prekill q" ), g_config->irelia.q_prekill )->set_tooltip(
                "will use Q if enemy is killable in < 2 autoattacks"
            );
            q_settings->checkbox( _( "use to lasthit" ), g_config->irelia.q_spellfarm )->set_tooltip(
                "hold CTRL to spellfarm"
            );

            //w_settings->checkbox(_("enable"), g_config->irelia.w_enabled);
            //e_only_recast
            e_settings->checkbox( _( "enable" ), g_config->irelia.e_enabled );
            e_settings->checkbox( _( "only cast E2 (?)" ), g_config->irelia.e_only_recast );
            e_settings->checkbox( _( "hide E1 if possible" ), g_config->irelia.e_hide );
            e_settings->select(
                _( "hitchance" ),
                g_config->irelia.e_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            r_settings->checkbox( _( "enable" ), g_config->irelia.r_enabled );
            r_settings->checkbox( _( "only in full combo" ), g_config->irelia.r_only_full_combo );
            r_settings->slider_int( "min target health %", g_config->irelia.r_min_health_percent, 25, 100, 1 );

            drawings->checkbox( _( "draw q range" ), g_config->irelia.q_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->irelia.e_draw_range );
            drawings->checkbox( _( "draw r range" ), g_config->irelia.r_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->irelia.dont_draw_on_cooldown );

            special->checkbox( _( "predict lasthit hp (?)" ), g_config->irelia.q_predict_health )->set_tooltip(
                _( "If script misses Q lasthits on minions, disabling this will help" )
            );
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->irelia.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->irelia.dont_draw_on_cooldown->get
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

            if ( g_config->irelia.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->irelia.dont_draw_on_cooldown->get
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

            if ( g_config->irelia.r_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::r );
                if ( slot && slot->level > 0 && ( slot->is_ready( ) || !g_config->irelia.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
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

            Vec2 sp{ };
            if ( !world_to_screen( g_local->position, sp ) ) return;

            /* std::string text = m_underturret_q ?
                                    "Towerdive: ON" :
                                    "Towerdive: OFF";
             const auto text_size = g_render->get_text_size( text, g_fonts->get_default_navbar( ), 16 );
             g_render->text_shadow( { sp.x - text_size.x / 2.f, sp.y + 13.f }, color::white( ), g_fonts->get_default_navbar( ), text.c_str( ), 16 );*/

            if ( m_e_index == 0 ) return;

            const auto& blade_obj = g_entity_list.get_by_index( m_e_index );
            if ( !blade_obj ) return;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return;

            g_render->circle_3d( blade_obj->position, Color( 50, 255, 50 ), 50.f, 2, 60, 2.f );

            g_render->circle_3d(
                blade_obj->position.extend( target->position, blade_obj->position.dist_to( target->position ) + 150.f ),
                Color( 255, 50, 50 ),
                40.f,
                2,
                60,
                2.f
            );
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            m_underturret_q = GetAsyncKeyState( VK_CONTROL ) && g_features->orbwalker->get_mode( ) ==
                Orbwalker::EOrbwalkerMode::combo;

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:
                spell_r( );
                spell_e( );
                spell_q( );
                q_gapclose( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                break;
            case Orbwalker::EOrbwalkerMode::lasthit:
            case Orbwalker::EOrbwalkerMode::laneclear:
                q_lasthit( );
                break;
            default:
                break;
            }

            // ireliamark | Unsteady buff
            // ireliapassivestacks


            bool found{ };

            if ( rt_hash( m_slot_e->get_name().c_str() ) != ct_hash( "IreliaE" ) ) {
                for ( const auto obj : g_entity_list.get_ally_minions( ) ) {
                    if ( !obj || obj->is_dead( ) || rt_hash( obj->name.text ) != ct_hash( "Blade" ) ) continue;

                    m_e_index = obj->index;
                    found     = true;
                    break;
                }
            }

            if ( !found ) m_e_index = 0;
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->irelia.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) > m_q_range ) return false;

            bool allow_q{ };
            if ( g_features->buff_cache->get_buff( target->index, ct_hash( "ireliamark" ) )
                || target->health < get_spell_damage( ESpellSlot::q, target ) )
                allow_q = true;
            else if ( g_config->irelia.q_prekill->get< bool >( ) ) {
                const auto damage    = get_spell_damage( ESpellSlot::q, target );
                const auto aa_damage = helper::get_aa_damage( target->index );

                allow_q = !g_features->orbwalker->is_attackable( target->index )
                              ? target->health - damage <= aa_damage * 1.9f
                              : target->health < damage;
            }

            if ( !allow_q || !m_underturret_q && is_position_in_turret_range( target->position ) && !
                is_position_in_turret_range( g_local->position ) )
                return false;

            if ( cast_spell( ESpellSlot::q, target->network_id ) ) {
                m_last_q_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{ return false; }

        auto spell_e( ) -> bool override{
            if ( !g_config->irelia.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.2f || !m_slot_e->
                is_ready( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            Vec3 cast_position{ };
            bool allow_cast{ };

            if ( rt_hash( m_slot_e->get_name().c_str() ) == ct_hash( "IreliaE" ) ) {
                if ( g_config->irelia.e_only_recast->get< bool >( ) || !m_slot_e->is_ready( true )
                    || target->dist_to_local( ) > m_e_range )
                    return false;

                auto aimgr = g_local->get_ai_manager( );
                if ( !aimgr ) return false;

                if ( aimgr->is_moving && aimgr->is_dashing ) {
                    const auto path = aimgr->get_path( );
                    if ( path.size( ) == 2 && path[ path.size( ) - 1 ].dist_to( g_local->position ) < m_e_range ) {
                        cast_position = path[ path.size( ) - 1 ];
                        allow_cast    = true;

                        //std::cout << "hide e in dash end\n";
                    }
                }

                if ( !allow_cast ) {
                    if ( g_config->irelia.e_hide->get< bool >( ) ) {
                        const auto pred = g_features->prediction->predict_default( g_local->index, 0.1f );
                        cast_position   = pred.has_value( ) ? *pred : g_local->position;
                    } else cast_position = g_local->position.extend( target->position, target->dist_to_local( ) / 2.f );
                }

                if ( cast_spell( ESpellSlot::e, cast_position ) ) {
                    m_last_e_time = *g_time;
                    return true;
                }

                return false;
            }

            if ( m_e_index == 0 ) return false;

            const auto& blade = g_entity_list.get_by_index( m_e_index );
            if ( !blade ) return false;

            auto pred = g_features->prediction->predict( target->index, m_e_range + 50.f, 2000.f, 40.f, 0.265f );
            if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->irelia.e_hitchance->get<
                int >( ) )
                return false;

            for ( auto i = 1; i <= 8; i++ ) {
                auto possible_position = blade->position.extend(
                    pred.position,
                    blade->position.dist_to( pred.position ) + 15.f * i
                );
                if ( possible_position.dist_to( g_local->position ) > m_e_range ) break;

                cast_position = possible_position;
            }

            if ( cast_position.length( ) == 0.f ) return false;

            if ( cast_spell( ESpellSlot::e, cast_position ) ) {
                m_last_e_time = *g_time;
                return true;
            }


            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->irelia.r_enabled->get< bool >( ) || *g_time - m_last_r_time <= 0.5f || !m_slot_r->is_ready(
                    true
                )
                || g_config->irelia.r_only_full_combo->get< bool >( ) && !GetAsyncKeyState( VK_CONTROL ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || g_features->orbwalker->is_attackable( target->index ) || target->health / target->max_health
                * 100.f < g_config->irelia.r_min_health_percent->get< int >( ) )
                return false;

            const auto pred = g_features->prediction->predict( target->index, m_r_range, 2000.f, 160.f, 0.4f );
            if ( !pred.valid ) return false;

            if ( cast_spell( ESpellSlot::r, pred.position ) ) {
                m_last_r_time = *g_time;
                return true;
            }

            return false;
        }

        auto q_lasthit( ) -> void{
            if ( !GetAsyncKeyState( VK_CONTROL ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->
                is_ready( true ) )
                return;

            const auto          lasthit_data = get_targetable_lasthit_target(
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                m_q_range,
                [this]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); }
            );
            if ( !lasthit_data ) return;

            const auto& target = g_entity_list.get_by_index( lasthit_data->index );
            if ( target && !m_underturret_q && is_position_in_turret_range( target->position ) && !
                is_position_in_turret_range( g_local->position ) )
                return;

            if ( cast_spell( ESpellSlot::q, lasthit_data->network_id ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->ignore_minion( lasthit_data->index, 1.f );
            }
        }

        auto q_gapclose( ) -> void{
            if ( *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready( true ) ) return;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return;

            auto          lowest_distance{ std::numeric_limits< float >::max( ) };
            const Object* target_minion{ };

            const auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "ireliapassivestacks" ) );
            const auto is_passive_stacked{ buff && buff->stacks( ) == 4 };

            for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
                if ( !minion ||
                    minion->is_dead( ) ||
                    minion->is_invisible( ) ||
                    minion->dist_to_local( ) > m_q_range ||
                    !minion->is_lane_minion( ) &&
                    !minion->is_jungle_monster( )
                )
                    continue;

                const auto minion_distance = minion->position.dist_to( target->position );
                if ( minion_distance > target->dist_to_local( ) ) continue;

                const auto damage = get_spell_damage( ESpellSlot::q, minion );
                if ( damage < minion->health && !g_features->buff_cache->get_buff(
                    minion->index,
                    ct_hash( "ireliamark" )
                ) )
                    continue;

                if ( minion_distance < lowest_distance && ( !is_passive_stacked || minion_distance + 125.f < target->
                    dist_to_local( ) ) ) {
                    target_minion   = minion;
                    lowest_distance = minion_distance;
                }
            }

            if ( !target_minion || !m_underturret_q && is_position_in_turret_range( target_minion->position ) && !
                is_position_in_turret_range( g_local->position ) )
                return;

            if ( cast_spell( ESpellSlot::q, target_minion->network_id ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.1f );
            }

            return;
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            if ( !target ) return 0.f;

            switch ( slot ) {
            case ESpellSlot::q:
            {
                if ( target->is_lane_minion( ) ) {
                    const auto bonus_damage = 43.f + 12.f * g_local->level;

                    //if (target->get_minion_type() == c_object::e_minion_type::ranged) std::cout << "dmg on ranged: " << helper::calculate_damage(m_q_damage[get_slot_q()->level] + g_local->attack_damage() * 0.6f + bonus_damage, target->index, true) + helper::get_onhit_damage(target->index) << std::endl;
                    return helper::calculate_damage(
                            m_q_damage[ get_slot_q( )->level ] + g_local->attack_damage( ) * 0.6f + bonus_damage,
                            target->index,
                            true
                        )
                        + helper::get_onhit_damage( target->index );
                }

                return helper::calculate_damage(
                        m_q_damage[ get_slot_q( )->level ] + g_local->attack_damage( ) * 0.6f,
                        target->index,
                        true
                    )
                    + helper::get_onhit_damage( target->index );
            }
            case ESpellSlot::e:
                return helper::calculate_damage(
                    m_e_damage[ get_slot_e( )->level ] + g_local->ability_power( ) * 0.8f,
                    target->index,
                    false
                );
            case ESpellSlot::r:
                return helper::calculate_damage(
                    m_r_damage[ get_slot_r( )->level ] + g_local->ability_power( ) * 0.4f,
                    target->index,
                    false
                );
            }

            return 0.f;
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                if ( !g_config->irelia.q_predict_health->get< bool >( ) ) return 0.f;

                return target->dist_to_local( ) / ( 1400.f + g_local->movement_speed );
            case ESpellSlot::e:
                return 0.25f;
            case ESpellSlot::r:
                return 0.25f;
            }

            return std::numeric_limits< float >::max( );
        }

    private:
        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_r_time{ };

        bool m_underturret_q{ };

        int32_t m_e_index{ };

        std::vector< float > m_q_damage = { 0.f, 5.f, 25.f, 45.f, 65.f, 85.f };
        std::vector< float > m_e_damage = { 0.f, 80.f, 105.f, 130.f, 155.f, 180.f };
        std::vector< float > m_r_damage = { 0.f, 80.f, 100.f, 120.f };

        float m_q_range{ 600.f };
        float m_w_range{ 0.f };
        float m_e_range{ 850.f };
        float m_r_range{ 1000.f };
    };
}
