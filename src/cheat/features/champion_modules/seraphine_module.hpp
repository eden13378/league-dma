#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class seraphine_module final : public IModule {
    public:
        virtual ~seraphine_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "seraphine_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Seraphine" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "seraphine" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            // const auto w_settings = navigation->add_section(_("w settings"));
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->seraphine.q_enabled );
            q_settings->checkbox( _( "use in harass" ), g_config->seraphine.q_harass );
            q_settings->select(
                _( "hitchance" ),
                g_config->seraphine.q_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            e_settings->checkbox( _( "enable" ), g_config->seraphine.e_enabled );
            e_settings->select(
                _( "hitchance" ),
                g_config->seraphine.e_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            r_settings->checkbox( _( "enable" ), g_config->seraphine.r_enabled );
            r_settings->select(
                _( "hitchance" ),
                g_config->seraphine.r_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );
            r_settings->select(
                _( "mode" ),
                g_config->seraphine.r_mode,
                { _( "Single target" ), _( "Multihit" ), _( "Both" ) }
            );
            r_settings->slider_int( _( "min multihit count" ), g_config->seraphine.r_multihit_count, 2, 5 );

            drawings->checkbox( _( "draw q range" ), g_config->seraphine.q_draw_range );
            drawings->checkbox( _( "draw w range" ), g_config->seraphine.w_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->seraphine.e_draw_range );
            drawings->checkbox( _( "draw r range" ), g_config->seraphine.r_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->seraphine.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->seraphine.q_draw_range->get< bool >( ) &&
                !g_config->seraphine.w_draw_range->get< bool >( ) &&
                !g_config->seraphine.e_draw_range->get< bool >( ) &&
                !g_config->seraphine.r_draw_range->get< bool >( ) ||
                g_local->is_dead( )
            )
                return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->seraphine.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->seraphine.dont_draw_on_cooldown->
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

            if ( g_config->seraphine.w_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->seraphine.dont_draw_on_cooldown->
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

            if ( g_config->seraphine.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->seraphine.dont_draw_on_cooldown->
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

            if ( g_config->seraphine.r_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::r );
                if ( slot && slot->level > 0 && ( slot->is_ready( ) || !g_config->seraphine.dont_draw_on_cooldown->get<
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
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return;

            m_double_cast = !!g_features->buff_cache->get_buff(
                g_local->index,
                ct_hash( "SeraphinePassiveEchoStage2" )
            );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_e( );
                spell_q( );
                spell_r( );
                break;

            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->seraphine.q_harass->get< bool >( ) ) spell_q( );
                break;
            default: ;
            }

            //SeraphinePassiveEchoStage2
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->seraphine.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->
                is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) > m_q_range + 50.f ) return false;

            auto pred = g_features->prediction->predict(
                target->index,
                m_q_range,
                1200.f,
                m_double_cast ? 30.f : 60.f,
                m_double_cast ? 0.5f : 0.25f
            );

            if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->seraphine.q_hitchance->get<
                int >( ) )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.25f );
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{ return false; }

        auto spell_e( ) -> bool override{
            if ( !g_config->seraphine.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->
                is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto pred = g_features->prediction->predict(
                target->index,
                m_e_range,
                1200.f,
                70.f,
                0.25f,
                { },
                m_double_cast ? false : true
            );
            if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->seraphine.e_hitchance->get<
                int >( ) )
                return false;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                m_last_e_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.25f );
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->seraphine.r_enabled->get< bool >( ) || *g_time - m_last_r_time <= 0.4f || !m_slot_r->
                is_ready( true ) )
                return false;

            if ( g_config->seraphine.r_mode->get< int >( ) > 0 ) {
                const auto multihit = get_multihit_position( m_r_range, 1200.f, 160.f, 0.5f, true );

                if ( multihit.hit_count > g_config->seraphine.r_multihit_count->get< int >( ) - 1 ) {
                    if ( cast_spell( ESpellSlot::r, multihit.position ) ) {
                        g_features->orbwalker->set_cast_time( 0.5f );
                        m_last_r_time = *g_time;
                        return true;
                    }
                }
            }

            if ( g_config->seraphine.r_mode->get< int >( ) == 1 ) return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto pred = g_features->prediction->predict( target->index, m_r_range, 1200.f, 160.f, 0.5f, { }, true );
            if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->seraphine.r_hitchance->get<
                int >( ) )
                return false;

            if ( cast_spell( ESpellSlot::r, pred.position ) ) {
                m_last_r_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.5f );
                return true;
            }

            return false;
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

        bool m_double_cast{ };

        std::vector< float >   m_q_ad_ratio = { 0.f, 0.85f, 0.95f, 0.105f, 0.115f, 0.125f };
        std::array< float, 6 > m_q_damage   = { 0.f, 0.f, 5.f, 10.f, 15.f, 20.f };
        std::array< float, 6 > m_e_damage   = { 0.f, 80.f, 105.f, 130.f, 155.f, 180.f };
        std::array< float, 6 > m_r_damage   = { 0.f, 80.f, 100.f, 120.f };

        float m_q_range{ 900.f };
        float m_w_range{ 800.f };
        float m_e_range{ 1300.f };
        float m_r_range{ 1200.f };
    };
}
