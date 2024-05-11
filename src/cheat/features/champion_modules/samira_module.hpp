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
    class samira_module final : public IModule {
    public:
        virtual ~samira_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "samira_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Samira" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "samira" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );
            // const auto passive = navigation->add_section(_("passive"));

            q_settings->checkbox( _( "enable" ), g_config->samira.q_enabled );
            q_settings->checkbox( _( "use in harass" ), g_config->samira.q_enabled );
            q_settings->checkbox( _( "use to lasthit" ), g_config->samira.q_farm );
            q_settings->select(
                _( "hitchance" ),
                g_config->samira.q_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            w_settings->checkbox( _( "enable (?)" ), g_config->samira.w_enabled )->set_tooltip(
                _( "Will be used to reach S grade faster" )
            );
            w_settings->checkbox( _( "only if can reset aa" ), g_config->samira.w_aa_reset );

            e_settings->checkbox( _( "enable" ), g_config->samira.e_enabled );
            e_settings->checkbox( _( "delay on aa weave (?)" ), g_config->samira.e_weave_aa )->set_tooltip(
                _( "Will delay E if can weave an AA before dashing and increase style grade. Ignored in full combo" )
            );
            e_settings->select( _( "dash mode" ), g_config->samira.e_mode, { _( "Always" ), _( "Smart" ) } );
            e_settings->slider_int( "allow dash range", g_config->samira.e_force_dash_range, 100, 300, 1 );

            r_settings->checkbox( _( "enable" ), g_config->samira.r_enabled );
            r_settings->slider_int( "min targets", g_config->samira.r_min_targets, 1, 5, 1 );

            //passive->checkbox(_("enable (?)"), g_config->samira.passive_orbwalk)->set_tooltip(_("Will automatically active passive in orbwalk on enemies who are stunned, rooted, etc"));

            drawings->checkbox( _( "draw q range" ), g_config->samira.q_draw_range );
            drawings->checkbox( _( "draw q melee range" ), g_config->samira.q_melee_draw_range ); //q_melee_draw_range
            drawings->checkbox( _( "draw w range" ), g_config->samira.w_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->samira.e_draw_range );
            drawings->checkbox( _( "draw r range" ), g_config->samira.r_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->samira.dont_draw_on_cooldown );

            drawings->checkbox( _( "draw force dash radius" ), g_config->samira.e_draw_dash_area );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->samira.q_draw_range->get< bool >( ) &&
                !g_config->samira.q_melee_draw_range->get< bool >( ) &&
                !g_config->samira.w_draw_range->get< bool >( ) &&
                !g_config->samira.e_draw_range->get< bool >( ) &&
                !g_config->samira.r_draw_range->get< bool >( ) &&
                !g_config->samira.e_draw_dash_area->get< bool >( ) ||
                g_local->is_dead( )
            )
                return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->samira.q_draw_range->get< bool >( ) || g_config->samira.q_melee_draw_range->get<
                bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->samira.dont_draw_on_cooldown->get
                    < bool >( ) ) ) {
                    if ( g_config->samira.q_draw_range->get< bool >( ) ) {
                        g_render->circle_3d(
                            g_local->position,
                            Color( 31, 88, 255, 255 ),
                            m_q_range,
                            Renderer::outline,
                            50,
                            2.f
                        );
                    }

                    if ( g_config->samira.q_melee_draw_range->get< bool >( ) ) {
                        g_render->circle_3d(
                            g_local->position,
                            Color( 31, 160, 255, 255 ),
                            m_melee_q_range,
                            Renderer::outline,
                            50,
                            2.f
                        );
                    }
                }
            }

            if ( g_config->samira.w_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->samira.dont_draw_on_cooldown->get
                    < bool >( ) ) ) {
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

            if ( g_config->samira.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->samira.dont_draw_on_cooldown->get
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

            if ( g_config->samira.r_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::r );
                if ( slot && slot->level > 0 && ( slot->is_ready( ) || !g_config->samira.dont_draw_on_cooldown->get<
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

            // std::string text = "Grade: " + std::to_string(m_grade);
            // g_render->text_3d(g_local->position + vec3(50, 100, 0), color(255, 255, 255), g_fonts->get_bold(), text.c_str(), 16.f, true);

            if ( g_config->samira.e_draw_dash_area->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( !slot || !slot->is_ready( ) ) return;

                auto hud = g_pw_hud->get_hud_manager( );
                if ( !hud ) return;

                const auto cursor = hud->cursor_position_unclipped;

                for ( const auto& handle : m_handles ) {
                    auto& obj = g_entity_list.get_by_index( handle );
                    if ( !obj || obj->is_dead( ) || obj->is_invisible( ) || obj->dist_to_local( ) > 1000.f ) continue;

                    obj.update( );

                    if ( obj->position.dist_to( cursor ) <= static_cast< float >( g_config->samira.e_force_dash_range->
                        get< int >( ) ) ) {
                        g_render->circle_3d(
                            obj->position,
                            Color( 255, 50, 50 ),
                            static_cast< float >( g_config->samira.e_force_dash_range->get< int >( ) ),
                            2,
                            40,
                            1
                        );
                    } else {
                        g_render->circle_3d(
                            obj->position,
                            Color( 50, 255, 50 ),
                            static_cast< float >( g_config->samira.e_force_dash_range->get< int >( ) ),
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

            m_grade = m_slot_r->get_usable_state( ) - 1;

            if ( g_config->samira.e_weave_aa->get< bool >( ) ) {
                if ( m_should_weave && *g_time - m_last_q_time <= 0.4f ) {
                    const auto target = g_features->target_selector->get_default_target( );
                    if ( target && g_features->orbwalker->is_attackable( target->index ) ) {
                        auto sci = g_local->spell_book.get_spell_cast_info( );
                        if ( sci && sci->slot == 0 && sci->server_cast_time > *g_time ) {
                            m_weave_end_time = g_features->orbwalker->get_next_possible_aa_time( );
                            m_should_weave   = false;
                        }
                    }
                } else m_should_weave = false;
            }

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return;

            update_enemies( );
            update_dash( );

            m_action_blocked = !!g_features->buff_cache->get_buff( g_local->index, ct_hash( "SamiraR" ) ) || !!
                g_features->buff_cache->get_buff( g_local->index, ct_hash( "SamiraW" ) );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_e( );
                spell_q( );
                spell_w( );
                spell_r( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->samira.q_harass->get< bool >( ) ) spell_q( );
                lasthit_q( );
                break;
            case Orbwalker::EOrbwalkerMode::lasthit:
                lasthit_q( );
                break;
            case Orbwalker::EOrbwalkerMode::laneclear:
                if ( g_config->samira.q_harass->get< bool >( ) && !
                    is_position_in_turret_range( g_local->position ) )
                    spell_q( );

                if ( GetAsyncKeyState( VK_CONTROL ) ) laneclear_q( );
                else lasthit_q( );
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->samira.q_enabled->get< bool >( ) || m_is_dash || m_action_blocked || *g_time - m_last_q_time
                <= 0.4f || !m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            if ( g_features->orbwalker->is_attackable( target->index ) && !g_features->orbwalker->
                should_reset_aa( ) )
                return false;

            auto cast_position{ target->position };

            const auto ac_pred = g_features->prediction->predict_default( target->index, 0.25f );
            if ( !ac_pred ) return false;

            if ( target->dist_to_local( ) > m_melee_q_range || g_local->position.dist_to( *ac_pred ) >
                m_melee_q_range ) {
                auto pred = g_features->prediction->predict( target->index, m_q_range, 2600.f, 60.f, 0.25f, { }, true );
                if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->samira.q_hitchance->get<
                    int >( ) )
                    return false;

                if ( g_features->prediction->minion_in_line( g_local->position, pred.position, 70.f ) ) return false;

                cast_position = pred.position;
            }

            if ( cast_spell( ESpellSlot::q, cast_position ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->on_cast( );
                m_should_weave = g_config->samira.e_weave_aa->get< bool >( ) && m_grade <= 1;
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->samira.w_enabled->get< bool >( ) || m_action_blocked || *g_time - m_last_w_time <= 0.4f || !
                m_slot_w->is_ready( true )
                || g_config->samira.w_aa_reset->get< bool >( ) && !g_features->orbwalker->should_reset_aa( ) || m_grade
                < 4
                || m_grade == 6 )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) > m_w_range ) return false;

            if ( cast_spell( ESpellSlot::w ) ) {
                m_last_w_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->samira.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready(
                    true
                )
                || g_config->samira.e_weave_aa->get< bool >( ) && should_delay_spell( ) )
                return false;

            auto hud = g_pw_hud->get_hud_manager( );
            if ( !hud ) return false;

            const auto cursor = hud->cursor_position_unclipped;

            bool          target_found{ };
            Vec3          cast_position{ };
            const Object* target{ };
            auto          lowest_distance{ std::numeric_limits< float >::max( ) };

            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) || enemy->dist_to_local( ) > m_e_range ||
                    enemy->position.dist_to( cursor ) > g_config->samira.e_force_dash_range->get< int >( )
                    || g_features->target_selector->is_bad_target( enemy->index ) )
                    continue;

                const auto distance = enemy->position.dist_to( cursor );

                if ( !target_found || distance < lowest_distance ) {
                    target          = enemy;
                    cast_position   = enemy->position;
                    lowest_distance = distance;
                    target_found    = true;
                }
            }

            if ( !target_found || !target ) return false;

            if ( g_config->samira.e_mode->get< int >( ) > 0 ) {
                const auto predict = g_features->prediction->predict_default( target->index, 0.15f );
                if ( !predict ) return false;

                const auto after_dash_position = g_local->position.extend( cast_position, 650.f );

                if ( g_local->position.dist_to( *predict ) < after_dash_position.dist_to( *predict ) ) return false;
            }

            if ( cast_spell( ESpellSlot::e, cast_position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time = *g_time;

                m_last_dash_time = *g_time;
                m_is_dash        = true;
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->samira.r_enabled->get< bool >( ) || *g_time - m_last_r_time <= 0.4f || !m_slot_r->
                is_ready( )
                || !g_features->buff_cache->get_buff( g_local->index, ct_hash( "samirarreadybuff" ) ) )
                return false;

            int hit_count{ };
            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) || enemy->dist_to_local( ) > m_r_range
                    || g_features->target_selector->is_bad_target( enemy->index ) )
                    continue;


                ++hit_count;
            }

            if ( hit_count < g_config->samira.r_min_targets->get< int >( ) ) return false;

            if ( cast_spell( ESpellSlot::r ) ) {
                g_features->orbwalker->on_cast( );
                m_last_r_time = *g_time;
                return true;
            }

            return false;
        }

        auto lasthit_q( ) -> bool{
            if ( !g_config->samira.q_farm->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready(
                true
            ) )
                return false;

            const auto          lasthit_data = get_line_lasthit_target(
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                m_q_range,
                2600.f,
                0.25f,
                60.f,
                true
            );
            if ( !lasthit_data ) return false;

            if ( cast_spell( ESpellSlot::q, lasthit_data->cast_position ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->ignore_minion( lasthit_data->index, lasthit_data->travel_time + 0.5f );
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto laneclear_q( ) -> bool{
            if ( !g_config->samira.q_farm->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready(
                true
            ) )
                return false;

            const auto          laneclear_data = get_line_laneclear_target(
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                [this]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                m_q_range,
                60.f,
                true,
                false,
                true
            );
            if ( !laneclear_data ) return false;

            if ( cast_spell( ESpellSlot::q, laneclear_data->cast_position ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto should_delay_spell( ) const -> bool{
            return m_grade <= 2 && *g_time <= m_weave_end_time + g_features->orbwalker->get_ping( );
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
            {
                if ( target->dist_to_local( ) < 340.f ) return 0.25f;

                const auto tt   = 0.25f + g_local->position.dist_to( target->position ) / 2600.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.25f + g_local->position.dist_to( pred.value( ) ) / 2600.f;
            }
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

        auto update_dash( ) -> void{
            if ( !m_is_dash ) return;

            auto aimgr = g_local->get_ai_manager( );
            if ( !aimgr ) return;

            if ( !m_dash_verified ) {
                if ( *g_time - m_last_dash_time > 0.25f ) {
                    debug_log( "no dash detected, reset" );
                    m_is_dash = false;
                    return;
                }

                if ( !aimgr->is_moving || !aimgr->is_dashing ) return;

                m_dash_verified = true;
                debug_log( "dash found!" );
                return;
            }

            if ( !aimgr->is_moving || !aimgr->is_dashing ) {
                debug_log( "dash ended" );

                m_is_dash = false;
            }
        }

    private:
        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_r_time{ };

        bool m_action_blocked{ };

        int m_grade{ };

        // e tracking
        bool  m_is_dash{ };
        bool  m_dash_verified{ };
        float m_last_dash_time{ };

        // aa weaving
        bool  m_should_weave{ };
        float m_weave_end_time{ };

        std::vector< int32_t > m_handles{ };
        bool                   m_enemies_saved{ };

        std::vector< float > m_q_ad_ratio = { 0.f, 0.85f, 0.95f, 0.105f, 0.115f, 0.125f };
        std::vector< float > m_q_damage   = { 0.f, 0.f, 5.f, 10.f, 15.f, 20.f };
        std::vector< float > m_e_damage   = { 0.f, 80.f, 105.f, 130.f, 155.f, 180.f };
        std::vector< float > m_r_damage   = { 0.f, 80.f, 100.f, 120.f };

        float m_q_range{ 950.f };
        float m_w_range{ 400.f };
        float m_e_range{ 600.f };
        float m_r_range{ 600.f };

        float m_melee_q_range{ 340.f };
    };
}
