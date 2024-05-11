#pragma once
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"
#include "module.hpp"

namespace features::champion_modules {
    class aurelion_module final : public IModule {
    public:
        virtual ~aurelion_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "aurelion_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "AurelionSol" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation     = g_window->push( _( "aurelion sol" ), menu_order::champion_module );
            const auto q_settings     = navigation->add_section( _( "q settings" ) );
            const auto drawings       = navigation->add_section( _( "drawings" ) );
            const auto w_settings     = navigation->add_section( _( "w settings" ) );
            const auto e_settings     = navigation->add_section( _( "e settings" ) );
            const auto r_settings     = navigation->add_section( _( "r settings" ) );
            const auto combo_settings = navigation->add_section( _( "combo logic" ) );

            q_settings->checkbox( _( "enable" ), g_config->aurelion.q_enabled );
            q_settings->checkbox( _( "harass q" ), g_config->aurelion.q_harass );
            q_settings->slider_int( _( "max range % to start channel" ), g_config->aurelion.q_max_range, 25, 100, 1 );

            w_settings->checkbox( _( "enable" ), g_config->aurelion.w_enabled );

            e_settings->checkbox( _( "enable" ), g_config->aurelion.e_enabled );
            e_settings->checkbox( _( "flee e" ), g_config->aurelion.e_flee );
            e_settings->checkbox( _( "killsteal e" ), g_config->aurelion.e_flee );
            e_settings->checkbox( _( "antigapclose" ), g_config->aurelion.e_antigapclose );
            e_settings->select(
                _( "hitchance" ),
                g_config->aurelion.e_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            r_settings->checkbox( _( "enable" ), g_config->aurelion.r_enabled );
            r_settings->slider_int( _( "min multihit count" ), g_config->aurelion.r_min_multihit_count, 1, 5, 1 );
            r_settings->select(
                _( "hitchance" ),
                g_config->aurelion.r_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            // special logic
            combo_settings->select(
                _( "prefer e over q" ),
                g_config->aurelion.prefer_e_over_q,
                { _( "Never" ), _( "In full combo" ), _( "Always" ) }
            );

            drawings->checkbox( _( "draw q range" ), g_config->aurelion.q_draw_range );
            drawings->checkbox( _( "draw w range" ), g_config->aurelion.w_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->aurelion.e_draw_range );
            drawings->checkbox( _( "draw r range" ), g_config->aurelion.r_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->aurelion.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->aurelion.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) || !g_config->aurelion.dont_draw_on_cooldown->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 31, 88, 255, 255 ),
                        m_q_range,
                        Renderer::outline,
                        60,
                        3.f
                    );
                }
            }

            if ( g_config->aurelion.w_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) || !g_config->aurelion.dont_draw_on_cooldown->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 132, 245, 66, 255 ),
                        m_w_range,
                        Renderer::outline,
                        80,
                        3.f
                    );
                }
            }

            if ( g_config->aurelion.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) || !g_config->aurelion.dont_draw_on_cooldown->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 144, 66, 245, 255 ),
                        m_e_range,
                        Renderer::outline,
                        60,
                        3.f
                    );
                }
            }

            if ( g_config->aurelion.r_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::r );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) || !g_config->aurelion.dont_draw_on_cooldown->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 173, 47, 68, 255 ),
                        m_r_range,
                        Renderer::outline,
                        60,
                        3.f
                    );
                }
            }

            if ( m_e_active ) {
                const auto time_left       = m_singularity_end_time - *g_time;
                const auto start_time      = m_e_server_cast_time - 0.25f;
                const auto modifier        = std::min( ( *g_time - start_time ) / 0.8f, 1.f );
                auto       post_modifier   = std::min( ( *g_time - start_time - 0.8f ) / 1.f, 1.f );
                auto       ending_modifier = std::min( ( time_left - 0.3f ) / 0.15f, 1.f );
                if ( post_modifier < 0.f ) post_modifier = 0.f;
                if ( ending_modifier < 0.f ) ending_modifier = 0.f;

                const auto outer_radius = m_e_radius - ( time_left > 0.45f
                                                             ? 0.f
                                                             : m_e_radius / 3.f * ( 1.f - ending_modifier ) );

                if ( modifier == 1.f && post_modifier < 1.f ) {
                    g_render->circle_3d(
                        m_singularity_position,
                        Color(
                            255.f,
                            255.f,
                            255.f,
                            255.f - 255.f * post_modifier
                        ),
                        m_e_radius + 45.f * post_modifier,
                        Renderer::outline,
                        50,
                        6.f
                    );
                }


                g_render->circle_3d(
                    m_singularity_position,
                    Color( 255.f, 255.f, 255.f, 255.f * ending_modifier ),
                    outer_radius,
                    Renderer::outline,
                    50,
                    6.f,
                    360.f * modifier
                );

                /* g_render->circle_3d( m_singularity_position,
                                     color( 255, 25, 50, 80 ),
                                     time_left > 1.f ? m_e_execute_radius * modifier : inner_radius,
                                     c_renderer::outline | c_renderer::filled,
                                     32,
                                     4.f );*/


                if ( *g_time - start_time > 0.8f ) {
                    auto text = std::to_string( time_left );
                    text.resize( 3 );

                    text += "s";

                    g_render->text_3d(
                        m_singularity_position,
                        Color( 255.f, 255.f, 255.f, 255.f * ending_modifier ),
                        g_fonts->get_zabel( ),
                        text.data( ),
                        32.f * ending_modifier,
                        true
                    );
                }
            }


            //if ( !m_channeling_q ) return;
        }

        auto run( ) -> void override{
            initialize_spell_slots( );


            cast_tracker( );

            m_channeling_q = !!g_features->buff_cache->get_buff( g_local->index, ct_hash( "AurelionSolQ" ) );

            m_q_range = 740.f + 10.f * g_local->level + 100.f;
            m_e_range = 740.f + 10.f * g_local->level;

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return;

            antigapclose_e( );
            if ( m_channeling_q ) {
                if ( m_is_released ) release_q( );
                else spell_q( );
            } else m_is_released = false;

            if ( g_features->orbwalker->in_attack( ) ) return;

            killsteal_e( );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_e( );
                spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->aurelion.q_harass->get< bool >( ) ) spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::flee:
                if ( g_config->aurelion.e_flee->get< bool >( ) ) spell_e( );
                break;
            default:
                break;
            }

            if ( m_chargeable_blocked != g_function_caller->is_update_chargeable_blocked( ) ) {
                g_function_caller->set_update_chargeable_blocked( m_chargeable_blocked );

                std::cout << "[ ASOL ] Set chargeable blockstate externally to " << ( m_chargeable_blocked
                    ? "BLOCKED "
                    : "UNBLOCKED" ) << std::endl;
            }


            const auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "AurelionSolPassive" ) );
            if ( buff ) m_stacks = buff->stacks( );


            m_e_radius         = sqrt( pow( 275.f, 2.f ) + pow( 16.93f, 2.f ) * m_stacks );
            m_e_execute_radius = sqrt( pow( 137.5f, 2.f ) + pow( 8.46f, 2.f ) * m_stacks );

            // AurelionSolWToggle - spell name


            // AurelionSolPassive  - stack count buff
            // AurelionSolW - W active buff
            // AurelionSolE - E active buff
            // AurelionSolR - R active buff

            // aurelionsolqwanimation - buff when channeling Q in W

            // AurelionSolEDebuff - enemy in e buff

            //AurelionSolR2 - empowered r
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->aurelion.q_enabled->get< bool >( ) ) return false;

            if ( m_channeling_q ) {
                if ( m_is_released ) return false;

                auto should_release{ true };

                switch ( g_features->orbwalker->get_mode( ) ) {
                case Orbwalker::EOrbwalkerMode::harass:
                    if ( !g_config->aurelion.q_harass->get< bool >( ) ) break;

                case Orbwalker::EOrbwalkerMode::combo:
                    should_release = false;
                    break;
                default:
                    break;
                }

                if ( should_release ) {
                    release_q( );
                    return false;
                }

                if ( m_q_target_index == 0 ) {
                    const auto target = g_features->target_selector->get_default_target( );
                    if ( !target || target->dist_to_local( ) > m_q_range ) return false;

                    m_q_target_index = target->index;
                }


                const auto target = g_entity_list.get_by_index( m_q_target_index );
                if ( !target || target->dist_to_local( ) > m_q_range || g_features->target_selector->is_bad_target(
                    target->index
                ) ) {
                    release_q( );
                    return false;
                }

                const auto pred = g_features->prediction->predict( target->index, m_q_range, 0.f, 25.f, 0.15f );
                if ( !pred.valid ) return false;


                should_release = g_local->position.dist_to( pred.position ) > m_q_range || g_features->prediction->
                    minion_in_line( g_local->position, pred.position, 40.f );
                if ( should_release ) {
                    release_q( );
                    return false;
                }

                if ( *g_time - m_last_cast_time < 0.05f ) return false;

                if ( !g_function_caller->is_update_chargeable_blocked( ) ) {
                    m_chargeable_blocked = true;
                    g_function_caller->set_update_chargeable_blocked( true );
                    return false;
                }

                if ( release_chargeable( ESpellSlot::q, pred.position, false ) ) {
                    m_last_cast_time        = *g_time;
                    m_last_channel_position = pred.position;

                    std::cout << "[ ASOL ] Updated Q target position | " << *g_time << std::endl;

                    return true;
                }

                return false;
            }

            if ( *g_time - m_last_cast_time <= 0.1f || *g_time - m_last_q_time <= 0.25f || !m_slot_q->
                is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            if ( !m_e_active && g_config->aurelion.prefer_e_over_q->get< int >( ) > 0
                && g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo && g_config->aurelion.
                e_enabled->get< bool >( ) && m_slot_e->is_ready( true ) ) {
                switch ( g_config->aurelion.prefer_e_over_q->get< int >( ) ) {
                case 1:
                {
                    if ( GetAsyncKeyState( VK_CONTROL ) ) return false;

                    break;
                }
                case 2:
                    return false;
                default:
                    break;
                }
            }

            const auto pred = g_features->prediction->predict(
                target->index,
                m_q_range * ( g_config->aurelion.q_max_range->get< int >( ) / 100.f ),
                0.f,
                25.f,
                0.15f
            );
            if ( !pred.valid || g_local->position.dist_to( pred.position ) > m_q_range || g_features->prediction->
                minion_in_line( g_local->position, pred.position, 50.f ) )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                m_q_target_index = target->index;

                std::cout << "[ Q ] Start channeling towards " << target->champion_name.text << std::endl;

                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{ return false; }

        auto spell_e( ) -> bool override{
            if ( !g_config->aurelion.e_enabled->get< bool >( ) || m_channeling_q || *g_time - m_last_e_time <= 0.4f || *
                g_time - m_last_cast_time <= 0.1f ||
                !m_slot_e->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_e_range, 0.f, m_e_radius, 0.75f );
            if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->aurelion.e_hitchance->get<
                int >( ) )
                return false;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->aurelion.r_enabled->get< bool >( ) || *g_time - m_last_r_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.1f || !m_slot_r->is_ready( true ) )
                return false;

            int enemy_count{ };

            if ( cast_spell( ESpellSlot::r ) ) {
                m_last_r_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.25f );
                return true;
            }

            return false;
        }

        auto antigapclose_e( ) -> void{
            if ( !g_config->aurelion.e_antigapclose->get< bool >( ) || *g_time - m_last_e_time <= 0.4f ||
                !m_slot_e->is_ready( true ) )
                return;

            const auto target = get_advanced_antigapclose_target( m_e_range, 0.f, m_e_radius, 0.75f );
            if ( !target ) return;

            const auto pred = g_features->prediction->predict( target->index, m_e_range, 0.f, m_e_radius, 0.75f );
            if ( !pred.valid || ( int )pred.hitchance < 3 ) return;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
            }
        }

        auto release_q( ) -> void{
            if ( *g_time - m_last_q_time <= 0.15f || GetAsyncKeyState( 0x51 ) ) return;


            if ( release_chargeable( ESpellSlot::q, Vec3( ), true ) ) {
                m_last_q_time        = *g_time;
                m_last_cast_time     = *g_time;
                m_chargeable_blocked = false;
                m_is_released        = true;

                std::cout << "[ Q ] Released channel | " << *g_time << std::endl;

                g_function_caller->set_update_chargeable_blocked( false );
            }
        }

        auto killsteal_e( ) -> bool{
            if ( !g_config->aurelion.e_killsteal->get< bool >( ) || m_e_active || *g_time - m_last_e_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.1f || !m_slot_e->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_killsteal_target(
                m_e_range,
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::e, unit ); },
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::e, unit ); }
            );
            if ( !target ) return false;

            const auto will_kill = get_spell_damage( ESpellSlot::e, target ) > helper::get_real_health(
                target->index,
                EDamageType::true_damage
            );
            if ( !will_kill ) return false;

            const auto predicted = g_features->prediction->predict(
                target->index,
                m_e_range,
                0.f,
                m_e_execute_radius,
                0.75f
            );

            if ( !predicted.valid || predicted.hitchance < static_cast< Prediction::EHitchance >( g_config->aurelion.
                e_hitchance->get< int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::e, predicted.position ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                std::cout << "[ KS E ] Order cast on " << target->champion_name.text << std::endl;
                return true;
            }

            return false;
        }

        auto cast_tracker( ) -> void{
            if ( !m_e_active ) {
                auto sci = g_local->spell_book.get_spell_cast_info( );
                if ( !sci || sci->slot != 2 || sci->server_cast_time < *g_time ) return;

                m_e_active             = true;
                m_e_server_cast_time   = sci->server_cast_time;
                m_singularity_end_time = sci->server_cast_time + 5.5f;
                m_singularity_position = sci->end_position;
            }

            if ( *g_time < m_singularity_end_time ) return;

            m_e_active = false;
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::e:
            {
                const auto execute_percent = 0.05f + m_stacks * 0.026f / 100.f;

                const auto execute_damage = target->max_health * execute_percent;
                const auto current_health = helper::get_real_health( target->index, EDamageType::true_damage );

                return current_health <= execute_damage ? 99999.f : 0.f;
            }
            default:
                return 0.f;
            }
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::e: { return 0.75f; }
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
        float m_last_cast_time{ };

        std::vector< float > m_q_damage{ 0.f, 80.f, 135.f, 190.f, 245.f, 300.f };
        std::vector< float > m_r_damage{ 0.f, 150.f, 225.f, 300.f };

        // q logic
        bool    m_channeling_q{ };
        int16_t m_q_target_index{ };
        bool    m_is_released{ };

        Vec3 m_last_channel_position{ };
        bool m_chargeable_blocked{ };

        // e logic
        bool  m_e_active{ };
        float m_e_server_cast_time{ };
        float m_singularity_end_time{ };
        Vec3  m_singularity_position{ };



        // stacks
        int m_stacks{ };



        float m_q_range{ 740.f };
        float m_w_range{ 1200.f };
        float m_e_range{ 740.f };
        float m_r_range{ 1250.f };

        float m_e_radius{ 275.f };
        float m_e_execute_radius{ 137.5f };
    };
}
