#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class lux_module final : public IModule {
    public:
        virtual ~lux_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "lux_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Lux" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "lux" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->lux.q_enabled );
            q_settings->checkbox( _( "harass q" ), g_config->lux.q_harass );
            q_settings->checkbox( _( "flee q" ), g_config->lux.q_flee );
            q_settings->checkbox( _( "antigapclose q" ), g_config->lux.q_antigapclose );
            q_settings->select(
                _( "hitchance" ),
                g_config->lux.q_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            w_settings->checkbox( _( "enable" ), g_config->lux.w_enabled );
            w_settings->checkbox( _( "autoshield w" ), g_config->lux.w_autoshield );
            w_settings->multi_select(
                _( "shield if target is " ),
                { g_config->lux.w_shield_self, g_config->lux.w_shield_allies },
                { _( "Self" ), _( "Ally" ) }
            );
            w_settings->slider_int( _( "min damage to shield" ), g_config->lux.w_shield_threshold, 5, 300, 1 );

            e_settings->checkbox( _( "enable" ), g_config->lux.e_enabled );
            e_settings->checkbox( _( "harass e" ), g_config->lux.e_harass );
            e_settings->checkbox( _( "flee e" ), g_config->lux.e_flee );
            e_settings->checkbox( _( "killsteal e" ), g_config->lux.e_killsteal );
            e_settings->checkbox( _( "antigapclose e" ), g_config->lux.e_antigapclose );
            e_settings->checkbox( _( "multihit e" ), g_config->lux.e_multihit );
            e_settings->select(
                _( "hitchance" ),
                g_config->lux.e_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            r_settings->checkbox( _( "enable" ), g_config->lux.r_enabled );
            r_settings->checkbox( _( "killsteal r" ), g_config->lux.r_killsteal );
            r_settings->select(
                _( "hitchance" ),
                g_config->lux.r_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );
            r_settings->checkbox( _( "recall ult (?)" ), g_config->lux.r_recall_ult )->set_tooltip(
                _( "Will R killable targets who are recalling" )
            );
            r_settings->checkbox( _( "prefer delay (?)" ), g_config->lux.r_recall_ult_delay )->set_tooltip(
                _( "Prefer to wait until last possible time to ult, recommended" )
            );

            drawings->checkbox( _( "draw q range" ), g_config->lux.q_draw_range );
            drawings->checkbox( _( "draw w range" ), g_config->lux.w_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->lux.e_draw_range );
            drawings->checkbox( _( "draw r range" ), g_config->lux.r_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->lux.dont_draw_on_cooldown );
            drawings->checkbox( _( "draw r damage" ), g_config->lux.r_draw_damage );
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) {
                m_recall_ult_active = false;
                return;
            }

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->lux.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->lux.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 31, 88, 255, 255 ),
                        m_q_range + 60.f,
                        Renderer::outline,
                        72,
                        3.f
                    );
                }
            }

            if ( g_config->lux.w_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->lux.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 132, 245, 66, 255 ),
                        m_w_range,
                        Renderer::outline,
                        72,
                        3.f
                    );
                }
            }

            if ( g_config->lux.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->lux.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 144, 66, 245, 255 ),
                        m_e_range,
                        Renderer::outline,
                        72,
                        3.f
                    );
                }
            }

            if ( g_config->lux.r_draw_range->get< bool >( ) || g_config->lux.r_draw_damage->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::r );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->lux.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    if ( g_config->lux.r_draw_range->get< bool >( ) ) {
                        g_render->circle_3d(
                            g_local->position,
                            Color( 173, 47, 47, 255 ),
                            m_r_range,
                            Renderer::outline,
                            120,
                            3.f
                        );

                        g_render->circle_minimap( g_local->position, Color::white( ), m_r_range, 50, 1.f );
                    }


                    if ( g_config->lux.r_draw_damage->get< bool >( ) ) {
                        for ( const auto index : g_features->tracker->get_enemies( ) ) {
                            auto enemy = g_entity_list.get_by_index( index );
                            if ( !enemy ) continue;

                            enemy.update( );

                            if ( enemy->is_dead( ) || enemy->is_invisible( ) || enemy->dist_to_local( ) >
                                m_r_range )
                                continue;

                            Vec2 sp{ };
                            if ( !world_to_screen( enemy->position, sp ) ) continue;

                            auto base_x = 1920.f;
                            auto base_y = 1080.f;

                            auto width_ratio  = base_x / static_cast< float >( g_render_manager->get_width( ) );
                            auto height_ratio = base_y / static_cast< float >( g_render_manager->get_height( ) );

                            auto width_offset  = width_ratio * 0.055f;
                            auto height_offset = height_ratio * 0.0222f;

                            auto bar_length = width_offset * static_cast< float >( g_render_manager->get_width( ) );
                            auto bar_height = height_offset * static_cast< float >( g_render_manager->get_height( ) );

                            auto base_position = enemy->get_hpbar_position( );

                            auto buffer = 1.f;

                            base_position.x -= bar_length * 0.43f;
                            base_position.y -= bar_height;

                            auto damage      = get_r_damage( index );
                            auto modifier    = enemy->health / enemy->max_health;
                            auto damage_mod  = damage / enemy->max_health;
                            auto is_killable = damage > enemy->health;

                            Vec2 box_start{
                                base_position.x + bar_length * modifier + buffer,
                                base_position.y - buffer
                            };
                            Vec2 box_size{
                                damage_mod * bar_length > box_start.x - base_position.x
                                    ? base_position.x - box_start.x - buffer * 1.f
                                    : -( bar_length * damage_mod ) - buffer * 1.f,
                                bar_height * 0.5f + buffer * 2.f
                            };

                            g_render->filled_box(
                                box_start,
                                box_size,
                                is_killable
                                    ? g_features->orbwalker
                                                ->animate_color( Color( 255, 50, 75 ), EAnimationType::pulse, 10 )
                                                .alpha( 175 )
                                    : Color( 40, 150, 255, 180 )
                            );
                        }
                    }
                }
            }

            if ( m_recall_ult_active ) {
                auto& target = g_entity_list.get_by_index( m_target_index );
                if ( !target ) return;

                const auto  duration   = m_predicted_cast_time - m_baseult_start_time;
                const auto  time_left  = m_predicted_cast_time - *g_time;
                std::string champ_name = target->champion_name.text;

                if ( time_left > 0.f ) {
                    auto angle = 360.f - 360.f * ( time_left / duration );
                    g_render->circle_3d( g_local->position, Color( 50, 255, 50 ), 150.f, 2, 50, 3.f, angle );
                }

                Vec2 indicator_base{ 960.f, 600.f };
                Vec2 texture_size{ 40.f, 40.f };

                Vec2 indicator_data{ indicator_base.x, indicator_base.y + 28.f };
                Vec2 texture_position{ indicator_base.x, indicator_base.y - texture_size.y * 0.5f };

                auto box_color = Color( 255, 255, 50 );

                if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo
                    || g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::flee ) {
                    std::string warning_text = _( "Wont cast due to orbwalker mode " );

                    if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo ) {
                        warning_text += _(
                            "COMBO"
                        );
                    } else warning_text += _( "FLEE" );

                    Vec2 indicator_warning{ indicator_base.x, indicator_base.y + 48.f };

                    const auto text_size_warning = g_render->get_text_size( warning_text, g_fonts->get_bold( ), 20 );
                    g_render->text_shadow(
                        Vec2( indicator_warning.x - text_size_warning.x / 2.f, indicator_warning.y ),
                        Color( 255, 50, 50 ),
                        g_fonts->get_bold( ),
                        warning_text.data( ),
                        20
                    );
                    box_color = Color( 200, 200, 200 );
                } else if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::recalling ) {
                    m_recall_ult_active = false;
                    return;
                }

                std::string text      = "RECALL ULT";
                const auto  text_size = g_render->get_text_size( text, g_fonts->get_bold( ), 32 );

                auto data_text = champ_name + " in ";
                auto time_text = std::to_string( time_left );
                time_text.resize( 4 );

                data_text += time_text + "s";
                const auto text_size_data = g_render->get_text_size( data_text, g_fonts->get_bold( ), 16 );

                Vec2 box_start = { indicator_base.x - text_size.x / 2.f - 3.f, indicator_base.y + 3.f };

                g_render->filled_box( box_start, { text_size.x + 6.f, text_size.y + 12.f }, Color( 20, 20, 20, 90 ) );
                g_render->box( box_start, { text_size.x + 6.f, text_size.y + 12.f }, box_color );

                auto square_texture =                     path::join(
                        {
                            directory_manager::get_resources_path( ),
                            "champions",
                            champ_name,
                            champ_name + "_square.png"
                        }
                    );
                auto texture = g_render->load_texture_from_file(
                    square_texture.has_value(  ) ? *square_texture : ""
                );
                if ( texture ) {
                    g_render->image(
                        { texture_position.x - texture_size.x / 2, texture_position.y - texture_size.y / 2 },
                        texture_size,
                        texture
                    );
                }

                g_render->text_shadow(
                    Vec2( indicator_base.x - text_size.x / 2.f, indicator_base.y ),
                    box_color,
                    g_fonts->get_bold( ),
                    text.data( ),
                    32
                );
                g_render->text_shadow(
                    Vec2( indicator_data.x - text_size_data.x / 2.f, indicator_data.y ),
                    Color( 255, 255, 255 ),
                    g_fonts->get_bold( ),
                    data_text.data( ),
                    16
                );
            }


            slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
            if ( rt_hash( slot->get_name( ).data( ) ) != ct_hash( "LuxLightstrikeToggle" ) ) return;

            if ( !m_hitbox_active ) {
                auto e_position = get_e_position( );
                auto modifier = 1.f - e_position.dist_to( m_end_position ) / m_start_position.dist_to( m_end_position );

                g_render->circle_3d(
                    m_end_position,
                    Color( 255, 255, 255 ),
                    310.f,
                    Renderer::outline,
                    72,
                    4.f,
                    360.f * modifier
                );
            } else {
                g_render->circle_3d(
                    m_end_position,
                    g_features->orbwalker->get_pulsing_color( ).alpha( 25 ),
                    310.f,
                    Renderer::outline | Renderer::filled,
                    72,
                    4.f
                );
            }
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            cast_tracker( );
            e_recast( );

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return;

            antigapclose_q( );
            killsteal_r( );
            killsteal_e( );
            recall_ult( );
            antigapclose_e( );

            if ( g_features->orbwalker->in_attack( ) ) return;

            if ( g_config->lux.w_autoshield->get< bool >( ) ) shield_w( );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:
                spell_r( );
                spell_e( );
                spell_q( );
                shield_w( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
            case Orbwalker::EOrbwalkerMode::laneclear:
                if ( g_config->lux.e_harass->get< bool >( ) ) spell_e( );
                if ( g_config->lux.q_harass->get< bool >( ) ) spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::flee:
                if ( g_config->lux.q_flee->get< bool >( ) ) spell_q( );
                flee_e( );
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->lux.q_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.075f || *g_time -
                m_last_q_time <= 0.4f || !m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_q_range,
                1200.f,
                70.f,
                0.25f,
                { },
                true
            );
            if ( !pred.valid ||
                pred.hitchance < static_cast< Prediction::EHitchance >( g_config->lux.q_hitchance->get< int >( ) ) ||
                g_features->prediction->minions_in_line(
                    g_local->position,
                    pred.position,
                    70.f,
                    0,
                    0.25f,
                    0.25f + g_local->position.dist_to( pred.position ) / 1200.f
                ) > 1 )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{ return false; }

        auto shield_w( ) -> bool{
            if ( !g_config->lux.w_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.075f ||
                *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready( true ) )
                return false;

            if ( shielding_self_w( ) || shielding_allies_w( ) ) return true;

            return false;
        }

        auto shielding_self_w( ) -> bool{
            if ( !g_config->lux.w_shield_self->get< bool >( ) ) return false;

            const auto incoming_damage = helper::get_incoming_damage( g_local->index, 0.75f );
            if ( incoming_damage <= static_cast< float >( g_config->lux.w_shield_threshold->get< int >( ) ) ) {
                return
                    false;
            }

            auto lowest_health{ FLT_MAX };
            auto cast_position{ g_pw_hud->get_hud_manager( )->cursor_position_unclipped };

            for ( const auto ally : g_entity_list.get_allies( ) ) {
                if ( !ally || ally->dist_to_local( ) > m_w_range * 1.1f || ally->is_dead( ) ||
                    ally->network_id == g_local->network_id || ally->get_selectable_flag( ) != 1 )
                    continue;

                const auto health = ally->health;
                if ( health > lowest_health ) continue;

                const auto pred = g_features->prediction->predict( ally->index, m_w_range, 2400.f, 175.f, 0.25f );
                if ( !pred.valid ) continue;

                cast_position = pred.position;
                lowest_health = ally->health;
            }

            if ( cast_spell( ESpellSlot::w, cast_position ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto shielding_allies_w( ) -> bool{
            if ( !g_config->lux.w_shield_allies->get< bool >( ) ) return false;

            bool  allow_cast{ };
            Vec3  cast_position{ };
            float target_damage{ };

            for ( const auto ally : g_entity_list.get_allies( ) ) {
                if ( !ally || ally->network_id == g_local->network_id || ally->dist_to_local( ) > m_w_range * 1.1f ||
                    ally->is_dead( ) || ally->is_invisible( ) ||
                    ally->get_selectable_flag( ) != 1 )
                    continue;

                const auto incoming_damage = helper::get_incoming_damage( g_local->index, 0.75f );

                if ( target_damage > incoming_damage ||
                    incoming_damage <= static_cast< float >( g_config->lux.w_shield_threshold->get< int >( ) ) )
                    continue;

                const auto pred = g_features->prediction->predict( ally->index, m_w_range, 2400.f, 175.f, 0.25f );
                if ( !pred.valid ) continue;

                allow_cast    = true;
                cast_position = pred.position;
                target_damage = incoming_damage;
            }

            if ( !allow_cast ) return false;

            if ( cast_spell( ESpellSlot::w, cast_position ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->lux.e_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.075f ||
                *g_time - m_last_e_time <= 0.4f ||
                rt_hash( m_slot_e->get_name( ).data( ) ) == ct_hash( "LuxLightstrikeToggle" ) ||
                !m_slot_e->is_ready( true ) )
                return false;

            if ( g_config->lux.e_multihit->get< bool >( ) ) {
                const auto multihit = get_multihit_position( m_e_range, 1200.f, 300.f, 0.25f, false );

                if ( multihit.hit_count > 2 ) {
                    if ( cast_spell( ESpellSlot::e, multihit.position ) ) {
                        m_last_e_time    = *g_time;
                        m_last_cast_time = *g_time;
                        g_features->orbwalker->on_cast( );
                        return true;
                    }
                }
            }

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_e_range, 1200.f, 155.f, 0.25f );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->lux.e_hitchance->get<
                int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto killsteal_e( ) -> bool{
            if ( !g_config->lux.e_killsteal->get< bool >( ) || *g_time - m_last_cast_time <= 0.075f ||
                *g_time - m_last_e_time <= 0.4f ||
                rt_hash( m_slot_e->get_name( ).data( ) ) == ct_hash( "LuxLightstrikeToggle" ) ||
                !m_slot_e->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_killsteal_target(
                m_e_range,
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::e, unit ); },
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::e, unit ); },
                2
            );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_e_range, 1200.f, 200.f, 0.25f );
            if ( !pred.valid ) return false;


            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Lux ] Killsteal E, target is " << target->champion_name.text << " | " << *g_time;
                return true;
            }

            return false;
        }

        auto flee_e( ) -> bool{
            if ( !g_config->lux.e_flee->get< bool >( ) || *g_time - m_last_cast_time <= 0.075f ||
                *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready( true ) ||
                rt_hash( m_slot_e->get_name( ).data( ) ) == ct_hash( "LuxLightstrikeToggle" ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_e_range, 1200.f, 25.f, 0.25f );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->lux.e_hitchance->get<
                int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Lux ] Flee E, target is " << target->champion_name.text << " | " << *g_time;
                return true;
            }

            return false;
        }

        auto e_recast( ) -> bool{
            if ( !g_config->lux.e_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f || *g_time -
                m_last_e_time <= 0.4f || *g_time - m_last_e_recast_time <= 1.f || !m_slot_e->is_ready( )
                || rt_hash( m_slot_e->get_name().data() ) != ct_hash( "LuxLightstrikeToggle" ) )
                return false;

            const auto e_position = get_e_position( );
            if ( e_position.dist_to( m_end_position ) > 25.f ) return false;

            m_hitbox_active = true;

            int         enemy_count{ };
            bool        should_cast{ };
            bool        killsteal_cast{ };
            std::string target_name{ };

            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->position.dist_to( e_position ) > 310.f ||
                    g_features->target_selector->is_bad_target( enemy->index ) )
                    continue;


                if ( g_config->lux.e_killsteal->get< bool >( ) ) {
                    const auto damage = get_spell_damage( ESpellSlot::e, enemy );
                    const auto health = helper::get_real_health(
                        enemy->index,
                        EDamageType::magic_damage,
                        0.1f,
                        true
                    );

                    if ( damage > health ) {
                        killsteal_cast = true;
                        should_cast    = true;
                        target_name    = enemy->champion_name.text;
                        break;
                    }
                }

                ++enemy_count;

                auto pred = g_features->prediction->predict( enemy->index, 500.f, 0.f, 0.f, 0.2f, e_position );
                if ( !pred.valid ) continue;

                should_cast = pred.position.dist_to( e_position ) >= 305.f;

                if ( should_cast ) break;
            }

            if ( !should_cast ) return false;

            if ( cast_spell( ESpellSlot::e ) ) {
                m_last_e_time        = *g_time;
                m_last_e_recast_time = *g_time;
                m_last_cast_time     = *g_time;

                if ( killsteal_cast ) std::cout << "[ Lux ] Killsteal E2 targeting " << target_name << " | " << *g_time;

                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->lux.r_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.075f ||
                *g_time - m_last_r_time <= 0.4f || !m_slot_r->is_ready( true ) )
                return false;

            for ( auto i = 0; i < 2; i++ ) {
                if ( g_features->target_selector->is_forced( ) && i > 0 ) break;

                const auto target = g_features->target_selector->get_default_target( i > 0 );
                if ( !target ) break;

                if ( combo_r( target ) ) return true;
            }

            return false;
        }

        auto combo_r( Object* target ) -> bool{
            if ( !target ) return false;

            auto damage = get_spell_damage( ESpellSlot::r, target );

            if ( m_hitbox_active && target->position.dist_to( m_end_position ) <= 300.f ) {
                auto extra_damage = get_spell_damage( ESpellSlot::e, target );

                const auto buff = g_features->buff_cache->get_buff(
                    target->index,
                    ct_hash( "LuxIlluminatingFraulein" )
                );
                if ( !buff || buff->buff_data->end_time - *g_time < 1.f ) {
                    extra_damage += helper::calculate_damage(
                        get_passive_damage( ),
                        target->index,
                        false
                    );
                }

                damage += extra_damage;
            }

            const auto health = helper::get_real_health( target->index, EDamageType::magic_damage, 1.f, true );
            if ( health > damage ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_r_range, 0.f, 75.f, 1.f, { }, true );
            if ( !pred.valid || ( int )pred.hitchance < g_config->lux.r_hitchance->get< int >( ) ) return false;

            if ( cast_spell( ESpellSlot::r, pred.position ) ) {
                m_last_r_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto killsteal_r( ) -> bool{
            if ( !g_config->lux.r_killsteal->get< bool >( ) || *g_time - m_last_cast_time <= 0.075f ||
                *g_time - m_last_r_time <= 0.4f || !m_slot_r->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_killsteal_target(
                m_r_range,
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::r, unit ); },
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::r, unit ); },
                2
            );
            if ( !target || g_features->orbwalker->is_attackable( target->index ) ) return false;

            const auto predicted = g_features->prediction->predict(
                target->index,
                m_r_range,
                0.f,
                75.f,
                1.f,
                { },
                true
            );

            if ( !predicted.valid ) return false;

            if ( cast_spell( ESpellSlot::r, predicted.position ) ) {
                m_last_r_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto cast_tracker( ) -> void{
            if ( !m_e_active ) {
                auto sci = g_local->spell_book.get_spell_cast_info( );
                if ( !sci || sci->slot != 2 || sci->server_cast_time < *g_time ) return;

                m_e_active         = true;
                m_server_cast_time = sci->server_cast_time;
                m_start_position   = sci->start_position;
                m_end_position     = sci->end_position;

                m_hitbox_active = false;

                return;
            }

            if ( m_server_cast_time < *g_time ) m_e_active = false;
        }

        auto get_e_position( ) const -> Vec3{
            if ( *g_time < m_server_cast_time ) return m_start_position;

            const auto time_traveled     = *g_time - m_server_cast_time;
            const auto distance_traveled = 1200.f * time_traveled;

            const auto simulated_position = m_start_position.extend(
                m_end_position,
                std::min( m_start_position.dist_to( m_end_position ), distance_traveled )
            );

            return simulated_position;
        }

        auto antigapclose_q( ) -> void{
            if ( !g_config->lux.q_antigapclose->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f || *g_time -
                m_last_q_time <= 0.4f ||
                !m_slot_q->is_ready( true ) )
                return;

            const auto target = get_advanced_antigapclose_target( m_q_range, 1200.f, 70.f, 0.25f, true );
            if ( !target ) return;

            const auto pred =
                g_features->prediction->predict( target->index, m_q_range, 1200.f, 70.f, 0.25f, { }, true );
            if ( !pred.valid || ( int )pred.hitchance < 3 ||
                g_features->prediction->minions_in_line(
                    g_local->position,
                    pred.position,
                    70.f,
                    0,
                    0.25f,
                    0.25f + g_local->position.dist_to( pred.position ) / 1200.f
                ) > 1 )
                return;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                std::cout << "[ Lux ] Antigapclose Q against " << target->champion_name.text << " | " << *g_time
                    << std::endl;
            }
        }

        auto antigapclose_e( ) -> void{
            if ( !g_config->lux.e_antigapclose->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_e_time <= 0.4f || rt_hash( m_slot_e->get_name( ).data( ) ) == ct_hash(
                    "LuxLightstrikeToggle"
                ) ||
                !m_slot_e->is_ready( true ) )
                return;

            const auto target = get_advanced_antigapclose_target( m_e_range, 1200.f, 310.f, 0.25f );
            if ( !target ) return;

            const auto pred =
                g_features->prediction->predict( target->index, m_e_range, 1200.f, 155.f, 0.25f );
            if ( !pred.valid || ( int )pred.hitchance < 3 ) return;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                std::cout << "[ Lux ] Antigapclose E against " << target->champion_name.text << " | " << *g_time
                    << std::endl;
            }
        }

        auto recall_ult( ) -> void{
            if ( !g_config->lux.r_recall_ult->get< bool >( ) || *g_time - m_last_r_time <= 0.5f || !m_slot_r->is_ready(
                true
            ) ) {
                m_recall_ult_active = false;
                return;
            }

            if ( m_recall_ult_active ) {
                base_ult_tracking( );
                return;
            }

            Object* target{ };
            bool    found_target{ };

            for ( auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->is_dead( ) || enemy->dist_to_local( ) > 3400.f || !enemy->
                    is_recalling( ) )
                    continue;

                auto recall = g_features->tracker->get_recall( enemy->index );
                if ( !recall ) continue;

                auto recall_time_left = recall->finish_time - *g_time;
                auto travel_time      = 1.f;
                if ( travel_time >= recall_time_left ) continue;

                float health_regenerated{ };

                if ( enemy->is_invisible( ) ) {
                    auto last_seen_data = g_features->tracker->get_last_seen_data( enemy->index );
                    if ( !last_seen_data ) continue;

                    const auto time_reduction = last_seen_data->total_recall_time + last_seen_data->time_in_recall;
                    const auto time_missing   = *g_time - last_seen_data->last_seen_time - time_reduction;
                    if ( time_missing * enemy->movement_speed > 100.f ) continue;

                    health_regenerated = ( *g_time - last_seen_data->last_seen_time ) * enemy->total_health_regen;
                    health_regenerated += std::ceil( travel_time ) * enemy->total_health_regen;
                } else health_regenerated = std::ceil( travel_time ) * enemy->total_health_regen;

                auto damage = get_spell_damage( ESpellSlot::r, enemy );
                if ( damage < enemy->health + health_regenerated ) continue;

                target       = enemy;
                found_target = true;
                break;
            }

            if ( !found_target || !target ) return;

            auto recall = g_features->tracker->get_recall( target->index );
            if ( !recall ) return;

            m_target_index       = target->index;
            m_baseult_start_time = *g_time;
            m_recall_ult_active  = true;

            base_ult_tracking( );
        }

        auto base_ult_tracking( ) -> void{
            if ( !m_recall_ult_active || !m_slot_r->is_ready( true ) ) {
                m_recall_ult_active = false;
                return;
            }

            const auto& target = g_entity_list.get_by_index( m_target_index );
            if ( !target || !target->is_recalling( ) || target->dist_to_local( ) > 3400.f ) {
                m_recall_ult_active = false;
                return;
            }

            const auto recall = g_features->tracker->get_recall( target->index );
            if ( !recall ) {
                m_recall_ult_active = false;
                return;
            }

            const auto recall_time_left = recall->finish_time - *g_time;
            const auto travel_time      = 1.f;
            const auto damage           = get_spell_damage( ESpellSlot::r, target.get( ) );

            const auto time_until_hit = recall_time_left - travel_time;
            auto       health_regenerated{ std::ceil( time_until_hit ) * target->total_health_regen };
            auto       min_possible_health_regen = std::ceil( travel_time ) * target->total_health_regen;

            if ( target->is_invisible( ) ) {
                const auto last_seen_data = g_features->tracker->get_last_seen_data( target->index );
                if ( !last_seen_data ) {
                    m_recall_ult_active = false;
                    return;
                }

                const auto time_reduction = last_seen_data->total_recall_time + last_seen_data->time_in_recall;
                const auto time_missing   = *g_time - last_seen_data->last_seen_time - time_reduction;
                if ( time_missing * target->movement_speed > 160.f ) {
                    m_recall_ult_active = false;
                    return;
                }

                health_regenerated += std::ceil( *g_time - last_seen_data->last_seen_time ) * target->
                    total_health_regen;
                min_possible_health_regen += std::ceil( *g_time - last_seen_data->last_seen_time ) * target->
                    total_health_regen;
            }

            bool should_cast{ };

            m_predicted_cast_time = recall->finish_time - travel_time - 0.5f;

            if ( damage > target->health + health_regenerated ) should_cast = time_until_hit <= 0.5f;
            else if ( damage > target->health + min_possible_health_regen ) {
                if ( g_config->lux.r_recall_ult_delay->get< bool >( ) ) {
                    int        max_wait_time{ };
                    const auto seconds_until_recall = static_cast< int >( std::floor( recall_time_left - 1.f ) );

                    for ( auto i = 1; i <= seconds_until_recall; i++ ) {
                        const auto regen_amount = min_possible_health_regen + target->total_health_regen * i;
                        if ( target->health + regen_amount >= damage ) break;

                        max_wait_time = i;
                    }

                    m_predicted_cast_time = recall->finish_time - travel_time - ( static_cast< float >( max_wait_time )
                        + 1.f ) - 0.5f;
                    should_cast = max_wait_time <= 1;
                } else should_cast = true;
            } else {
                m_recall_ult_active = false;
                return;
            }

            if ( !should_cast ) return;

            if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo
                || g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::flee ) {
                m_recall_ult_active = false;
                return;
            }

            if ( cast_spell( ESpellSlot::r, target->position ) ) {
                m_recall_ult_active = false;
                m_last_r_time       = *g_time;
                g_features->orbwalker->on_cast( );
            }
        }

        static auto get_passive_damage( ) -> float{
            return 10.f + 10.f * std::min( g_local->level, 18 ) + g_local->ability_power( ) * 0.2f;
        }

        auto get_r_damage( const int16_t target_index ) -> float{
            const auto damage = m_r_damage[ m_slot_r->level ] + g_local->ability_power( ) * 1.2f;
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
            case ESpellSlot::e:
                return helper::calculate_damage(
                    m_e_damage[ m_slot_e->level ] + g_local->ability_power( ) * 0.8f,
                    target->index,
                    false
                );
            case ESpellSlot::r:
            {
                auto       damage = m_r_damage[ m_slot_r->level ] + g_local->ability_power( ) * 1.2f;
                const auto buff   = g_features->buff_cache->get_buff(
                    target->index,
                    ct_hash( "LuxIlluminatingFraulein" )
                );
                if ( buff && buff->buff_data->end_time - *g_time > 1.f + g_features->orbwalker->get_ping( ) ) {
                    damage += 10.f + 10.f * static_cast< float >( std::min( g_local->level, 18 ) + g_local->
                        ability_power( ) * 0.2f );
                }

                return helper::calculate_damage( damage, target->index, false );
            }
            }
            return 0.f;
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
            case ESpellSlot::e:
            {
                const auto tt   = 0.25f + g_local->position.dist_to( target->position ) / 1200.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.25f + g_local->position.dist_to( pred.value( ) ) / 1200.f;
            }
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

        float m_last_e_recast_time{ };

        // recast logic
        bool m_hitbox_active{ };

        // cast tracking
        bool  m_e_active{ };
        float m_server_cast_time{ };
        Vec3  m_start_position{ };
        Vec3  m_end_position{ };

        // silent baseult
        bool    m_recall_ult_active{ };
        int16_t m_target_index{ };
        float   m_baseult_start_time{ };
        float   m_predicted_cast_time{ };

        std::array< float, 6 > m_q_damage = { 0.f, 80.f, 120.f, 160.f, 200.f, 240.f };
        std::array< float, 6 > m_e_damage = { 0.f, 70.f, 120.f, 170.f, 220.f, 270.f };
        std::array< float, 6 > m_r_damage = { 0.f, 300.f, 400.f, 500.f };

        float m_q_range{ 1240.f };
        float m_w_range{ 1175.f };
        float m_e_range{ 1100.f };
        float m_r_range{ 3400.f };
    };
}
