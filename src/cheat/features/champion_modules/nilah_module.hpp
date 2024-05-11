#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"
#include "../../sdk/game/hud_manager.hpp"

namespace features::champion_modules {
    class nilah_module final : public IModule {
    public:
        virtual ~nilah_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "nilah_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Nilah" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "nilah" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );
            //const auto spellclear = navigation->add_section( _( "spellclear" ) );

            q_settings->checkbox( _( "enable" ), g_config->nilah.q_enabled );
            q_settings->checkbox( _( "use in harass" ), g_config->nilah.q_enabled );
            q_settings->checkbox( _( "use in laneclear" ), g_config->nilah.q_spellclear );
            q_settings->select(
                _( "hitchance" ),
                g_config->nilah.q_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            w_settings->checkbox( _( "enable" ), g_config->nilah.w_enabled );
            w_settings->slider_int( "min health %", g_config->nilah.w_min_health_percent, 20, 100, 1 );

            e_settings->checkbox( _( "enable" ), g_config->nilah.e_enabled );
            e_settings->checkbox( _( "enable exploit (?)" ), g_config->nilah.e_exploit )->set_tooltip(
                _( "exploit allows using E on dead champions" )
            );
            e_settings->checkbox( _( "always if killable" ), g_config->nilah.e_on_killable );
            e_settings->slider_int( "min dash distance (?)", g_config->nilah.e_min_dash_range, 100, 500, 5 )->
                        set_tooltip( _( "if enemy is closer than X distance, will not use E to dash" ) );
            e_settings->slider_int( "selection circle radius (?)", g_config->nilah.e_force_dash_range, 100, 300, 1 )->
                        set_tooltip(
                            _( "the size of the circle on enemies that your mouse needs to be inside to cast E" )
                        );

            r_settings->checkbox( _( "enable" ), g_config->nilah.r_enabled );
            r_settings->slider_int( "min targets", g_config->nilah.r_min_targets, 1, 5, 1 );

            drawings->checkbox( _( "draw q range" ), g_config->nilah.q_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->nilah.e_draw_range );
            drawings->checkbox( _( "draw r range" ), g_config->nilah.r_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->nilah.dont_draw_on_cooldown );
            drawings->checkbox( _( "draw force dash radius" ), g_config->nilah.e_draw_dash_area );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->nilah.q_draw_range->get< bool >( ) &&
                !g_config->nilah.e_draw_range->get< bool >( ) &&
                !g_config->nilah.r_draw_range->get< bool >( ) &&
                !g_config->nilah.e_draw_dash_area->get< bool >( ) ||
                g_local->is_dead( ) )
                return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->nilah.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->nilah.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
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

            if ( g_config->nilah.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->nilah.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
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

            if ( g_config->nilah.r_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::r );
                if ( slot && slot->level > 0 && ( slot->is_ready( ) || !g_config->nilah.dont_draw_on_cooldown->get<
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

            if ( g_config->nilah.e_draw_dash_area->get< bool >( ) && g_config->nilah.e_enabled->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( !slot || !slot->is_ready( ) ) return;

                auto hud = g_pw_hud->get_hud_manager( );
                if ( !hud ) return;

                const auto cursor = hud->cursor_position_unclipped;

                for ( const auto& handle : m_handles ) {
                    auto& obj = g_entity_list.get_by_index( handle );
                    if ( !obj || !g_config->nilah.e_exploit->get< bool >( ) && obj->is_dead( ) || obj->is_invisible( )
                        || obj->dist_to_local( ) > 1000.f )
                        continue;

                    obj.update( );

                    if ( obj->position.dist_to( cursor ) <= static_cast< float >( g_config->nilah.e_force_dash_range->
                        get< int >( ) ) ) {
                        g_render->circle_3d(
                            obj->position,
                            Color( 255, 50, 50 ),
                            static_cast< float >( g_config->nilah.e_force_dash_range->get< int >( ) ),
                            2,
                            40,
                            1
                        );
                    } else {
                        g_render->circle_3d(
                            obj->position,
                            Color( 50, 255, 50 ),
                            static_cast< float >( g_config->nilah.e_force_dash_range->get< int >( ) ),
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

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return;

            update_enemies( );


            m_action_blocked = !!g_features->buff_cache->get_buff( g_local->index, ct_hash( "NilahR" ) );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_w( );
                spell_e( );
                spell_q( );
                spell_r( );
                break;

            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->nilah.q_harass->get< bool >( ) ) spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::laneclear:
                laneclear_q( );
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->nilah.q_enabled->get< bool >( ) || m_action_blocked || *g_time - m_last_q_time <= 0.4f || !
                m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto pred = g_features->prediction->predict( target->index, m_q_range, 0.f, 75.f, 0.25f, { }, true );
            if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->nilah.q_hitchance->get<
                int >( ) )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->nilah.w_enabled->get< bool >( ) ||
                m_action_blocked ||
                *g_time - m_last_w_time <= 0.4f ||
                !m_slot_w->is_ready( true ) ||
                g_features->buff_cache->get_buff( g_local->index, ct_hash( "NilahW" ) )
            )
                return false;

            if ( g_local->health / g_local->max_health > g_config->nilah.w_min_health_percent->get< int >( ) *
                0.01f )
                return false;

            bool allow_w{ };
            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) || enemy->dist_to_local( ) > 3000.f ) {
                    return
                        false;
                }

                auto sci = enemy->spell_book.get_spell_cast_info( );
                if ( !sci || sci->server_cast_time <= *g_time || sci->get_target_index( ) != g_local->index ) continue;

                allow_w = true;
                break;
            }

            if ( !allow_w ) return false;

            if ( cast_spell( ESpellSlot::w ) ) {
                m_last_w_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->nilah.e_enabled->get< bool >( ) || m_action_blocked || *g_time - m_last_e_time <= 0.4f || !
                m_slot_e->is_ready( true ) || m_slot_e->charges == 0 )
                return false;

            bool          killable{ };
            const Object* target{ };
            if ( g_config->nilah.e_on_killable->get< bool >( ) ) {
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

                for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                    if ( !enemy || enemy->is_invisible( ) || enemy->dist_to_local( ) > m_e_range ||
                        enemy->position.dist_to( cursor ) > g_config->nilah.e_force_dash_range->get< int >( )
                        || g_features->target_selector->is_bad_target(
                            enemy->index,
                            g_config->nilah.e_exploit->get< bool >( )
                        ) )
                        continue;

                    const auto distance = enemy->position.dist_to( cursor );

                    if ( !target || distance < lowest_distance ) {
                        target          = enemy;
                        lowest_distance = distance;
                    }
                }
            }

