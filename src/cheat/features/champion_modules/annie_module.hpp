#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class annie_module final : public IModule {
    public:
        virtual ~annie_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "annie_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Annie" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "annie" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->annie.q_enabled );
            q_settings->checkbox( _( "harass q" ), g_config->annie.q_harass );
            q_settings->checkbox( _( "killsteal q" ), g_config->annie.q_killsteal );
            q_settings->checkbox( _( "lasthit q" ), g_config->annie.q_lasthit );

            e_settings->checkbox( _( "enable" ), g_config->annie.e_enabled );
            e_settings->checkbox( _( "flee e" ), g_config->annie.e_enabled );
            e_settings->checkbox( _( "shield allies" ), g_config->annie.e_shielding );

            drawings->checkbox( _( "draw q range" ), g_config->annie.q_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->annie.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };


            if ( g_config->annie.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) || !g_config->mundo.dont_draw_on_cooldown->get< bool >( ) ) ) {
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
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            spell_e( );

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) ||
                g_features->evade->is_active( ) )
                return;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:
                spell_q( );

                break;
            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->annie.q_harass->get< bool >( ) && spell_q( ) ) break;

                lasthit_q( );
                break;
            case Orbwalker::EOrbwalkerMode::laneclear:
            case Orbwalker::EOrbwalkerMode::lasthit:

                if ( g_config->annie.q_harass->get< bool >( ) && spell_q( ) ) break;

                lasthit_q( );
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->annie.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f ||
                !m_slot_q->is_ready( true ) )
                return false;

            for ( auto i = 0; i < 2; i++ ) {
                if ( g_features->target_selector->is_forced( ) && i > 0 ) break;

                const auto target = g_features->target_selector->get_default_target( i > 0 );
                if ( !target ) break;

                if ( combo_q( target ) ) return true;
            }

            return false;
        }

        auto combo_q( Object* target ) -> bool{
            if ( !target ) return false;

            if ( !g_features->orbwalker->is_attackable( target->index, m_q_range ) ) return false;

            if ( cast_spell( ESpellSlot::q, target->network_id ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->annie.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.5f ||
                !m_slot_w->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto pred = g_features->prediction->predict( target->index, m_w_range, 0.f, 200.f, 0.25f );
            if ( !pred.valid || ( int )pred.hitchance < g_config->annie.w_hitchance->get< int >( )
                || pred.default_position.dist_to( g_local->position ) > 550.f )
                return false;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_w_time = *g_time;

                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->annie.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f ||
                !m_slot_e->is_ready( true ) )
                return false;

            const Object* target{ };
            auto          target_priority{ -1 };

            // cast when ally will get damaged
            if ( g_config->annie.e_shielding->get< bool >( ) ) {
                for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                    if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) || enemy->dist_to_local( ) >
                        1000.f )
                        continue;

                    auto sci = enemy->spell_book.get_spell_cast_info( );
                    if ( !sci || sci->get_target_index( ) == 0 ) continue;

                    auto& cast_target = g_entity_list.get_by_index( sci->get_target_index( ) );
                    if ( !cast_target || !cast_target->is_hero( ) || cast_target->dist_to_local( ) > m_e_range ||
                        cast_target->network_id == g_local->network_id )
                        continue;

                    const auto priority = g_features->target_selector->get_target_priority(
                        cast_target->champion_name.text
                    );
                    if ( priority < target_priority ) continue;

                    target_priority = priority;
                    target          = cast_target.get( );
                }

                if ( !target ) {
                    for ( const auto ally : g_entity_list.get_allies( ) ) {
                        if ( !ally || ally->is_dead( ) || ally->dist_to_local( ) > m_e_range ||
                            ally->network_id == g_local->network_id && !g_config->lulu.e_self->get< bool >( ) )
                            continue;

                        const auto pred_health = helper::get_real_health(
                            ally->index,
                            EDamageType::true_damage,
                            0.25f,
                            false
                        );
                        if ( pred_health >= ally->health - 80.f ) continue;

                        const auto priority = g_features->target_selector->get_target_priority(
                            ally->champion_name.text
                        );
                        if ( priority < target_priority ) continue;

                        target_priority = priority;
                        target          = ally;
                    }
                }
            }


            if ( !target ) return false;


            if ( cast_spell( ESpellSlot::e, target->network_id ) ) {
                m_last_e_time = *g_time;

                return true;
            }

            return false;
        }

        auto lasthit_q( ) -> bool{
            if ( !g_config->annie.q_lasthit->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || *g_time -
                m_last_cast_time <= 0.1f ||
                !m_slot_q->is_ready( true ) )
                return false;

            const auto            lasthit_data = get_targetable_lasthit_target(
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                m_q_range,
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); }
            );
            if ( !lasthit_data ) return false;

            if ( cast_spell( ESpellSlot::q, lasthit_data->network_id ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                g_features->orbwalker->ignore_minion( lasthit_data->index, lasthit_data->travel_time * 2.f );
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{ return false; }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ m_slot_q->level ] + g_local->ability_power( ) * 0.8f,
                    target->index,
                    false
                );
            default:
                break;
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

                return 0.25f + g_local->position.dist_to( pred.value( ) ) / 2000.f;
            }
            case ESpellSlot::w:
                return 0.25f;
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

        std::vector< float > m_q_damage = { 0.f, 80.f, 115.f, 150.f, 185.f, 220.f };


        float m_q_range{ 625.f };
        float m_w_range{ 600.f };
        float m_e_range{ 800.f };
        float m_r_range{ 600.f };
    };
} // namespace features::champion_modules
