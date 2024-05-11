#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"
#include "../../sdk/game/buff.hpp"
#include "../utils/c_function_caller.hpp"

namespace features::champion_modules {
    class cassiopeia_module final : public IModule {
    public:
        virtual ~cassiopeia_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "cassiopeia_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Cassiopeia" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        struct highlight_instance_t {
            int16_t  index{ };
            unsigned network_id{ };

            float poison_expire_time{ };
            float poison_update_time{ };

            float miasma_expire_time{ };

            bool is_glowing{ };
        };

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "cassiopeia" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );
            const auto options    = navigation->add_section( _( "general" ) );
            const auto combat     = navigation->add_section( _( "combat mode" ) );

            q_settings->checkbox( _( "enable" ), g_config->cassiopeia.q_enabled );
            q_settings->checkbox( _( "harass q" ), g_config->cassiopeia.q_harass );
            q_settings->checkbox( _( "spellfarm" ), g_config->cassiopeia.q_spellfarm );
            q_settings->select(
                _( "hitchance" ),
                g_config->cassiopeia.q_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            w_settings->checkbox( _( "enable" ), g_config->cassiopeia.w_enabled );
            //w_settings->slider_int( _( "max range %" ), g_config->cassiopeia.w_max_range, 25, 100, 1 );
            w_settings->checkbox( _( "antigapclose w" ), g_config->cassiopeia.w_antigapclose );
            w_settings->select(
                _( "hitchance" ),
                g_config->cassiopeia.w_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            e_settings->checkbox( _( "enable" ), g_config->cassiopeia.e_enabled );
            e_settings->checkbox( _( "combo e only if poisoned" ), g_config->cassiopeia.e_only_poisoned );
            e_settings->checkbox( _( "spellfarm e" ), g_config->cassiopeia.e_spellfarm );
            e_settings->checkbox(
                _( "integrate e lasthit in orb" ),
                g_config->cassiopeia.e_integrate_lasthitting_in_orbwalker
            );

            options->checkbox( _( "using coup de grace" ), g_config->cassiopeia.calculate_coup_de_grace );

            // combat mode
            combat->checkbox( _( "enable combat logic" ), g_config->cassiopeia.combat_mode_enabled );
            combat->checkbox( _( "force toggle (hotkey)" ), g_config->ezreal.combat_mode_toggle );
            combat->multi_select(
                _( "combat logic options " ),
                {
                    g_config->cassiopeia.combat_ignore_q_hitchance,
                    g_config->cassiopeia.combat_cast_w,
                    g_config->cassiopeia.combat_allow_r_stun
                },
                {
                    _( "Ignore q hitchance" ),
                    _( "Allow casting W" ),
                    _( "Allow casting single-target R" )
                }
            );

            combat->slider_int( _( "auto toggle range" ), g_config->cassiopeia.combat_max_threshold, 200, 750, 25 );

            r_settings->checkbox( _( "enable" ), g_config->cassiopeia.r_enabled );
            r_settings->checkbox(
                _( "single target only full combo (?)" ),
                g_config->cassiopeia.r_full_combo_single_target
            )->set_tooltip( _( "Will only cast on single target if in full combo" ) );
            r_settings->select(
                _( "hitchance" ),
                g_config->cassiopeia.r_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );
            r_settings->slider_int( _( "min multihit count" ), g_config->cassiopeia.r_minimum_multihit, 2, 5, 1 );

            drawings->checkbox( _( "draw q range" ), g_config->cassiopeia.q_draw_range );
            drawings->checkbox( _( "draw w range" ), g_config->cassiopeia.w_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->cassiopeia.e_draw_range );
            drawings->checkbox( _( "draw r range" ), g_config->cassiopeia.r_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->cassiopeia.dont_draw_on_cooldown );
            drawings->checkbox( _( "highlight poisoned enemies" ), g_config->cassiopeia.highlight_poison );
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->cassiopeia.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->cassiopeia.dont_draw_on_cooldown
                    ->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 31, 88, 255, 255 ),
                        m_q_range,
                        Renderer::outline,
                        72,
                        3.f
                    );
                }
            }

            if ( g_config->cassiopeia.w_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->cassiopeia.dont_draw_on_cooldown
                    ->get< bool >( ) ) ) {
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

            if ( g_config->cassiopeia.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->cassiopeia.dont_draw_on_cooldown
                    ->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 255, 255, 0, 255 ),
                        m_e_range,
                        Renderer::outline,
                        75,
                        3.f
                    );
                }
            }

            if ( g_config->cassiopeia.r_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::r );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->cassiopeia.dont_draw_on_cooldown
                    ->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 173, 47, 68, 255 ),
                        m_r_range,
                        Renderer::outline,
                        72,
                        3.f
                    );
                }
            }

            if ( g_config->cassiopeia.combat_draw_threshold->get< bool >( ) && m_threat_nearby ) {
                g_render->circle_3d(
                    g_local->position,
                    m_in_combat
                        ? ( m_threat_level >= 1.f
                                ? g_features->orbwalker->get_pulsing_color( )
                                : g_features->orbwalker->get_pulsing_color( ).alpha(
                                    static_cast< int32_t >(
                                        255.f * m_threat_level
                                    )
                                ) )
                        : Color( 255.f, 255.f, 255.f, 255.f * m_threat_level ),
                    g_config->cassiopeia.combat_max_threshold->get< int >( ),
                    Renderer::outline,
                    76,
                    3.f
                );
            }

            if ( m_q_expire_time > *g_time ) {
                auto modifier = std::clamp( ( m_q_expire_time - *g_time ) / 0.65f, 0.f, 1.f );
                modifier      = utils::ease::ease_in_cubic( modifier );


                g_render->circle_3d(
                    m_q_target_position,
                    Color( 255.f, 255.f, 255.f, 75.f * modifier ),
                    200.f,
                    Renderer::filled,
                    48,
                    1.f
                );

                g_render->circle_3d(
                    m_q_target_position,
                    Color( 255.f, 255.f, 255.f, 255.f * modifier ),
                    200.f,
                    Renderer::outline,
                    48,
                    3.f
                );
            }

            Vec2 sp{ };
            if ( world_to_screen( g_local->position, sp ) &&
                ( g_config->cassiopeia.combat_draw_text_mode->get< int >( ) == 1 && m_in_combat ||
                    g_config->cassiopeia.combat_draw_text_mode->get< int >( ) == 2 ) ) {
                sp.x += 30.f;
                sp.y -= 8.f;

                std::string text{ "COMBAT MODE: " };
                // std::string toggle_text{ "[ MOUSE 5 ] FORCE COMBAT " };
                const auto text_size = g_render->get_text_size( text, g_fonts->get_block( ), 8.f );

                std::string text_state{
                    m_in_combat
                        ? "ACTIVE"
                        : g_config->cassiopeia.combat_mode_enabled->get< bool >( )
                              ? "INACTIVE"
                              : "DISABLED"
                };
                auto state_color = m_in_combat ? g_features->orbwalker->get_pulsing_color( ) : Color( 50, 255, 50 );

                if ( !g_config->cassiopeia.combat_mode_enabled->get< bool >( ) ) state_color = Color( 255, 50, 50 );

                g_render->text_shadow( sp, Color( 255, 255, 255 ), g_fonts->get_block( ), text.c_str( ), 8 );

                g_render->text_shadow(
                    { sp.x + text_size.x, sp.y },
                    state_color,
                    g_fonts->get_block( ),
                    text_state.c_str( ),
                    8
                );

                // sp.y += 8.f;
                // g_render->text_shadow( sp, toggle_color, g_fonts->get_block( ), toggle_text.c_str( ), 8 );
            }

            if ( !g_config->cassiopeia.highlight_poison->get< bool >( ) ) return;

            for ( const auto inst : m_highlights ) {
                if ( inst.poison_expire_time < *g_time && inst.miasma_expire_time < *g_time ) continue;

                auto unit = g_entity_list.get_by_index( inst.index );
                if ( !unit ) continue;
                unit.update( );

                Vec2 sp;
                if ( !world_to_screen( unit->position, sp ) ) continue;

                auto base_position = unit->get_hpbar_position( );
                if ( base_position.x <= 0 || base_position.y <= 0 ) continue;

                auto bar_length = static_cast< float >( g_render_manager->get_width( ) ) * 0.0546875f;
                auto bar_height = static_cast< float >( g_render_manager->get_height( ) ) * 0.022222222f;

                base_position.x -= bar_length * 0.425f;
                base_position.y -= static_cast< float >( g_render_manager->get_height( ) ) * 0.003f;

                auto small_bar_height = bar_height * 0.9f;
                Vec2 gold_base{
                    base_position.x + bar_length + 4.f,
                    base_position.y - bar_height * 0.95f + small_bar_height
                };

                // gold drawing

                Vec2 gold_size{ 20.f, 20.f };
                auto gold_text_position = gold_base + Vec2( 20.f, 10.f );

                static auto poison = path::join(
                        { directory_manager::get_resources_path( ), "common", "poison.png" }
                    );
                auto texture = g_render->load_texture_from_file(
                    poison.has_value(  ) ? *poison : ""
                );
                if ( inst.poison_expire_time > *g_time && texture ) {
                    auto  text_color = Color( 180, 90, 255 );
                    auto  opacity    = 255;
                    float extra_offset{ };

                    auto font_size{ 20 };

                    if ( *g_time - inst.poison_update_time < 0.275f ) {
                        auto modifier = std::clamp( 1.f - ( *g_time - inst.poison_update_time ) / 0.275f, 0.f, 1.f );
                        auto alt_modifier = utils::ease::ease_in_quint( modifier );
                        auto fade_modifier = utils::ease::ease_out_quint( modifier );

                        text_color.r += ( 255 - text_color.r ) * fade_modifier;
                        text_color.g += ( 255 - text_color.g ) * fade_modifier;
                        text_color.b += ( 255 - text_color.b ) * fade_modifier;

                        extra_offset = 5.f * alt_modifier;


                        text_color.alpha( 255 - 255 * alt_modifier );
                        gold_text_position.x += extra_offset;
                        font_size += extra_offset;
                    }

                    auto text = std::to_string( inst.poison_expire_time - *g_time );
                    text.resize( 3 );

                    const auto text_size  = g_render->get_text_size( text, g_fonts->get_zabel( ), font_size );
                    const auto dummy_size = g_render->get_text_size( "3.0.", g_fonts->get_zabel( ), font_size );

                    // gold_text_position.x -= text_size.x / 3.f;
                    gold_text_position.y -= text_size.y / 2.f;

                    g_render->filled_box(
                        gold_base,
                        Vec2( gold_size.x + dummy_size.x + extra_offset, gold_size.y ),
                        Color( 0, 0, 0, 100 ),
                        -1
                    );

                    g_render->image( gold_base, gold_size, texture );

                    g_render->text_shadow(
                        gold_text_position,
                        text_color,
                        g_fonts->get_zabel( ),
                        text.data( ),
                        font_size
                    );

                    gold_base.y += gold_size.y;
                }

                if ( *g_time > inst.miasma_expire_time ) continue;

                static auto Cassiopeia_w =                     path::join(
                        {
                            directory_manager::get_resources_path( ),
                            "champions",
                            "Cassiopeia",
                            "spells",
                            "Cassiopeia_w.png"
                        }
                    );
                auto second_texture = g_render->load_texture_from_file(
                    Cassiopeia_w.has_value(  ) ? *Cassiopeia_w : ""
                );
                if ( !second_texture ) continue;

                std::string text      = "miasma";
                const auto  text_size = g_render->get_text_size( text, g_fonts->get_zabel( ), 20 );

                gold_text_position = gold_base + Vec2( 20.f, 10.f );
                gold_text_position.y -= text_size.y / 2.f;

                g_render->filled_box(
                    gold_base,
                    Vec2( gold_size.x + text_size.x, gold_size.y ),
                    Color( 50, 10, 90, 100 ),
                    -1
                );

                g_render->image( gold_base, gold_size, second_texture );

                g_render->text_shadow( gold_text_position, Color::white( ), g_fonts->get_zabel( ), text.data( ), 20 );
            }
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            update_poisoned( );
            cast_tracking( );

            if ( g_config->cassiopeia.combat_mode_enabled->get< bool >( ) ) {
                // m_in_combat = g_config->ezreal.combat_mode_toggle->get< bool >( );

                const auto target = g_features->target_selector->get_default_target( );
                if ( target && target->is_alive( ) ) {
                    if ( !g_config->ezreal.combat_mode_toggle->get< bool >( ) ) {
                        const auto distance = target->dist_to_local( );
                        if ( !m_in_combat ) {
                            m_threat_level = distance < g_config->cassiopeia.combat_max_threshold->get< int >( )
                                                 ? 1.f
                                                 : 1.f -
                                                 std::min(
                                                     ( distance - g_config->cassiopeia.combat_max_threshold->get<
                                                         int >( ) ) /
                                                     200.f,
                                                     1.f
                                                 );

                            m_in_combat     = distance < g_config->cassiopeia.combat_max_threshold->get< int >( );
                            m_threat_nearby = m_threat_level > 0.f;
                        } else {
                            m_in_combat = distance < g_config->cassiopeia.combat_max_threshold->get< int >( ) + 75.f;
                            m_threat_nearby = m_in_combat;
                        }
                    } else {
                        m_in_combat = g_config->ezreal.combat_mode_toggle->get< bool >( );

                        m_threat_level =
                            target->dist_to_local( ) < g_config->cassiopeia.combat_max_threshold->get< int >( )
                                ? 1.f
                                : 1.f -
                                std::min(
                                    ( target->dist_to_local( ) -
                                        g_config->cassiopeia.combat_max_threshold->get< int >( ) ) /
                                    200.f,
                                    1.f
                                );

                        m_threat_nearby = m_threat_level > 0.f;
                    }
                } else {
                    m_in_combat     = g_config->ezreal.combat_mode_toggle->get< bool >( );
                    m_threat_nearby = false;
                }
            } else {
                m_in_combat     = false;
                m_threat_nearby = false;
            }

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return;

            antigapclose_w( );

            if ( g_features->orbwalker->in_attack( ) ) return;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_r( );
                spell_q( );
                spell_e( );
                spell_w( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->cassiopeia.q_harass->get< bool >( ) ) spell_q( );

                spell_e( );

                orbwalker_lasthit_e( );
                lasthit_e( );
                break;
            case Orbwalker::EOrbwalkerMode::laneclear:

                orbwalker_lasthit_e( );
                laneclear_q( );
                lasthit_e( );
                laneclear_e( );

                if ( g_config->cassiopeia.q_harass->get< bool >( ) && !helper::is_position_under_turret(
                    g_local->position
                ) )
                    spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::lasthit:

                orbwalker_lasthit_e( );
                lasthit_e( );
                break;
            case Orbwalker::EOrbwalkerMode::flee:

                spell_q( );
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->cassiopeia.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.35f || *g_time -
                m_last_cast_time <= 0.016f || !m_slot_q->
                is_ready( true ) )
                return false;

            if ( g_config->cassiopeia.q_multihit->get< bool >( ) ) {
                const auto multihit = get_multihit_position( m_q_range, 0.f, 200.f, 0.75f, false );

                if ( multihit.hit_count > 1 ) {
                    if ( cast_spell( ESpellSlot::q, multihit.position ) ) {
                        m_last_q_time = *g_time;
                        g_features->orbwalker->on_cast( );
                        return true;
                    }
                }
            }

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;


            const auto buff = g_features->buff_cache->get_buff( target->index, ct_hash( "cassiopeiaqdebuff" ) );
            if ( buff && buff->buff_data->end_time - *g_time > 1.4f ||
                g_features->buff_cache->get_buff( target->index, ct_hash( "cassiopeiawpoison" ) ) )
                return false;


            const auto pred = g_features->prediction->predict( target->index, m_q_range, 0.f, 200.f, 0.75f );
            if ( !pred.valid && pred.hitchance < static_cast< Prediction::EHitchance >( g_config->cassiopeia.
                    q_hitchance->get< int >( ) ) &&
                ( !g_config->cassiopeia.combat_ignore_q_hitchance->get< bool >( ) || !m_in_combat ) )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !m_in_combat && !g_config->cassiopeia.w_enabled->get< bool >( ) ||
                m_in_combat && !g_config->cassiopeia.combat_cast_w->get< bool >( ) ||
                *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            if ( !m_in_combat &&
                ( g_features->buff_cache->get_buff( target->index, ct_hash( "cassiopeiaqdebuff" ) )
                    || m_slot_q->is_ready( true ) || *g_time - m_last_q_time < 1.f ) )
                return false;

            const auto pred = g_features->prediction->predict( target->index, m_w_range, 3000.f, 50.f, 0.25f );
            if ( !pred.valid || !m_in_combat &&
                pred.hitchance < static_cast< Prediction::EHitchance >( g_config->cassiopeia.w_hitchance->get<
                    int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                std::cout << "[ Cassio: W ] Combo cast at " << target->get_name( ) << " | InCombat: " << m_in_combat
                    << " | T: " << *g_time << std::endl;
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->cassiopeia.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.25f || *g_time -
                m_last_cast_time <= 0.01f || !can_cast_e( ) )
                return false;

            const auto target = get_e_target( );
            if ( !target ) return false;

            const auto damage        = get_spell_damage( ESpellSlot::e, target );
            const int  casts_to_kill = std::ceil(
                helper::get_real_health( target->index, EDamageType::magic_damage, 0.15f, true ) / damage
            );

            if ( g_config->cassiopeia.e_only_poisoned->get< bool >( ) && casts_to_kill > 2
                && !is_unit_poisoned( target->index ) )
                return false;

            if ( cast_spell( ESpellSlot::e, target->network_id ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Cassio: E ] Combo at target " << target->get_name( )
                    << " | casts to kill: " << casts_to_kill << " | T: " << *g_time << std::endl;

                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->cassiopeia.r_enabled->get< bool >( ) || *g_time - m_last_r_time <= 0.5f || !m_slot_r->
                is_ready( true ) )
                return false;

            int  max_count{ };
            Vec3 cast_position{ };

            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->dist_to_local( ) > m_r_range || g_features->target_selector->is_bad_target(
                    enemy->index
                ) )
                    continue;

                auto predict = g_features->prediction->predict_default( enemy->index, 0.5f );
                if ( !predict ) continue;

                auto sect    = sdk::math::Sector( g_local->position, *predict, 80.f, m_r_range );
                auto polygon = sect.to_polygon_new( );

                int count{ };

                for ( const auto hero : g_entity_list.get_enemies( ) ) {
                    if ( !hero || hero->dist_to_local( ) > m_r_range || g_features->target_selector->is_bad_target(
                        hero->index
                    ) )
                        continue;

                    auto pred = g_features->prediction->predict_default( hero->index, 0.5f );
                    if ( !pred || polygon.is_outside( *pred ) ) continue;

                    if ( hero->position.dist_to( *pred ) < 1.f ) {
                        const auto direction = hero->position + hero->get_direction( );

                        if ( hero->dist_to_local( ) < g_local->position.dist_to( direction ) ) continue;
                    } else if ( hero->dist_to_local( ) < g_local->position.dist_to( *pred ) ) continue;


                    ++count;
                }

                if ( count > max_count ) {
                    max_count     = count;
                    cast_position = *predict;
                }
            }

            if ( max_count >= g_config->cassiopeia.r_minimum_multihit->get< int >( ) ) {
                if ( cast_spell( ESpellSlot::r, cast_position ) ) {
                    debug_log( "[ CASSIO ]: R Multihit with {} targets", max_count );
                    m_last_r_time = *g_time;
                    g_features->orbwalker->on_cast( );
                    return true;
                }

                return false;
            }

            if ( g_config->cassiopeia.r_full_combo_single_target->get< bool >( ) && !
                GetAsyncKeyState( VK_CONTROL ) &&
                g_config->cassiopeia.combat_allow_r_stun->get< bool >( ) && !m_in_combat )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) >= m_r_range ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_r_range - 50.f, 0.f, 0.f, 0.5f );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->cassiopeia.
                r_hitchance->get< int >( ) ) )
                return false;

            const auto dir_pred = g_features->prediction->predict_default( target->index, 0.1f );
            if ( !dir_pred ) return false;

            if ( target->position == dir_pred.value( ) ) {
                const auto direction = target->position + target->get_direction( );

                if ( target->dist_to_local( ) < g_local->position.dist_to( direction ) ) return false;
            } else if ( target->dist_to_local( ) < g_local->position.dist_to( dir_pred.value( ) ) ) return false;


            if ( cast_spell( ESpellSlot::r, pred.position ) ) {
                m_last_r_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                std::cout << "[ Cassio: R ] Combo cast | InCombat: " << m_in_combat << " | T: " << std::endl;
                return true;
            }

            return false;
        }

        auto lasthit_e( ) -> bool{
            if ( !g_config->cassiopeia.e_spellfarm->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !
                can_cast_e( ) )
                return false;

            const auto          lasthit_data = get_targetable_lasthit_target(
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::e, unit ); },
                695.f,
                [this]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::e, unit ); }
            );
            if ( !lasthit_data ) return false;

            if ( cast_spell( ESpellSlot::e, lasthit_data->network_id ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                g_features->orbwalker->ignore_minion( lasthit_data->index, lasthit_data->travel_time + 0.25f );

                std::cout << "[ Cassio: E ] Lasthit E | T: " << *g_time << std::endl;
                return true;
            }

            return false;
        }

        auto orbwalker_lasthit_e( ) -> bool{
            if ( !g_config->cassiopeia.e_integrate_lasthitting_in_orbwalker->get< bool >( )
                || *g_time - m_last_e_time <= 0.25f || *g_time - m_last_cast_time <= 0.025f || !can_cast_e( ) )
                return false;

            const auto index = g_features->orbwalker->get_spellfarm_target_index( );
            if ( !index ) return false;

            const auto target = g_entity_list.get_by_index( index.value( ) );
            if ( !target || target->is_dead( ) || target->
                is_invisible( )// || g_features->orbwalker->is_ignored( target->index )
                || target->dist_to_local( ) > m_e_range )
                return false;

            if ( cast_spell( ESpellSlot::e, target->network_id ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                g_features->orbwalker->ignore_minion(
                    target->index,
                    get_spell_travel_time( ESpellSlot::e, target.get( ) )
                );

                std::cout << "[ Cassio: E ] Integrated lasthit | T:" << *g_time << std::endl;
                return true;
            }

            return false;
        }

        auto laneclear_e( ) -> bool{
            if ( !g_config->cassiopeia.e_spellfarm->get< bool >( ) || *g_time - m_last_e_time <= 0.4f ||
                !can_cast_e( ) )
                return false;

            unsigned            target_nid{ };
            Object::EMinionType target_type{ };
            int                 monster_priority{ };
            auto                target_health{ 9999.f };

            bool delay{ };

            const auto is_fastclear = GetAsyncKeyState( VK_CONTROL ) && g_features->orbwalker->get_mode( ) ==
                Orbwalker::EOrbwalkerMode::laneclear;

            for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
                if ( !minion ||
                    g_local->position.dist_to( minion->position ) > m_e_range ||
                    minion->is_dead( ) ||
                    minion->is_invisible( ) ||
                    !minion->is_lane_minion( ) &&
                    !minion->is_jungle_monster( ) ||
                    !is_fastclear && !minion->is_jungle_monster( ) ||
                    g_features->orbwalker->is_ignored( minion->index ) ||
                    !is_unit_poisoned( minion->index )
                )
                    continue;

                const auto travel_time    = 0.125f + g_local->position.dist_to( minion->position ) / 2500.f;
                const auto health_on_hit  = g_features->prediction->predict_health( minion, travel_time );
                const auto health_on_next = g_features->prediction->predict_health(
                    minion,
                    m_slot_e->cooldown + travel_time
                );
                const auto damage = get_spell_damage( ESpellSlot::e, minion );

                if ( health_on_hit < 0 || target_type > minion->get_minion_type( ) || monster_priority > minion->
                    get_monster_priority( ) || minion->health > target_health )
                    continue;

                if ( minion->is_lane_minion( ) && g_features->prediction->is_minion_in_danger( minion->index ) &&
                    health_on_hit > damage && health_on_next - damage <= damage * 0.5f ) {
                    delay = true;
                    break;
                }

                target_nid       = minion->network_id;
                target_type      = minion->get_minion_type( );
                monster_priority = minion->get_monster_priority( );
                target_health    = minion->health;
            }

            if ( target_nid == 0 || delay ) return false;

            if ( cast_spell( ESpellSlot::e, target_nid ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                std::cout << "[ Cassio: E ] Laneclear E | T:" << *g_time << std::endl;

                return true;
            }

            return false;
        }

        auto laneclear_q( ) -> bool{
            if ( !g_config->cassiopeia.q_spellfarm->get< bool >( ) || *g_time - m_last_q_time <= 2.5f || !m_slot_q->
                is_ready( true ) )
                return false;

            const auto min_count = GetAsyncKeyState( VK_CONTROL ) ? 3 : 10;

            const auto farm_pos = get_best_laneclear_position( m_q_range, 100.f, true, true, 0.5f );
            if ( farm_pos.value <= min_count ) return false;

            if ( cast_spell( ESpellSlot::q, farm_pos.position ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->on_cast( );

                return true;
            }

            return false;
        }

        auto antigapclose_w( ) -> void{
            if ( !g_config->cassiopeia.w_antigapclose->get< bool >( ) || *g_time - m_last_w_time <= 0.3f
                | *g_time - m_last_cast_time <= 0.01f || !m_slot_w->is_ready( true ) )
                return;

            const auto target = get_advanced_antigapclose_target( m_w_range, 3000.f, 100.f, 0.25f, false );
            if ( !target ) return;

            const auto pred = g_features->prediction->predict( target->index, m_w_range, 3000.f, 100.f, 0.25f );
            if ( !pred.valid || ( int )pred.hitchance < 2 ) return;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                std::cout << "[ Cassio: W ] Antigapclose against " << target->champion_name.text << std::endl;
            }

            return;
        }

        auto get_e_target( ) -> Object*{
            Object* target{ };
            bool    target_poisoned{ };
            auto    target_casts_to_kill{ 1000 };
            bool    target_executable{ };
            int     target_priority{ };

            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->dist_to_local( ) > 700.f || g_features->target_selector->is_bad_target(
                    enemy->index
                ) )
                    continue;

                const auto damage        = get_spell_damage( ESpellSlot::e, enemy );
                const int  casts_to_kill = std::ceil(
                    helper::get_real_health( enemy->index, EDamageType::magic_damage, 0.15f, true ) / damage
                );
                const auto executable = casts_to_kill <= 2;
                const auto priority   = g_features->target_selector->get_target_priority( enemy->champion_name.text );

                const auto is_poisoned = is_unit_poisoned( enemy->index );

                if ( target_executable && !executable ||
                    target_executable && executable && ( priority < target_priority || priority == target_priority &&
                        casts_to_kill > target_casts_to_kill ) )
                    continue;

                if ( !executable ) {
                    if ( target_poisoned && !is_poisoned ||
                        target_poisoned && is_poisoned &&
                        ( priority < target_priority && casts_to_kill + 2 > target_casts_to_kill
                            || priority == target_priority && casts_to_kill > target_casts_to_kill ) )

                        continue;
                }

                target               = enemy;
                target_poisoned      = is_poisoned;
                target_priority      = priority;
                target_executable    = executable;
                target_casts_to_kill = casts_to_kill;
            }

            return target;
        }

        auto can_cast_e( ) -> bool{
            return m_slot_e->get_manacost( ) <= g_local->mana &&
            ( m_slot_e->is_ready( ) ||
                m_slot_e->cooldown_expire - *g_time <= g_features->orbwalker->get_ping( ) );
        }

        auto update_poisoned( ) -> void{
            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || is_unit_highlighted( enemy->network_id ) || enemy->is_dead( ) || enemy->is_invisible( )
                    || enemy->dist_to_local( ) > 1500.f )
                    continue;

                const auto is_poisoned =
                    !!g_features->buff_cache->get_buff( enemy->index, ct_hash( "cassiopeiaqdebuff" ) ) ||
                    !!g_features->buff_cache->get_buff( enemy->index, ct_hash( "cassiopeiawpoison" ) );
                if ( !is_poisoned ) continue;

                highlight_instance_t inst{ enemy->index, enemy->network_id, 0.f, *g_time, 0.f, false };
                m_highlights.push_back( inst );
            }

            for ( auto& inst : m_highlights ) {
                const auto unit = g_entity_list.get_by_index( inst.index );
                if ( !unit || unit->is_dead( ) || unit->is_invisible( ) ) {
                    if ( inst.is_glowing && g_function_caller->is_glow_queueable( ) ) {
                        g_function_caller->enable_glow(
                            inst.network_id,
                            D3DCOLOR_ARGB( 255, 145, 100, 255 ),
                            3,
                            4,
                            0,
                            true
                        );

                        inst.is_glowing = false;
                    } else if ( inst.is_glowing ) continue;

                    remove_highlight( inst.network_id );
                    continue;
                }

                const auto is_poisoned =
                    !!g_features->buff_cache->get_buff( inst.index, ct_hash( "cassiopeiaqdebuff" ) ) ||
                    !!g_features->buff_cache->get_buff( inst.index, ct_hash( "cassiopeiawpoison" ) );
                if ( !is_poisoned ) {
                    if ( inst.is_glowing && g_function_caller->is_glow_queueable( ) ) {
                        g_function_caller->enable_glow(
                            inst.network_id,
                            D3DCOLOR_ARGB( 255, 149, 66, 245 ),
                            3,
                            4,
                            0,
                            true
                        );

                        inst.is_glowing = false;
                    } else if ( inst.is_glowing ) continue;

                    remove_highlight( inst.network_id );
                    continue;
                }

                //float poison_expire_time{ };

                auto buff = g_features->buff_cache->get_buff( inst.index, ct_hash( "cassiopeiaqdebuff" ) );
                if ( buff && buff->buff_data->end_time > inst.poison_expire_time ) {
                    inst.poison_update_time = *g_time;
                    inst.poison_expire_time = buff->buff_data->end_time;
                }

                buff = g_features->buff_cache->get_buff( inst.index, ct_hash( "cassiopeiawpoison" ) );
                if ( buff && buff->buff_data->end_time > inst.miasma_expire_time ) {
                    inst.miasma_expire_time = buff->
                                              buff_data->end_time;
                }

                if ( inst.is_glowing || !g_function_caller->is_glow_queueable( ) ) continue;

                g_function_caller->enable_glow(
                    inst.network_id,
                    D3DCOLOR_ARGB( 255, 180, 90, 255 ),
                    3,
                    5,
                    1
                );

                inst.is_glowing = true;
            }
        }

        auto cast_tracking( ) -> void{
            if ( !m_q_active ) {
                auto sci = g_local->spell_book.get_spell_cast_info( );
                if ( !sci || sci->slot != 0 || sci->server_cast_time < *g_time || sci->is_autoattack ||
                    sci->is_special_attack )
                    return;

                m_q_active           = true;
                m_q_server_cast_time = sci->server_cast_time;
                m_q_expire_time      = sci->start_time + 0.65f;
                m_q_target_position  = sci->end_position;

                return;
            }

            if ( *g_time > m_q_server_cast_time ) m_q_active = false;
        }

        auto is_unit_poisoned( const int16_t index ) const -> bool{
            const auto unit = g_entity_list.get_by_index( index );
            if ( unit->is_dead( ) || unit->is_invisible( ) ) return false;

            const auto is_poisoned =
                !!g_features->buff_cache->get_buff( unit->index, ct_hash( "cassiopeiaqdebuff" ) ) ||
                !!g_features->buff_cache->get_buff( unit->index, ct_hash( "cassiopeiawpoison" ) );
            if ( !is_poisoned ) {
                if ( m_q_expire_time < *g_time ) return false;

                const auto pred = g_features->prediction->get_server_position( index );
                if ( pred.length( ) > 0.f ) {
                    if ( pred.dist_to( m_q_target_position ) <= 200.f && unit->position.dist_to( pred ) <=
                        200.f )
                        return true;

                    return false;
                }
            } else return true;

            return false;
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->ability_power( ) * 0.9f,
                    target->index,
                    false
                );
            case ESpellSlot::e:
            {
                auto damage = 48.f + 4.f * static_cast< float >( g_local->level ) + g_local->ability_power( ) * 0.1f;

                const auto is_poisoned = !!g_features->buff_cache->get_buff(
                        target->index,
                        ct_hash( "cassiopeiaqdebuff" )
                    ) ||
                    !!g_features->buff_cache->get_buff( target->index, ct_hash( "cassiopeiawpoison" ) );

                if ( is_poisoned ) damage += m_e_bonus_damage[ m_slot_e->level ] + g_local->ability_power( ) * 0.6f;

                if ( target->is_hero( ) && g_config->cassiopeia.calculate_coup_de_grace->get< bool >( ) && target->
                    health / target->max_health < 0.4f )
                    damage *= 1.08f;

                return helper::calculate_damage( damage, target->index, false );
            }
            case ESpellSlot::r:
                return helper::calculate_damage(
                    m_r_damage[ get_slot_r( )->level ] + g_local->ability_power( ) * 0.9f,
                    target->index,
                    false
                );
            default:
                return 0.f;
            }
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return 0.65f;
            case ESpellSlot::w:
                return 0.3f;
            case ESpellSlot::e:
                return 0.125f + g_local->position.dist_to( target->position ) / 2500.f + g_features->orbwalker->
                    get_ping( ) / 2.f;
            default:
                return 0.f;
            }

            return std::numeric_limits< float >::max( );
        }

        auto remove_highlight( const unsigned network_id ) -> void{
            if ( m_highlights.empty( ) ) return;

            const auto to_remove =
                std::ranges::remove_if(
                    m_highlights,
                    [ & ]( const highlight_instance_t& instance ) -> bool{ return instance.network_id == network_id; }
                );

            if ( to_remove.empty( ) ) return;

            m_highlights.erase( to_remove.begin( ), to_remove.end( ) );
        }

        auto is_unit_highlighted( const unsigned network_id ) const -> bool{
            for ( const auto inst : m_highlights ) if ( inst.network_id == network_id ) return true;

            return false;
        }

    private:
        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_r_time{ };
        float m_last_cast_time{ };

        // poison highlight
        std::vector< highlight_instance_t > m_highlights{ };

        // combat mode
        bool  m_in_combat{ };
        bool  m_threat_nearby{ };
        float m_threat_level{ };

        // q cast tracking
        bool  m_q_active{ };
        float m_q_server_cast_time{ };
        float m_q_expire_time{ };
        Vec3  m_q_target_position{ };

        std::array< float, 6 > m_q_damage       = { 0.f, 75.f, 110.f, 145.f, 180.f, 215.f };
        std::array< float, 6 > m_w_damage       = { 0.f, 60.f, 95.f, 130.f, 165.f, 200.f };
        std::array< float, 6 > m_e_bonus_damage = { 0.f, 20.f, 40.f, 60.f, 80.f, 100.f };
        std::array< float, 6 > m_r_damage       = { 0.f, 150.f, 250.f, 350.f };

        float m_q_range{ 850.f };
        float m_w_range{ 700.f };
        float m_e_range{ 700.f };
        float m_r_range{ 850.f };
    };
}
