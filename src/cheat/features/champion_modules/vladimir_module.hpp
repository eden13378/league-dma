#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class vladimir_module final : public IModule {
    public:
        virtual ~vladimir_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "vladimir_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Vladimir" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "vladimir" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );

            q_settings->checkbox( _( "enable" ), g_config->vladimir.q_enabled );
            q_settings->checkbox( _( "use in harass" ), g_config->vladimir.q_harass );
            q_settings->checkbox( _( "use to lasthit" ), g_config->vladimir.q_harass );

            w_settings->checkbox( _( "enable" ), g_config->vladimir.w_enabled );

            e_settings->checkbox( _( "enable" ), g_config->vladimir.e_enabled );

            r_settings->checkbox( _( "enable" ), g_config->vladimir.r_enabled );
            r_settings->slider_int( "min targets", g_config->vladimir.r_min_targets, 1, 5, 1 );

            drawings->checkbox( _( "draw q range" ), g_config->vladimir.q_draw_range );
            drawings->checkbox( _( "draw w range" ), g_config->vladimir.e_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->vladimir.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->vladimir.q_draw_range->get< bool >( ) &&
                !g_config->vladimir.e_draw_range->get< bool >( ) ||
                g_local->is_dead( )
            )
                return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->vladimir.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->vladimir.dont_draw_on_cooldown->
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

            if ( g_config->vladimir.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->vladimir.dont_draw_on_cooldown->
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
        }

        auto run( ) -> void override{
            initialize_spell_slots( );
            //VladimirE
            m_in_pool = !!g_features->buff_cache->get_buff( g_local->index, ct_hash( "VladimirSanguinePool" ) );

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "VladimirE" ) ) ) {
                    spell_e( );
                    return;
                }
                if ( *g_time - m_e_cast_begin > 0.2f ) m_channeling_e = false;

                spell_r( );
                spell_e( );
                spell_q( );
            // spell_w();
                break;

            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->vladimir.q_harass->get< bool >( ) ) spell_q( );
                q_lasthit( );
                break;
            case Orbwalker::EOrbwalkerMode::laneclear:
                q_lasthit( );
                break;
            case Orbwalker::EOrbwalkerMode::lasthit:
                q_lasthit( );
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->vladimir.q_enabled->get< bool >( ) || m_in_pool || *g_time - m_last_q_time <= 0.4f || !
                m_slot_q->is_ready( ) || m_channeling_e )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) > m_q_range ) return false;

            if ( cast_spell( ESpellSlot::q, target->network_id ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{ return false; }

        auto spell_e( ) -> bool override{
            if ( !g_config->vladimir.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->
                is_ready( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );

            const auto buff = ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "VladimirE" ) ) );
            if ( !target ) return false;

            if ( !buff ) {
                if ( *g_time - m_e_cast_begin <= 0.2f || !m_slot_e->is_ready( true ) ) return false;
                const auto pred = g_features->prediction->predict(
                    target->index,
                    m_e_range,
                    4000.f,
                    50.f,
                    0.25f,
                    { },
                    true
                );
                m_channeling_e = true;

                if ( !pred.valid || g_features->prediction->minion_in_line( g_local->position, pred.position, 60.f ) ) {
                    m_channeling_e = false;
                    return false;
                }

                if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                    //g_features->orbwalker->set_cast_time(0.025f);
                    m_e_cast_begin = *g_time;
                    m_channeling_e = false;
                    g_features->orbwalker->on_cast( );
                    return true;
                }
            }

            auto pred = g_features->prediction->predict( target->index, m_e_range, 1900.f, 50.f, 0.25f, { }, true );
            if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->illaoi.e_hitchance->get< int >( )
                || g_features->prediction->minion_in_line( g_local->position, target->position, 90.f ) )
                return false;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                m_last_e_time  = *g_time;
                m_channeling_e = false;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->vladimir.r_enabled->get< bool >( ) || *g_time - m_last_r_time <= 0.4f || !m_slot_r->
                is_ready( true ) )
                return false;

            const auto multihit = get_multihit_position( m_r_range, 0.f, 375.f, 0.f, false );
            if ( multihit.hit_count >= g_config->vladimir.r_min_targets->get< int >( ) ) {
                if ( cast_spell( ESpellSlot::r, multihit.position ) ) {
                    m_last_r_time = *g_time;
                    g_features->orbwalker->on_cast( );
                    return true;
                }
            }

            return false;
        }

        auto q_lasthit( ) -> bool{
            if ( !g_config->vladimir.q_lasthit->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->
                is_ready( ) )
                return false;

            const auto          lasthit_data = get_targetable_lasthit_target(
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                m_q_range,
                [this]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); }
            );
            if ( !lasthit_data ) return false;

            if ( cast_spell( ESpellSlot::q, lasthit_data->network_id ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->ignore_minion( lasthit_data->index, 0.5f );
                return true;
            }
            return false;
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->ability_power( ) * 0.6f,
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
                const auto tt   = 0.125f + g_local->position.dist_to( target->position ) / 2500.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.125f + g_local->position.dist_to( pred.value( ) ) / 2500.f;
            }
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

        float m_e_cast_begin{ };

        std::vector< float > m_q_damage           = { 0.f, 80.f, 100.f, 120.f, 140.f, 160.f };
        std::vector< float > m_q_empowered_damage = { 0.f, 148.f, 185.f, 222.f, 259.f, 296.f };

        std::array< float, 6 > m_e_damage = { 0.f, 80.f, 105.f, 130.f, 155.f, 180.f };
        std::array< float, 6 > m_r_damage = { 0.f, 80.f, 100.f, 120.f };

        float m_q_range{ 600.f };
        float m_w_range{ 350.f };
        float m_e_range{ 665.f };
        float m_r_range{ 625.f };

        bool m_in_pool{ };
        bool m_channeling_e{ };
    };
}
