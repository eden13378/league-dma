#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class katarina_module final : public IModule {
    public:
        virtual ~katarina_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "katarina_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Katarina" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        struct dagger_instance_t {
            unsigned network_id{ };
            int16_t  index{ };

            float spawn_time{ };
            float ready_time{ };
            Vec3  position{ };
        };

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "katarina" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->katarina.q_enabled );
            q_settings->checkbox( _( "harass q" ), g_config->katarina.q_harass );
            q_settings->checkbox( _( "lasthit q" ), g_config->katarina.q_lasthit );

            w_settings->checkbox( _( "enable" ), g_config->katarina.w_enabled );
            w_settings->checkbox( _( "w after shunpo" ), g_config->katarina.w_after_shunpo );

            e_settings->checkbox( _( "enable" ), g_config->katarina.e_enabled );
            e_settings->checkbox( _( "killsteal e" ), g_config->katarina.e_killsteal );
            e_settings->checkbox( _( "dagger e" ), g_config->katarina.e_dagger );
            e_settings->checkbox( _( "moving dagger (?)" ), g_config->katarina.e_moving_dagger )
                      ->set_tooltip( _( "Dagger -> Flash -> Dagger -> E combo, only in FULL COMBO" ) );

            r_settings->checkbox( _( "enable" ), g_config->katarina.r_enabled );
            r_settings->checkbox( _( "allow execute" ), g_config->katarina.r_execute );
            r_settings->checkbox( _( "overpredict damage " ), g_config->katarina.r_overpredict );
            r_settings->slider_int( _( "min targets" ), g_config->katarina.r_min_targets, 1, 4, 1 );

            drawings->checkbox( _( "draw q range" ), g_config->katarina.q_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->katarina.e_draw_range );
            drawings->checkbox( _( "draw r range" ), g_config->katarina.r_draw_range );
            drawings->checkbox( _( "draw dagger pickup range" ), g_config->katarina.draw_dagger_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->katarina.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->katarina.q_draw_range->get< bool >( ) &&
                !g_config->katarina.e_draw_range->get< bool >( ) &&
                !g_config->katarina.r_draw_range->get< bool >( ) &&
                !g_config->katarina.draw_dagger_range->get< bool >( ) ||
                g_local->is_dead( )
            )
                return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->katarina.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( ) || !g_config->katarina.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
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

            if ( g_config->katarina.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( ) || !g_config->katarina.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 255, 255, 0, 255 ),
                        m_e_range,
                        Renderer::outline,
                        64,
                        3.f
                    );
                }
            }

            if ( g_config->katarina.r_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::r );
                if ( slot && slot->level > 0 && ( slot->is_ready( ) || !g_config->katarina.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 200, 50, 500, 255 ),
                        m_r_range,
                        Renderer::outline,
                        64,
                        3.f
                    );
                }
            }

            //g_render->circle_3d( g_local->position, Color::white( ), 145.f + 340.f, Renderer::outline, 64, 2.f );

            Vec2 sp{ };
            if ( world_to_screen( g_local->position, sp ) ) {
                sp.x += 30.f;
                sp.y -= 20.f;

                // sp.y += 16.f;

                std::string text1{ "[ U ] COMBO MODE: " };
                std::string text_mode{ m_dagger_combo_toggle ? "Q->E" : "E->W" };

                auto size = g_render->get_text_size( text1, g_fonts->get_zabel_12px( ), 12 );

                g_render->text_shadow( sp, Color( 255, 255, 255 ), g_fonts->get_zabel_12px( ), text1.c_str( ), 12 );

                g_render->text_shadow(
                    { sp.x + size.x, sp.y },
                    Color( 255, 200, 0 ),
                    g_fonts->get_zabel_12px( ),
                    text_mode.c_str( ),
                    12
                );

                sp.y += 12.f;

                text1     = { "[ MB3 ] FLASH COMBO: " };
                text_mode = { m_flash_moving_dagger ? "ON" : "OFF" };

                size = g_render->get_text_size( text1, g_fonts->get_zabel_12px( ), 12 );

                g_render->text_shadow( sp, Color( 255, 255, 255 ), g_fonts->get_zabel_12px( ), text1.c_str( ), 12 );
                g_render->text_shadow(
                    { sp.x + size.x, sp.y },
                    m_flash_moving_dagger ? Color( 50, 255, 50 ) : Color( 255, 50, 50 ),
                    g_fonts->get_zabel_12px( ),
                    text_mode.c_str( ),
                    12
                );
            }

            if ( !g_config->katarina.draw_dagger_range->get< bool >( ) ) return;

            const auto dagger_radius = 150.f;

            for ( const auto dagger : m_logged_daggers ) {
                if ( !world_to_screen( dagger.position, sp ) ) continue;

                auto text = std::to_string( 5.2f - ( *g_time - dagger.spawn_time ) );
                text.resize( 3 );

                const auto ready_delay = dagger.ready_time - dagger.spawn_time;

                //text = std::to_string( static_cast< int >( g_local->position.dist_to( dagger.position ) ) );

                const auto time_until_pickup = *g_time - dagger.spawn_time;
                const auto local_distance    = g_local->position.dist_to( dagger.position );

                if ( g_local->position.dist_to( dagger.position ) <= 775.f ) {
                    auto start_extend = g_local->position.extend( dagger.position, 50.f );
                    auto extended     = g_local->position.extend( dagger.position, local_distance - 150.f );

                    if ( dagger.position.dist_to( g_local->position ) > 200.f ) {
                        g_render->line_3d(
                            start_extend,
                            extended,
                            time_until_pickup < 1.15f ? Color( 255, 255, 255 ) : Color( 25, 255, 25 ),
                            6.f
                        );
                    }


                    for ( auto flash_dagger : m_logged_daggers ) {
                        if ( dagger.index == flash_dagger.index ) continue;

                        const auto distance = dagger.position.dist_to( flash_dagger.position );
                        if ( distance > 400.f + m_pickup_range * 2.f || distance < 300.f ) continue;

                        auto flash_extend_start = dagger.position.extend( flash_dagger.position, 160.f );
                        auto flash_extend_end   = dagger.position.extend( flash_dagger.position, distance - 160.f );

                        //g_render->circle_3d( dagger.position, Color( 255, 255, 0 ), 160.f, Renderer::outline, 32, 2.f );
                        //g_render->circle_3d( flash_dagger.position, Color( 255, 255, 0 ), 160.f, Renderer::outline, 32, 2.f );

                        g_render->line_3d( flash_extend_start, flash_extend_end, Color( 255, 255, 0 ), 5.f );
                    }
                }

                if ( time_until_pickup < ready_delay ) {
                    auto modifier = std::clamp( time_until_pickup / ready_delay, 0.f, 1.f );
                    modifier      = utils::ease::ease_out_sine( modifier );

                    g_render->circle_3d(
                        dagger.position,
                        time_until_pickup > ready_delay ? Color( 255, 255, 255, 40 ) : Color( 10, 10, 10, 40 ),
                        dagger_radius,
                        Renderer::outline | Renderer::filled,
                        32,
                        2.f
                    );

                    g_render->circle_3d(
                        dagger.position,
                        Color( 255, 255, 255 ),
                        dagger_radius,
                        Renderer::outline,
                        32,
                        4.f,
                        360.f * modifier
                    );
                } else {
                    const auto modifier =
                        std::clamp( ( *g_time - dagger.ready_time ) / 4.f, 0.f, 1.f );

                    g_render->circle_3d(
                        dagger.position,
                        Color( 25, 20, 25, 40 ),
                        dagger_radius,
                        Renderer::outline | Renderer::filled,
                        32,
                        2.f
                    );

                    g_render->circle_3d(
                        dagger.position,
                        Color( 25, 255, 25 ),
                        dagger_radius,
                        Renderer::outline,
                        32,
                        4.f,
                        360.f - 360.f * modifier
                    );
                }

                const auto size = g_render->get_text_size( text, g_fonts->get_zabel( ), 32 );
                g_render->text_shadow(
                    { sp.x - size.x / 2.f, sp.y - size.y / 2.f },
                    Color( 255, 255, 255 ),
                    g_fonts->get_zabel( ),
                    text.c_str( ),
                    32
                );
            }
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            forced_shunpo( );
            update_daggers( );
            cast_tracking( );

            if ( !m_key_down && GetAsyncKeyState( 0x55 ) ) {
                m_dagger_combo_toggle = !m_dagger_combo_toggle;
                m_key_down            = true;
            } else if ( m_key_down && !GetAsyncKeyState( 0x55 ) ) m_key_down = false;

            if ( !m_flash_key_down && GetAsyncKeyState( 0x04 ) ) {
                m_flash_moving_dagger = !m_flash_moving_dagger;
                m_flash_key_down      = true;
            } else if ( m_flash_key_down && !GetAsyncKeyState( 0x04 ) ) m_flash_key_down = false;

            update_ult_state( );

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                moving_dagger_walk( );
                moving_dagger_flash( );

                spell_e( );
            //spell_w( );
                spell_r( );
                spell_q( );
                break;

            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->katarina.q_harass->get< bool >( ) ) spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::laneclear:
            case Orbwalker::EOrbwalkerMode::lasthit:
                if ( g_config->katarina.q_harass->get< bool >( ) && !helper::is_position_under_turret(
                    g_local->position
                ) )
                    spell_q( );

                lasthit_q( );
                break;
            default:
                break;
            }

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return;

            if ( GetAsyncKeyState( 0x38 ) ) {
                std::cout << "\n[ Passive ]: " << get_dagger_damage( target->index )
                    << "\n[ E ]: " << get_shunpo_damage( target->index ) << std::endl;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->katarina.q_enabled->get< bool >( ) || !m_allow_spells || *g_time - m_last_q_time <= 0.4f ||
                !m_slot_q->is_ready( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) >= m_q_range
                || g_features->prediction->windwall_in_line( g_local->position, target->position ) )
                return false;

            if ( cast_spell( ESpellSlot::q, target->network_id ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( true || !g_config->katarina.w_enabled->get< bool >( ) || !m_allow_spells || *g_time - m_last_w_time <=
                0.4f || !m_slot_w->is_ready( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) > 200.f ) return false;

            auto pred = g_features->prediction->predict_default( target->index, 0.25f );
            if ( !pred || g_local->position.dist_to( *pred ) > 275.f ) return false;

            if ( cast_spell( ESpellSlot::w ) ) {
                m_last_w_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->katarina.e_enabled->get< bool >( ) || !m_allow_spells || *g_time - m_last_e_time <= 0.3f ||
                !m_slot_e->is_ready( ) )
                return false;

            for ( auto i = 0; i < 2; i++ ) {
                if ( g_features->target_selector->is_forced( ) && i > 0 ) break;

                const auto target = g_features->target_selector->get_default_target( i > 0 );
                if ( !target ) break;

                if ( killsteal_e( target ) || dagger_e( target ) ) return true;
            }


            //aa_reset_e( );
            if ( combo_e( ) ) return true;

            return false;
        }

        auto dagger_e( Object* target ) -> float{
            if ( !target || !g_config->katarina.e_dagger->get< bool >( ) || !m_allow_spells ) return false;

            Vec3 cast_position{ };
            bool cast_allowed{ };

            for ( const auto dagger : m_logged_daggers ) {
                if ( dagger.ready_time > *g_time + g_features->orbwalker->get_ping( ) || g_local->position.dist_to(
                        dagger.position
                    ) > 775.f ||
                    g_local->position.dist_to( dagger.position ) <= 250.f ||
                    target->position.dist_to( dagger.position ) >= 480.f )
                    continue;

                cast_position = dagger.position.extend(
                    target->position,
                    target->position.dist_to( dagger.position ) <= 160.f
                        ? target->position.dist_to( dagger.position )
                        : 160.f
                );
                cast_allowed = true;
                break;
            }

            if ( !cast_allowed || helper::is_position_under_turret( cast_position ) && !
                helper::is_position_under_turret( g_local->position ) )
                return false;

            if ( cast_spell( ESpellSlot::e, cast_position ) ) {
                m_last_e_time = *g_time;
                g_features->orbwalker->on_cast( );
                std::cout << "[ Katarina: E ] Dagger cast targeting " << target->get_name( ) << std::endl;
                return true;
            }

            return false;
        }

        auto killsteal_e( Object* target ) -> bool{
            if ( !target || !g_config->katarina.e_killsteal->get< bool >( ) || target->dist_to_local( ) >
                m_e_range )
                return false;

            const auto damage = get_shunpo_damage( target->index ) + helper::get_aa_damage( target->index, true );
            const auto health = helper::get_real_health( target->index, EDamageType::magic_damage, 0.f, false );
            if ( damage < health ) return false;

            if ( cast_spell( ESpellSlot::e, target->position ) ) {
                m_last_e_time = *g_time;
                g_features->orbwalker->on_cast( );
                std::cout << "[ Katarina: E ] Killsteal cast on " << target->get_name( ) << std::endl;
                return true;
            }

            return false;
        }

        auto aa_reset_e( ) -> bool{
            if ( g_features->orbwalker->get_mode( ) != Orbwalker::EOrbwalkerMode::combo
                || *g_time - m_last_e_time <= 0.3f || !g_features->orbwalker->should_reset_aa( )
                || g_features->orbwalker->get_last_target( ) != ETargetType::hero || !m_slot_e->is_ready( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || !g_features->orbwalker->
                                         is_attackable( target->index, g_local->attack_range + 50.f ) )
                return false;

            if ( g_features->orbwalker->get_last_target( ) != ETargetType::hero ) return false;

            if ( cast_spell(
                ESpellSlot::e,
                g_local->position.extend( target->position, target->dist_to_local( ) + 100.f )
            ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                std::cout << "[ Katarina: E ] AA reset | " << *g_time << std::endl;
                return true;
            }

            return false;
        }

        auto combo_e( ) -> bool{
            if ( m_dagger_combo_toggle || !m_slot_w->is_ready( ) ) return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) > 720.f ||
                helper::is_position_under_turret( target->position ) )
                return false;

            if ( cast_spell(
                ESpellSlot::e,
                target->position.extend( target->position + target->get_direction( ), 75.f )
            ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                std::cout << "[ Katarina: E ] Combo cast at " << target->get_name( ) << std::endl;
                return true;
            }

            return false;
        }

        auto moving_dagger_flash( ) -> bool{
            if ( !g_config->katarina.e_moving_dagger->get< bool >( ) || !m_flash_moving_dagger
                || *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready( ) && m_slot_e->cooldown_expire > *g_time +
                get_shunpo_cooldown_refund( ) ||
                m_e_active && m_e_server_cast_time > *g_time + g_features->orbwalker->get_ping( ) || *g_time -
                m_last_pickup_time > 0.225f )
                return false;

            auto flash_slot{ ESpellSlot::d };
            auto spell = g_local->spell_book.get_spell_slot( ESpellSlot::d );
            if ( !spell || spell->get_name( ).find( "Flash" ) == std::string::npos ) {
                spell = g_local->spell_book.get_spell_slot( ESpellSlot::f );
                if ( !spell || spell->get_name( ).find( "Flash" ) == std::string::npos ) return false;

                flash_slot = ESpellSlot::f;
            }

            if ( !spell->is_ready( ) ) return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            bool found_dagger{ };
            Vec3 dagger_position{ };
            for ( auto inst : m_logged_daggers ) {
                if ( inst.ready_time > *g_time || inst.position.dist_to( g_local->position ) > 750.f || inst.position.
                    dist_to( g_local->position ) <= 225.f || inst.position.dist_to( target->position ) > m_e_range +
                    m_pickup_range )
                    continue;

                auto points = g_render->get_3d_circle_points( inst.position, 175.f, 32 );

                Vec3 best_point{ };
                auto lowest_distance{ 1000.f };

                for ( auto point : points ) {
                    if ( point.dist_to( g_local->position ) > 425.f || point.dist_to( target->position ) >= 710.f ||
                        g_navgrid->is_wall( point ) )
                        continue;

                    const auto distance = point.dist_to( target->position );
                    if ( distance > lowest_distance ) continue;

                    best_point      = point;
                    lowest_distance = distance;
                }

                if ( best_point.length( ) <= 0.f ) continue;

                dagger_position = best_point;
                found_dagger    = true;
                break;
            }

            if ( !found_dagger ) return false;

            if ( cast_spell( flash_slot, dagger_position ) ) {
                m_last_e_time = *g_time;
                g_features->orbwalker->on_cast( );

                std::cout << "[ Katarina: E ] Moving dagger combo at " << target->name.text << " | " << *g_time
                    << std::endl;

                m_shunpo_target_nid  = target->network_id;
                m_shunpo_position    = target->position;
                m_shunpo_expire_time = *g_time + 0.033f;
                m_shunpo_ordered     = true;
                return true;
            }

            return false;
        }

        auto moving_dagger_walk( ) -> bool{
            if ( *g_time - m_last_e_time <= 0.4f ||
                !m_slot_e->is_ready( ) && m_slot_e->cooldown_expire > *g_time + get_shunpo_cooldown_refund( ) ||
                *g_time - m_last_pickup_time > 0.25f )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) >= m_e_range ) return false;

            auto       local_position = g_local->position;
            const auto pred           = g_features->prediction->predict_movement( g_local->index, 0.1f );
            if ( pred.has_value( ) ) local_position = pred.value( );

            bool found_dagger{ };

            if ( !found_dagger ) {
                for ( auto inst : m_logged_daggers ) {
                    if ( inst.ready_time >= *g_time + g_features->orbwalker->get_ping( )
                        || inst.position.dist_to( local_position ) >= m_pickup_range && inst.position.dist_to(
                            g_local->position
                        ) >= m_pickup_range )
                        continue;

                    found_dagger = true;
                    break;
                }
            }

            if ( !found_dagger ) return false;

            m_shunpo_ordered     = true;
            m_shunpo_position    = target->position;
            m_shunpo_expire_time = *g_time + 0.3f;

            std::cout << "[ Katarina: E ] Moving dagger walking combo at " << target->name.text << " | " << *g_time
                << std::endl;

            return false;
        }

        auto forced_shunpo( ) -> void{
            if ( !m_shunpo_ordered || m_shunpo_expire_time <= *g_time ) return;

            if ( cast_spell( ESpellSlot::e, m_shunpo_position ) ) {
                m_shunpo_ordered    = m_shunpo_expire_time > *g_time;
                m_shunpo_target_nid = 0;
                g_features->orbwalker->on_cast( );
            }
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->katarina.r_enabled->get< bool >( ) || !m_allow_spells || *g_time - m_last_r_time <= 0.4f ||
                !m_slot_r->is_ready( ) )
                return false;

            /* if (m_slot_r->get_usable_state() != 0 && m_slot_r->get_usable_state() != 256)
             {
                 if (m_slot_r->get_usable_state() != 1 && m_slot_r->get_usable_state() != 257)
                     std::cout << "state: " << m_slot_r->get_usable_state() << std::endl;
 
                 return false;
             }*/


            bool allow_cast{ };
            int  hit_count{ };
            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->dist_to_local( ) >= m_r_range || g_features->target_selector->is_bad_target(
                    enemy->index
                ) )
                    continue;

                ++hit_count;
            }

            if ( hit_count >= g_config->katarina.r_min_targets->get< int >( ) ) allow_cast = true;

            if ( !allow_cast && g_config->katarina.r_execute->get< bool >( ) ) {
                const auto target = g_features->target_selector->get_default_target( );
                if ( !target || target->dist_to_local( ) >= m_r_range ) return false;

                const auto escape_distance = m_r_range - target->dist_to_local( );
                auto       damage_time     = escape_distance / target->movement_speed;
                auto       dagger_damage   = helper::calculate_damage(
                    m_r_dagger_damage[ m_slot_r->level ] + g_local->ability_power( ) * 0.19f,
                    target->index,
                    false
                );
                dagger_damage += helper::calculate_damage(
                    g_local->bonus_attack_damage( ) * 0.18f,
                    target->index,
                    true
                );

                if ( g_config->katarina.r_overpredict->get< bool >( ) ) damage_time += 0.166f * 3.f;

                if ( damage_time >= 2.5f ) damage_time = 2.5f;

                if ( std::ceil( damage_time / 0.166f ) * dagger_damage >= target->health ) allow_cast = true;
            }

            if ( !allow_cast ) return false;

            if ( m_slot_w->is_ready( ) ) {
                if ( cast_spell( ESpellSlot::w ) ) {
                    m_last_w_time = *g_time;
                    return true;
                }
            }

            if ( cast_spell( ESpellSlot::r ) ) {
                m_last_r_time = *g_time;
                return true;
            }

            return false;
        }

        auto update_ult_state( ) -> void{
            if ( m_slot_r->level == 0 ) return;

            auto sci = g_local->spell_book.get_spell_cast_info( );
            if ( !sci || sci->slot != 3 ) {
                m_allow_spells = true;
                return;
            }

            bool found_target{ };
            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->dist_to_local( ) > 725.f || g_features->target_selector->is_bad_target(
                    enemy->index
                ) )
                    continue;

                if ( m_slot_e->is_ready( ) && killsteal_e( enemy ) ) {
                    m_allow_spells = true;
                    g_features->orbwalker->disable_for( 0.f );
                    return;
                }

                if ( enemy->dist_to_local( ) > 425.f ) continue;

                found_target = true;
            }

            if ( found_target ) m_allow_spells = false;
            else m_allow_spells                = true;

            if ( !m_allow_spells ) g_features->orbwalker->disable_for( 0.05f );
        }

        auto cast_tracking( ) -> void{
            if ( !m_q_active ) {
                auto sci = g_local->spell_book.get_spell_cast_info( );
                if ( sci && sci->slot == 0 && sci->server_cast_time > *g_time ) {
                    m_q_active           = true;
                    m_q_server_cast_time = sci->server_cast_time;
                    m_q_target_index     = sci->get_target_index( );
                }
            } else {
                if ( m_dagger_combo_toggle && *g_time - m_last_cast_time > 0.1f && m_slot_e->is_ready( ) &&
                    m_slot_w->is_ready( ) && *g_time + g_features->orbwalker->get_ping( ) * 2.f > m_q_server_cast_time
                    && *g_time ) {
                    const auto target = g_entity_list.get_by_index( m_q_target_index );
                    if ( target && target->is_alive( ) && target->is_visible( ) && m_e_range > target->
                        dist_to_local( ) ) {
                        auto       allow_shunpo{ true };
                        const auto cast_position = g_local->position.extend(
                            target->position,
                            target->dist_to_local( ) + 75.f
                        );
                        if ( helper::is_position_under_turret( cast_position ) ) allow_shunpo = false;

                        if ( allow_shunpo && cast_spell( ESpellSlot::e, cast_position ) ) {
                            m_last_e_time    = *g_time;
                            m_last_cast_time = *g_time;
                            g_features->orbwalker->on_cast( );
                        }
                    }
                }


                if ( *g_time > m_q_server_cast_time ) {
                    //m_waiting_for_dagger = true;
                    m_q_active = false;
                }
            }

            if ( !m_e_active ) {
                auto sci = g_local->spell_book.get_spell_cast_info( );
                if ( sci && sci->slot == 49 && sci->server_cast_time > *g_time ) {
                    m_e_active           = true;
                    m_e_server_cast_time = sci->server_cast_time;
                    m_e_name             = sci->get_spell_name( );
                }
            } else {
                if ( g_config->katarina.w_after_shunpo->get< bool >( ) &&
                    *g_time - m_last_w_time > 0.25f && rt_hash( m_e_name.data( ) ) == ct_hash( "KatarinaE" )
                    && m_slot_w->is_ready( ) ) {
                    if ( cast_spell( ESpellSlot::w ) ) {
                        m_last_w_time    = *g_time;
                        m_last_cast_time = *g_time;
                        g_features->orbwalker->on_cast( );

                        std::cout << "[ Kata: W ] Force cast after E to dagger\n";
                    }
                }

                if ( *g_time > m_e_server_cast_time ) m_e_active = false;
            }
        }

        static auto get_dagger_damage( const int16_t target_index ) -> float{
            float raw_damage{ };

            switch ( g_local->level ) {
            case 1:
                raw_damage = 68.f;
                break;
            case 2:
                raw_damage = 72.f;
                break;
            case 3:
                raw_damage = 77.f;
                break;
            case 4:
                raw_damage = 82.f;
                break;
            case 5:
                raw_damage = 89.f;
                break;
            case 6:
                raw_damage = 96.f;
                break;
            case 7:
                raw_damage = 103.f;
                break;
            case 8:
                raw_damage = 112.f;
                break;
            case 9:
                raw_damage = 121.f;
                break;
            case 10:
                raw_damage = 131.f;
                break;
            case 11:
                raw_damage = 142.f;
                break;
            case 12:
                raw_damage = 154.f;
                break;
            case 13:
                raw_damage = 166.f;
                break;
            case 14:
                raw_damage = 180.f;
                break;
            case 15:
                raw_damage = 194.f;
                break;
            case 16:
                raw_damage = 208.f;
                break;
            case 17:
                raw_damage = 224.f;
                break;
            default:
                raw_damage = 240.f;
                break;
            }

            auto ap_ratio = 0.7f;
            if ( g_local->level >= 16 ) ap_ratio = 1.f;
            else if ( g_local->level >= 11 ) ap_ratio = 0.9f;
            else if ( g_local->level >= 6 ) ap_ratio = 0.8f;

            raw_damage += g_local->ability_power( ) * ap_ratio;
            raw_damage += g_local->bonus_attack_damage( ) * 0.6f;

            return helper::calculate_damage( raw_damage, target_index, false ) + helper::get_onhit_damage(
                target_index
            );
        }

        auto get_shunpo_damage( const int16_t target_index ) -> float{
            const auto raw_damage = m_e_damage[ m_slot_e->level ] + g_local->attack_damage( ) * 0.4f + g_local->
                ability_power( ) * 0.25f;

            return helper::calculate_damage( raw_damage, target_index, false ) + helper::get_onhit_damage(
                target_index
            );
        }

        auto get_shunpo_cooldown_refund( ) -> float{
            const auto base_cooldown = m_slot_e->cooldown;

            if ( g_local->level >= 16 ) return base_cooldown * 0.96f;
            if ( g_local->level >= 11 ) return base_cooldown * 0.9f;
            if ( g_local->level >= 6 ) return base_cooldown * 0.84f;

            return base_cooldown * 0.78f;
        }

        auto lasthit_q( ) -> bool{
            if ( !g_config->katarina.q_lasthit->get< bool >( ) || *g_time - m_last_q_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.1f || !m_slot_q->is_ready( ) )
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

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ m_slot_q->level ] + g_local->ability_power( ) * 0.35f,
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
                return 0.25f + g_local->position.dist_to( target->position ) / 1600.f;
            default:
                return 0.f;
            }

            return std::numeric_limits< float >::max( );
        }

        auto is_dagger_logged( const unsigned network_id ) const -> bool{
            for ( auto& inst : m_logged_daggers ) if ( inst.network_id == network_id ) return true;

            return false;
        }

        auto update_daggers( ) -> void{
            for ( auto& inst : m_logged_daggers ) {
                if ( inst.spawn_time + 5.175f <= *g_time ) {
                    m_ignored_daggers.push_back( inst.network_id );
                    remove_dagger( inst.network_id );
                }

                auto dagger = g_entity_list.get_by_index( inst.index );
                if ( !dagger ) {
                    m_ignored_daggers.push_back( inst.network_id );
                    remove_dagger( inst.network_id );
                    continue;
                }

                dagger.update( );

                if ( dagger->is_dead( ) ) {
                    std::cout << "Picked up dagger | " << *g_time - inst.spawn_time
                        << " Pickup delay: " << *g_time - m_last_pickup_time << " | time: " << *g_time << std::endl;

                    if ( *g_time - m_last_pickup_time < 0.25f ) m_can_move_dagger = true;
                    else m_can_move_dagger                                        = false;

                    m_last_pickup_time = *g_time;
                    m_ignored_daggers.push_back( inst.network_id );
                    remove_dagger( inst.network_id );
                }
            }

            for ( const auto obj : g_entity_list.get_ally_minions( ) ) {
                if ( !obj || obj->is_dead( ) || rt_hash( obj->name.text ) != ct_hash( "HiddenMinion" ) ||
                    is_dagger_logged( obj->network_id ) || is_dagger_ignored( obj->network_id ) || obj->
                    get_owner_index( ) != g_local->index )
                    continue;

                bool is_w_dagger{ };
                auto spawn_time = *g_time;


                if ( obj->dist_to_local( ) < 100.f ) {
                    m_slot_w.update( );

                    if ( m_slot_w->cooldown_expire > *g_time && *g_time + 0.1f > m_slot_w->cooldown_expire - m_slot_w->
                        cooldown ) {
                        spawn_time = m_slot_w->cooldown_expire - m_slot_w->cooldown;

                        //std::cout << "[ W Dagger ] Calculated spawn: " << spawn_time << " | gtime: " << *g_time
                        //       << std::endl;
                        is_w_dagger = true;
                    }
                }


                //std::cout << "Added dagger | W: " << is_w_dagger << std::endl;

                m_logged_daggers.push_back(
                    {
                        obj->network_id,
                        obj->index,
                        spawn_time,
                        is_w_dagger ? spawn_time + 1.255f : spawn_time + 1.f,
                        obj->position
                    }
                );
            }
        }

        auto remove_dagger( const unsigned network_id ) -> void{
            const auto to_remove = std::ranges::remove_if(
                m_logged_daggers,
                [&]( const dagger_instance_t& dagger ) -> bool{ return dagger.network_id == network_id; }
            );

            if ( to_remove.empty( ) ) return;

            m_logged_daggers.erase( to_remove.begin( ), to_remove.end( ) );
        }

        auto get_dagger_instance( const Vec3& position ) const -> std::optional< dagger_instance_t >{
            for ( auto& inst : m_logged_daggers ) if ( inst.position == position ) return inst;

            return std::nullopt;
        }

        auto is_dagger_ignored( const unsigned network_id ) const -> bool{
            for ( const auto inst : m_ignored_daggers ) if ( inst == network_id ) return true;

            return false;
        }

    private:
        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_r_time{ };
        float m_last_cast_time{ };

        bool m_waiting_for_dagger{ };

        // shit
        bool m_dagger_combo_toggle{ };
        bool m_key_down{ };

        bool m_flash_moving_dagger{ };
        bool m_flash_key_down{ };

        // cast tracking
        bool    m_q_active{ };
        float   m_q_server_cast_time{ };
        int16_t m_q_target_index{ };

        bool        m_e_active{ };
        float       m_e_server_cast_time{ };
        std::string m_e_name{ };

        // force shunpo
        unsigned m_shunpo_target_nid{ };
        Vec3     m_shunpo_position{ };
        bool     m_shunpo_ordered{ };
        float    m_shunpo_expire_time{ };

        std::vector< float > m_q_damage = { 0.f, 80.f, 110.f, 140.f, 170.f, 200.f };
        std::vector< float > m_e_damage = { 0.f, 20.f, 35.f, 50.f, 65.f, 80.f };
        std::vector< float > m_r_damage = { 0.f, 80.f, 100.f, 120.f };

        std::vector< float > m_r_dagger_damage = { 0.f, 25.f, 37.5f, 50.f };

        std::mutex                       m_daggers_mutex;
        std::vector< Vec3 >              m_daggers{ };
        std::vector< dagger_instance_t > m_logged_daggers{ };
        std::vector< unsigned >          m_ignored_daggers{ };

        // dagger pickup track
        float m_last_pickup_time{ };
        bool  m_can_move_dagger{ };
        bool  m_in_dagger_cast{ };

        float m_e_ready_time{ };

        float m_q_range{ 625.f };
        float m_w_range{ 0.f };
        float m_e_range{ 725.f };
        float m_r_range{ 550.f };

        bool m_allow_spells{ true };

        float m_dagger_range{ 340.f };
        float m_pickup_range{ 200.f };
    };
}
