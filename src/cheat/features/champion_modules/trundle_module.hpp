#pragma once
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"
#include "module.hpp"

namespace features::champion_modules {
    class trundle_module final : public IModule {
    public:
        virtual ~trundle_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "trundle_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Trundle" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "trundle" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );

            q_settings->checkbox( _( "combo q" ), g_config->trundle.q_enabled );
            q_settings->checkbox( _( "spellclear q" ), g_config->trundle.q_spellclear );

            w_settings->checkbox( _( "combo w" ), g_config->trundle.w_enabled );
            w_settings->checkbox( _( "spellclear w" ), g_config->trundle.w_spellclear );

            e_settings->checkbox( _( "enable" ), g_config->trundle.e_enabled );
            e_settings->checkbox( _( "auto interrupt (?)" ), g_config->trundle.e_autointerrupt )
                      ->set_tooltip( _( "Automatically interrupts spells like Katarina R, Malzahar R, etc" ) );
            e_settings->checkbox( _( "antigapclose" ), g_config->trundle.e_antigapclose );
            e_settings->select(
                _( "hitchance" ),
                g_config->trundle.e_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            drawings->checkbox( _( "draw w range" ), g_config->trundle.w_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->trundle.e_draw_range );
            drawings->checkbox( _( "draw r range" ), g_config->trundle.r_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->trundle.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };


            if ( g_config->trundle.w_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) || !g_config->trundle.dont_draw_on_cooldown->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 132, 245, 66, 255 ),
                        m_w_range,
                        Renderer::outline,
                        70,
                        3.f
                    );
                }
            }

            if ( g_config->trundle.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) || !g_config->trundle.dont_draw_on_cooldown->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 144, 66, 245, 255 ),
                        m_e_range,
                        Renderer::outline,
                        70,
                        3.f
                    );
                }
            }

            if ( g_config->trundle.r_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::r );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) || !g_config->trundle.dont_draw_on_cooldown->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 255, 50, 50, 255 ),
                        m_r_range,
                        Renderer::outline,
                        70,
                        3.f
                    );
                }
            }
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            track_q( );

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return;

            antigapclose_e( );
            autointerrupt_e( );

            jungleclear_w( );

            if ( g_features->orbwalker->in_attack( ) ) return;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_w( );
                spell_r( );
                spell_e( );
                spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                spell_q( );
                spellclear_q( );
                break;
            case Orbwalker::EOrbwalkerMode::laneclear:
                spellclear_q( );
                break;
            default:
                break;
            }

            //std::cout << "Q state" << std::dec << ( int )m_slot_q->get_active_state( )
            //          << " | usable STATE: " << ( int )m_slot_q->get_usable_state( ) << " | addrtess" << std::hex << m_slot_q.get_address(  ) << std::endl;
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->trundle.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.025f ||
                !g_features->orbwalker->should_reset_aa( )
                || g_features->orbwalker->get_last_target( ) != ETargetType::hero ||
                g_features->buff_cache->get_buff( g_local->index, ct_hash( "TrundleTrollSmash" ) ) ||
                !m_slot_q->is_ready( true ) )
                return false;

            if ( cast_spell( ESpellSlot::q ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Trundle: Q ] Combo cast | " << *g_time << std::endl;
                return true;
            }

            return false;
        }

        auto spellclear_q( ) -> bool{
            if ( !g_config->trundle.q_spellclear->get< bool >( ) || *g_time - m_last_q_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.025f || !g_features->orbwalker->should_reset_aa( ) ||
                g_features->buff_cache->get_buff( g_local->index, ct_hash( "TrundleTrollSmash" ) ) ||
                !m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_entity_list.get_by_index( g_features->orbwalker->get_last_target_index( ) );
            if ( !target || target->is_dead( ) ||
                !target->is_jungle_monster( ) && !target->is_turret_object( ) && !target->is_ward( ) )
                return false;

            if ( cast_spell( ESpellSlot::q ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Trundle: Q ] Spellclear cast | " << *g_time << std::endl;
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->trundle.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || *g_time -
                m_last_cast_time <= 0.025f ||
                !m_slot_w->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) > g_local->attack_range + 125.f ) return false;

            if ( cast_spell( ESpellSlot::w, target->position ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Trundle: W ] Combo cast | " << *g_time << std::endl;
                return true;
            }

            return false;
        }

        auto jungleclear_w( ) -> bool{
            if ( !g_config->trundle.w_spellclear->get< bool >( ) || g_features->orbwalker->get_mode( ) !=
                Orbwalker::EOrbwalkerMode::laneclear
                || *g_time - m_last_cast_time <= 0.025f || *g_time - m_last_w_time <= 0.4f || !g_features->orbwalker->
                should_reset_aa( ) || !m_slot_w->is_ready( true ) )
                return false;

            const auto target = g_entity_list.get_by_index( g_features->orbwalker->get_last_target_index( ) );
            if ( !target || target->is_dead( ) || !target->is_jungle_monster( ) ) return false;

            if ( cast_spell( ESpellSlot::w, g_local->position ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Trundle: W ] Spellclear cast | " << *g_time << std::endl;
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->trundle.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || *g_time -
                m_last_cast_time <= 0.025f ||
                !m_slot_e->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_e_range,
                0.f,
                0.f,
                0.275f,
                { }
            );

            if ( !pred.valid ||
                pred.hitchance < static_cast< Prediction::EHitchance >( g_config->trundle.e_hitchance->get<
                    int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Trundle: E ] Combo cast | " << *g_time << std::endl;
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !GetAsyncKeyState( VK_CONTROL ) || *g_time - m_last_cast_time <= 0.025f || *g_time - m_last_r_time <=
                0.3f ||
                !m_slot_r->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || !g_features->orbwalker->is_attackable( target->index ) ) return false;


            if ( cast_spell( ESpellSlot::r, target->network_id ) ) {
                g_features->orbwalker->on_cast( );
                m_last_r_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Trundle: R ] Combo at " << target->get_name( ) << " | " << *g_time << std::endl;
                return true;
            }

            return false;
        }

        auto antigapclose_e( ) -> void{
            if ( !g_config->trundle.e_antigapclose->get< bool >( ) || *g_time - m_last_e_time <= 0.4f ||
                !m_slot_e->is_ready( true ) )
                return;

            const auto target = get_antigapclose_target( 800.f );
            if ( !target ) return;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_e_range,
                0.f,
                0.f,
                0.275f,
                { }
            );

            if ( !pred.valid || ( int )pred.hitchance < 3 ) return;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                m_last_cast_time = *g_time;
                m_last_e_time    = *g_time;
                g_features->orbwalker->on_cast( );

                std::cout << "[ Trundle: E ] Antigapclose against " << target->get_name( ) << std::endl;
            }
        }

        auto autointerrupt_e( ) -> void{
            if ( !g_config->trundle.e_autointerrupt->get< bool >( ) || *g_time - m_last_e_time <= 0.4f ||
                !m_slot_e->is_ready( true ) )
                return;

            const auto target = get_interruptable_target( m_e_range );
            if ( !target ) return;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_e_range,
                0.f,
                0.f,
                0.275f,
                { },
                true
            );
            if ( !pred.valid || ( int )pred.hitchance < 3 ) return;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                m_last_cast_time = *g_time;
                m_last_e_time    = *g_time;
                g_features->orbwalker->on_cast( );

                std::cout << "[ Trundle: E ] Auto-interrupt against " << target->get_name( ) << std::endl;
            }
        }

        auto track_q( ) -> void{
            if ( m_q_is_cooling_down ) {
                m_q_is_cooling_down = !m_slot_q->is_ready( );
                m_q_expire_time     = m_slot_q->cooldown_expire;
            }

            if ( m_slot_q->cooldown_expire > *g_time + 1.f ) {
                m_q_is_cooling_down = true;
                return;
            }

            if ( m_q_expire_time < m_slot_q->cooldown_expire &&
                m_slot_q->cooldown_expire < *g_time + 0.25f &&
                m_slot_q->cooldown_expire > *g_time - 0.25f ) {
                std::cout << "[trundle] detected AA reset Q\n";
                g_features->orbwalker->reset_aa_timer( );

                m_q_expire_time = m_slot_q->cooldown_expire;
            }
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
                    m_e_damage[ get_slot_e( )->level ] + g_local->ability_power( ) * 0.6f,
                    target->index,
                    false
                );
            }
            return 0.f;
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
            case ESpellSlot::e:
            {
                const auto tt   = 0.25f + g_local->position.dist_to( target->position ) / 1550.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.25f + g_local->position.dist_to( pred.value( ) ) / 1550.f;
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

        // q track
        bool  m_q_is_cooling_down{ };
        float m_q_expire_time{ };

        std::array< float, 6 > m_q_damage = { 0.f, 40.f, 65.f, 90.f, 115.f, 140.f };
        std::array< float, 6 > m_e_damage = { 0.f, 80.f, 110.f, 140.f, 170.f, 200.f };

        float m_q_range{ 0.f };
        float m_w_range{ 750.f };
        float m_e_range{ 1000.f };
        float m_r_range{ 650.f };
    };
} // namespace features::champion_modules
