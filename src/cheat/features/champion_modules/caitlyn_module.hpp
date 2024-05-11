#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class caitlyn_module final : public IModule {
    public:
        virtual ~caitlyn_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "caitlyn_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Caitlyn" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        struct trap_instance_t {
            int16_t index{ };
            Vec3    position{ };

            float spawn_time{ };
            float arm_time{ };
        };

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "caitlyn" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto combo      = navigation->add_section( _( "combo" ) );
            // const auto r_settings = navigation->add_section(_("r settings"));

            q_settings->checkbox( _( "enable" ), g_config->caitlyn.q_enabled );
            q_settings->checkbox( _( "use in harass" ), g_config->caitlyn.q_harass );
            q_settings->checkbox( _( "only out of aa range" ), g_config->caitlyn.q_only_out_of_range );
            q_settings->select(
                _( "hitchance" ),
                g_config->caitlyn.q_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            w_settings->checkbox( _( "enable" ), g_config->caitlyn.w_enabled );
            w_settings->checkbox( _( "anti melee w" ), g_config->caitlyn.w_antimelee );
            w_settings->checkbox( _( "antigapclose w" ), g_config->caitlyn.w_antigapclose );
            w_settings->checkbox( _( "auto-interrupt w (?)" ), g_config->caitlyn.w_autointerrupt )->set_tooltip(
                _( "Automatically interrupts spells like Katarina R, Malzahar R, etc" )
            );
            w_settings->checkbox( _( "ew animation cancel (?)" ), g_config->caitlyn.w_animation_cancel )->set_tooltip(
                _( "Will animation cancel E to place trap during dash" )
            );
            w_settings->checkbox( _( "save charge (?)" ), g_config->caitlyn.w_save_charge_for_cc )->set_tooltip(
                _( "Will save 1 charge for guaranteed hit" )
            );
            w_settings->select(
                _( "hitchance" ),
                g_config->caitlyn.w_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            e_settings->checkbox( _( "enable" ), g_config->caitlyn.e_enabled );
            e_settings->checkbox( _( "antigapclose e" ), g_config->caitlyn.e_antigapcloser );
            e_settings->slider_int( _( "max range %" ), g_config->caitlyn.e_max_range, 25, 100, 1 );
            e_settings->select(
                _( "hitchance" ),
                g_config->caitlyn.e_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            combo->select(
                _( "combo mode" ),
                g_config->caitlyn.combo_logic,
                { _( "Off" ), _( "On cast trap" ), _( "On trap trigger" ) }
            );
            combo->checkbox( _( "accelerate combo with w" ), g_config->caitlyn.w_fast_combo );
            combo->slider_int( _( "galeforce max dash angle" ), g_config->caitlyn.galeforce_max_angle, 10, 90, 1 );

            // r_settings->checkbox(_("enable"), g_config->caitlyn.r_enabled);

            drawings->checkbox( _( "draw q range" ), g_config->caitlyn.q_draw_range );
            drawings->checkbox( _( "draw w range" ), g_config->caitlyn.w_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->caitlyn.e_draw_range );
            drawings->checkbox( _( "draw r range" ), g_config->caitlyn.r_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->caitlyn.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->caitlyn.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->caitlyn.dont_draw_on_cooldown->
                    get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 31, 88, 255, 255 ),
                        m_q_range,
                        Renderer::outline,
                        70,
                        2.f
                    );
                }
            }

            if ( g_config->caitlyn.w_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->caitlyn.dont_draw_on_cooldown->
                    get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 132, 245, 66, 255 ),
                        m_w_range,
                        Renderer::outline,
                        70,
                        2.f
                    );
                }
            }

            if ( g_config->caitlyn.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->caitlyn.dont_draw_on_cooldown->
                    get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 144, 66, 245, 255 ),
                        m_e_range,
                        Renderer::outline,
                        70,
                        2.f
                    );
                }
            }

            if ( g_config->caitlyn.r_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::r );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->caitlyn.dont_draw_on_cooldown->
                    get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 173, 47, 68, 255 ),
                        m_r_range,
                        Renderer::outline,
                        70,
                        2.f
                    );
                    g_render->circle_minimap( g_local->position, Color( 173, 47, 68, 255 ), m_r_range, 60, 1.f );
                }
            }

            if ( *g_time - m_last_galeforce_time <= 1.5f ) {
                g_render->line_3d( m_galeforce_start, m_galeforce_end, Color( 25, 255, 25 ), 3.f );
                g_render->circle_3d(
                    m_galeforce_end,
                    Color( 10, 185, 10, 75 ),
                    15.f,
                    Renderer::outline | Renderer::filled,
                    16,
                    2.f
                );
            }

            for ( const auto trap : m_traps ) {
                Vec2 sp{ };
                if ( !world_to_screen( trap.position, sp ) ) continue;

                if ( trap.arm_time > *g_time ) {
                    const auto modifier = std::clamp( ( trap.arm_time - *g_time ) / 0.9f, 0.f, 1.f );

                    g_render->circle_3d(
                        trap.position,
                        Color( 0, 0, 0, 90 ),
                        75.f,
                        Renderer::outline | Renderer::filled,
                        32,
                        4.f,
                        360.f
                    );

                    g_render->circle_3d(
                        trap.position,
                        Color( 255, 255, 255, 255 ),
                        75.f,
                        Renderer::outline,
                        32,
                        4.f,
                        360.f - 360.f * modifier
                    );

                    auto text = std::to_string( trap.arm_time - *g_time );
                    text.resize( 4 );

                    const auto size          = g_render->get_text_size( text, g_fonts->get_nexa( ), 32 );
                    const Vec2 text_position = { sp.x - size.x / 2.f, sp.y - size.y / 2.f };

                    g_render->text_shadow(
                        text_position,
                        Color( 255, 255, 255 ),
                        g_fonts->get_nexa( ),
                        text.data( ),
                        32
                    );

                    continue;
                }

                g_render->circle_3d(
                    trap.position,
                    Color( 255, 255, 255 ),
                    80.f,
                    Renderer::outline,
                    32,
                    5.f
                );
            }

            Vec2 sp{ };
            if ( world_to_screen( g_local->position, sp ) ) {
                sp.x += 30.f;
                sp.y -= 20.f;

                std::string text1{ "[ CAPS ] GALEFORCE: " };
                std::string text_mode{ m_galeforce_toggle ? "ON" : "OFF" };

                auto size = g_render->get_text_size( text1, g_fonts->get_zabel_12px( ), 12 );

                g_render->text_shadow( sp, Color( 255, 255, 255 ), g_fonts->get_zabel_12px( ), text1.c_str( ), 12 );

                g_render->text_shadow(
                    { sp.x + size.x, sp.y },
                    m_galeforce_toggle ? Color( 50, 255, 50 ) : Color( 255, 75, 50 ),
                    g_fonts->get_zabel_12px( ),
                    text_mode.c_str( ),
                    12
                );

                return;
                text1     = { "[ CAPS ] : " };
                text_mode = { m_galeforce_toggle ? "ON" : "OFF" };

                size = g_render->get_text_size( text1, g_fonts->get_zabel_12px( ), 12 );

                g_render->text_shadow( sp, Color( 255, 255, 255 ), g_fonts->get_zabel_12px( ), text1.c_str( ), 12 );

                g_render->text_shadow(
                    { sp.x + size.x, sp.y },
                    m_galeforce_toggle ? Color( 50, 255, 50 ) : Color( 255, 75, 50 ),
                    g_fonts->get_zabel_12px( ),
                    text_mode.c_str( ),
                    12
                );
            }
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            update_traps( );
            update_galeforce( );

            if ( !m_key_down && GetAsyncKeyState( VK_CAPITAL ) ) {
                m_galeforce_toggle = !m_galeforce_toggle;
                m_key_down         = true;
            } else if ( m_key_down && !GetAsyncKeyState( VK_CAPITAL ) ) m_key_down = false;

            if ( m_should_galeforce && !m_galeforce_toggle ) m_should_galeforce = false;

            if ( g_features->evade->is_active( ) ) return;

            cancel_trap_animation( );
            galeforce_cancel_dash( );

            if ( g_features->orbwalker->in_action( ) ) return;

            on_trap_trigger( );
            antigapcloser_w( );
            antigapcloser_e( );
            autointerrupt_w( );

            if ( g_features->orbwalker->in_attack( ) ) return;

            w_dash_animation_cancel( );
            crowdcontrol_w( );

            w_reset_aa( );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                antimelee_w( );

                spell_e( );
                spell_w( );
                spell_q( );
                break;

            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->caitlyn.q_harass->get< bool >( ) ) spell_q( );
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->caitlyn.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || *g_time -
                m_last_cast_time <= 0.1f || !m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || g_config->caitlyn.q_only_out_of_range->get< bool >( ) && g_features->orbwalker->
                is_attackable( target->index ) )
                return false;

            auto pred = g_features->prediction->predict( target->index, m_q_range, 2200.f, 60.f, 0.625f, { }, true );
            if ( !pred.valid || ( int )pred.hitchance < g_config->caitlyn.q_hitchance->get< int >( ) ) return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->caitlyn.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 1.25f || *g_time -
                m_last_cast_time <= 0.1f || !m_slot_w->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || g_features->buff_cache->get_buff( target->index, ct_hash( "caitlynwsight" ) ) ||
                target->network_id == m_last_w_target_nid && *g_time - m_last_w_time <= 2.f )
                return false;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_w_range,
                0.f,
                15.f,
                m_w_delay,
                { },
                true
            );
            if ( !pred.valid || ( int )pred.hitchance < g_config->caitlyn.w_hitchance->get< int >( ) ||
                get_nearest_trap_distance( pred.position ) <= 125.f )
                return false;

            if ( g_config->caitlyn.w_save_charge_for_cc->get< bool >( ) && ( int )pred.hitchance <= 3 && m_slot_w->
                charges < 2 )
                return false;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                m_last_w_time     = *g_time;
                m_last_cast_time  = *g_time;
                m_last_w_position = pred.position;
                g_features->orbwalker->on_cast( );
                m_last_w_target_nid = target->network_id;
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->caitlyn.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || *g_time -
                m_last_cast_time <= 0.1f || !m_slot_e->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_e_range / 100.f * g_config->caitlyn.e_max_range->get< int >( ),
                1600.f,
                70.f,
                0.15f
            );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->caitlyn.e_hitchance->
                    get< int >( ) )
                || g_features->prediction->minion_in_line( g_local->position, pred.position, 100.f ) )
                return false;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                if ( m_has_galeforce ) {
                    m_should_galeforce     = true;
                    m_last_net_target_time = *g_time;
                    m_net_target_index     = target->index;
                }
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{ return false; }

        auto w_reset_aa( ) -> bool{
            if ( !g_config->caitlyn.w_fast_combo->get< bool >( ) || *g_time - m_last_w_time <= 1.f ||
                g_local->bonus_attack_speed > 0.5f || *g_time - m_last_cast_time <= 0.1f || m_slot_w->charges < 0 ||
                !g_features->orbwalker->should_reset_aa( ) || !m_slot_w->is_ready( true ) )
                return false;

            std::cout << "atacksped: " << g_local->bonus_attack_speed << std::endl;

            const auto attack_name = g_features->orbwalker->get_last_attack_name( );
            if ( rt_hash( attack_name.data( ) ) != ct_hash( "CaitlynPassiveMissile" ) ) return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || !g_features->orbwalker->is_unit_headshottable( target->index ) ) return false;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_w_range,
                0.f,
                15.f,
                m_w_delay,
                { },
                true
            );
            if ( !pred.valid ) return false;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                m_last_w_time       = *g_time;
                m_last_cast_time    = *g_time;
                m_last_w_reset_time = *g_time;
                g_features->orbwalker->on_cast( );
                m_last_w_target_nid = target->network_id;

                std::cout << "[ Caitlyn: W-AA Reset ] " << *g_time << std::endl;
            }

            return false;
        }

        auto w_dash_animation_cancel( ) -> void{
            if ( !g_config->caitlyn.w_animation_cancel->get< bool >( ) || *g_time - m_last_w_time <= 0.5f || !m_slot_w->
                is_ready( true ) || m_slot_w->charges < 1 )
                return;

            if ( !m_active ) {
                auto sci = g_local->spell_book.get_spell_cast_info( );
                if ( !sci || sci->slot != 2 ) return;

                m_active             = true;
                m_cast_time          = sci->server_cast_time;
                m_net_start_position = sci->start_position;
                m_net_end_position   = sci->start_position.extend( sci->end_position, m_e_range );
                return;
            }

            const auto time_left = std::max( m_cast_time - *g_time, 0.f );
            if ( time_left > 0.04f ) {
                if ( *g_time > m_cast_time + 0.075f ) {
                    m_active    = false;
                    m_cast_time = 0.f;
                }

                return;
            } else if ( *g_time > m_cast_time + 0.075f ) {
                m_active    = false;
                m_cast_time = 0.f;
            }

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || g_features->buff_cache->get_buff( target->index, ct_hash( "caitlynwsight" ) ) ) return;

            auto closest_point =
                g_features->evade->get_closest_line_point( m_net_start_position, m_net_end_position, target->position );

            auto       travel_time  = time_left + m_net_start_position.dist_to( closest_point ) / 1600.f;
            const auto missile_pred = g_features->prediction->predict_default( target->index, travel_time );
            if ( !missile_pred ) return;

            closest_point = g_features->evade->get_closest_line_point(
                m_net_start_position,
                m_net_end_position,
                missile_pred.value( )
            );

            if ( closest_point.dist_to( missile_pred.value( ) ) > 70.f + target->get_bounding_radius( ) ) {
                std::cout << "Invalid E prediction, target will be " << closest_point.dist_to( missile_pred.value( ) )
                    << " units away from missile on collision\n";

                return;
            }

            travel_time = time_left + ( m_net_start_position.dist_to( closest_point ) + 35.f ) / 1600.f;

            const auto slowed_duration{ 1.f - travel_time };
            const auto compensation_amount = target->movement_speed * slowed_duration / 2.f;

            auto pred = g_features->prediction->predict(
                target->index,
                m_w_range,
                0.f,
                15.f + compensation_amount,
                m_w_delay + time_left,
                { },
                true
            );
            if ( !pred.valid ) return;

            if ( g_config->caitlyn.w_save_charge_for_cc->get< bool >( ) && ( int )pred.hitchance < 3 && m_slot_w->
                charges < 2 )
                return;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                m_last_w_time = *g_time;
                m_active      = false;
                m_cast_time   = 0.f;
                g_features->orbwalker->on_cast( );
                m_last_w_target_nid = target->network_id;

                m_should_galeforce = false;

                std::cout << "[ Caitlyn: W ] Dash animation cancel, target: " << target->champion_name.text <<
                    std::endl;
            }
        }

        auto cancel_trap_animation( ) -> void{
            if ( !g_config->caitlyn.e_enabled->get< bool >( ) || g_config->caitlyn.combo_logic->get< int >( ) != 1 ||
                !m_slot_e->is_ready( true ) || *g_time - m_last_cast_time <= 0.1f || *g_time - m_last_galeforce_time <=
                2.f )
                return;

            if ( !m_w_active ) {
                auto sci = g_local->spell_book.get_spell_cast_info( );
                if ( !sci || sci->slot != 1 || sci->server_cast_time < *g_time ) return;

                m_w_active           = true;
                m_w_server_cast_time = sci->server_cast_time;
                g_features->orbwalker->disable_autoattack_until( m_w_server_cast_time );
            }

            const auto time_left = m_w_server_cast_time - *g_time;
            if ( time_left >= 0.03f ) {
                if ( *g_time > m_w_server_cast_time + 0.075f ) m_w_active = false;


                return;
            }

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_e_range,
                1600.f,
                70.f,
                0.15f + time_left,
                { },
                true
            );
            if ( !pred.valid || g_features->prediction->minion_in_line_predicted(
                g_local->position,
                pred.position,
                70.f,
                0.15f,
                1600.f
            ) )
                return;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                m_w_active = false;

                if ( m_has_galeforce ) {
                    m_should_galeforce     = true;
                    m_last_net_target_time = *g_time;
                    m_net_target_index     = target->index;
                }

                std::cout << "[ Caitlyn: E ] Cancel W animation, target " << target->champion_name.text << " | " << *
                    g_time
                    << std::endl;
            }
        }

        auto galeforce_cancel_dash( ) -> void{
            if ( g_config->caitlyn.combo_logic->get< int >( ) == 0 || !m_galeforce_toggle || !m_has_galeforce ||
                !m_should_galeforce || *g_time - m_last_net_target_time > 0.75f
                || *g_time - m_last_galeforce_time <= 1.f )
                return;

            if ( !m_active ) {
                auto sci = g_local->spell_book.get_spell_cast_info( );
                if ( !sci || sci->slot != 2 ) return;

                m_active           = true;
                m_cast_time        = sci->server_cast_time;
                m_net_end_position = sci->start_position.extend( sci->end_position, m_e_range );
                return;
            }

            const auto time_left = m_cast_time - *g_time;
            if ( time_left >= 0.035f ) {
                if ( *g_time > m_cast_time + 0.05f ) {
                    m_active    = false;
                    m_cast_time = 0.f;
                }

                return;
            }

            const auto target = g_entity_list.get_by_index( m_net_target_index );
            if ( !target ) return;

            const auto pred = g_features->prediction->predict_default( target->index, 0.2f );
            if ( !pred ) return;

            const auto local_position = g_local->position.extend( m_net_end_position, -50.f );

            const auto points = g_render->get_3d_circle_points( local_position, 200.f, 18 );
            const auto cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;

            Vec3  selected_point{ };
            float best_weight{ };

            for ( const auto point : points ) {
                if ( point.dist_to( target->position ) > 675.f || point.dist_to( *pred ) > 675.f
                    || g_navgrid->is_wall( point ) )
                    continue;


                float weight{ };

                auto       v1            = point - local_position;
                auto       v2            = cursor - local_position;
                const auto dot           = v1.normalize( ).dot_product( v2.normalize( ) );
                const auto current_angle = acos( dot ) * 180.f / 3.14159265358979323846f;
                if ( current_angle > g_config->caitlyn.galeforce_max_angle->get< int >( ) ) continue;


                // angle weight
                weight += std::min( 1.f - current_angle / 180.f, 1.f );

                // turret weight
                if ( helper::is_position_under_turret( point ) ) weight -= 1.f;

                // distance weight
                weight += std::min( point.dist_to( target->position ) / 650.f, 0.66f );

                if ( selected_point.length( ) > 0.f && ( weight < best_weight || !g_features->evade->is_position_safe(
                        point
                    ) ||
                    helper::is_wall_in_line( point, g_local->position ) ) )
                    continue;

                selected_point = point;
                best_weight    = weight;
            }

            if ( selected_point.length( ) <= 0.f ) return;

            if ( cast_spell( m_galeforce_slot, selected_point ) ) {
                m_last_cast_time      = *g_time;
                m_last_galeforce_time = *g_time;
                m_active              = false;
                m_cast_time           = 0.f;
                g_features->orbwalker->on_cast( );

                m_galeforce_start = local_position;
                m_galeforce_end   = selected_point;

                m_should_galeforce = false;
                m_net_target_index = 0;

                std::cout << "[ Galeforce ] Cancel E dash " << *g_time << std::endl;
            }
        }

        auto on_trap_trigger( ) -> void{
            if ( g_config->caitlyn.combo_logic->get< int >( ) != 2 || *g_time - m_last_combo_time <= 1.75f ||
                *g_time - m_last_cast_time <= 0.05f || *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready( true ) )
                return;

            const Object* target{ };

            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) || enemy->dist_to_local( ) >
                    m_e_range )
                    continue;

                const auto buff = g_features->buff_cache->get_buff( enemy->index, ct_hash( "CaitlynWSnare" ) );
                if ( buff &&
                    buff->buff_data->end_time - *g_time > 0.25f + g_features->orbwalker->get_attack_cast_delay( ) ) {
                    if ( g_features->prediction->minion_in_line( g_local->position, enemy->position, 70.f ) ) continue;

                    std::cout << "[ Snare ] buff->start_time: " << buff->buff_data->start_time << std::endl;
                    target = enemy;
                    break;
                }
            }

            if ( !target ) return;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_e_range,
                1600.f,
                70.f,
                0.15f,
                { },
                true
            );
            if ( !pred.valid || g_features->prediction->minion_in_line_predicted(
                g_local->position,
                pred.position,
                70.f,
                0.15f,
                1600.f
            ) )
                return;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time     = *g_time;
                m_last_cast_time  = *g_time;
                m_last_combo_time = *g_time;

                if ( m_has_galeforce ) {
                    m_should_galeforce     = true;
                    m_last_net_target_time = *g_time;
                    m_net_target_index     = target->index;
                }

                std::cout << "[ Caitlyn: E ] On trapped enemy: " << target->champion_name.text << " | " << *g_time <<
                    std::endl;
            }
        }

        auto antigapcloser_e( ) -> void{
            if ( !g_config->caitlyn.e_antigapcloser->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->
                is_ready( true ) )
                return;

            const auto target = get_advanced_antigapclose_target( m_e_range, 1600, 70.f, 0.15f, true );
            if ( !target ) return;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_e_range,
                1600.f,
                70.f,
                0.15f,
                { },
                true
            );
            if ( !pred.valid || ( int )pred.hitchance < 3 || g_features->prediction->minion_in_line(
                g_local->position,
                pred.position,
                80.f
            ) )
                return;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                std::cout << "[ Caitlyn: E ] Antigapclose against " << target->champion_name.text << std::endl;
            }
        }

        auto antimelee_w( ) -> void{
            if ( !g_config->caitlyn.w_antimelee->get< bool >( ) || *g_time - m_last_w_time <= 0.5f || *g_time -
                m_last_antimelee_time <= 1.3f || *g_time - m_last_cast_time <= 0.1f || !m_slot_w->is_ready( true ) )
                return;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->attack_range > 300.f ||
                g_features->buff_cache->get_buff( target->index, ct_hash( "CaitlynWSnare" ) ) ||
                target->network_id == m_last_w_target_nid && *g_time - m_last_w_time <= 1.25f )
                return;

            const auto buff = g_features->buff_cache->get_buff( target->index, ct_hash( "caitlynwsight" ) );
            if ( buff && buff->buff_data->end_time - *g_time > 1.f ) return;

            if ( target->dist_to_local( ) > 275.f ) return;

            const auto pred = g_features->prediction->predict_default( target->index, 0.2f );
            if ( !pred || g_local->position.dist_to( *pred ) + 25.f < target->dist_to_local( ) ) return;

            if ( get_nearest_trap_distance( g_local->position ) < 100.f ) return;

            if ( cast_spell( ESpellSlot::w, g_local->position ) ) {
                m_last_w_time         = *g_time;
                m_last_cast_time      = *g_time;
                m_last_antimelee_time = *g_time;
                g_features->orbwalker->on_cast( );
                m_last_w_target_nid = target->network_id;

                std::cout << "[ Caitlyn: W ] Anti-melee target: " << target->champion_name.text << std::endl;
            }
        }

        auto autointerrupt_w( ) -> void{
            if ( !g_config->caitlyn.w_autointerrupt->get< bool >( ) || *g_time - m_last_w_time <= 1.3f || !m_slot_w->
                is_ready( true ) )
                return;

            const auto target = get_interruptable_target( m_w_range );
            if ( !target || target->network_id == m_last_w_target_nid && *g_time - m_last_w_time <= 2.5f ||
                g_features->buff_cache->get_buff( target->index, ct_hash( "CaitlynWSnare" ) ) )
                return;

            const auto buff = g_features->buff_cache->get_buff( target->index, ct_hash( "caitlynwsight" ) );
            if ( buff && buff->buff_data->end_time - *g_time > 1.f ) return;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_w_range,
                0.f,
                15.f,
                m_w_delay,
                { },
                true
            );
            if ( !pred.valid || ( int )pred.hitchance < 3 ) return;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                m_last_w_target_nid = target->network_id;

                std::cout << "[ Caitlyn: W ] Auto-interrupt target: " << target->champion_name.text << std::endl;
            }
        }

        auto antigapcloser_w( ) -> void{
            if ( !g_config->caitlyn.w_antigapclose->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || m_slot_w->
                charges < 1 ||
                !m_slot_w->is_ready( true ) )
                return;

            const auto target = get_advanced_antigapclose_target( m_w_range, 0.f, 15.f, m_w_delay, true );
            if ( !target || target->network_id == m_last_w_target_nid && *g_time - m_last_w_time <= 1.f ) return;

            const auto buff = g_features->buff_cache->get_buff( target->index, ct_hash( "caitlynwsight" ) );
            if ( buff && buff->buff_data->end_time - *g_time > 0.8f ) return;

            const auto pred =
                g_features->prediction->predict( target->index, m_w_range, 0.f, 15.f, m_w_delay, { }, true );
            if ( !pred.valid || ( int )pred.hitchance < 3 ) return;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                m_last_w_target_nid = target->network_id;

                std::cout << "[ Caitlyn: W ] Antigapcloser target: " << target->champion_name.text << std::endl;
            }
        }

        auto crowdcontrol_w( ) -> void{
            if ( !g_config->caitlyn.w_antigapclose->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f || *g_time -
                m_last_w_time <= 0.4f ||
                m_slot_w->charges < 1 || !m_slot_w->is_ready( true ) )
                return;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->network_id == m_last_w_target_nid && *g_time - m_last_w_time <= 1.5f ) return;

            const auto buff = g_features->buff_cache->get_buff( target->index, ct_hash( "caitlynwsight" ) );
            if ( buff && buff->buff_data->end_time - *g_time >= 0.8f ) return;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_w_range,
                0.f,
                15.f,
                m_w_delay,
                { },
                true
            );
            if ( !pred.valid || ( int )pred.hitchance < 4 ) return;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                m_last_w_target_nid = target->network_id;

                std::cout << "[ Caitlyn: W ] Crowdcontrol cast, target: " << target->champion_name.text << std::endl;
            }
        }

        auto update_traps( ) -> void{
            for ( const auto unit : g_entity_list.get_ally_minions( ) ) {
                if ( !unit || unit->is_dead( ) || unit->dist_to_local( ) > 1000.f ||
                    unit->get_owner_index( ) != g_local->index || rt_hash( unit->name.text ) != ct_hash(
                        "Cupcake Trap"
                    ) ||
                    is_trap_active( unit->index ) )
                    continue;

                const auto type = unit->get_ward_type( );
                switch ( type ) {
                case Object::EWardType::caitlyn_trap:
                    break;
                default:
                    std::cout << "Name: " << unit->name.text << std::endl;
                    break;
                }

                m_traps.push_back( { unit->index, unit->position, *g_time, *g_time + 0.925f } );
            }

            for ( const auto trap : m_traps ) {
                const auto object = g_entity_list.get_by_index( trap.index );
                if ( !object || object->is_dead( ) ) {
                    std::cout << "[ Caitlyn: Trap ] Died " << *g_time - trap.spawn_time << "s after spawn\n";
                    remove_trap( trap.index );
                    continue;
                }
            }
        }

        auto update_galeforce( ) -> void{
            bool       has_galeforce{ };
            ESpellSlot galeforce_slot{ };

            for ( auto i = 1; i < 8; i
                  ++ ) // cuz Inventory Slot starts at 1, so we iterate all, 7 is trinket so no check
            {
                const auto spell_slot_object = static_cast< ESpellSlot >( 5 + i );
                auto       spell_slot        = g_local->spell_book.get_spell_slot( spell_slot_object );

                if ( !spell_slot || !spell_slot->is_ready( ) ) continue;

                auto slot = g_local->inventory.get_inventory_slot( i );
                if ( !slot ) continue;
                auto item_base = slot->get_base_item( );
                if ( !item_base ) continue;
                auto item_data = item_base->get_item_data( );
                if ( !item_data ) continue;

                switch ( static_cast< EItemId >( item_data->id ) ) {
                default:
                    break;
                case EItemId::galeforce:
                    galeforce_slot = spell_slot_object;
                    has_galeforce = true;
                    break;
                }

                if ( has_galeforce ) break;
            }


            if ( !has_galeforce ) {
                if ( m_has_galeforce ) {
                    const auto target = g_features->target_selector->get_default_target( );
                    if ( target ) {
                        const auto damage = get_galeforce_damage( target->index );


                        std::cout << "[ GALEFORCE ] Damage: " << damage << " against " << target->champion_name.text
                            << std::endl;
                    }
                }

                m_has_galeforce = false;
                return;
            }

            m_has_galeforce  = true;
            m_galeforce_slot = galeforce_slot;
        }

        static auto get_galeforce_damage( const int16_t target_index ) -> float{
            auto target = g_entity_list.get_by_index( target_index );
            if ( !target ) return 0.f;

            target.update( );
            if ( target->is_dead( ) || target->is_invisible( ) ) return 0.f;

            auto total_damage = 180.f;
            if ( g_local->level > 9 ) total_damage += std::min( ( g_local->level - 9 ) * 15.f, 135.f );

            total_damage += g_local->bonus_attack_damage( ) * 0.45f;

            const auto missing_health  = 1.f - target->health / target->max_health;
            const auto damage_modifier = std::min( std::floor( missing_health / 0.07f ) * 0.05f, 0.5f );

            return helper::calculate_damage( total_damage * ( 1.f + damage_modifier ), target_index, false );
        }

        auto get_nearest_trap_distance( const Vec3& position ) const -> float{
            auto lowest_distance = 9999.f;

            for ( auto trap : m_traps ) {
                if ( trap.position.dist_to( position ) > lowest_distance ) continue;

                lowest_distance = trap.position.dist_to( position );
            }

            return lowest_distance;
        }

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

        auto remove_trap( const int16_t index ) -> void{
            const auto to_remove = std::ranges::remove_if(
                m_traps,
                [ & ]( const trap_instance_t& inst ) -> bool{ return inst.index == index; }
            );

            if ( to_remove.empty( ) ) return;

            m_traps.erase( to_remove.begin( ), to_remove.end( ) );
        }

        auto is_trap_active( const int16_t index ) const -> bool{
            for ( const auto trap : m_traps ) if ( trap.index == index ) return true;

            return false;
        }

    private:
        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_r_time{ };
        float m_last_cast_time{ };

        float m_last_galeforce_time{ };
        float m_last_combo_time{ };

        Vec3 m_galeforce_start{ };
        Vec3 m_galeforce_end{ };

        // galeforce spell
        bool       m_has_galeforce{ };
        ESpellSlot m_galeforce_slot{ };

        // E data
        bool    m_should_galeforce{ };
        int16_t m_net_target_index{ };
        float   m_last_net_target_time{ };

        // trap tracking
        std::vector< trap_instance_t > m_traps{ };


        // galeforce
        bool  m_galeforce_toggle{ };
        bool  m_key_down{ };
        float m_last_w_reset_time{ };

        float m_last_antimelee_time{ };

        Vec3     m_last_w_position{ };
        unsigned m_last_w_target_nid{ };

        // W cast track
        float m_w_active{ };
        float m_w_server_cast_time{ };

        // EW combo tracking
        bool    m_active{ };
        float   m_cast_time{ };
        Vec3    m_net_start_position{ };
        Vec3    m_net_end_position{ };
        int16_t m_target_index{ };

        std::vector< float > m_q_damage = { 0.f, 65.f, 95.f, 125.f, 155.f, 185.f };
        std::vector< float > m_e_damage = { 0.f, 80.f, 105.f, 130.f, 155.f, 180.f };
        std::vector< float > m_r_damage = { 0.f, 80.f, 100.f, 120.f };

        float m_q_range{ 1240.f };
        float m_w_range{ 799.f };
        float m_e_range{ 740.f };
        float m_r_range{ 3500.f };

        float m_w_delay{ 1.125f };
    };
}