            if ( !target || g_local->position.dist_to( target->position ) < g_config->nilah.e_min_dash_range->get<
                int >( ) )
                return false;

            if ( cast_spell( ESpellSlot::e, target->network_id ) ) {
                g_features->orbwalker->set_cast_time( 0.1f );
                g_features->orbwalker->reset_aa_timer( );

                if ( target->is_dead( ) )
                    debug_log( "casting on dead target << {}", *g_time );

                m_last_e_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->nilah.r_enabled->get< bool >( ) || *g_time - m_last_r_time <= 0.4f || !m_slot_r->is_ready(
                true
            ) )
                return false;

            const auto local_pred = g_features->prediction->predict_default( g_local->index, 0.1f );
            if ( !local_pred ) return false;

            int hit_count{ };
            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy ||
                    enemy->is_dead( ) ||
                    enemy->is_invisible( ) ||
                    enemy->position.dist_to( *local_pred ) > m_r_range ||
                    g_features->target_selector->is_bad_target( enemy->index )
                )
                    continue;


                ++hit_count;
            }

            if ( hit_count < g_config->nilah.r_min_targets->get< int >( ) ) return false;

            if ( cast_spell( ESpellSlot::r ) ) {
                g_features->orbwalker->set_cast_time( 0.1f );
                m_last_r_time = *g_time;
                return true;
            }

            return false;
        }

        auto laneclear_q( ) -> void{
            if ( !g_config->nilah.q_spellclear->get< bool >( ) || !GetAsyncKeyState( VK_CONTROL ) || *g_time -
                m_last_q_time <= 0.5f || !m_slot_q->is_ready( true ) )
                return;

            const auto          laneclear_data = get_line_laneclear_target(
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                [this]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                m_q_range,
                75.f,
                false,
                false
            );

            if ( !laneclear_data ) return;

            if ( cast_spell( ESpellSlot::q, laneclear_data->cast_position ) ) m_last_q_time = *g_time;
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->attack_damage( ) * m_q_ad_ratio[ get_slot_q( )->
                        level ],
                    target->index,
                    true
                );
            case ESpellSlot::e:
                return helper::calculate_damage(
                    m_e_damage[ get_slot_e( )->level ] + g_local->attack_damage( ) * 0.2f,
                    target->index,
                    true
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
                return 0.25f;
            case ESpellSlot::e:
                return 0.25f;
            case ESpellSlot::r:
                return 0.25f;
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
                //std::cout << "all enemies saved: " << m_handles.size() << std::endl;
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

        bool m_action_blocked{ };

        std::vector< int32_t > m_handles{ };
        bool                   m_enemies_saved{ };

        std::vector< float > m_q_ad_ratio = { 0.f, 0.9f, 1.f, 1.1f, 1.2f, 1.3f };
        std::vector< float > m_q_damage   = { 0.f, 5.f, 10.f, 15.f, 20.f, 25.f };
        std::vector< float > m_e_damage   = { 0.f, 65.f, 90.f, 115.f, 140.f, 165.f };
        std::vector< float > m_r_damage   = { 0.f, 80.f, 100.f, 120.f };

        float m_q_range{ 600.f };
        float m_w_range{ 0.f };
        float m_e_range{ 550.f };
        float m_r_range{ 450.f };

        float m_e_dash_range{ 450.f };
    };
}
