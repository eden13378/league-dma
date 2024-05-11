#pragma once
#include "../../menu/menu.hpp"
// #include "../../sdk/game/ai_manager.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"
// #include "../tracker.hpp"
#include "module.hpp"

namespace features::champion_modules {
    class diana_module final : public IModule {
    public:
        virtual ~diana_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "diana_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Diana" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell, e_spell, r_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation  = g_window->push( _( "dirty diana" ), menu_order::champion_module );
            const auto q_settings  = navigation->add_section( _( "q settings" ) );
            const auto drawings    = navigation->add_section( _( "drawings" ) );
            const auto w_settings  = navigation->add_section( _( "w settings" ) );
            const auto e_settings  = navigation->add_section( _( "e settings" ) );
            const auto r_settings  = navigation->add_section( _( "r settings" ) );
            const auto spellclear  = navigation->add_section( _( "spellclear" ) );
            const auto jungleclear = navigation->add_section( _( "jungleclear" ) );

            q_settings->checkbox( _( "enable" ), g_config->diana.q_enabled );
            q_settings->checkbox( _( "use in harass" ), g_config->diana.q_harass );
            q_settings->select(
                _( "hitchance" ),
                g_config->diana.q_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            w_settings->checkbox( _( "enable" ), g_config->diana.w_enabled );
            //w_settings->checkbox( _( "cast on incoming damage" ), g_config->diana.w_incoming_damage );

            e_settings->checkbox( _( "enable" ), g_config->diana.e_enabled );
            e_settings->checkbox( _( "always if killable" ), g_config->diana.e_on_killable );
            e_settings->checkbox( _( "only to passive (?)" ), g_config->diana.e_only_to_passive )->set_tooltip(
                _( "always killable will ignore passive mark" )
            );
            //e_settings->checkbox( _( "tower range check" ), g_config->diana.e_tower_range_check );
            e_settings->slider_int( "selection circle radius (?)", g_config->diana.e_force_dash_range, 100, 500, 1 )->
                        set_tooltip(
                            _( "the size of the circle on enemies that your mouse needs to be inside to cast E" )
                        );

            r_settings->checkbox( _( "enabled" ), g_config->diana.r_enabled );
            r_settings->slider_int( _( "min enemies within range" ), g_config->diana.r_min_enemy_count, 1, 5, 1 );
            r_settings->checkbox( _( "cast if at least one killable enemy" ), g_config->diana.r_if_killable );

            spellclear->checkbox( _( "laneclear q" ), g_config->diana.q_laneclear );
            spellclear->slider_int( _( "min mana %" ), g_config->diana.q_laneclear_min_mana, 1, 70, 5 );

            jungleclear->checkbox( _( "jungleclear q" ), g_config->diana.q_jungleclear );
            jungleclear->checkbox( _( "jungleclear w" ), g_config->diana.w_jungleclear );
            jungleclear->checkbox( _( "jungleclear e" ), g_config->diana.e_jungleclear );

            drawings->checkbox( _( "draw q range" ), g_config->diana.q_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->diana.e_draw_range );
            drawings->checkbox( _( "draw r range" ), g_config->diana.r_draw_range );
        }

