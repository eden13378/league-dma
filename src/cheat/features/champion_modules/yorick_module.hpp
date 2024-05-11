#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class yorick_module final : public IModule {
    public:
        virtual ~yorick_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "yorick_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Yorick" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell, e_spell, r_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "yorick" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( ( "e settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->yorick.q_enabled );
            q_settings->checkbox( _( "enable q AA reset" ), g_config->yorick.q_AA_reset );
            q_settings->checkbox( _( "enable q3 laneclear" ), g_config->yorick.q_lasthit );

            w_settings->checkbox( _( "enable" ), g_config->yorick.w_enabled );
            w_settings->select(
                _( "hitchance" ),
                g_config->yorick.w_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            e_settings->checkbox( _( "enable" ), g_config->yorick.e_enabled );
            e_settings->select(
                _( "hitchance" ),
                g_config->yorick.e_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            r_settings->checkbox( _( "use in fast clear" ), g_config->yorick.r_enabled );

            drawings->checkbox( _( "draw w range" ), g_config->yorick.w_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->yorick.e_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->yorick.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            //draw w range
            if ( g_config->yorick.w_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->yorick.dont_draw_on_cooldown->get
                    < bool >( ) ) ) {
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

            //draw e range
            if ( g_config->yorick.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->yorick.dont_draw_on_cooldown->get
                    < bool >( ) ) ) {
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
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return;

            auto spell_info = m_slot_q->get_spell_info( );
            if ( !spell_info ) return;

            const auto spell_data = spell_info->get_spell_data( );
            if ( !spell_data ) return;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:
                spell_q( );
                spell_w( );
                spell_e( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                spell_e( );
                break;
            case Orbwalker::EOrbwalkerMode::laneclear:
                laneclear_q( );
                spell_r( );
                break;
            default: ;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->yorick.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready(
                true
            ) )
                return false;

            //mist walker usage here
            if ( m_slot_q->get_name( ) == "YorickQ2" ) {
                if ( cast_spell( ESpellSlot::q ) ) {
                    m_last_q_time = *g_time;
                    return true;
                }
            }

            if ( g_config->yorick.q_AA_reset->get< bool >( ) && !g_features->orbwalker->should_reset_aa( ) ) {
                return
                    false;
            }

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || !g_features->orbwalker->
                                         is_attackable( target->index, g_local->attack_range + 50.f ) )
                return false;

            if ( cast_spell( ESpellSlot::q ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }


        auto spell_w( ) -> bool override{
            if ( !g_config->yorick.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || *g_time -
                m_last_cast_time <= 0.05f || !m_slot_w->is_ready( true ) )
                return false;

            const Object* target = { };

            if ( !target ) target = g_features->target_selector->get_default_target( );

            if ( !target ) return false;

            auto pred = g_features->prediction->predict( target->index, m_w_range, 0, 200 / 2, 0.f );
            if ( ( !pred.valid || ( int )pred.hitchance < g_config->yorick.w_hitchance->get< int >( ) ) ) return false;


            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
            }

            return false;

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->yorick.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || *g_time -
                m_last_cast_time <= 0.05f || !m_slot_e->is_ready( true ) )
                return false;

            const Object* target = { };

            if ( !target ) target = g_features->target_selector->get_default_target( );

            if ( !target ) return false;

            auto pred = g_features->prediction->predict( target->index, m_e_range, 2500, 1500.f / 2, 0.33f );
            if ( ( !pred.valid || ( int )pred.hitchance < g_config->yorick.e_hitchance->get< int >( ) ) ) return false;


            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
            }

            return false;
        }

        auto spell_r( ) -> bool override{
            const auto is_fastclear = GetAsyncKeyState( VK_CONTROL ) && g_features->orbwalker->get_mode( ) ==
                Orbwalker::EOrbwalkerMode::laneclear;
            debug_log( "{}", m_slot_r->get_name( ) );
            if ( !m_slot_r->is_ready( ) || !g_config->yorick.r_enabled->get< bool >( ) || !is_fastclear || m_slot_r->
                get_name( ) != "YorickR" )
                return false;

            if ( m_slot_q->get_name( ) == "YorickQ2" ) {
                if ( cast_spell( ESpellSlot::q ) ) {
                    m_last_q_time    = *g_time;
                    m_last_cast_time = *g_time;
                    g_features->orbwalker->on_cast( );
                }
            }

            if ( cast_spell( ESpellSlot::r ) ) {
                m_last_r_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }


        auto laneclear_q( ) -> bool{
            if ( m_slot_q->get_name( ) == "YorickQ2" || !g_config->yorick.q_enabled->get< bool >( ) || !g_config->yorick
                .q_lasthit->get< bool >( ) || !m_slot_q->is_ready( ) )
                return false;
            unsigned            target_nid{ };
            Object::EMinionType target_type{ };
            int                 monster_priority{ };
            auto                target_health{ 9999.f };

            //bool delay{};
            for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
                if ( !minion ||
                    g_local->position.dist_to( minion->position ) > m_q_range ||
                    minion->is_dead( ) ||
                    minion->is_invisible( ) ||
                    !minion->is_lane_minion( ) &&
                    !minion->is_jungle_monster( ) ||
                    !minion->is_jungle_monster( ) ||
                    g_features->orbwalker->is_ignored( minion->index ) )
                    continue;

                const auto damage = get_spell_damage( ESpellSlot::q, minion );
                debug_log( "health: {} damage: {}", minion->health, damage );

                if ( minion->health < damage ) target_nid = minion->network_id;
            }

            if ( target_nid == 0 ) return false;

            if ( cast_spell( ESpellSlot::q, target_nid ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                return true;
            }

            return false;
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->attack_damage( ) * 1.40f,
                    target->index,
                    true
                );
            default:
                return 0.f;
            }
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
            {
                auto       cast_time = 0.4f;
                const auto bas       = g_local->bonus_attack_speed;

                if ( bas >= 1.11f ) cast_time = 0.133f;
                else if ( bas >= 1.05f ) cast_time = 0.148f;
                else if ( bas >= 0.9f ) cast_time = 0.184f;
                else if ( bas >= 0.75f ) cast_time = 0.22f;
                else if ( bas >= 0.6f ) cast_time = 0.256f;
                else if ( bas >= 0.45f ) cast_time = 0.292f;
                else if ( bas >= 0.3f ) cast_time = 0.328f;
                else if ( bas >= 0.15f ) cast_time = 0.364f;

                //if (!m_tornado_q) return cast_time;

                const auto tt   = cast_time + g_local->position.dist_to( target->position ) / 1500.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return cast_time + g_local->position.dist_to( target->position ) / 1500.f;

                return cast_time + g_local->position.dist_to( pred.value( ) ) / 1500.f;
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
        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_r_time{ };
        float m_last_cast_time{ };

        std::array< float, 6 > m_q_damage{ 0.f, 30.f, 55.f, 80.f, 105.f, 130.f };
        std::array< float, 6 > m_e_damage{ 0.f, 70.f, 105.f, 140.f, 175.f, 210.f };

        float m_q_range{ g_local->attack_range + 50 };
        float m_w_range{ 600.f };
        float m_e_range{ 800.f };
    };
}
