#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class sion_module final : public IModule {
    public:
        virtual ~sion_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "sion_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Sion" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "sion" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->sion.q_enabled );
            q_settings->select(
                _( "hitchance" ),
                g_config->sion.q_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            w_settings->checkbox( _( "enable" ), g_config->sion.w_enabled );

            e_settings->checkbox( _( "enable" ), g_config->sion.e_enabled );
            e_settings->select(
                _( "hitchance" ),
                g_config->sion.e_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            r_settings->checkbox( _( "enable exploit" ), g_config->sion.r_enabled );

            drawings->checkbox( _( "draw q range" ), g_config->sion.q_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->sion.e_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->sion.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->sion.q_draw_range->get< bool >( ) &&
                !g_config->sion.e_draw_range->get< bool >( ) ||
                g_local->is_dead( ) )
                return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };


            if ( g_config->sion.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->sion.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 31, 88, 255, 255 ),
                        750.f,
                        Renderer::outline,
                        60,
                        2.f
                    );
                }
            }

            if ( g_config->sion.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->sion.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
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

            auto sci = g_local->spell_book.get_spell_cast_info( );
            if ( !sci || sci->slot != 0 ) return;

            const auto rect = sdk::math::Rectangle(
                g_local->position,
                sci->start_position.extend( sci->end_position, 700.f ),
                175.f
            );
            const auto polygon = rect.to_polygon( );

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

                g_render->line( sp_start, sp_end, Color( 255, 255, 255 ), 1.f );
            }
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            if ( g_features->orbwalker->in_attack( ) || g_features->evade->is_active( ) || g_features->orbwalker->
                in_action( ) )
                return;

            auto sci = g_local->spell_book.get_spell_cast_info( );

            if ( sci && sci->slot == 0 ) {
                spell_q( );
                return;
            }

            if ( *g_time - m_q_cast_begin > 0.2f ) m_channeling_q = false;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                if ( spell_r( ) ) return;

                spell_q( );
                spell_w( );
                spell_e( );
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->sion.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->
                is_ready( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            //auto buff = g_features->buff_cache->get_buff(g_local->index, ct_hash("SionQ"));

            auto sci = g_local->spell_book.get_spell_cast_info( );

            if ( !sci ) {
                if ( *g_time - m_q_cast_begin <= 0.2f || !m_slot_q->is_ready( true ) ) return false;

                const auto pred = g_features->prediction->predict( target->index, 600.f, 0.f, 0.f, 0.5f );

                if ( !pred.valid ) {
                    m_channeling_q = false;
                    return false;
                }

                if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                    m_q_cast_begin = *g_time;
                    m_channeling_q = true;
                    return false;
                }
            } else {
                if ( sci->slot != 0 ) {
                    m_q_cast_begin = 0.f;
                    m_channeling_q = false;
                    return false;
                }

                m_q_cast_begin = 0.f;
                m_channeling_q = true;

                auto bonus_range{ 0.f };
                int  charge_amount{ };
                if ( *g_time - sci->start_time > 0.25f ) {
                    charge_amount = static_cast< int32_t >( std::floor(
                        ( *g_time - sci->start_time ) / 0.25f
                    ) );
                }

                if ( charge_amount == 1 ) bonus_range = 100.f;
                else if ( charge_amount == 2 ) bonus_range = 175.f;
                else if ( charge_amount >= 3 ) bonus_range = 275.f;

                m_q_range = m_q_base_range + bonus_range;

                const auto rect = sdk::math::Rectangle(
                    g_local->position,
                    g_local->position.extend( sci->end_position, m_q_range ),
                    160.f
                );
                auto polygon = rect.to_polygon( );

                auto prediction_amount{ 0.025f };

                auto mgr = target->get_ai_manager( );
                if ( !mgr ) return false;

                if ( mgr->is_moving && mgr->is_dashing ) prediction_amount = 1.f;

                const auto pred = g_features->prediction->predict_default( target->index, prediction_amount );
                if ( !pred ) return false;

                if ( polygon.is_inside( target->position ) && !polygon.is_inside( *pred ) ) {
                    if ( release_chargeable( ESpellSlot::q, target->position ) ) {
                        m_last_q_time  = *g_time;
                        m_q_range      = 0.f;
                        m_channeling_q = false;
                    }
                }
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->sion.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready(
                true
            ) )
                return false;

            auto info = m_slot_w->get_spell_info( );
            if ( !info ) return false;

            auto data = info->get_spell_data( );
            if ( !data || rt_hash( data->get_name().c_str() ) == ct_hash( "SionWDetonate" ) ) return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto sci = target->spell_book.get_spell_cast_info( );
            if ( !sci || sci->get_target_index( ) != g_local->index ) return false;

            if ( cast_spell( ESpellSlot::w ) ) {
                m_last_w_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->sion.e_enabled->get< bool >( ) || m_channeling_q || *g_time - m_last_e_time <= 0.4f || !
                m_slot_e->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_e_range,
                1800.f,
                80.f,
                0.25f,
                { },
                true
            );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->sion.e_hitchance->get
                < int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                m_last_e_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->sion.r_enabled->get< bool >( ) ) return false;

            auto sci = g_local->spell_book.get_spell_cast_info( );
            if ( !sci || sci->slot != 3 ) return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            g_input->issue_order_attack( target->network_id );

            return true;
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->ability_power( ) * 0.4f,
                    target->index,
                    false
                );
            case ESpellSlot::e:
                return helper::calculate_damage(
                    m_e_damage[ get_slot_e( )->level ] + g_local->ability_power( ) * 0.3f,
                    target->index,
                    false
                );
            }
            return 0.f;
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return g_features->orbwalker->get_ping( );
            case ESpellSlot::e:
            {
                const auto tt   = 0.25f + g_local->position.dist_to( target->position ) / 1750.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.25f + g_local->position.dist_to( pred.value( ) ) / 1750.f;
            }
            }

            return std::numeric_limits< float >::max( );
        }

    private:
        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_r_time{ };

        float m_q_cast_begin{ };
        bool  m_channeling_q{ };

        Vec3 m_cast_direction{ };

        std::array< float, 6 > m_q_damage = { 0.f, 65.f, 90.f, 115.f, 140.f, 165.f };
        std::array< float, 6 > m_e_damage = { 0.f, 50.f, 70.f, 90.f, 110.f, 130.f };

        float m_q_base_range{ 500.f };
        float m_q_range{ 0.f };
        float m_w_range{ 0.f };
        float m_e_range{ 800.f };
    };
}
