#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class sett_module final : public IModule {
    public:
        virtual ~sett_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "sett_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Sett" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "sett" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            //const auto r_settings = navigation->add_section(_("r settings"));

            q_settings->checkbox( _( "enable" ), g_config->sett.q_enabled );
            q_settings->checkbox( _( "use in laneclear" ), g_config->sett.q_laneclear );

            w_settings->checkbox( _( "enable" ), g_config->sett.w_enabled );
            w_settings->select(
                _( "hitchance" ),
                g_config->sett.w_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );
            w_settings->slider_int( _( "min grit %" ), g_config->sett.w_min_grit_percent, 1, 90, 1 );

            e_settings->checkbox( _( "enable" ), g_config->sett.e_enabled );
            e_settings->checkbox( _( "disable in aa range" ), g_config->sett.e_disable_aa_range );
            e_settings->checkbox( _( "only if can stun" ), g_config->sett.e_only_stun );
            e_settings->select(
                _( "hitchance" ),
                g_config->sett.e_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            //  r_settings->checkbox(_("enable"), g_config->kassadin.r_enabled);
            //  r_settings->checkbox(_("only out of aa range"), g_config->kassadin.r_out_of_range);
            //  r_settings->select(_("hitchance"), g_config->kassadin.r_hitchance, { _("Low"), _("Medium"), _("High"), _("Very high"), _("Immobile") });
            //  r_settings->slider_int(_("max stacks to use"), g_config->kassadin.r_max_stacks, 1, 4, 1);

            drawings->checkbox( _( "draw q range" ), g_config->sett.q_draw_range );
            drawings->checkbox( _( "draw w range" ), g_config->sett.w_draw_range );
            drawings->select(
                _( "draw e mode" ),
                g_config->sett.e_draw_mode,
                { _( "Disabled" ), _( "Circle" ), _( "Dynamic" ) }
            );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->sett.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->sett.q_draw_range->get< bool >( ) &&
                !g_config->sett.w_draw_range->get< bool >( ) &&
                !g_config->sett.e_draw_mode->get< int >( ) == 0 ||
                g_local->is_dead( )
            )
                return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->sett.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( ) || !g_config->sett.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
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

            if ( g_config->sett.w_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 && ( slot->is_ready( ) || !g_config->sett.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
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

            if ( g_config->sett.e_draw_mode->get< int >( ) == 1 ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( ) || !g_config->sett.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
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

            if ( g_config->sett.e_draw_mode->get< int >( ) != 2 ) return;

            slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
            if ( !slot || slot->level == 0 || !slot->is_ready( ) && g_config->sett.dont_draw_on_cooldown->get<
                bool >( ) )
                return;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return;

            const auto in_range = target->dist_to_local( ) <= m_e_range + 50.f;

            auto rect = sdk::math::Rectangle(
                g_local->position,
                g_local->position.extend( target->position, m_e_range ),
                140.f
            );
            auto polygon = rect.to_polygon( );

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

                g_render->line(
                    sp_start,
                    sp_end,
                    in_range ? Color( 25, 255, 25 ) : Color( 255, 255, 255 ),
                    1.f
                );
            }

            rect = sdk::math::Rectangle(
                g_local->position,
                g_local->position.extend( target->position, -m_e_range ),
                140.f
            );
            polygon = rect.to_polygon( );

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

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return;

            if ( m_slot_q->is_ready( true ) ) m_q_range = g_local->attack_range + 65.f + 50.f;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_w( );
                spell_e( );
                spell_q( );
                break;

            case Orbwalker::EOrbwalkerMode::harass:
                // if (g_config->kassadin.q_harass->get< bool >()) spell_q();
                break;
            case Orbwalker::EOrbwalkerMode::laneclear:
                if ( g_config->sett.q_laneclear->get< bool >( ) ) spell_q( );
                break;
            default: ;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->sett.q_enabled->get< bool >( ) ||
                *g_time - m_last_q_time <= 0.4f ||
                !m_slot_q->is_ready( ) ||
                !g_features->orbwalker->should_reset_aa( )
            )
                return false;

            if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo ) {
                const auto target = g_features->target_selector->get_default_target( );
                if ( !target || !g_features->orbwalker->is_attackable( target->index, m_q_range ) ) return false;
            }

            if ( cast_spell( ESpellSlot::q ) ) {
                m_last_q_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->sett.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready( )
                ||
                g_local->mana / g_local->max_mana < g_config->sett.w_min_grit_percent->get< int >( ) / 100.f )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto pred = g_features->prediction->predict( target->index, m_w_range, 0.f, 60.f, 0.75f, { }, true );
            if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->sett.w_hitchance->get<
                int >( ) )
                return false;


            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                m_last_w_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->sett.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->
                is_ready( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || g_config->sett.e_disable_aa_range->get< bool >( ) && g_features->orbwalker->is_attackable(
                target->index
            ) )
                return false;

            auto pred = g_features->prediction->predict( target->index, m_e_range + 55.f, 0.f, 20.f, 0.25f );
            if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->sett.e_hitchance->get<
                int >( ) )
                return false;

            const auto rect = sdk::math::Rectangle(
                g_local->position,
                g_local->position.extend( pred.position, -475.f ),
                140.f
            );
            auto polygon = rect.to_polygon( );

            bool stun_found{ };

            for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
                if ( !minion || minion->is_dead( ) || minion->is_invisible( ) || minion->dist_to_local( ) > m_e_range +
                    300.f
                    || !minion->is_lane_minion( ) && !minion->is_jungle_monster( ) )
                    continue;

                if ( polygon.is_inside( minion->position ) ) {
                    stun_found = true;
                    break;
                }
            }

            if ( !stun_found ) {
                for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                    if ( !enemy || enemy->dist_to_local( ) > m_e_range + 500.f || g_features->target_selector->
                        is_bad_target( enemy->index ) )
                        continue;

                    auto pred = g_features->prediction->predict_default( enemy->index, 0.25f );
                    if ( !pred ) continue;

                    if ( polygon.is_inside( *pred ) ) {
                        stun_found = true;
                        break;
                    }
                }
            }

            if ( !stun_found && g_config->sett.e_only_stun->get< bool >( ) ) return false;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                m_last_e_time = *g_time;
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
                    m_q_damage[ get_slot_q( )->level ] + g_local->ability_power( ) * 0.7f,
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

        std::array< float, 6 > m_q_damage = { 0.f, 65.f, 95.f, 125.f, 155.f, 185.f };
        std::array< float, 6 > m_e_damage = { 0.f, 80.f, 105.f, 130.f, 155.f, 180.f };
        std::array< float, 6 > m_r_damage = { 0.f, 80.f, 100.f, 120.f };

        float m_q_range{ 0.f };
        float m_w_range{ 725.f };
        float m_e_range{ 450.f };
        float m_r_range{ 400.f };
    };
}
