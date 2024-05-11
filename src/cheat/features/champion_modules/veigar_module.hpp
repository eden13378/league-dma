#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class veigar_module final : public IModule {
    public:
        virtual ~veigar_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "veigar_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Veigar" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "veigar" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->veigar.q_enabled );
            q_settings->checkbox( _( "harass q" ), g_config->veigar.q_harass );
            q_settings->checkbox( _( "lasthit q" ), g_config->veigar.q_lasthit );
            q_settings->select(
                _( "hitchance" ),
                g_config->veigar.q_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            w_settings->checkbox( _( "enable" ), g_config->veigar.w_enabled );
            w_settings->select(
                _( "hitchance" ),
                g_config->veigar.w_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            e_settings->checkbox( _( "enable" ), g_config->veigar.e_enabled );
            e_settings->checkbox( _( "try hit cage edge" ), g_config->veigar.e_prefer_stun );
            e_settings->select(
                _( "hitchance" ),
                g_config->veigar.e_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            r_settings->checkbox( _( "killsteal r" ), g_config->veigar.r_enabled );

            drawings->checkbox( _( "draw q range" ), g_config->veigar.q_draw_range );
            drawings->checkbox( _( "draw w range" ), g_config->veigar.w_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->veigar.e_draw_range );
            drawings->checkbox( _( "draw r range" ), g_config->veigar.r_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->veigar.dont_draw_on_cooldown );
            drawings->checkbox( _( "draw r damage" ), g_config->veigar.r_draw_damage );
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };


            if ( g_config->veigar.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( ) || !g_config->veigar.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 31, 88, 255, 255 ),
                        m_q_range,
                        Renderer::outline,
                        70,
                        3.f
                    );
                }
            }

            if ( g_config->veigar.w_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 && ( slot->is_ready( ) || !g_config->veigar.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
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

            if ( g_config->veigar.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( ) || !g_config->veigar.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
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

            if ( g_config->veigar.r_draw_range->get< bool >( ) || g_config->veigar.r_draw_damage->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::r );
                if ( slot && slot->level > 0 && ( slot->is_ready( ) || !g_config->veigar.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    if ( g_config->veigar.r_draw_range->get< bool >( ) ) {
                        g_render->circle_3d(
                            g_local->position,
                            Color( 173, 47, 68, 255 ),
                            m_r_range,
                            Renderer::outline,
                            64,
                            3.f
                        );
                    }

                    if ( g_config->veigar.r_draw_damage->get< bool >( ) ) {
                        Vec2 sp{ };

                        for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                            if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) ||
                                !world_to_screen( enemy->position, sp ) )
                                continue;

                            const auto base_x = 1920.f;
                            const auto base_y = 1080.f;

                            const auto width_ratio  = base_x / static_cast< float >( g_render_manager->get_width( ) );
                            const auto height_ratio = base_y / static_cast< float >( g_render_manager->get_height( ) );

                            const auto width_offset  = width_ratio * 0.055f;
                            const auto height_offset = height_ratio * 0.0222f;

                            const auto bar_length = width_offset * static_cast< float >( g_render_manager->
                                get_width( ) );
                            const auto bar_height = height_offset * static_cast< float >( g_render_manager->
                                get_height( ) );

                            auto base_position = enemy->get_hpbar_position( );

                            const auto buffer = 1.f;

                            base_position.x -= bar_length * 0.43f;
                            base_position.y -= bar_height;

                            const auto damage      = get_ult_damage( enemy->index );
                            const auto modifier    = enemy->health / enemy->max_health;
                            const auto damage_mod  = damage / enemy->max_health;
                            const auto is_killable = damage > enemy->health + enemy->shield;

                            const Vec2 box_start{
                                base_position.x + bar_length * modifier + buffer,
                                base_position.y - buffer
                            };
                            const Vec2 box_size{
                                damage_mod * bar_length > box_start.x - base_position.x
                                    ? base_position.x - box_start.x - buffer * 1.f
                                    : -( bar_length * damage_mod ) - buffer * 1.f,
                                bar_height * 0.5f + buffer * 2.f
                            };

                            g_render->filled_box(
                                box_start,
                                box_size,
                                is_killable
                                    ? g_features->orbwalker->animate_color(
                                        Color( 255, 50, 75 ),
                                        EAnimationType::pulse,
                                        10
                                    ).alpha( 180 )
                                    : Color( 255, 255, 25, 180 )
                            );
                        }
                    }
                }
            }
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return;

            spell_r( );

            if ( g_features->orbwalker->in_attack( ) ) return;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_e( );
                spell_w( );
                spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->veigar.q_harass->get< bool >( ) && spell_q( ) ) break;

                lasthit_q( );
            case Orbwalker::EOrbwalkerMode::lasthit:
            case Orbwalker::EOrbwalkerMode::laneclear:
                if ( g_config->veigar.q_harass->get< bool >( ) && spell_q( ) ) break;

                lasthit_q( );
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->veigar.q_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.025f ||
                *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready( true ) )
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

            const auto pred =
                g_features->prediction->predict( target->index, m_q_range, 2200.f, 70.f, 0.25f, { }, true );
            if ( !pred.valid ||
                pred.hitchance < static_cast< Prediction::EHitchance >( g_config->veigar.q_hitchance->get< int >( ) )
                ||
                g_features->prediction->count_minions_in_line( g_local->position, pred.position, 70.f ) > 1 )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Veigar: Q ] Combo at " << target->get_name( ) << std::endl;
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->veigar.w_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.025f ||
                *g_time - m_last_w_time <= 0.3f || !m_slot_w->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_w_range, 0.f, 240.f, 1.225f );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->veigar.w_hitchance->
                get< int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Veigar: W ] Combo at " << target->get_name( ) << std::endl;
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->veigar.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f
                || *g_time - m_last_cast_time <= 0.1f || !m_slot_e->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto radius = g_config->veigar.e_prefer_stun->get< bool >( ) ? 750.f : 260.f;

            const auto pred = g_features->prediction->predict( target->index, m_e_range, 0.f, radius, 1.f );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->veigar.e_hitchance->
                get< int >( ) ) )
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
            if ( !g_config->veigar.r_enabled->get< bool >( ) || *g_time - m_last_r_time <= 0.5f ||
                *g_time - m_last_cast_time <= 0.05f || !m_slot_r->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_killsteal_target(
                m_r_range,
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::r, unit ); },
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::r, unit ); },
                2
            );
            if ( !target ) return false;

            if ( cast_spell( ESpellSlot::r, target->network_id ) ) {
                m_last_r_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                std::cout << "[ Veigar: R ] Killsteal cast, target: " << target->champion_name.text << std::endl;
                return true;
            }

            return false;
        }

        auto lasthit_q( ) -> bool{
            if ( !g_config->veigar.q_lasthit->get< bool >( ) || *g_time - m_last_cast_time <= 0.1f ||
                *g_time - m_last_q_time <= 0.4f ||
                !m_slot_q->is_ready( true ) )
                return false;

            const auto            lasthit_data = get_line_lasthit_target_advanced(
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                m_q_range,
                70.f,
                0.25f,
                2,
                false
            );

            if ( !lasthit_data ) return false;

            if ( cast_spell( ESpellSlot::q, lasthit_data->cast_position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                g_features->orbwalker->ignore_minion( lasthit_data->index, lasthit_data->travel_time );
                return true;
            }

            return false;
        }

        auto get_ult_damage( const int16_t target_index ) -> float{
            auto target = g_entity_list.get_by_index( target_index );
            if ( !target ) return 0.f;

            target.update( );

            float damage{ };
            switch ( m_slot_r->level ) {
            case 1:
                damage = 175.f;
                break;
            case 2:
                damage = 250.f;
                break;
            case 3:
                damage = 325.f;
                break;
            default:
                damage = 175.f;
                break;
            }

            damage += g_local->ability_power( ) * 0.75f;

            const auto missing_health  = 1.f - target->health / target->max_health;
            const auto damage_modifier = std::min( missing_health / 0.6667f, 1.f );

            damage *= 1.f + damage_modifier;

            return helper::calculate_damage( damage, target_index, false );
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ m_slot_q->level ] + g_local->ability_power( ) * 0.6f,
                    target->index,
                    false
                );
            case ESpellSlot::w:
                return helper::calculate_damage(
                    m_w_damage[ get_slot_w( )->level ] + g_local->ability_power( ),
                    target->index,
                    false
                );
            case ESpellSlot::r:
                return get_ult_damage( target->index );
            default:
                break;
            }
            return 0.f;
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
            {
                const auto tt   = 0.25f + g_local->position.dist_to( target->position ) / 2200.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.25f + g_local->position.dist_to( pred.value( ) ) / 2200.f;
            }
            case ESpellSlot::w:
                return 1.2f;
            case ESpellSlot::e:
                return 0.75f;
            case ESpellSlot::r:
                return 1.f;
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

        std::vector< float > m_q_damage = { 0.f, 80.f, 120.f, 160.f, 200.f, 240.f };
        std::vector< float > m_w_damage = { 0.f, 100.f, 150.f, 200.f, 250.f, 300.f };
        std::vector< float > m_r_damage = { 0.f, 175.f, 250.f, 325.f };

        float m_q_range{ 990.f };
        float m_w_range{ 900.f };
        float m_e_range{ 725.f };
        float m_r_range{ 650.f };
    };
}
