#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class zed_module final : public IModule {
    public:
        virtual ~zed_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "zed_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Zed" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell, e_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "zed" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "Q settings" ) );
            const auto drawings   = navigation->add_section( _( "Drawings" ) );
            const auto w_settings = navigation->add_section( _( "W settings" ) );
            const auto e_settings = navigation->add_section( _( "E settings" ) );

            q_settings->checkbox( _( "Enable" ), g_config->zed.q_enabled );
            q_settings->checkbox( _( "Use in harass" ), g_config->zed.q_harass );
            q_settings->checkbox( _( "Q Lasthit" ), g_config->zed.q_lasthit );
            q_settings->select(
                _( "Shuriken hitchance" ),
                g_config->zed.q_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );


            w_settings->checkbox( _( "Enable" ), g_config->zed.w_enabled );
            w_settings->checkbox( _( "Only use in full combo" ), g_config->zed.w_full_combo_only );

            e_settings->checkbox( _( "Enable" ), g_config->zed.e_enabled );

            drawings->checkbox( _( "Draw q range" ), g_config->zed.q_draw_range );
            drawings->checkbox( _( "Draw w range" ), g_config->zed.w_draw_set );
            drawings->checkbox( _( "Draw r range" ), g_config->zed.r_draw_range );
            drawings->checkbox( _( "Only draw off cooldown" ), g_config->zed.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };
            //draw Q range
            if ( g_config->zed.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->zed.dont_draw_on_cooldown->get<
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

            //draw W range, default to off
            if ( g_config->zed.w_draw_set->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->zed.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 132, 245, 66, 255 ),
                        m_w_range,
                        Renderer::outline,
                        60,
                        2.f
                    );
                }
            }

            //draw R range
            if ( g_config->zed.r_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::r );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->zed.dont_draw_on_cooldown->get<
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
        }

        auto run( ) -> void override{
            m_shadows.clear( );
            update_shadows( );
            initialize_spell_slots( );

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_r( );
                spell_w( );
                spell_e( );
                spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->zed.q_harass->get< bool >( ) ) spell_q( );
            //lasthit_q();
                break;
            case Orbwalker::EOrbwalkerMode::lasthit:
            case Orbwalker::EOrbwalkerMode::laneclear:
                lasthit_q( );
                break;
            default:
                break;
            }
        }

    private:
        //goes to check for all shadows currently placed, and will update vector accordingly
        auto update_shadows( ) -> void{
            for ( const auto obj : g_entity_list.get_ally_minions( ) ) {
                if ( !obj || obj->dist_to_local( ) > 1500.f || obj->is_dead( ) ) continue;
                const auto hashed_name = rt_hash( obj->get_name().data() );
                if ( hashed_name == ct_hash( "ZedShadow" ) ) {
                    bool skip{ };
                    for ( const auto index : m_shadows ) {
                        if ( index == obj->index ) {
                            skip = true;
                            break;
                        }
                    }

                    if ( skip ) continue;

                    m_shadows.push_back( obj->index );
                }
            }
        }

        static auto GetRTarget( auto* result ) -> bool{
            const auto enemies  = g_entity_list.get_enemies( );
            auto       has_buff = false;
            for ( const auto enemy : enemies ) {
                if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) ) continue;
                has_buff = !!g_features->buff_cache->get_buff( enemy->index, ct_hash( "zedrtargetmark" ) );
                if ( has_buff ) {
                    debug_log( "R has been located on target" );
                    *result = enemy->index;
                    return true;
                }
            }
            *result = 0;
            return false;
        }



        auto spell_q( ) -> bool override{
            ///zedrtargetmark buff name
            auto count = 0; //DEBUG
            if ( !g_config->zed.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || *g_time -
                m_last_cast_time <= 0.05f || !m_slot_q->is_ready( true ) )
                return false;

            const Object* target = { };

            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || g_features->target_selector->is_bad_target( enemy->index ) || !g_features->buff_cache->
                    get_buff( enemy->index, ct_hash( "zedrtargetmark" ) ) )
                    continue;

                target = enemy;
                break;
            }

            if ( !target ) target = g_features->target_selector->get_default_target( );

            if ( !target ) return false;

            float shadow_distance{ };
            bool  valid_shadow_pred{ };
            Vec3  shadow_pred{ };
            Vec3  cast_position{ };

            if ( !m_shadows.empty( ) ) {
                for ( const auto index : m_shadows ) {
                    const auto& shadow = g_entity_list.get_by_index( index );
                    if ( !shadow ) continue;
                    auto pred = g_features->prediction->predict(
                        target->index,
                        m_q_range,
                        1700.f,
                        50.f,
                        0.25f,
                        shadow->position
                    );
                    if ( !pred.valid || ( int )pred.hitchance < g_config->zed.q_hitchance->get< int >( ) ) continue;
                    if ( pred.position.dist_to( shadow->position ) < shadow_distance ) continue;
                    shadow_distance   = pred.position.dist_to( shadow->position );
                    shadow_pred       = pred.position;
                    valid_shadow_pred = true;
                }
            }

            auto pred = g_features->prediction->predict( target->index, m_q_range, 1700.f, 50.f, 0.25f );
            if ( !valid_shadow_pred && ( !pred.valid || ( int )pred.hitchance < g_config->zed.q_hitchance->get<
                int >( ) ) )
                return false;

            if ( valid_shadow_pred && ( !pred.valid || pred.position.dist_to( g_local->position ) <
                shadow_distance ) )
                cast_position = shadow_pred;
            else if ( pred.valid ) cast_position = pred.position;
            else return false;

            if ( cast_spell( ESpellSlot::q, cast_position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
            }

            return false;
        }


        auto spell_w( ) -> bool override{
            const auto is_fullcombo = GetAsyncKeyState( VK_CONTROL ) && g_features->orbwalker->get_mode( ) ==
                Orbwalker::EOrbwalkerMode::combo;
            if ( !g_config->zed.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || *g_time -
                m_last_cast_time <= 0.05f || !m_slot_w->is_ready( true ) || ( g_config->zed.w_full_combo_only->get<
                    bool >( ) && !is_fullcombo ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;


            if ( m_slot_w->get_name( ).find( _( "ZedW2" ) ) == std::string::npos ) {
                const auto predicted = g_features->prediction->predict(
                    target->index,
                    m_w_range,
                    m_w_speed,
                    m_e_range,
                    0
                );

                if ( !predicted.valid || predicted.hitchance < static_cast< Prediction::EHitchance >( 2 ) ) {
                    return
                        false;
                }

                if ( cast_spell( ESpellSlot::w, predicted.position ) ) {
                    m_last_w_time    = *g_time;
                    m_last_cast_time = *g_time;
                    return true;
                }
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->zed.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || *g_time -
                m_last_cast_time <= 0.05f || !m_slot_e->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict_default(
                target->index,
                g_features->orbwalker->get_ping( )
            );
            if ( !pred ) return false;

            auto allow_e{ g_local->position.dist_to( pred.value( ) ) < m_e_range };

            if ( !allow_e && !m_shadows.empty( ) ) {
                for ( const auto index : m_shadows ) {
                    const auto& shadow = g_entity_list.get_by_index( index );
                    if ( !shadow ) continue;

                    allow_e = shadow->position.dist_to( *pred ) < m_e_range;

                    if ( allow_e ) break;
                }
            }

            if ( !allow_e ) return false;

            if ( cast_spell( ESpellSlot::e ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
            }

            return false;
        }

        auto spell_r( ) -> bool override{ return false; }

        auto lasthit_q( ) -> bool{
            if ( !g_config->zed.q_lasthit->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready(
                true
            ) )
                return false;

            const auto          lasthit_data = get_line_lasthit_target(
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                m_q_range,
                1700.f,
                0.25f,
                80.f,
                true
            );
            if ( !lasthit_data ) return false;

            if ( cast_spell( ESpellSlot::q, lasthit_data->cast_position ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->ignore_minion( lasthit_data->index, lasthit_data->travel_time );
                return true;
            }

            return false;
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->bonus_attack_damage( ) * 1.1f,
                    target->index,
                    true
                );
            case ESpellSlot::e:
                return helper::calculate_damage(
                    m_e_damage[ get_slot_e( )->level ] + g_local->ability_power( ) * 0.5f,
                    target->index,
                    false
                );
            default:
                return 0.f;
            }
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
            {
                const auto tt   = 0.25f + g_local->position.dist_to( target->position ) / 1700.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.25f + g_local->position.dist_to( pred.value( ) ) / 1700.f;
            }
            case ESpellSlot::e:
            {
                const auto tt   = 0.25f + g_local->position.dist_to( target->position ) / 1150.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.25f + g_local->position.dist_to( pred.value( ) ) / 1150.f;
            }
            default:
                return 0.f;
            }

            return std::numeric_limits< float >::max( );
        }

    private:
        std::vector< int16_t > m_shadows{ };

        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_r_time{ };
        float m_last_cast_time{ };

        std::array< float, 6 > m_q_damage{ 0.f, 80.f, 115.f, 150.f, 185.f, 220.f };
        std::array< float, 6 > m_e_damage{ 0.f, 70.f, 90.f, 110.f, 130.f, 150.f };

        float m_q_range{ 925.f };
        float m_q_width{ 50.f };
        float m_q_speed{ 1700.f };
        float m_q_cast_time{ 0.25f };

        float m_w_range{ 650.f };
        float m_w_speed{ 1750.f };

        float m_e_range{ 290.f };

        float m_r_range{ 625.f };

        bool m_tornado_q{ };
    };
}
