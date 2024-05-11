#pragma once
#include "module.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"
#include "../evade.hpp"
#include "../../menu/menu.hpp"
#include "../../sdk/game/ai_manager.hpp"

namespace features::champion_modules {
    class zeri_module final : public IModule {
    public:
        virtual ~zeri_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "zeri_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Zeri" ); }

        auto initialize( ) -> void override{
        }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "zeri" ), menu_order::champion_module );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto special    = navigation->add_section( _( "orbwalker" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );
            const auto bonus      = navigation->add_section( _( "bonus" ) );

            w_settings->checkbox( _( "enable" ), g_config->zeri.w_enabled );
            w_settings->checkbox( _( "extend w on walls" ), g_config->zeri.w_extend_wall );
            w_settings->checkbox( _( "dont use if enemy in aa range" ), g_config->zeri.w_closerange_limit );
            w_settings->select(
                _( "hitchance" ),
                g_config->zeri.w_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            e_settings->checkbox( _( "enable autoslide" ), g_config->zeri.e_autoslide_enable );

            r_settings->checkbox( _( "enable" ), g_config->zeri.r_enabled );
            r_settings->slider_int( _( "minimum hitcount" ), g_config->zeri.r_minimum_hitcount, 1, 5, 1 );

           // special->checkbox( _( "use zeri orbwalker" ), g_config->zeri.use_zeri_orbwalker );
            special->checkbox( _( "allow aa if full-charge" ), g_config->zeri.allow_aa_on_fullcharge );
            special->checkbox( _( "allow aa if executable" ), g_config->zeri.allow_aa_on_low_hp_enemy );
            special->checkbox( _( "DONT aa minion if full-charge" ), g_config->zeri.dont_aa_minion_on_full_charge );

            drawings->checkbox( _( "draw q range" ), g_config->zeri.q_draw_range );
            drawings->checkbox( _( "draw w range" ), g_config->zeri.w_draw_range );
            drawings->checkbox( _( "draw r range" ), g_config->zeri.r_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->zeri.dont_draw_on_cooldown );

            bonus->checkbox( _( "increase q speed (?)" ), g_config->zeri.buffer_q_cast )->set_tooltip(
                _( "buffers q cast before its ready, increases DPS" )
            );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->zeri.q_draw_range->get< bool >( ) && !g_config->zeri.w_draw_range->get< bool >( ) && !
                g_config->zeri.r_draw_range->get< bool >( ) || g_local->is_dead( ) )
                return;

            g_local.update( );

            auto q_slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
            if ( g_config->zeri.q_draw_range->get< bool >( ) && q_slot && q_slot->level > 0 && ( q_slot->
                is_ready( true ) || !g_config->zeri.dont_draw_on_cooldown->get< bool >( ) ) ) {
                g_render->circle_3d(
                    g_local->position,
                    Color( 31, 88, 255, 255 ),
                    m_q_range,
                    Renderer::outline,
                    80,
                    2.f
                );
            }

            auto w_slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
            if ( g_config->zeri.w_draw_range->get< bool >( ) && w_slot && w_slot->level > 0 && ( w_slot->
                is_ready( true ) || !g_config->zeri.dont_draw_on_cooldown->get< bool >( ) ) ) {
                g_render->circle_3d(
                    g_local->position,
                    Color( 132, 245, 66, 255 ),
                    m_w_range,
                    Renderer::outline,
                    80,
                    2.f
                );
            }

            auto r_slot = g_local->spell_book.get_spell_slot( ESpellSlot::r );
            if ( g_config->zeri.r_draw_range->get< bool >( ) && r_slot && r_slot->level > 0 && ( r_slot->
                is_ready( true ) && !m_in_ult || !g_config->zeri.dont_draw_on_cooldown->get< bool >( ) ) ) {
                g_render->
                    circle_3d( g_local->position, Color( 173, 47, 68, 255 ), 800.f, Renderer::outline, 80, 2.f );
            }

            if ( !g_config->zeri.e_autoslide_enable->get< bool >( ) ) return;

            auto e_slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
            if ( e_slot && e_slot->is_ready( true ) && g_features->orbwalker->get_mode( ) ==
                Orbwalker::EOrbwalkerMode::none ) {
                for ( auto i = 1; i <= m_slide_count; i++ ) {
                    Vec3 slide_start{ };
                    Vec3 slide_end{ };

                    switch ( i ) {
                    case 1:
                        slide_start = m_slide1_stand_position;
                        slide_end = m_slide2_stand_position;
                        break;
                    case 2:
                        slide_start = m_slide2_stand_position;
                        slide_end = m_slide1_stand_position;
                        break;
                    case 3:
                        slide_start = m_slide3_stand_position;
                        slide_end = m_slide4_stand_position;
                        break;
                    case 4:
                        slide_start = m_slide4_stand_position;
                        slide_end = m_slide3_stand_position;
                        break;
                    case 5:
                        slide_start = m_slide5_stand_position;
                        slide_end = m_slide6_stand_position;
                        break;
                    case 6:
                        slide_start = m_slide6_stand_position;
                        slide_end = m_slide5_stand_position;
                        break;
                    case 7:
                        slide_start = m_slide7_stand_position;
                        slide_end = m_slide8_stand_position;
                        break;
                    case 8:
                        slide_start = m_slide8_stand_position;
                        slide_end = m_slide7_stand_position;
                        break;
                    case 9:
                        slide_start = m_slide9_stand_position;
                        slide_end = m_slide10_stand_position;
                        break;
                    case 10:
                        slide_start = m_slide10_stand_position;
                        slide_end = m_slide9_stand_position;
                        break;
                    case 11:
                        slide_start = m_slide11_stand_position;
                        slide_end = m_slide12_stand_position;
                        break;
                    case 12:
                        slide_start = m_slide12_stand_position;
                        slide_end = m_slide11_stand_position;
                        break;
                    case 13:
                        slide_start = m_slide13_stand_position;
                        slide_end = m_slide14_stand_position;
                        break;
                    case 14:
                        slide_start = m_slide14_stand_position;
                        slide_end = m_slide13_stand_position;
                        break;
                    case 15:
                        slide_start = m_slide15_stand_position;
                        slide_end = m_slide16_stand_position;
                        break;
                    case 16:
                        slide_start = m_slide16_stand_position;
                        slide_end = m_slide15_stand_position;
                        break;
                    case 17:
                        slide_start = m_slide17_stand_position;
                        slide_end = m_slide18_stand_position;
                        break;
                    case 18:
                        slide_start = m_slide18_stand_position;
                        slide_end = m_slide17_stand_position;
                        break;
                    case 19:
                        slide_start = m_slide19_stand_position;
                        slide_end = m_slide20_stand_position;
                        break;
                    case 20:
                        slide_start = m_slide20_stand_position;
                        slide_end = m_slide19_stand_position;
                        break;
                    case 21:
                        slide_start = m_slide21_stand_position;
                        slide_end = m_slide22_stand_position;
                        break;
                    case 22:
                        slide_start = m_slide22_stand_position;
                        slide_end = m_slide21_stand_position;
                        break;
                    case 23:
                        slide_start = m_slide23_stand_position;
                        slide_end = m_slide24_stand_position;
                        break;
                    case 24:
                        slide_start = m_slide24_stand_position;
                        slide_end = m_slide23_stand_position;
                        break;
                    default:
                        break;
                    }

                    if ( m_active_slide == slide_start ) {
                        g_render->circle_3d(
                            slide_start,
                            Color::green( ),
                            m_slide_position_range,
                            2,
                            50,
                            2.f
                        );
                    } else {
                        g_render->circle_3d(
                            slide_start,
                            Color( 31, 88, 255, 255 ),
                            m_slide_position_range,
                            2,
                            50,
                            2.f
                        );
                    }

                    Vec2 line_start{ };
                    Vec2 line_end{ };

                    if ( world_to_screen( slide_start.extend( slide_end, m_slide_position_range + 25.f ), line_start )
                        && world_to_screen(
                            slide_start.extend( slide_end, m_slide_position_range + 200.f ),
                            line_end
                        ) )
                        g_render->line( line_start, line_end, Color::white( ), 2.f );

                    if ( g_local->position.dist_to( slide_start ) <= 10.f ) {
                        Vec2 sp{ };
                        if ( !sdk::math::world_to_screen( g_local->position, sp ) ) return;

                        g_render->text_shadow(
                            { sp.x - 40, sp.y + 20.f },
                            Color::white( ),
                            g_fonts->get_bold( ),
                            "PRESS U TO SLIDE",
                            16
                        );
                    }
                }
            }
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return;

            m_in_ult      = m_slot_q->get_usable_state( ) > 1;
            m_is_piercing = m_slot_q->get_usable_state( ) == 1 || m_slot_q->get_usable_state( ) == 3;

            m_q_range = 750.f + ( g_local->attack_range - 500.f );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                if ( spell_r( ) ) return;
                if ( spell_w( ) ) return;
                if ( w_extend_walls( ) ) return;
                if ( spell_q( ) ) return;
                break;
            case Orbwalker::EOrbwalkerMode::laneclear:
                if ( !is_position_in_turret_range( ) && spell_q( ) ) return;
                if ( q_laneclear( ) ) return;
                break;
            case Orbwalker::EOrbwalkerMode::lasthit:
                if ( q_lasthit( ) ) return;
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                if ( spell_q( ) ) return;
                if ( q_lasthit( ) ) return;
                break;
            case Orbwalker::EOrbwalkerMode::none:
                e_helper( );
                break;
            default:
                break;
            }

            if ( g_features->orbwalker->get_mode( ) != Orbwalker::EOrbwalkerMode::none ) {
                m_active_slide = Vec3(
                    0,
                    0,
                    0
                );
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( *g_time - m_last_q_time <= 0.15f || *g_time - m_last_cast_time <= 0.025f ||
                !can_cast_q( ) )
                return false;

            for ( auto i = 0; i < 3; i++ ) {
                if ( g_features->target_selector->is_forced( ) && i > 0 ) break;


                Object* target{ };

                switch ( i ) {
                case 0:
                    target = g_features->target_selector->get_spell_specific_target(
                        m_q_range,
                        [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                        [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); }
                    );
                    break;
                default:
                    target = g_features->target_selector->get_default_target( i > 1 );
                    break;
                }

                if ( combo_q( target ) ) return true;
            }

            return false;
        }

        auto combo_q( Object* target ) -> bool{
            if ( !target ) return false;

            const auto is_penetrating = m_in_ult || m_is_piercing;

            const auto missile_speed = m_in_ult ? 3400.f : 2600.f;
            const auto pred          = g_features->prediction->predict(
                target->index,
                m_q_range,
                missile_speed,
                40.f,
                g_features->orbwalker->get_attack_cast_delay( ) + get_buffer_delay( ),
                { },
                true
            );

            std::cout << "Ping: " << g_features->orbwalker->get_ping( ) << std::endl;

            if ( !pred.valid || !is_penetrating && g_features->prediction->minion_in_line(
                g_local->position,
                pred.position,
                40.f
            ) )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;

                if ( get_buffer_delay( ) > 0.f ) {
                    g_features->orbwalker->disable_for( m_slot_q->cooldown_expire - *g_time + 0.075f );
                    std::cout << "Disabled orbwalker for " << m_slot_q->cooldown_expire - *g_time + 0.075f
                        << " | T: " << *g_time << std::endl;
                }

                std::cout << "[ Zeri: Q ] Cast " << *g_time << std::endl;

                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->zeri.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || *g_time -
                m_last_cast_time <= 0.1f || !m_slot_w->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            if ( g_config->zeri.w_closerange_limit->get< bool >( ) && target->dist_to_local( ) <= g_local->attack_range
                + 65.f )
                return false;

            const auto delay = std::max(
                0.55f - g_local->bonus_attack_speed * 0.1f + g_local->bonus_attack_speed * 0.01f,
                0.3f
            );

            const auto pred = g_features->prediction->predict(
                target->index,
                m_w_range,
                m_w_speed,
                m_w_width,
                delay,
                { },
                true
            );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->zeri.w_hitchance->get
                    < int >( ) )
                || g_features->prediction->minion_in_line( g_local->position, pred.position, m_w_width )
                || get_line_wall_position( g_local->position, pred.position ).has_value( ) )
                return false;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{ return false; }

        auto spell_r( ) -> bool{
            if ( !g_config->zeri.r_enabled->get< bool >( ) || m_in_ult || *g_time - m_last_r_time <= 0.4f || *g_time -
                m_last_cast_time <= 0.1f || !m_slot_r->is_ready( true ) )
                return false;

            int enemy_count{ };

            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy ||
                    g_features->target_selector->is_bad_target( enemy->index ) ||
                    enemy->dist_to_local( ) > m_r_range
                )
                    continue;

                auto predict = g_features->prediction->predict_default( enemy->index, 0.25f );
                if ( !predict || predict.value( ).dist_to( g_local->position ) > m_r_range ) continue;

                enemy_count++;
            }

            if ( enemy_count < g_config->zeri.r_minimum_hitcount->get< int >( ) ) return false;

            if ( cast_spell( ESpellSlot::r ) ) {
                m_last_r_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                return true;
            }

            return false;
        }

        auto e_helper( ) -> void{
            if ( *g_time - m_last_e_time < 1.0f || !g_config->zeri.e_autoslide_enable->get< bool >( ) ) return;

            auto e_slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
            if ( e_slot && e_slot->is_ready( true ) && GetAsyncKeyState( 0x55 ) ) {
                for ( auto i = 1; i <= m_slide_count; i++ ) {
                    Vec3 stand_position{ };
                    Vec3 cast_position{ };

                    switch ( i ) {
                    case 1:
                        stand_position = m_slide1_stand_position;
                        cast_position = m_slide1_cast_position;
                        break;
                    case 2:
                        stand_position = m_slide2_stand_position;
                        cast_position = m_slide2_cast_position;
                        break;
                    case 3:
                        stand_position = m_slide3_stand_position;
                        cast_position = m_slide3_cast_position;
                        break;
                    case 4:
                        stand_position = m_slide4_stand_position;
                        cast_position = m_slide4_cast_position;
                        break;
                    case 5:
                        stand_position = m_slide5_stand_position;
                        cast_position = m_slide5_cast_position;
                        break;
                    case 6:
                        stand_position = m_slide6_stand_position;
                        cast_position = m_slide6_cast_position;
                        break;
                    case 7:
                        stand_position = m_slide7_stand_position;
                        cast_position = m_slide7_cast_position;
                        break;
                    case 8:
                        stand_position = m_slide8_stand_position;
                        cast_position = m_slide8_cast_position;
                        break;
                    case 9:
                        stand_position = m_slide9_stand_position;
                        cast_position = m_slide9_cast_position;
                        break;
                    case 10:
                        stand_position = m_slide10_stand_position;
                        cast_position = m_slide10_cast_position;
                        break;
                    case 11:
                        stand_position = m_slide11_stand_position;
                        cast_position = m_slide11_cast_position;
                        break;
                    case 12:
                        stand_position = m_slide12_stand_position;
                        cast_position = m_slide12_cast_position;
                        break;
                    case 13:
                        stand_position = m_slide13_stand_position;
                        cast_position = m_slide13_cast_position;
                        break;
                    case 14:
                        stand_position = m_slide14_stand_position;
                        cast_position = m_slide14_cast_position;
                        break;
                    case 15:
                        stand_position = m_slide15_stand_position;
                        cast_position = m_slide15_cast_position;
                        break;
                    case 16:
                        stand_position = m_slide16_stand_position;
                        cast_position = m_slide16_cast_position;
                        break;
                    case 17:
                        stand_position = m_slide17_stand_position;
                        cast_position = m_slide17_cast_position;
                        break;
                    case 18:
                        stand_position = m_slide18_stand_position;
                        cast_position = m_slide18_cast_position;
                        break;
                    case 19:
                        stand_position = m_slide19_stand_position;
                        cast_position = m_slide19_cast_position;
                        break;
                    case 20:
                        stand_position = m_slide20_stand_position;
                        cast_position = m_slide20_cast_position;
                        break;
                    case 21:
                        stand_position = m_slide21_stand_position;
                        cast_position = m_slide21_cast_position;
                        break;
                    case 22:
                        stand_position = m_slide22_stand_position;
                        cast_position = m_slide22_cast_position;
                        break;
                    case 23:
                        stand_position = m_slide23_stand_position;
                        cast_position = m_slide23_cast_position;
                        break;
                    case 24:
                        stand_position = m_slide24_stand_position;
                        cast_position = m_slide24_cast_position;
                        break;
                    default:
                        break;
                    }

                    if ( g_local->position.dist_to( stand_position ) <= 10.f && cast_spell(
                        ESpellSlot::e,
                        cast_position
                    ) ) {
                        m_last_e_time  = *g_time;
                        m_active_slide = Vec3( 0, 0, 0 );
                        g_features->orbwalker->set_cast_time( 0.25f );
                        return;
                    }
                }
            }

            // path fixer
            auto aimgr = g_local->get_ai_manager( );
            if ( !aimgr ) return;

            auto path = aimgr->get_path( );
            if ( !aimgr->is_moving || path.size( ) <= 1 || path.size( ) == aimgr->next_path_node ) {
                m_active_slide = Vec3{ 0, 0, 0 };
                return;
            }

            auto path_end = path[ path.size( ) - 1 ];
            bool is_active{ };

            for ( auto i = 1; i <= m_slide_count; i++ ) {
                Vec3 slide_start{ };

                switch ( i ) {
                case 1:
                    slide_start = m_slide1_stand_position;
                    break;
                case 2:
                    slide_start = m_slide2_stand_position;
                    break;
                case 3:
                    slide_start = m_slide3_stand_position;
                    break;
                case 4:
                    slide_start = m_slide4_stand_position;
                    break;
                case 5:
                    slide_start = m_slide5_stand_position;
                    break;
                case 6:
                    slide_start = m_slide6_stand_position;
                    break;
                case 7:
                    slide_start = m_slide7_stand_position;
                    break;
                case 8:
                    slide_start = m_slide8_stand_position;
                    break;
                case 9:
                    slide_start = m_slide9_stand_position;
                    break;
                case 10:
                    slide_start = m_slide10_stand_position;
                    break;
                case 11:
                    slide_start = m_slide11_stand_position;
                    break;
                case 12:
                    slide_start = m_slide12_stand_position;
                    break;
                case 13:
                    slide_start = m_slide13_stand_position;
                    break;
                case 14:
                    slide_start = m_slide14_stand_position;
                    break;
                case 15:
                    slide_start = m_slide15_stand_position;
                    break;
                case 16:
                    slide_start = m_slide16_stand_position;
                    break;
                case 17:
                    slide_start = m_slide17_stand_position;
                    break;
                case 18:
                    slide_start = m_slide18_stand_position;
                    break;
                case 19:
                    slide_start = m_slide19_stand_position;
                    break;
                case 20:
                    slide_start = m_slide20_stand_position;
                    break;
                case 21:
                    slide_start = m_slide21_stand_position;
                    break;
                case 22:
                    slide_start = m_slide22_stand_position;
                    break;
                case 23:
                    slide_start = m_slide23_stand_position;
                    break;
                case 24:
                    slide_start = m_slide24_stand_position;
                    break;
                default:
                    break;
                }

                if ( path_end.dist_to( slide_start ) <= 10.f ) {
                    m_active_slide = slide_start;
                    is_active      = true;
                    break;
                }
            }

            if ( *g_time - m_last_path_fix_time < 0.5f || is_active ) return;

            for ( auto i = 1; i <= m_slide_count; i++ ) {
                Vec3 stand_position{ };

                switch ( i ) {
                case 1:
                    stand_position = m_slide1_stand_position;
                    break;
                case 2:
                    stand_position = m_slide2_stand_position;
                    break;
                case 3:
                    stand_position = m_slide3_stand_position;
                    break;
                case 4:
                    stand_position = m_slide4_stand_position;
                    break;
                case 5:
                    stand_position = m_slide5_stand_position;
                    break;
                case 6:
                    stand_position = m_slide6_stand_position;
                    break;
                case 7:
                    stand_position = m_slide7_stand_position;
                    break;
                case 8:
                    stand_position = m_slide8_stand_position;
                    break;
                case 9:
                    stand_position = m_slide9_stand_position;
                    break;
                case 10:
                    stand_position = m_slide10_stand_position;
                    break;
                case 11:
                    stand_position = m_slide11_stand_position;
                    break;
                case 12:
                    stand_position = m_slide12_stand_position;
                    break;
                case 13:
                    stand_position = m_slide13_stand_position;
                    break;
                case 14:
                    stand_position = m_slide14_stand_position;
                    break;
                case 15:
                    stand_position = m_slide15_stand_position;
                    break;
                case 16:
                    stand_position = m_slide16_stand_position;
                    break;
                case 17:
                    stand_position = m_slide17_stand_position;
                    break;
                case 18:
                    stand_position = m_slide18_stand_position;
                    break;
                case 19:
                    stand_position = m_slide19_stand_position;
                    break;
                case 20:
                    stand_position = m_slide20_stand_position;
                    break;
                case 21:
                    stand_position = m_slide21_stand_position;
                    break;
                case 22:
                    stand_position = m_slide22_stand_position;
                    break;
                case 23:
                    stand_position = m_slide23_stand_position;
                    break;
                case 24:
                    stand_position = m_slide24_stand_position;
                    break;
                default:
                    break;
                }

                if ( g_local->position.dist_to( stand_position ) > 65.f && path_end.dist_to( stand_position ) <=
                    m_slide_position_range && path_end.dist_to( stand_position ) > 10.f ) {
                    if ( g_features->orbwalker->send_move_input( stand_position, true ) ) {
                        m_last_path_fix_time = *g_time;
                        break;
                    }
                }
            }
        }

        auto q_lasthit( ) -> bool{
            if ( !g_config->zeri.use_zeri_orbwalker->get< bool >( ) || *g_time - m_last_q_time <= 0.25f || *g_time -
                m_last_cast_time <= 0.1f || !m_slot_q->is_ready( ) )
                return false;

            const auto          lasthit_data = get_zeri_lasthit_target(
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                [this]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                m_q_range,
                40.f,
                !m_is_piercing
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

        auto q_laneclear( ) -> bool{
            if ( !g_config->zeri.use_zeri_orbwalker->get< bool >( ) || *g_time - m_last_q_time <= 0.25f || *g_time -
                m_last_cast_time <= 0.025f || !can_cast_q( ) )
                return false;

            const auto          spellclear = get_zeri_laneclear_target(
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                [this]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                m_q_range,
                40.f,
                !m_is_piercing
            );

            if ( !spellclear ) return false;

            const auto target = g_entity_list.get_by_index( spellclear->target_index );
            if ( !target ) return false;

            if ( cast_spell( ESpellSlot::q, spellclear->cast_position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                if ( spellclear->will_execute ) {
                    g_features->orbwalker->ignore_minion(
                        spellclear->target_index,
                        spellclear->travel_time
                    );
                } else if ( !target->is_turret( ) ) {
                    g_features->prediction->add_special_attack(
                        spellclear->target_index,
                        spellclear->damage,
                        spellclear->travel_time,
                        true
                    );
                }

                if ( get_buffer_delay( ) > 0.f ) {
                    g_features->orbwalker->disable_for(
                        m_slot_q->cooldown_expire - *g_time +
                        g_features->orbwalker->get_ping( ) + 0.05f
                    );
                }

                return true;
            }

            return false;
        }

        auto w_extend_walls( ) -> bool{
            if ( !g_config->zeri.w_extend_wall->get< bool >( ) || *g_time - m_last_w_time <= 0.5f || *g_time -
                m_last_cast_time <= 0.1f || !m_slot_w->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            bool should_cast{ };
            Vec3 cast_position{ };

            const auto cast_windup = std::max(
                0.55f - g_local->bonus_attack_speed * 0.1f + g_local->bonus_attack_speed * 0.01f,
                0.3f
            );
            const auto delay = 0.85f + cast_windup;

            const auto predicted = g_features->prediction->predict_default( target->index, delay );
            if ( !predicted ) return false;

            auto wall = get_line_wall_position( g_local->position, g_local->position.extend( *predicted, m_w_range ) );
            if ( !wall ) return false;

            auto       travel_time  = delay + g_local->position.dist_to( *wall ) / m_w_speed;
            auto       missile_time = g_local->position.dist_to( *wall ) / m_w_speed;
            const auto tolerance    = 0.01f;

            auto pred = g_features->prediction->predict(
                target->index,
                m_w_range + 1500.f,
                0.f,
                m_w_special_width,
                travel_time
            );
            if ( !pred.valid || ( int )pred.hitchance < g_config->zeri.w_hitchance->get< int >( ) ) return false;

            for ( auto i = 0; i <= 3; i++ ) {
                wall = get_line_wall_position(
                    g_local->position,
                    g_local->position.extend( pred.position, m_w_range )
                );
                if ( !wall ) break;

                const auto time            = g_local->position.dist_to( *wall ) / m_w_speed;
                const auto time_difference = time > missile_time ? time - missile_time : missile_time - time;

                if ( 1450.f > g_local->position.dist_to( pred.position ) - g_local->position.dist_to( *wall )
                    && time_difference <= tolerance ) {
                    if ( g_local->position.dist_to( *wall ) > g_local->position.dist_to( pred.position )
                        || g_features->prediction->minion_in_line( g_local->position, *wall, m_w_width * 1.5f ) )
                        return false;

                    cast_position = pred.position;
                    should_cast   = true;

                    break;
                }

                //std::cout << "#" << i << " | diff: " << time_difference << std::endl;
                //std::cout << "distance: " << g_local->position.dist_to(pred.position) - g_local->position.dist_to(*wall) << std::endl;

                missile_time = g_local->position.dist_to( *wall ) / m_w_speed;
                travel_time  = delay + missile_time;

                pred = g_features->prediction->predict(
                    target->index,
                    m_w_range + 1500.f,
                    0.f,
                    m_w_special_width,
                    travel_time
                );
                if ( !pred.valid || ( int )pred.hitchance < g_config->zeri.w_hitchance->get< int >( ) ) return false;
            }

            if ( !should_cast ) return false;

            if ( cast_spell( ESpellSlot::w, cast_position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        static auto get_line_wall_position( const Vec3& start, const Vec3& end ) -> std::optional< Vec3 >{
            const auto check_distance{ 20.f };
            const int  loop_amount = std::ceilf( start.dist_to( end ) / check_distance );

            for ( auto i = 1; i <= loop_amount; i++ ) {
                auto position = start.extend( end, check_distance * i );

                if ( g_navgrid->is_wall( position ) ) return std::make_optional( position );
            }

            return std::nullopt;
        }

        auto can_cast_q( ) -> bool{
            return m_slot_q->is_ready( ) || g_config->zeri.buffer_q_cast->get< bool >( ) &&
                m_slot_q->cooldown_expire <= *g_time + g_features->orbwalker->get_ping( ) / 2.f + 0.033f;
        }

        auto get_buffer_delay( ) -> float{
            if ( m_slot_q->is_ready( ) ) return 0.f;

            return std::max( m_slot_q->cooldown_expire - *g_time, 0.f );
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
            {
                auto damage = m_q_damage[ m_slot_q->level ] + g_local->attack_damage( ) * m_q_ad_modifier[
                    get_slot_q( )->level ];
                if ( g_local->crit_chance >= 0.95f || target->is_hero( ) && g_local->crit_chance >= 0.75f ) {
                    damage *=
                        1.75f;
                }

                auto total_damage = helper::calculate_damage( damage, target->index, true ) + helper::get_onhit_damage(
                    target->index
                );

                if ( target->is_ward( ) ) {
                    const auto ward_type = target->get_ward_type( );

                    switch ( ward_type ) {
                    case Object::EWardType::blue:
                    case Object::EWardType::control:
                    case Object::EWardType::teemo_shroom:
                    case Object::EWardType::jhin_trap:
                    case Object::EWardType::zombie:
                    case Object::EWardType::nidalee_trap:
                    case Object::EWardType::normal:
                    case Object::EWardType::fiddlesticks_effigy:
                        return 1.f;
                    case Object::EWardType::shaco_box:
                        return total_damage;
                    default:
                        return 0.f;
                    }
                }

                if ( m_is_piercing ) {
                    auto extra_damage = m_q_pierce_damage[ m_slot_q->level ] + g_local->bonus_attack_damage( ) * 0.12f
                        + g_local->ability_power( ) * 0.2f;

                    if ( g_local->crit_chance > 0.f ) {
                        const auto damage_mod = std::floor( g_local->crit_chance / 0.1f );
                        extra_damage *= 1.f + 0.065f * damage_mod;
                    }

                    if ( m_slot_q->level < 5 ) {
                        const auto pred = g_features->prediction->predict_default(
                            target->index,
                            g_features->orbwalker->get_attack_cast_delay( )
                        );
                        if ( pred && g_features->prediction->minion_in_line(
                            g_local->position,
                            *pred,
                            40.f,
                            target->network_id
                        ) )
                            extra_damage *= m_q_pierce_modifier[ m_slot_q->level ];
                    }

                    total_damage += helper::calculate_damage( extra_damage, target->index, false );
                }

                return total_damage;
            }
            case ESpellSlot::w:
                return helper::calculate_damage(
                    m_w_damage[ m_slot_w->level ] + g_local->attack_damage( ) * 1.3f + g_local->ability_power( ) *
                    0.25f,
                    target->index,
                    true
                );
            }

            return 0.f;
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
            {
                const auto missile_speed = m_in_ult ? 3400.f : 2600.f;
                const auto windup        = g_features->orbwalker->get_attack_cast_delay( ) + get_buffer_delay( );

                const auto tt   = windup + g_local->position.dist_to( target->position ) / missile_speed;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return windup + g_local->position.dist_to( target->position ) / missile_speed;

                return windup + g_local->position.dist_to( pred.value( ) ) / missile_speed;
            }
            case ESpellSlot::w:
            {
                const auto cast_windup = std::max(
                    0.55f - g_local->bonus_attack_speed * 0.1f + g_local->bonus_attack_speed * 0.01f,
                    0.3f
                );

                const auto tt   = cast_windup + g_local->position.dist_to( target->position ) / m_w_speed;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return cast_windup + g_local->position.dist_to( pred.value( ) ) / m_w_speed;
            }
            default:
                return 0.f;
            }
        }

    private:
        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_r_time{ };

        float m_last_cast_time{ };

        float m_q_range{ 750.f };
        float m_w_range{ 1200.f };
        float m_r_range{ 725.f };

        bool  m_printed{ };
        float m_last_path_fix_time{ };

        // blueside base to bot inhib turret
        Vec3 m_slide1_stand_position{ 988.f, 167.6141f, 376.f };
        Vec3 m_slide1_cast_position{ 3852.f, 95.748047f, 524.1578f };

        // reverse of slide1
        Vec3 m_slide2_stand_position{ 4124.f, 95.74f, 558.f };
        Vec3 m_slide2_cast_position{ 1457.13f, 94.44f, 326.07f };

        // blueside bot inhib tower to 2nd bot turret bush
        Vec3 m_slide3_stand_position{ 4674.f, 97.839f, 608.f };
        Vec3 m_slide3_cast_position{ 7905.986f, 49.4985f, 741.015f };

        // reverse of slide3
        Vec3 m_slide4_stand_position{ 7720.f, 49.443f, 734.f };
        Vec3 m_slide4_cast_position{ 4758.151f, 108.968f, 590.793f };

        // redside base to bot inhib turret
        Vec3 m_slide5_stand_position{ 14472.f, 171.97f, 14040.f };
        Vec3 m_slide5_cast_position{ 14322.6f, 91.41f, 11342.2f };

        // reverse of slide 5
        Vec3 m_slide6_stand_position{ 14296.f, 91.42f, 10894.f };
        Vec3 m_slide6_cast_position{ 14410.79f, 92.06f, 13301.75f };

        // redside bot ihib tower to 2nd bot turret bush
        Vec3 m_slide7_stand_position{ 14196.f, 91.42f, 10256.f };
        Vec3 m_slide7_cast_position{ 14148.65f, 52.3f, 7197.37f };

        // reverse of slide 7
        Vec3 m_slide8_stand_position{ 14148.f, 52.3f, 7358.f };
        Vec3 m_slide8_cast_position{ 14192.69f, 63.6f, 9940.87f };

        // bot alcove to blueside turret
        Vec3 m_slide9_stand_position{ 13050, 47.9934f, 1040 };
        Vec3 m_slide9_cast_position{ 11626.1f, 51.2534f, 902.669f };

        Vec3 m_slide10_stand_position{ 10410, 51.3336f, 786 };
        Vec3 m_slide10_cast_position{ 13038.3f, 48.0586f, 1047.45f };

        // bot alcove to redside turret
        Vec3 m_slide11_stand_position{ 13832, 46.2675f, 1844 };
        Vec3 m_slide11_cast_position{ 14123.7f, 53.9436f, 4450.62f };
        Vec3 m_slide12_stand_position{ 14076, 51.7848f, 4030 };
        Vec3 m_slide12_cast_position{ 13840.5f, 46.1445f, 1830.92f };

        // bot to dragon
        Vec3 m_slide13_stand_position{ 12716, 51.3669f, 3222 };
        Vec3 m_slide13_cast_position{ 11253.1f, 13.5898f, 5618.47f };
        Vec3 m_slide14_stand_position{ 11386, -19.3993f, 5400 };
        Vec3 m_slide14_cast_position{ 12719.2f, 51.3669f, 3218.74f };

        // mid to bot tribush
        Vec3 m_slide15_stand_position{ 8880, -71.2406f, 5474 };
        Vec3 m_slide15_cast_position{ 9522.94f, 66.3418f, 3648.01f };
        Vec3 m_slide16_stand_position{ 9560, 62.6191f, 3540 };
        Vec3 m_slide16_cast_position{ 8882.03f, -69.5325f, 5470.91f };

        // mid to top tribush
        Vec3 m_slide17_stand_position{ 5902.f, -67.9485f, 9402.f };
        Vec3 m_slide17_cast_position{ 5214.82f, 56.8242f, 11528 };
        Vec3 m_slide18_stand_position{ 5272, 56.8176f, 11352 };
        Vec3 m_slide18_cast_position{ 5900.61f, -67.8704f, 9400.5f };

        // river to toplane
        Vec3 m_slide19_stand_position{ 3456, -3.82729f, 9534 };
        Vec3 m_slide19_cast_position{ 2081.47f, 52.8381f, 11652.4f };
        Vec3 m_slide20_stand_position{ 2098, 52.8381f, 11634 };
        Vec3 m_slide20_cast_position{ 3522.57f, -8.58569f, 9431.34f };

        // top tribush to redside redbuff
        Vec3 m_slide21_stand_position{ 4576, 56.5155f, 12056 };
        Vec3 m_slide21_cast_position{ 7289.13f, 55.9795f, 11778.4f };

        Vec3 m_slide22_stand_position{ 7210, 56.2637f, 11784 };
        Vec3 m_slide22_cast_position{ 4566.79f, 56.532f, 12065.4f };

        // bot tribush to blueside redbuff
        Vec3 m_slide23_stand_position{ 7620, 52.576f, 3078 };
        Vec3 m_slide23_cast_position{ 10253.5f, 49.1909f, 2825.5f };
        Vec3 m_slide24_stand_position{ 10306, 49.2203f, 2820 };
        Vec3 m_slide24_cast_position{ 7612.87f, 52.5735f, 3085.56f };




        // W2 WALL PREDICTION
        Vec3 m_cast_position{ };
        Vec3 m_cast_start_position{ };

        Vec3  m_active_slide{ };
        float m_slide_position_range{ 90.f };

        int  m_slide_count{ 24 };
        bool m_in_ult{ };
        bool m_can_print{ };

        // q modifier
        bool m_is_piercing{ };


        // w spelldata
        float m_w_speed{ 2500.f };
        float m_w_width{ 40.f };
        float m_w_special_width{ 100.f };

        std::vector< float > m_q_damage
        {
            0.f,
            15.f,
            18.f,
            21.f,
            24.f,
            27.f
        };

        std::vector< float > m_q_ad_modifier
        {
            0.f,
            1.04f,
            1.08f,
            1.12f,
            1.16f,
            1.20f
        };

        std::vector< float > m_q_pierce_damage   = { 0.f, 20.f, 22.f, 24.f, 26.f, 28.f };
        std::vector< float > m_q_pierce_modifier = { 0.f, 0.8f, 0.85f, 0.9f, 0.95f, 1.f };

        std::vector< float > m_w_damage
        {
            0.f,
            20.f,
            60.f,
            100.f,
            140.f,
            180.f
        };
    };
}