        //diana Q radius is 180
        auto on_draw( ) -> void override{
            if ( !g_config->diana.q_draw_range->get< bool >( ) &&
                !g_config->diana.e_draw_range->get< bool >( ) &&
                !g_config->diana.r_draw_range->get< bool >( ) ||
                g_local->is_dead( ) )
                return;

            g_local.update( );
            // auto local = g_local.create_copy( );
            // local.update( );

            if ( g_config->diana.q_draw_range->get< bool >( ) &&
                g_local->spell_book.get_spell_slot( ESpellSlot::q )->level > 0 )
                g_render->circle_3d( g_local->position, Color( 31, 88, 255, 255 ), m_q_range, 2, 80, 2.f );

            if ( g_config->diana.e_draw_range->get< bool >( ) &&
                g_local->spell_book.get_spell_slot( ESpellSlot::e )->level > 0 )
                g_render->circle_3d( g_local->position, Color( 144, 66, 245, 255 ), m_e_range, 2, 80, 2.f );

            if ( g_config->diana.r_draw_range->get< bool >( ) &&
                g_local->spell_book.get_spell_slot( ESpellSlot::r )->level > 0 )
                g_render->circle_3d( g_local->position, Color( 173, 47, 68, 255 ), m_r_range, 2, 80, 2.f );

            if ( g_config->diana.e_draw_dash_area->get< bool >( ) && g_config->diana.e_enabled->get< bool >( ) ) {
                //slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( !g_local->spell_book.get_spell_slot( ESpellSlot::e ) ||
                    !g_local->spell_book.get_spell_slot( ESpellSlot::e )->is_ready( ) )
                    return;

                auto hud = g_pw_hud->get_hud_manager( );
                if ( !hud ) return;

                const auto cursor = hud->cursor_position_unclipped;

                for ( const auto& handle : m_handles ) {
                    auto& obj = g_entity_list.get_by_index( handle );
                    if ( !obj || obj->is_dead( ) || obj->is_invisible( )
                        || obj->dist_to_local( ) > 1000.f )
                        continue;

                    obj.update( );

                    if ( obj->position.dist_to( cursor ) <= static_cast< float >( g_config->diana.e_force_dash_range->
                        get< int >( ) ) ) {
                        g_render->circle_3d(
                            obj->position,
                            Color( 255, 50, 50 ),
                            static_cast< float >( g_config->diana.e_force_dash_range->get< int >( ) ),
                            2,
                            40,
                            1
                        );
                    } else {
                        g_render->circle_3d(
                            obj->position,
                            Color( 50, 255, 50 ),
                            static_cast< float >( g_config->diana.e_force_dash_range->get< int >( ) ),
                            2,
                            40,
                            1
                        );
                    }
                }
            }
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) ||
                g_features->evade->is_active( ) )
                return;

            update_enemies( );
            targets_in_r_range( );


            m_action_blocked = !!g_features->buff_cache->get_buff( g_local->index, ct_hash( "DianaR" ) );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:
                spell_w( );
                spell_e( );
                spell_q( );
                spell_r( );
                break;
            case Orbwalker::EOrbwalkerMode::laneclear:
                laneclear_q( );
                if ( g_config->diana.q_jungleclear->get< bool >( ) && jungleclear_q( ) ) return;
                if ( g_config->diana.e_jungleclear->get< bool >( ) && jungleclear_e( ) ) return;
                if ( g_config->diana.w_jungleclear->get< bool >( ) && jungleclear_w( ) ) return;
                break;
            case Orbwalker::EOrbwalkerMode::harass:

                if ( m_slot_q->is_ready( true ) ) spell_q( );
                break;

            default: ;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            m_special_dash_target = { };
            if ( !g_config->diana.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.1f || !m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_spell_specific_target(
                m_q_range,
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); }
            );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_q_range, 1900.f, 90.f, 0.25f );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->diana.q_hitchance->
                get< int >( ) ) )
                return false;

            m_special_dash_target = target;
            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->set_cast_time( 0.25f );
                m_last_q_time = *g_time;
                return true;
            }
            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->diana.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.1f || !m_slot_w->is_ready( true ) )
                return false;

            const Object* target{ };
            const bool    incoming_damage{ };
            //block incoming damage not currently working
            /*if ( g_config->diana.w_incoming_damage->get< bool >( ) ) {
                auto pred_health = g_features->prediction->predict_health( g_local.get( ), 0.1f, false, false, true );
                if ( pred_health >= g_local->health - 80.f ) return false;
                incoming_damage = true;
            }*/

            target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            if ( target->dist_to_local( ) < m_w_range || incoming_damage ) {
                if ( cast_spell( ESpellSlot::w ) ) {
                    m_last_w_time = *g_time;
                    return true;
                }
            }
            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->diana.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !
                m_slot_e->is_ready( true ) || m_action_blocked )
                return false;

            bool          killable{ };
            const Object* target{ };
            if ( g_config->diana.e_on_killable->get< bool >( ) ) {
                const auto kill_target = g_features->target_selector->get_default_target( );
                if ( kill_target && kill_target->dist_to_local( ) <= m_e_range && !g_features->orbwalker->
                    is_attackable( kill_target->index ) ) {
                    const auto damage = get_spell_damage( ESpellSlot::e, kill_target ) + helper::get_aa_damage(
                        kill_target->index,
                        true
                    ) * 2.f;

                    if ( damage >= kill_target->health ) {
                        killable = true;
                        target   = kill_target;
                    }
                }
            }

            if ( !killable ) {
                auto hud = g_pw_hud->get_hud_manager( );
                if ( !hud ) return false;

                const auto cursor = hud->cursor_position_unclipped;
                auto       lowest_distance{ std::numeric_limits< float >::max( ) };

                if ( !m_special_dash_target || m_special_dash_target->is_invisible( ) ||
                    m_special_dash_target->dist_to_local( ) > m_e_range ||
                    m_special_dash_target->position.dist_to( cursor ) >
                    g_config->diana.e_force_dash_range->get< int >( ) ) {
                    for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                        if ( !enemy || enemy->is_invisible( ) || enemy->dist_to_local( ) > m_e_range ||
                            enemy->position.dist_to( cursor ) > g_config->diana.e_force_dash_range->get< int >( ) )
                            continue;

                        if ( !g_config->diana.e_only_to_passive->get< bool >( ) ) {
                            const auto distance = enemy->position.dist_to( cursor );

                            if ( !target || distance < lowest_distance ) {
                                target          = enemy;
                                lowest_distance = distance;
                            }
                        }

                        if ( g_config->diana.e_only_to_passive->get< bool >( ) &&
                            !g_features->buff_cache->get_buff( enemy->index, ct_hash( "dianamoonlight" ) ) )
                            continue;

                        const auto distance = enemy->position.dist_to( cursor );

                        if ( !target || distance < lowest_distance ) {
                            target          = enemy;
                            lowest_distance = distance;
                        }
                    }
                } else target = m_special_dash_target;
            }

            if ( !target ) return false;

            if ( cast_spell( ESpellSlot::e, target->network_id ) ) {
                m_last_e_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->diana.r_enabled->get< bool >( ) || *g_time - m_last_r_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.1f || !m_slot_r->is_ready( true ) )
                return false;

            if ( m_r_target_kill ) {
                if ( cast_spell( ESpellSlot::r ) ) {
                    m_last_r_time = *g_time;
                    g_features->orbwalker->on_cast( );
                    return true;
                }
            }

            if ( m_r_in_range >= g_config->diana.r_min_enemy_count->get< int >( ) ) {
                if ( cast_spell( ESpellSlot::r ) ) {
                    m_last_r_time = *g_time;
                    g_features->orbwalker->on_cast( );
                    return true;
                }
            }
            return false;
        }

        auto laneclear_q( ) -> bool{
            if ( !m_slot_q->is_ready( true ) ||
                *g_time - m_last_q_time <= 0.4f ||
                g_local->mana < g_local->max_mana / 100.f * g_config->diana.q_laneclear_min_mana->get< int >( ) ||
                !g_config->diana.q_laneclear->get< bool >( ) )
                return false;

            const auto farm_pos = get_best_laneclear_position( m_q_range, 180, true, false, 0.25f );
            if ( farm_pos.value < 3 ) return false;

            if ( cast_spell( ESpellSlot::q, farm_pos.position ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto jungleclear_q( ) -> bool{
            if ( !m_slot_q->is_ready( true ) || *g_time - m_last_q_time <= 0.4f ) return false;

            const auto farm_pos = get_best_laneclear_position( m_q_range, 180, false, true, 0.25f );
            if ( farm_pos.value < 1 ) return false;

            if ( cast_spell( ESpellSlot::q, farm_pos.position ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.25f );
                return true;
            }

            return false;
        }

        auto jungleclear_w( ) -> bool{
            if ( !m_slot_w->is_ready( true ) || *g_time - m_last_w_time <= 0.4f ) return false;
            auto in_range = false;
            for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
                if ( !minion ||
                    minion->is_dead( ) ||
                    minion->is_invisible( ) ||
                    !minion->is_jungle_monster( ) ||
                    minion->position.dist_to( g_local->position ) > m_w_range
                )
                    continue;

                in_range = true;
            }

            if ( in_range ) {
                if ( cast_spell( ESpellSlot::w ) ) {
                    m_last_q_time = *g_time;
                    g_features->orbwalker->set_cast_time( 0.25f );
                    return true;
                }
            }

            return false;
        }

        auto jungleclear_e( ) -> bool{
            if ( !m_slot_e->is_ready( true ) || *g_time - m_last_e_time <= 0.4f ) return false;
            for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
                if ( !minion ||
                    minion->is_dead( ) ||
                    minion->is_invisible( ) ||
                    !minion->is_jungle_monster( ) ||
                    minion->position.dist_to( g_local->position ) > m_e_range ||
                    !g_features->buff_cache->get_buff( minion->index, ct_hash( "dianamoonlight" ) )
                )
                    continue;

                if ( cast_spell( ESpellSlot::e, minion->network_id ) ) {
                    m_last_q_time = *g_time;
                    g_features->orbwalker->set_cast_time( 0.25f );
                    return true;
                }
            }

            return false;
        }

        auto targets_in_r_range( ) -> void{
            auto r_count    = 0.f;
            m_r_target_kill = false;
            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->is_dead( ) || enemy->dist_to_local( ) > 375.f || !enemy->
                    is_targetable( ) )
                    continue;
                if ( get_spell_damage( ESpellSlot::r, enemy ) > enemy->health &&
                    g_config->diana.r_if_killable->get< bool >( ) )
                    m_r_target_kill = true;
                r_count++;
            }

            m_r_in_range = r_count;
        }

        static auto generate_dash_list( ) -> void{
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
            case ESpellSlot::w:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_w( )->level ] + g_local->ability_power( ) * 0.15f,
                    target->index,
                    false
                );
            case ESpellSlot::e:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->ability_power( ) * 0.7f,
                    target->index,
                    false
                );
            case ESpellSlot::r:
                return helper::calculate_damage(
                    m_r_damage[ get_slot_r( )->level ] + g_local->ability_power( ) * 0.6f +
                    m_r_in_range * 0.15f *
                    m_r_bonus_damage_per_enemy[ get_slot_r( )->level ],
                    target->index,
                    false
                );
            }

            return 0.f;
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
            {
                const auto tt   = 0.25f + g_local->position.dist_to( target->position ) / 2000.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.25f + g_local->position.dist_to( pred.value( ) ) / 1900.f;
            }
            }

            return std::numeric_limits< float >::max( );
        }

        auto is_enemy_saved( const int16_t index ) const -> bool{
            for ( auto& handle : m_handles ) if ( handle == index ) return true;

            return false;
        }

        auto update_enemies( ) -> void{
            const auto enemies = g_entity_list.get_enemies( );
            if ( enemies.size( ) == m_handles.size( ) ) {
                m_enemies_saved = true;
                return;
            }

            for ( const auto enemy : enemies ) {
                if ( !enemy || is_enemy_saved( enemy->index ) ) continue;

                m_handles.push_back( enemy->index );
            }
        }

    private:
        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_r_time{ };
        float m_last_cast_time{ };

        float m_q_range{ 870.f };
        float m_w_range{ 200.f };
        float m_e_range{ 825.f };
        float m_r_range{ 475.f };

        bool  m_r_target_kill{ };
        float m_r_in_range{ };

        bool m_action_blocked{ };

        Object* m_special_dash_target{ };

        std::vector< int32_t > m_handles{ };
        bool                   m_enemies_saved{ };

        Vec3                m_target_current_position{ };
        std::vector< Vec3 > m_cast_path{ };
        int                 m_next_path_node{ };
        Vec3                m_cast_pos{ };
        float               m_cast_time{ };

        std::array< float, 6 > m_q_damage{ 0.f, 60.f, 95.f, 130.f, 165.f, 200.f };
        std::array< float, 6 > m_w_damage{ 0.f, 18.f, 30.f, 42.f, 54.f, 66.f };
        std::array< float, 6 > m_e_damage{ 0.f, 50.f, 70.f, 90.f, 110.f, 130.f };
        std::vector< float >   m_r_damage{ 0.f, 200.f, 300.f, 400.f };
        std::vector< float >   m_r_bonus_damage_per_enemy{ 0.f, 35.f, 60.f, 85.f };
    };
} // namespace features::champion_modules
