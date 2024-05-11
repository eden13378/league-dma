#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class kaisa_module final : public IModule {
    public:
        virtual ~kaisa_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "kaisa_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Kaisa" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "kaisa" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->kaisa.q_enabled );
            q_settings->checkbox( _( "harass q" ), g_config->kaisa.q_harass );

            w_settings->checkbox( _( "enable" ), g_config->kaisa.w_enabled );
            w_settings->checkbox(_("weave between AAs"), g_config->kaisa.w_spellweaving);
            //w_settings->checkbox( _( "flash w combo" ), g_config->kaisa.flash_w_enabled );
            w_settings->select(
                _( "hitchance" ),
                g_config->kaisa.w_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );
            w_settings->slider_int( _( "min passive stacks to w" ), g_config->kaisa.w_minimum_stacks, 0, 4, 1 );
            w_settings->checkbox(_("semi manual key [ hotkey: T ]"), g_config->kaisa.w_semi_manual);
            w_settings->select(_("semi manual hitchance"), g_config->kaisa.w_semi_manual_hitchance,
                               { _("Fast"), _("Medium"), _("High") });

            e_settings->checkbox( _( "enable" ), g_config->kaisa.e_enabled );
            e_settings->checkbox( _( "flee e" ), g_config->kaisa.flee_e );

            drawings->checkbox( _( "draw q range" ), g_config->kaisa.q_draw_range );
            drawings->checkbox( _( "draw w range" ), g_config->kaisa.w_draw_range );
            drawings->checkbox( _( "draw r range" ), g_config->kaisa.r_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->kaisa.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };


            if ( g_config->kaisa.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) || !g_config->lucian.dont_draw_on_cooldown->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 31, 88, 255, 255 ),
                        m_q_range,
                        Renderer::outline,
                        64,
                        2.f
                    );
                }
            }

            if ( g_config->kaisa.w_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) || !g_config->lucian.dont_draw_on_cooldown->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 132, 245, 66, 255 ),
                        m_w_range,
                        Renderer::outline,
                        100,
                        3.f
                    );
                }
            }

            if ( g_config->kaisa.r_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::r );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) || !g_config->kaisa.dont_draw_on_cooldown->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 173, 47, 68, 255 ),
                        m_r_range,
                        Renderer::outline,
                        80,
                        2.f
                    );
                }
            }

             Vec2 sp{};
            if ( g_config->kaisa.w_semi_manual->get<bool>() && world_to_screen(g_local->position, sp))
            {
                sp.x += 30.f;
                sp.y -= 20.f;
             

                Vec2 second_position = { sp.x, sp.y };
                std::string text1                = "[ T ] SEMI-MANUAL W";

                g_render->text_shadow(second_position, Color(255, 255, 255), g_fonts->get_zabel_12px(), text1.c_str(),
                                      12);
            }
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            //cast_tracker( );
            //combo_flash_w( );

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return;

            semi_manual_w();

            m_r_range    = 750.f + 750.f * m_slot_r->level;
            m_e_charging = g_features->buff_cache->get_buff( g_local->index, ct_hash( "KaisaE" ) ) || g_features->
                buff_cache->get_buff( g_local->index, ct_hash( "KaisaEStealth" ) );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_e( );
                spell_q( );
                spell_w( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->kaisa.q_harass->get< bool >( ) ) harass_q( );
                break;
            case Orbwalker::EOrbwalkerMode::flee:
                flee_e( );
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->kaisa.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || *g_time -
                m_last_cast_time <= 0.1f || !m_slot_q->is_ready( true ) )
                return false;

            auto should_q{ false };

            if ( g_features->orbwalker->in_attack( ) ) {
                auto sci = g_local->spell_book.get_spell_cast_info( );
                if ( !sci || !sci->is_autoattack ) return false;

                const auto target = g_entity_list.get_by_index( sci->get_target_index( ) );
                if ( !target || !g_features->orbwalker->is_attackable( target->index, m_q_range, true, false ) ) {
                    return
                        false;
                }

                should_q = true;
            }

            if ( !should_q ) {
                int count{ };
                for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                    if ( !enemy || enemy->dist_to_local( ) > m_q_range * 1.5f ||
                        g_features->target_selector->is_bad_target( enemy->index ) )
                        continue;

                    if ( !g_features->orbwalker->is_attackable( enemy->index, m_q_range, true, false ) ) continue;

                    ++count;
                }

                should_q = count > 1;
            }


            if ( !should_q ) return false;

            if ( cast_spell( ESpellSlot::q ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto harass_q( ) -> bool{
            if ( !g_config->kaisa.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !g_features->orbwalker
                ->in_attack( ) ||
                !m_slot_q->is_ready( true ) )
                return false;

            auto sci = g_local->spell_book.get_spell_cast_info( );
            if ( !sci || !sci->is_autoattack ) return false;

            const auto target = g_entity_list.get_by_index( sci->get_target_index( ) );
            if ( !target || !g_features->orbwalker->is_attackable( target->index, m_q_range, true, false ) ) {
                return
                    false;
            }

            if ( cast_spell( ESpellSlot::q ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->kaisa.w_enabled->get< bool >( ) || m_e_charging || g_features->orbwalker->in_attack( ) ||
                *g_time - m_last_cast_time <= 0.05f || *g_time - m_last_w_time <= 1.f ||
                !m_slot_w->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if (!target ||
                g_config->kaisa.w_spellweaving->get<bool>() && g_features->orbwalker->is_autoattack_available() &&
                    g_features->orbwalker->is_attackable(target->index))
                return false;

            if ( g_config->kaisa.w_minimum_stacks->get< int >( ) > 0 ) {
                const auto buff = g_features->buff_cache->get_buff( target->index, ct_hash( "kaisapassivemarker" ) );
                if ( !buff ) return false;

                auto stacks = buff->stacks( );
                if ( g_features->prediction->get_incoming_attack_count( target->index ) > 0 ) {
                    stacks =
                        buff->stacks( ) == 4 ? 0 : buff->stacks( ) + 1;
                }

                if ( stacks < g_config->kaisa.w_minimum_stacks->get< int >( ) ) return false;
            }

            auto in_ult_dash{ false };
            auto aimgr = g_local->get_ai_manager( );
            if ( aimgr && aimgr->is_dashing && m_slot_r->cooldown_expire - m_slot_r->cooldown <= 1.f ) {
                in_ult_dash =
                    true;
            }

            auto source_position{ g_local->position };
            if ( in_ult_dash ) {
                const auto pred = g_features->prediction->predict_default(
                    g_local->index,
                    0.4f + g_features->orbwalker->get_ping( )
                );
                if ( pred.has_value( ) ) source_position = pred.value( );
            }


            const auto predicted = g_features->prediction->predict(
                target->index,
                m_w_range,
                1750.f,
                100.f,
                0.4f,
                source_position,
                true,
                Prediction::include_ping,
                Prediction::ESpellType::linear
            );

            if ( !predicted.valid ||
                predicted.hitchance < static_cast< Prediction::EHitchance >( g_config->kaisa.w_hitchance->get<
                    int >( ) ) ||
                g_features->prediction->minion_in_line_predicted(
                    g_local->position,
                    predicted.position,
                    100.f,
                    0.4f,
                    1750.f
                ) )
                return false;


            if ( cast_spell( ESpellSlot::w, predicted.position ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto semi_manual_w() -> bool {

            if (!g_config->kaisa.w_semi_manual->get<bool>() || g_features->orbwalker->in_attack() ||
                *g_time - m_last_cast_time <= 0.05f || *g_time - m_last_w_time <= 1.f || m_e_charging
                || !m_slot_w->is_ready(true) || !GetAsyncKeyState( 0x54 ) ) return false;


            Object *target{};
            float lowest_distance{ 300.f };
            Vec3 cursor = g_pw_hud->get_hud_manager()->cursor_position_unclipped;

            for ( auto enemy : g_entity_list.get_enemies() ) {
                if ( !enemy || enemy->is_dead() || enemy->is_invisible() || enemy->dist_to_local( ) > m_w_range ) continue;

                if ( enemy->position.dist_to(cursor) < lowest_distance)
                {
                    target = enemy;
                    lowest_distance = enemy->position.dist_to(cursor);
                }
            }

            if (!target) return false;

            auto in_ult_dash{ false };
            auto aimgr = g_local->get_ai_manager();
            if (aimgr && aimgr->is_dashing && m_slot_r->cooldown_expire - m_slot_r->cooldown <= 1.f) {
                in_ult_dash = true;
            }

            auto source_position{ g_local->position };
            if (in_ult_dash)
            {
                const auto pred =
                    g_features->prediction->predict_default(g_local->index, 0.4f + g_features->orbwalker->get_ping());
                if (pred.has_value()) source_position = pred.value();
            }


            const auto predicted =
                g_features->prediction->predict(target->index, m_w_range, 1750.f, 100.f, 0.4f, source_position, true,
                                                Prediction::include_ping, Prediction::ESpellType::linear);

            if (!predicted.valid ||
                predicted.hitchance < static_cast<Prediction::EHitchance>(g_config->kaisa.w_semi_manual_hitchance->get<int>()) ||
                g_features->prediction->minion_in_line_predicted(g_local->position, predicted.position, 100.f, 0.4f,
                                                                 1750.f))
                return false;


            if (cast_spell(ESpellSlot::w, predicted.position)) {

                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast();
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->kaisa.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.5f ||
                g_features->orbwalker->in_attack( ) || m_e_charging ||
                *g_time - m_last_cast_time <= 0.1f ||
                !m_slot_e->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || g_features->orbwalker->is_attackable( target->index ) ) return false;

            const auto cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;

            const auto extend_position = g_local->position.extend( cursor, g_local->movement_speed * 1.5f );

            const auto charge_time = 1.2f - std::min( g_local->bonus_attack_speed, 1.f ) * 0.6f;
            const auto target_pred = g_features->prediction->predict_default( target->index, charge_time );
            if ( !target_pred ) return false;

            if ( target_pred.value( ).dist_to( extend_position ) > g_local->attack_range +
                g_features->orbwalker->get_bounding_radius( ) )
                return false;

            if ( cast_spell( ESpellSlot::e ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{ return false; }

        auto flee_e( ) -> bool{
            if ( !g_config->kaisa.flee_e->get< bool >( ) || *g_time - m_last_e_time <= 0.5f
                || *g_time - m_last_cast_time <= 0.1f || g_features->orbwalker->in_attack( )
                || m_e_charging || !m_slot_e->is_ready( true ) )
                return false;

            auto closest_enemy{ 9999.f };
            bool was_moving_towards{ };
            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) ) continue;

                if ( !was_moving_towards ) {
                    auto pred = g_features->prediction->predict_default( enemy->index, 0.2f );
                    if ( pred && pred.value( ).dist_to( g_local->position ) < enemy->
                        dist_to_local( ) )
                        was_moving_towards = true;
                }

                if ( enemy->dist_to_local( ) < closest_enemy ) closest_enemy = enemy->dist_to_local( );
            }

            if ( closest_enemy >= 950.f || !was_moving_towards ) return false;

            if ( cast_spell( ESpellSlot::e ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto cast_tracker( ) -> void{
            if ( !m_w_active ) {
                auto sci = g_local->spell_book.get_spell_cast_info( );
                if ( !sci || sci->slot != 1 || sci->server_cast_time < *g_time ) return;

                m_w_active         = true;
                m_server_cast_time = sci->server_cast_time;
                m_cast_position    = sci->end_position;

                //std::cout << "[ Found ] W cast, windup complete in " << sci->server_cast_time - *g_time << " seconds"
                //          << std::endl;
            }

            if ( *g_time > m_server_cast_time ) {
                std::cout << "W Cast finished | " << *g_time << std::endl;
                m_w_active = false;
            }
        }

        auto combo_flash_w( ) -> bool{
            if ( !g_config->kaisa.flash_w_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.1f || !
                m_w_active )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || !g_features->target_selector->is_forced( ) ) return false;

            const auto time_left = m_server_cast_time - *g_time;
            if ( time_left > 0.0775f || time_left <= 0.f ) return false;

            auto spellslot = ESpellSlot::d;
            auto slot      = g_local->spell_book.get_spell_slot( ESpellSlot::d );
            if ( !slot || rt_hash( slot->get_name( ).data( ) ) != ct_hash( "SummonerFlash" ) ) {
                slot      = g_local->spell_book.get_spell_slot( ESpellSlot::f );
                spellslot = ESpellSlot::f;
                if ( !slot || rt_hash( slot->get_name( ).data( ) ) != ct_hash( "SummonerFlash" ) ) return false;
            }

            if ( !slot->is_ready( ) ) return false;

            Vec3 cast_position{ };

            if ( g_features->prediction->champion_in_line(
                    g_local->position,
                    m_cast_position,
                    100.f,
                    true,
                    time_left,
                    1750.f,
                    target->network_id
                ) ||
                g_features->prediction->minion_in_line_predicted(
                    g_local->position,
                    m_cast_position,
                    100.f,
                    time_left,
                    1750.f
                ) ) {
                const auto flash_points = g_render->get_3d_circle_points( g_local->position, 400.f, 16 );

                Vec3 best_point{ };
                bool found_point{ };

                for ( auto point : flash_points ) {
                    auto flash_position = get_flash_position( point );
                    if ( g_features->prediction->champion_in_line(
                            flash_position,
                            m_cast_position,
                            100.f,
                            true,
                            time_left,
                            1750.f,
                            target->network_id
                        ) ||
                        g_features->prediction->minion_in_line_predicted(
                            flash_position,
                            m_cast_position,
                            100.f,
                            time_left,
                            1750.f
                        ) )
                        continue;

                    auto rect = sdk::math::Rectangle(
                        flash_position,
                        flash_position.extend( m_cast_position, 1000.f ),
                        100.f
                    );
                    auto poly = rect.to_polygon(
                        static_cast< int32_t >(
                            g_features->prediction->get_champion_radius( rt_hash( target->champion_name.text ) )
                        )
                    );

                    auto pred = g_features->prediction->predict_default(
                        target->index,
                        time_left + flash_position.dist_to( target->position ) / 1750.f
                    );
                    if ( !pred ) continue;

                    if ( poly.is_outside( *pred ) ) continue;

                    if ( best_point.length( ) > 0.f &&
                        flash_position.dist_to( m_cast_position ) > best_point.dist_to( m_cast_position ) )
                        continue;

                    best_point  = point;
                    found_point = true;
                }

                if ( !found_point ) return false;

                cast_position = best_point;
            } else return false;

            if ( cast_spell( spellslot, cast_position ) ) {
                std::cout << "[ FLASH ] Executed flash w combo, time left: " << time_left << std::endl;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        static auto get_flash_position( const Vec3& cast_position ) -> Vec3{
            if ( !g_navgrid->is_wall( cast_position ) ) return cast_position;

            for ( auto i = 1; i <= 20; i++ ) {
                auto points = g_render->get_3d_circle_points( cast_position, 25.f * i, 16 );

                for ( auto point : points ) {
                    if ( point.dist_to( g_local->position ) > 750.f ) continue;

                    if ( !g_navgrid->is_wall( point ) ) return point;
                }
            }

            return cast_position;
        }

    private:
        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_cast_time{ };

        float m_q_range{ 600.f };
        float m_w_range{ 3000.f };
        float m_r_range{ 1500.f };

        // w track
        bool  m_w_active{ };
        float m_server_cast_time{ };
        Vec3  m_cast_position{ };

        bool m_e_charging{ };
    };
}
