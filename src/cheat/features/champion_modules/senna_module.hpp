#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"
#include "../utils/c_function_caller.hpp"

namespace features::champion_modules {
    class senna_module final : public IModule {
    public:
        virtual ~senna_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "senna_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Senna" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        struct senna_minion_t {
            int16_t  index{ };
            unsigned network_id{ };

            float spawn_time{ };

            int glow_state{ };

            float last_glow_time{ };
            bool  is_glowing{ };

            int16_t particle_index{};
        };

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "senna" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );
            const auto special    = navigation->add_section( _( "special" ) );
            const auto orbwalk    = navigation->add_section( _( "orbwalking" ) );

            q_settings->checkbox( _( "enable" ), g_config->senna.q_enabled );
            q_settings->checkbox( _( "use in harass" ), g_config->senna.q_harass );
            q_settings->checkbox( _( "heal allies" ), g_config->senna.q_heal_allies );
            q_settings->checkbox( _( "only if can reset aa" ), g_config->senna.q_aa_reset );
            q_settings->checkbox( _( "extend only if soul" ), g_config->senna.q_extend_for_soul_only );
            q_settings->select(
                _( "hitchance" ),
                g_config->senna.q_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            w_settings->checkbox( _( "enable" ), g_config->senna.w_enabled );
            w_settings->checkbox( _( "flee w" ), g_config->senna.w_flee );
            w_settings->checkbox( _( "antigapclose" ), g_config->senna.w_antigapclose );
            w_settings->checkbox( _( "auto interrupt (?)" ), g_config->senna.w_autointerrupt )->set_tooltip(
                _( "Automatically interrupts spells like Shen R, Fiddlesticks R, etc" )
            );
            w_settings->select(
                _( "hitchance" ),
                g_config->senna.w_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            e_settings->checkbox( _( "flee e" ), g_config->senna.e_flee );

            r_settings->checkbox( _( "r killsteal" ), g_config->senna.r_killsteal );
            r_settings->select(
                _( "killsteal hitchance" ),
                g_config->senna.r_killsteal_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );
            r_settings->checkbox( _( "recall ult (?)" ), g_config->senna.r_silent_baseult )->set_tooltip(
                _( "Will R killable targets who are recalling" )
            );
            r_settings->checkbox( _( "^ prefer delay (?)" ), g_config->senna.r_prefer_delay )->set_tooltip(
                _( "Prefer to wait until last possible time to ult, recommended" )
            );

            drawings->checkbox( _( "draw q range" ), g_config->senna.q_draw_range );
            drawings->checkbox( _( "draw w range" ), g_config->senna.w_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->senna.dont_draw_on_cooldown );
            drawings->checkbox( _( "draw soul timer" ), g_config->senna.draw_soul_timer );
            drawings->checkbox( _( "highlight souls" ), g_config->senna.highlight_souls );
            drawings->checkbox(_("souls per minute indicator"), g_config->senna.highlight_souls);

            orbwalk->checkbox( _( "collect souls in combo" ), g_config->senna.combo_collect_souls );

            special->checkbox( _( "w instant snare" ), g_config->senna.w_instant_snare );
            special->select(
                _( "snare hitchance" ),
                g_config->senna.w_instant_snare_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );
            special->checkbox( _( "use to help snare" ), g_config->senna.w_snare_use_q_allowed );
            special->slider_int( "max distance from minion", g_config->senna.w_max_snare_distance, 100, 275, 5 );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->senna.q_draw_range->get< bool >( ) &&
                !g_config->senna.w_draw_range->get< bool >( ) &&
                !g_config->senna.r_silent_baseult->get< bool >( ) &&
                !g_config->senna.draw_soul_timer->get< bool >( ) || g_local->is_dead( ) ) {
                m_baseult_active = false;
                return;
            }

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };


            if ( g_config->senna.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->senna.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 31, 88, 255, 255 ),
                        m_q_max_range,
                        Renderer::outline,
                        60,
                        2.f
                    );
                }
                // else if (false)
                // {
                //
                //     auto       info      = slot->get_spell_info();
                //     auto       data      = info->get_spell_data();
                //
                //     const auto mana_cost = data->get_mana_cost();
                //     if (!mana_cost.empty() && mana_cost.size() > slot->level - 1u)
                //     {
                //     }
                //     
                // }
            }

            if ( g_config->senna.w_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->senna.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 255, 200, 0, 255 ),
                        m_w_range,
                        Renderer::outline,
                        60,
                        2.f
                    );
                }
            }

            // simulate q hitbox
            /* auto cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;
            auto main_hitbox = sdk::math::Rectangle( g_local->position, g_local->position.extend( cursor, m_q_max_range ), 50.f ).to_polygon( 55 );

            auto direction = ( cursor - g_local->position ).normalize( );
            auto modified  = ( cursor - g_local->position ).rotated_raw( 90.f ).normalize( );

            auto offset_start = g_local->position.extend( g_local->position + modified, 65.f );
            auto offset_end   = offset_start.extend( offset_start + direction, m_q_max_range );

            auto first_edge = sdk::math::Rectangle( offset_start, offset_end, 0.f ).to_polygon( 50 );

            modified = ( cursor - g_local->position ).rotated_raw( -90.f ).normalize( );
            offset_start = g_local->position.extend( g_local->position + modified, 65.f );
            offset_end   = offset_start.extend( offset_start + direction, m_q_max_range );

            auto second_edge = sdk::math::Rectangle( offset_start, offset_end, 0.f ).to_polygon( 50 );

            g_render->polygon_3d( main_hitbox, Color::white( ), Renderer::outline, 3.f );
            g_render->polygon_3d( first_edge, Color::green( ), Renderer::outline, 2.f );
            g_render->polygon_3d( second_edge, Color::red( ), Renderer::outline, 2.f );*/

              Vec2 sp{};
            if (g_config->senna.souls_per_minute_indicator->get<bool>( ) && world_to_screen(g_local->position, sp))
            {
                sp.x += 40.f;
                sp.y -= 30.f;

              
                // sp.y += 16.f;

                 static auto senna_passive = path::join(
                    { directory_manager::get_resources_path(), "champions", "Senna", "spells", "Senna_passive.png" });

                const auto texture = g_render->load_texture_from_file(senna_passive.has_value() ? *senna_passive : "");

                int indicator_size = 20;

                float minutes_passed =  *g_time / 60.f;
                float souls_per_minute = static_cast<float>(current_souls) / minutes_passed;

                std::string count = std::to_string(souls_per_minute);
                count.resize(3);

                std::string text_mode{ count };

                auto text_size = g_render->get_text_size(text_mode, g_fonts->get_zabel(), indicator_size);

                //g_render->text_shadow(sp, Color(255, 255, 255), g_fonts->get_zabel_16px(), text1.c_str(), 16);


                if (texture) {

                    const Vec2 size     = { static_cast<float>(indicator_size), static_cast<float>(indicator_size) };
                    const Vec2 position = { sp.x, sp.y };

                    g_render->filled_box(position, { size.x + text_size.x + 4.f, size.y }, Color(0, 0, 0, 175));
                    g_render->image(position, size, texture);

                     g_render->text_shadow({ sp.x + size.x + 2.f, sp.y }, Color( 255, 255, 255 ), g_fonts->get_zabel(), text_mode.c_str(), indicator_size);
                }

                // auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "2065passivemovespeed" ) );
                /*if ( buff ) {

                    std::string texte{ "DURATION: " };
                    texte += std::to_string( buff->buff_data->end_time - *g_time );


                    size = g_render->get_text_size( texte, g_fonts->get_zabel_12px( ), 12 );
                    g_render->text_shadow( { sp.x, sp.y + size.y },
                                           Color( 255, 200, 255 ),
                                           g_fonts->get_zabel_12px( ),
                                           texte.c_str( ),
                                           12 );
                }*/
            }

            if ( g_config->senna.draw_soul_timer->get< bool >( ) ) {
                for ( const auto soul : m_souls ) {
                    auto object = g_entity_list.get_by_index( soul.index );
                    if ( !object || object->is_dead( ) ) continue;

                    object.update( );

                    if ( object->dist_to_local( ) > g_local->attack_range + g_local->get_bounding_radius( ) ) {

                        auto soul_position = object->position;
                        soul_position.y    = g_local->position.y;

                         g_render->line_3d(g_local->position.extend(
                                              soul_position, g_local->attack_range + g_local->get_bounding_radius()),
                                          soul_position.extend(g_local->position, 30.f),
                                          g_features->tracker->get_rainbow_color(), 4.f);

                        g_render->circle_3d(soul_position, g_features->tracker->get_rainbow_color(), 30.f,
                                             Renderer::outline, -1, 3.f);

                    }

                    Vec2 sp{ };
                    if ( !world_to_screen( object->position, sp ) ) continue;

                    auto time_left = 8.f - ( *g_time - soul.spawn_time );
                    if ( time_left <= 0.f ) time_left = 0.f;

                    auto text = std::to_string( time_left );
                    text.resize( 3 );

                    const auto text_size = g_render->get_text_size( text, g_fonts->get_zabel( ), 30 );
                    const Vec2 text_pos{ sp.x - text_size.x / 2.f, sp.y - text_size.y / 2.f };
                    Color      text_color{ 255, 255, 255 };

                    if ( time_left < 2.f ) text_color = Color( 255, 255, 25 );

                    //g_render->filled_box({ text_pos.x, text_pos.y }, text_size, Color(0, 0, 0, 200), -1);
                    g_render->text_shadow(text_pos, text_color, g_fonts->get_zabel(), text.c_str(), 30);
                }
            }
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return;

            antigapclose_w( );
            autointerrupt_w( );
            spell_r( );

            killsteal_q_direct( );
            killsteal_q_indirect( );

            if ( g_features->orbwalker->in_attack( ) //|| g_features->orbwalker->is_autospacing( )
                ) return;

            if ( g_features->orbwalker->get_mode( ) != Orbwalker::EOrbwalkerMode::flee ) instant_snare_w( );
            help_snare_q( );
            recall_ult( );


            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_q( );
                spell_w( );
                heal_q( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->senna.q_harass->get< bool >( ) ) spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::flee:
                if ( g_config->senna.w_flee->get< bool >( ) ) spell_w( );

                spell_e( );
                break;
            default:
                break;
            }


            const auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "SennaPassiveStacks" ) );
            if (buff) {

                float value = buff->buff_data->get_senna_passive_attack_damage();
                int stacks = static_cast<int>( std::floor(value / 0.75f) );

                current_souls = stacks;
                m_q_range     = 600.f + 20.f * static_cast<float>(std::floor(stacks / 20));
            }
            for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
                if ( !minion || minion->is_dead( ) || minion->dist_to_local( ) > 2100.f || !minion->is_senna_minion( )
                    || is_soul_tracked( minion->network_id ) )
                    continue;

                senna_minion_t inst{ minion->index, minion->network_id, *g_time };

                m_souls.push_back( inst );
            }

            for ( auto& soul : m_souls ) {
                const auto unit = g_entity_list.get_by_index( soul.index );
                if ( !unit || unit->is_dead( ) ) {
                    remove_soul( soul.network_id );
                    continue;
                }

                if ( !g_config->senna.highlight_souls->get< bool >( ) || soul.glow_state == 3 ) continue;

                if ( g_function_caller->is_glow_queueable( ) && *g_time - soul.last_glow_time > 0.05f ) {


                    // zeus
                    /*switch (soul.glow_state)
                    {
                    case 0:
                        g_function_caller->enable_glow(unit->network_id, D3DCOLOR_ARGB(255, 0, 82, 255), 0, 6, 30);

                        soul.glow_state = 1;
                        break;
                    case 1:
                        g_function_caller->enable_glow(unit->network_id, D3DCOLOR_ARGB(255, 0, 255, 255), 1, 4, 4);

                        soul.glow_state = 2;
                        break;
                    case 2:
                        g_function_caller->enable_glow(unit->network_id, D3DCOLOR_ARGB(255, 255, 255, 255), 2, 4, 0);

                        soul.glow_state = 3;
                        break;
                    default:
                        break;
                    }*/

                    // divine
                    /*switch (soul.glow_state)
                    {
                    case 0:
                        g_function_caller->enable_glow(unit->network_id, D3DCOLOR_ARGB(255, 213, 170, 0), 0, 10, 16);

                        soul.glow_state = 1;
                        break;
                    case 1:
                        g_function_caller->enable_glow(unit->network_id, D3DCOLOR_ARGB(255, 255, 255, 255), 1, 4, 1);

                        soul.glow_state = 2;
                        break;
                    case 2:
                        g_function_caller->enable_glow(unit->network_id, D3DCOLOR_ARGB(255, 255, 225, 0), 2, 3, 1);

                        soul.glow_state = 3;
                        break;
                    }*/

                    // zeri
                    /*switch (soul.glow_state)
                    {
                    case 0:
                        g_function_caller->enable_glow(unit->network_id, D3DCOLOR_ARGB(255, 0, 0, 0), 0, 6, 8);

                        soul.glow_state = 1;
                        break;
                    case 1:
                        g_function_caller->enable_glow(unit->network_id, D3DCOLOR_ARGB(255, 0, 232, 10), 1, 6, 4);

                        soul.glow_state = 2;
                        break;
                    case 2:
                        g_function_caller->enable_glow(unit->network_id, D3DCOLOR_ARGB(255, 215, 255, 120), 2, 6, 0);

                        soul.glow_state = 3;
                        break;
                    }*/

                    const auto rainbow    = g_features->tracker->get_rainbow_color( );
                    const auto glow_color = D3DCOLOR_ARGB( 255, rainbow.r, rainbow.g, rainbow.b );
                    g_function_caller->enable_glow( soul.network_id, glow_color, 0, 6, 2 );

                    soul.is_glowing     = true;
                    soul.last_glow_time = *g_time;
                }
            }

            for (auto particle : g_entity_list.get_enemy_uncategorized()) {
                if (!particle || is_particle_ignored(particle->network_id) || particle->dist_to_local() > 2500.f) continue;

                auto name = particle->get_alternative_name();
                if (name.find("Senna_Base_P_Soul_Spawn") == std::string::npos)
                {
                    m_ignored_particles.push_back(particle->network_id);
                    continue;
                }

                register_soul_particle(particle->get_particle_source_index(), particle->index,
                                       particle->get_particle_spawn_time());

                m_ignored_particles.push_back(particle->network_id);
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->senna.q_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.025f || *g_time -
                m_last_q_time <= 0.3f || !can_cast_q( ) )
                return false;

            for ( auto i = 0; i < 2; i++ ) {
                if ( g_features->target_selector->is_forced( ) && i > 0 ) break;

                const auto target = g_features->target_selector->get_default_target( i > 0 );
                if ( !target ) break;

                if ( direct_q( target ) || indirect_q( target ) ) return true;
            }

            return false;
        }

        auto direct_q( Object* target ) -> bool{
            if ( !target || !g_features->orbwalker->is_attackable( target->index ) ) return false;

            if ( g_config->senna.q_aa_reset->get< bool >( ) && target->is_hero( ) && target->is_enemy( ) && !g_features
                ->orbwalker->should_reset_aa( ) && g_features->orbwalker->can_attack( target->index ) )
                return false;

            if ( cast_spell( ESpellSlot::q, target->network_id ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Senna: Q ] Direct Q at " << target->champion_name.text << std::endl;
                return true;
            }

            return false;
        }

        auto indirect_q( Object* target ) -> bool{
            if ( !target ) return false;

            const auto cast_target = get_indirect_q_target( target );
            if ( !cast_target || cast_target->network_id == target->network_id ) return false;

            if ( g_config->senna.q_extend_for_soul_only->get< bool >( ) && g_features->orbwalker->get_mode( ) ==
                Orbwalker::EOrbwalkerMode::harass &&
                target->network_id != cast_target->network_id && !g_features->buff_cache->get_buff(
                    target->index,
                    ct_hash( "sennapassivemarker" )
                ) )
                return false;


            if ( cast_spell( ESpellSlot::q, cast_target->network_id ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "Damage: " << std::dec << get_spell_damage( ESpellSlot::q, target ) << std::endl;

                std::cout << "[ Senna: Q ] Indirect at " << target->champion_name.text << " via " << cast_target->
                    get_name( ) << " | " << *g_time << std::endl;
                return true;
            }

            return false;
        }

        auto heal_q( ) -> bool{
            if ( !g_config->senna.q_heal_allies->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready( true ) )
                return false;

            const auto orb_target = g_features->target_selector->get_default_target( );
            const auto in_combat{ orb_target && g_features->orbwalker->is_attackable( orb_target->index ) };

            const auto    heal_health_threshold{ in_combat ? 0.15f : 0.35f };
            const Object* target{ };

            for ( const auto ally : g_entity_list.get_allies( ) ) {
                if ( !ally || ally->network_id == g_local->network_id ||
                    !g_features->orbwalker->is_attackable( ally->index ) || g_features->target_selector->is_bad_target(
                        ally->index
                    ) )
                    continue;

                const auto hp_percent = ally->health / ally->max_health;
                if ( hp_percent >= heal_health_threshold || target && target->health < ally->health ) continue;

                target = ally;
            }

            if ( !target ) return false;

            if ( cast_spell( ESpellSlot::q, target->network_id ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Senna: Q ] Heal " << target->champion_name.text << std::endl;
                return true;
            }

            return false;
        }

        auto killsteal_q_direct( ) -> bool{
            if ( *g_time - m_last_q_time <= 0.4f || *g_time - m_last_cast_time <= 0.05f || !m_slot_q->
                is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_killsteal_target(
                g_local->attack_range + 65.f + 50.f,
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                1
            );
            if ( !target || !g_features->orbwalker->is_attackable( target->index ) ) return false;

            if ( cast_spell( ESpellSlot::q, target->network_id ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                std::cout << "[ KS: Direct Q ] Order cast at " << target->champion_name.text << std::endl;
                return true;
            }

            return false;
        }

        auto killsteal_q_indirect( ) -> bool{
            if ( *g_time - m_last_q_time <= 0.4f || *g_time - m_last_cast_time <= 0.05f || !m_slot_q->
                is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_killsteal_target(
                m_q_max_range,
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                1
            );
            if ( !target ) return false;

            const auto cast_target = get_indirect_q_target( target );
            if ( !cast_target || cast_target->network_id == target->network_id ) return false;

            if ( cast_spell( ESpellSlot::q, cast_target->network_id ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                std::cout << "[ KS: Indirect Q ] Order cast at " << target->champion_name.text << " via " << cast_target
                    ->get_name( ) << std::endl;
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->senna.w_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto damage    = get_spell_damage( ESpellSlot::w, target );
            const auto will_kill = damage > target->health + target->total_health_regen;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_w_range,
                1200.f,
                70.f,
                0.25f,
                { },
                true,
                Prediction::include_ping,
                Prediction::ESpellType::linear
            );
            if ( !pred.valid ||
                pred.hitchance < static_cast< Prediction::EHitchance >( g_config->senna.w_hitchance->get< int >( ) ) ||
                g_features->prediction->minion_in_line_predicted(
                    g_local->position,
                    pred.position,
                    75.f,
                    0.25f,
                    1200.f
                ) )
                return false;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->senna.e_flee->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_e_time <= 0.8f || !m_slot_e->is_ready( true )
                || g_features->buff_cache->get_buff( g_local->index, ct_hash( "SennaE" ) ) )
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

        auto spell_r( ) -> bool override{
            if ( !g_config->senna.r_killsteal->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_r_time <= 0.4f || !m_slot_r->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_killsteal_target(
                25000.f,
                [this]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::r, unit ); },
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::r, unit ); },
                1
            );

            if ( !target || g_features->orbwalker->is_attackable( target->index ) ) return false;

            auto lowest_distance{ 12500.f };
            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || g_features->target_selector->is_bad_target( enemy->index ) || enemy->dist_to_local( ) >=
                    lowest_distance )
                    continue;

                lowest_distance = enemy->dist_to_local( );
            }

            if ( lowest_distance <= 1000.f ) return false;

            const auto predicted = g_features->prediction->predict(
                target->index,
                25000.f,
                20000.f,
                150.f,
                1.f,
                { },
                true,
                Prediction::include_ping,
                Prediction::ESpellType::linear
            );

            if ( !predicted.valid || predicted.hitchance < static_cast< Prediction::EHitchance >( g_config->senna.
                r_killsteal_hitchance->get< int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::r, predicted.position ) ) {
                m_last_r_time = *g_time;
                g_features->orbwalker->on_cast( );
                m_last_cast_time = *g_time;

                debug_log( "[ Senna ] R killsteal targeted at {}", target->champion_name.text );
                return true;
            }

            return false;
        }

        auto instant_snare_w( ) -> void{
            if ( !g_config->senna.w_instant_snare->get< bool >( ) || *g_time - m_last_w_time <= 0.5f || *g_time -
                m_last_cast_time <= 0.1f || !m_slot_w->is_ready( true ) )
                return;

            bool    found_opportunity{ };
            Vec3    cast_position{ };
            Object* cast_target{ };

            const auto can_q    = g_config->senna.w_snare_use_q_allowed->get< bool >( ) && m_slot_q->is_ready( true );
            const auto q_windup = g_features->orbwalker->get_attack_cast_delay( ) * 0.8f;
            bool       need_q{ };
            int16_t    q_target_index{ };

            for ( const auto target : g_entity_list.get_enemies( ) ) {
                if ( !target || target->dist_to_local( ) > m_w_range || g_features->target_selector->is_bad_target(
                    target->index
                ) )
                    continue;

                const auto tt = get_spell_travel_time( ESpellSlot::w, target );

                for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
                    if ( !minion || minion->is_dead( ) || minion->is_invisible( ) || minion->dist_to_local( ) >
                        m_w_range || !minion->is_lane_minion( ) )
                        continue;

                    auto aimgr = minion->get_ai_manager( );
                    if ( !aimgr || aimgr->is_moving ) continue;

                    const auto sci = minion->spell_book.get_spell_cast_info( );
                    if ( !sci ) continue;

                    const auto travel_time        = get_spell_travel_time( ESpellSlot::w, minion );
                    const auto damage             = get_spell_damage( ESpellSlot::w, minion );
                    const auto future_health      = g_features->prediction->predict_health( minion, travel_time );
                    const auto near_future_health = g_features->prediction->predict_health(
                        minion,
                        travel_time - 0.05f
                    );

                    if ( near_future_health <= 10.f || travel_time > tt || g_features->prediction->minion_in_line(
                        g_local->position,
                        minion->position,
                        70.f,
                        minion->network_id
                    ) )
                        continue;

                    if ( future_health > damage ) {
                        if ( can_q && future_health < damage + get_spell_damage( ESpellSlot::q, minion ) && g_features->
                            orbwalker->is_attackable( minion->index, m_q_range ) && travel_time > q_windup + g_features
                            ->orbwalker->get_ping( ) )
                            need_q = true;
                        else continue;
                    }

                    auto target_pred = g_features->prediction->predict(
                        target->index,
                        m_w_range,
                        0.f,
                        0.f,
                        travel_time
                    );
                    if ( !target_pred.valid || ( int )target_pred.hitchance < g_config->senna.w_instant_snare_hitchance
                        ->get< int >( )
                        || target_pred.position.dist_to( minion->position ) >= static_cast< float >( g_config->senna.
                            w_max_snare_distance->get< int >( ) ) ) {
                        need_q = false;
                        continue;
                    }

                    if ( need_q ) q_target_index = minion->index;

                    found_opportunity = true;
                    cast_position     = minion->position;
                    cast_target       = target;
                    break;
                }

                if ( found_opportunity ) break;
            }

            if ( !found_opportunity ) return;


            if ( cast_spell( ESpellSlot::w, cast_position ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                if ( need_q ) {
                    g_features->orbwalker->allow_attacks( false );
                    m_force_q        = true;
                    m_q_target_index = q_target_index;
                }

                debug_log( "[ SENNA ] Trying instant snare against {}", cast_target->champion_name.text );
            }
        }

        auto help_snare_q( ) -> void {
            if ( !m_force_q ) return;

            if ( *g_time - m_last_w_time > 0.4f || !m_slot_q->is_ready( true ) || g_features->orbwalker->get_mode( ) ==
                Orbwalker::EOrbwalkerMode::flee ) {
                debug_log( "helper q was too late, ready: {}", m_slot_q->is_ready( true ) );
                g_features->orbwalker->allow_attacks( true );
                m_force_q = false;
                return;
            }

            const auto& target = g_entity_list.get_by_index( m_q_target_index );
            if ( !target || target->is_dead( ) || target->is_invisible( ) || !g_features->orbwalker->is_attackable(
                m_q_target_index
            ) ) {
                g_features->orbwalker->allow_attacks( true );
                m_force_q = false;
                return;
            }

            if ( cast_spell( ESpellSlot::q, target->network_id ) ) {
                debug_log( "[ SENNA ] Casted helper q" );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                g_features->orbwalker->allow_attacks( true );
                m_force_q = false;
            }
        }

        auto get_target_q( ) -> Object*{
            auto target{ g_features->target_selector->get_default_target( ) };
            if ( !target ) return { };
            if ( g_features->orbwalker->is_attackable( target->index ) ) return target;

            auto cast_time = g_features->orbwalker->get_attack_cast_delay( ) * 0.8f;

            auto       pred_health = g_features->prediction->predict_health( target, cast_time );
            const auto damage      = get_spell_damage( ESpellSlot::q, target );
            const auto will_kill   = damage > pred_health;

            auto pred = g_features->prediction->predict( target->index, 1250.f, 0.f, 0.f, cast_time );
            if ( !pred.valid ) return { };

            auto bounding_radius{ 40 };

            for ( auto hero : g_entity_list.get_allies( ) ) {
                if ( !hero ||
                    g_local->network_id == hero->network_id ||
                    !g_features->orbwalker->is_attackable( hero->index ) ||
                    g_features->target_selector->is_bad_target( hero->index )
                )
                    continue;

                auto rect = sdk::math::Rectangle(
                    g_local->position,
                    g_local->position.extend( hero->position, m_q_max_range ),
                    50.f
                );
                auto polygon = rect.to_polygon( 20 );

                if ( polygon.is_inside( pred.position ) ) return hero;
            }

            if ( ( int )pred.hitchance == 0 ) return { };

            for ( auto hero : g_entity_list.get_enemies( ) ) {
                if ( !hero ||
                    target->network_id == hero->network_id ||
                    !g_features->orbwalker->is_attackable( hero->index ) ||
                    g_features->target_selector->is_bad_target( hero->index )
                )
                    continue;

                auto rect = sdk::math::Rectangle(
                    g_local->position,
                    g_local->position.extend( hero->position, m_q_max_range ),
                    50.f
                );
                auto polygon = rect.to_polygon( bounding_radius );

                if ( polygon.is_inside( pred.position ) ) return hero;
            }

            for ( auto minion : g_entity_list.get_enemy_minions( ) ) {
                if ( !minion ||
                    minion->is_dead( ) ||
                    minion->is_invisible( ) ||
                    !minion->is_lane_minion( ) &&
                    !minion->is_jungle_monster( ) &&
                    !minion->is_misc_minion( ) &&
                    !minion->is_senna_minion( ) &&
                    !minion->is_ward( ) ||
                    g_features->orbwalker->is_ignored( minion->index ) ||
                    !g_features->orbwalker->is_attackable( minion->index ) )
                    continue;

                auto rect = sdk::math::Rectangle(
                    g_local->position,
                    g_local->position.extend( minion->position, m_q_max_range ),
                    50.f
                );
                auto polygon = rect.to_polygon( bounding_radius );

                if ( polygon.is_inside( pred.position ) ) return minion;
            }

            for ( auto minion : g_entity_list.get_ally_minions( ) ) {
                if ( !minion ||
                    minion->is_dead( ) ||
                    minion->is_invisible( ) ||
                    !minion->is_lane_minion( ) &&
                    !minion->is_jungle_monster( ) &&
                    !minion->is_misc_minion( ) ||
                    g_features->orbwalker->is_ignored( minion->index ) ||
                    !g_features->orbwalker->is_attackable( minion->index )
                )
                    continue;

                auto rect = sdk::math::Rectangle(
                    g_local->position,
                    g_local->position.extend( minion->position, m_q_max_range ),
                    50.f
                );
                auto polygon = rect.to_polygon( bounding_radius );

                if ( polygon.is_inside( pred.position ) ) return minion;
            }

            return { };
        }

        auto get_indirect_q_target( Object* target ) -> Object*{
            if ( !target ) return { };

            if ( g_features->orbwalker->is_attackable( target->index ) ) return target;

            auto local_position = g_local->position;
            auto delay = g_features->orbwalker->get_attack_cast_delay( ) * 0.8f;

            auto aimgr = g_local->get_ai_manager();
            if (aimgr) local_position += aimgr->velocity * 0.5f;

            auto pred = g_features->prediction->predict( target->index, m_q_max_range, 0.f, 50.f, delay, { }, true );
            if ( !pred.valid ) return { };

            const auto hitchance        = pred.hitchance;
            const auto is_bad_hitchance = hitchance <= Prediction::EHitchance::high;
            const auto bounding_radius  = target->get_bounding_radius( );
            const auto hitbox_size{ 50.f };

            for ( auto ally : g_entity_list.get_allies( ) ) {
                if ( !ally || ally->network_id == g_local->network_id || ally->dist_to_local( ) < 125.f ||
                    g_features->target_selector->is_bad_target( ally->index ) ||
                    !g_features->orbwalker->is_attackable( ally->index ) )
                    continue;

                auto       ally_position        = ally->position;
                const auto ally_server_position = g_features->prediction->get_server_position( ally->index );
                if ( ally_server_position.length( ) > 0.f ) ally_position = ally_server_position;

                if ( is_bad_hitchance ) {
                    auto direction = ( ally_position - local_position ).normalize( );
                    auto modified  = ( ally_position - local_position ).rotated_raw( 90.f ).normalize( );

                    auto offset_start = local_position.extend( local_position + modified, hitbox_size );
                    auto offset_end   = offset_start.extend( offset_start + direction, m_q_max_range );

                    auto first_edge = sdk::math::Rectangle( offset_start, offset_end, 0.f )
                        .to_polygon( static_cast< int >( hitbox_size ) );

                    modified     = ( ally_position - local_position ).rotated_raw( -90.f ).normalize( );
                    offset_start = local_position.extend( local_position + modified, hitbox_size );
                    offset_end   = offset_start.extend( offset_start + direction, m_q_max_range );

                    auto second_edge = sdk::math::Rectangle( offset_start, offset_end, 0.f )
                        .to_polygon( static_cast< int >( hitbox_size ) );


                    if ( ( first_edge.is_inside( target->position ) || first_edge.is_inside( pred.position ) ) &&
                        second_edge.is_inside( pred.default_position ) ||
                        ( second_edge.is_inside( target->position ) || second_edge.is_inside( pred.position ) ) &&
                        first_edge.is_inside( pred.default_position ) )
                        return ally;

                    continue;
                }

                auto main_hitbox = sdk::math::Rectangle(
                        local_position,
                        local_position.extend( ally_position, m_q_max_range ),
                        hitbox_size
                    )
                    .to_polygon( static_cast< int >( hitbox_size ) );

                if ( main_hitbox.is_inside( pred.default_position ) ) return ally;
            }

            for ( auto minion : g_entity_list.get_ally_minions( ) ) {
                if ( !minion || minion->is_dead( ) || minion->is_invisible( ) || minion->dist_to_local( ) < 100.f ||
                    !minion->is_lane_minion( ) && !minion->is_jungle_monster( ) && !minion->is_misc_minion( ) ||
                    minion->is_minion_only_autoattackable( ) && ( !minion->is_ward( )
                        || minion->get_ward_type( ) != Object::EWardType::normal &&
                        minion->get_ward_type( ) != Object::EWardType::control ) ||
                    g_features->orbwalker->is_ignored( minion->index ) ||
                    !g_features->orbwalker->is_attackable( minion->index ) )
                    continue;

                if ( is_bad_hitchance ) {
                    auto direction = ( minion->position - local_position ).normalize( );
                    auto modified  = ( minion->position - local_position ).rotated_raw( 90.f ).normalize( );

                    auto offset_start = local_position.extend( local_position + modified, hitbox_size );
                    auto offset_end   = offset_start.extend( offset_start + direction, m_q_max_range );

                    auto first_edge = sdk::math::Rectangle( offset_start, offset_end, 0.f )
                        .to_polygon( static_cast< int >( hitbox_size ) );

                    modified     = ( minion->position - local_position ).rotated_raw( -90.f ).normalize( );
                    offset_start = local_position.extend( local_position + modified, hitbox_size );
                    offset_end   = offset_start.extend( offset_start + direction, m_q_max_range );

                    auto second_edge = sdk::math::Rectangle( offset_start, offset_end, 0.f )
                        .to_polygon( static_cast< int >( hitbox_size ) );


                    edge2_poly = second_edge;
                    edge1_poly = first_edge;

                    if ( ( first_edge.is_inside( target->position ) || first_edge.is_inside( pred.position ) ) &&
                        second_edge.is_inside( pred.default_position ) ||
                        ( second_edge.is_inside( target->position ) || second_edge.is_inside( pred.position ) ) &&
                        first_edge.is_inside( pred.default_position ) )
                        return minion;

                    continue;
                }

                auto main_hitbox = sdk::math::Rectangle(
                        local_position,
                        local_position.extend( minion->position, m_q_max_range ),
                        hitbox_size
                    )
                    .to_polygon( static_cast< int >( hitbox_size ) );

                if ( main_hitbox.is_inside( pred.default_position ) ) return minion;
            }

            for ( auto minion : g_entity_list.get_enemy_minions( ) ) {
                if ( !minion || minion->is_dead( ) || minion->is_invisible( ) || minion->dist_to_local( ) < 100.f ||
                    !minion->is_lane_minion( ) && !minion->is_jungle_monster( ) && !minion->is_misc_minion( ) && !minion
                    ->is_senna_minion( ) ||
                    minion->is_minion_only_autoattackable( ) &&
                    ( !minion->is_ward( ) ||
                        minion->get_ward_type( ) != Object::EWardType::normal &&
                        minion->get_ward_type( ) != Object::EWardType::control ) ||
                    g_features->orbwalker->is_ignored( minion->index ) ||
                    !g_features->orbwalker->is_attackable( minion->index ) )
                    continue;

                if ( is_bad_hitchance ) {
                    auto direction = ( minion->position - local_position ).normalize( );
                    auto modified  = ( minion->position - local_position ).rotated_raw( 90.f ).normalize( );

                    auto offset_start = local_position.extend( local_position + modified, hitbox_size );
                    auto offset_end   = offset_start.extend( offset_start + direction, m_q_max_range );

                    auto first_edge = sdk::math::Rectangle( offset_start, offset_end, 0.f )
                        .to_polygon( static_cast< int >( hitbox_size ) );

                    modified     = ( minion->position - local_position ).rotated_raw( -90.f ).normalize( );
                    offset_start = local_position.extend( local_position + modified, hitbox_size );
                    offset_end   = offset_start.extend( offset_start + direction, m_q_max_range );

                    auto second_edge = sdk::math::Rectangle( offset_start, offset_end, 0.f )
                        .to_polygon( static_cast< int >( hitbox_size ) );


                    if ( ( first_edge.is_inside( target->position ) || first_edge.is_inside( pred.position ) ) &&
                        second_edge.is_inside( pred.default_position ) ||
                        ( second_edge.is_inside( target->position ) || second_edge.is_inside( pred.position ) ) &&
                        first_edge.is_inside( pred.default_position ) )
                        return minion;

                    continue;
                }

                auto main_hitbox = sdk::math::Rectangle(
                        local_position,
                        local_position.extend( minion->position, m_q_max_range ),
                        hitbox_size
                    )
                    .to_polygon( static_cast< int >( hitbox_size ) );

                if ( main_hitbox.is_inside( pred.default_position ) ) return minion;
            }

            for ( auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->network_id == target->network_id ||
                    g_features->target_selector->is_bad_target( enemy->index ) ||
                    !g_features->orbwalker->is_attackable( enemy->index ) )
                    continue;

                auto       ally_position        = enemy->position;
                const auto ally_server_position = g_features->prediction->get_server_position( enemy->index );
                if ( ally_server_position.length( ) > 0.f ) ally_position = ally_server_position;

                if ( is_bad_hitchance ) {
                    auto direction = ( ally_position - local_position ).normalize( );
                    auto modified  = ( ally_position - local_position ).rotated_raw( 90.f ).normalize( );

                    auto offset_start = local_position.extend( local_position + modified, hitbox_size );
                    auto offset_end   = offset_start.extend( offset_start + direction, m_q_max_range );

                    auto first_edge = sdk::math::Rectangle( offset_start, offset_end, 0.f )
                        .to_polygon( static_cast< int >( hitbox_size ) );

                    modified     = ( ally_position - local_position ).rotated_raw( -90.f ).normalize( );
                    offset_start = local_position.extend( local_position + modified, hitbox_size );
                    offset_end   = offset_start.extend( offset_start + direction, m_q_max_range );

                    auto second_edge = sdk::math::Rectangle( offset_start, offset_end, 0.f )
                        .to_polygon( static_cast< int >( hitbox_size ) );


                    if ( ( first_edge.is_inside( target->position ) || first_edge.is_inside( pred.position ) ) &&
                        second_edge.is_inside( pred.default_position ) ||
                        ( second_edge.is_inside( target->position ) || second_edge.is_inside( pred.position ) ) &&
                        first_edge.is_inside( pred.default_position ) )
                        return enemy;

                    continue;
                }

                auto main_hitbox = sdk::math::Rectangle(
                        local_position,
                        local_position.extend( ally_position, m_q_max_range ),
                        hitbox_size
                    )
                    .to_polygon( static_cast< int >( hitbox_size ) );

                if ( main_hitbox.is_inside( pred.default_position ) ) return enemy;
            }

            return { };
        }

        auto antigapclose_w( ) -> void{
            if ( !g_config->senna.w_antigapclose->get< bool >( ) || *g_time - m_last_cast_time <= 0.025f || *g_time -
                m_last_w_time <= 0.4f || !m_slot_w->is_ready( true ) )
                return;

            const auto target = get_advanced_antigapclose_target( m_w_range, 1200.f, 70.f, 0.25f, true );
            if ( !target ) return;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_w_range,
                1200.f,
                70.f,
                0.25f,
                { },
                true,
                Prediction::include_ping,
                Prediction::ESpellType::linear
            );

            if ( !pred.valid || ( int )pred.hitchance < 3
                || g_features->prediction->minion_in_line_predicted(
                    g_local->position,
                    pred.position,
                    75.f,
                    0.25f,
                    1200.f
                ) )
                return;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_cast_time = *g_time;
                m_last_w_time    = *g_time;

                std::cout << "[ Senna: W ] Antigapclose against " << target->champion_name.text << std::endl;
            }
        }

        auto autointerrupt_w( ) -> void{
            if ( !g_config->senna.w_autointerrupt->get< bool >( ) || *g_time - m_last_cast_time <= 0.025f ||
                *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready( true ) )
                return;

            const auto target = get_interruptable_target( m_w_range );
            if ( !target ) return;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_w_range,
                1200.f,
                70.f,
                0.25f,
                { },
                true,
                Prediction::include_ping,
                Prediction::ESpellType::linear
            );

            if ( !pred.valid ||
                ( int )pred.hitchance < 3 || g_features->prediction->minion_in_line_predicted(
                    g_local->position,
                    pred.position,
                    75.f,
                    0.25f,
                    1200.f
                ) )
                return;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_cast_time = *g_time;
                m_last_w_time    = *g_time;

                std::cout << "[ Senna: W ] Auto-interrupt against " << target->champion_name.text << std::endl;
            }
        }

        auto recall_ult( ) -> void{
            if ( !g_config->senna.r_silent_baseult->get< bool >( ) || *g_time - m_last_r_time <= 0.5f || !m_slot_r->
                is_ready( true ) ) {
                m_baseult_active = false;
                return;
            }

            if ( m_baseult_active ) {
                base_ult_tracking( );
                return;
            }

            Object* target{ };
            bool    found_target{ };

            for ( auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->is_dead( ) || !enemy->is_recalling( ) ) continue;

                auto recall = g_features->tracker->get_recall( enemy->index );
                if ( !recall ) continue;

                auto recall_time_left = recall->finish_time - *g_time;
                auto travel_time      = 1.f + g_local->position.dist_to( enemy->position ) / 20000.f;
                if ( travel_time >= recall_time_left ) continue;

                float health_regenerated{ };

                if ( enemy->is_invisible( ) ) {
                    auto last_seen_data = g_features->tracker->get_last_seen_data( enemy->index );
                    if ( !last_seen_data ) continue;

                    const auto time_reduction = last_seen_data->total_recall_time + last_seen_data->time_in_recall;
                    const auto time_missing   = *g_time - last_seen_data->last_seen_time - time_reduction;
                    if ( time_missing * enemy->movement_speed > 160.f ) continue;

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
            m_baseult_active     = true;

            base_ult_tracking( );
        }

        auto base_ult_tracking( ) -> void{
            if ( !m_baseult_active || !m_slot_r->is_ready( true ) ) {
                m_baseult_active = false;
                return;
            }

            const auto& target = g_entity_list.get_by_index( m_target_index );
            if ( !target || !target->is_recalling( ) ) {
                m_baseult_active = false;
                return;
            }

            const auto recall = g_features->tracker->get_recall( target->index );
            if ( !recall ) {
                m_baseult_active = false;
                return;
            }

            const auto recall_time_left = recall->finish_time - *g_time;
            const auto travel_time      = 1.f + target->dist_to_local( ) / 20000.f;
            const auto damage           = get_spell_damage( ESpellSlot::r, target.get( ) );

            const auto time_until_hit = recall_time_left - travel_time;
            auto       health_regenerated{ std::ceil( time_until_hit ) * target->total_health_regen };
            auto       min_possible_health_regen = std::ceil( travel_time ) * target->total_health_regen;

            if ( target->is_invisible( ) ) {
                const auto last_seen_data = g_features->tracker->get_last_seen_data( target->index );
                if ( !last_seen_data ) {
                    m_baseult_active = false;
                    return;
                }

                const auto time_reduction = last_seen_data->total_recall_time + last_seen_data->time_in_recall;
                const auto time_missing   = *g_time - last_seen_data->last_seen_time - time_reduction;
                if ( time_missing * target->movement_speed > 160.f ) {
                    m_baseult_active = false;
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
                if ( g_config->senna.r_prefer_delay->get< bool >( ) ) {
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
                m_baseult_active = false;
                return;
            }

            if ( !should_cast ) return;

            if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo
                || g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::flee ) {
                m_baseult_active = false;
                return;
            }

            if ( cast_spell( ESpellSlot::r, target->position ) ) {
                m_baseult_active = false;
                m_last_r_time    = *g_time;
                g_features->orbwalker->on_cast( );
            }
        }

        auto is_soul_tracked( const unsigned network_id ) const -> bool{
            for ( const auto soul : m_souls ) if ( soul.network_id == network_id ) return true;

            return false;
        }

        auto is_particle_ignored( unsigned network_id ) -> bool
        {
            return std::any_of(m_ignored_particles.begin(), m_ignored_particles.end(),
                               [&network_id](unsigned &x) { return x == network_id; });

            for (const auto nid : m_ignored_particles)
                if (nid == network_id) return true;

            return false;
        }

        auto register_soul_particle(int16_t index, int16_t particle_index, float spawn_time ) -> void {

            for (auto& soul : m_souls) {
                if (soul.index != index) continue;


                std::cout << "Updated soul spawn-time, [ OLD: " << soul.spawn_time << " | NEW: " << spawn_time
                          << std::endl;

                soul.particle_index = particle_index;
                soul.spawn_time     = spawn_time;
                break;
            }
        }

        auto remove_soul( const unsigned network_id ) -> void{
            const auto to_remove = std::ranges::remove_if(
                m_souls,
                [&]( const senna_minion_t& inst ) -> bool{ return inst.network_id == network_id; }
            );

            if ( to_remove.empty( ) ) return;

            m_souls.erase( to_remove.begin( ), to_remove.end( ) );
        }

        static auto get_cutdown_damage( const float raw_damage, const float target_max_health ) -> float{
            if ( g_local->max_health >= target_max_health ) return 0.f;


            const auto modifier = std::clamp( target_max_health / g_local->max_health - 1.f, 0.f, 1.f );

            float damage_mod{ };
            auto  alt_damage_mod = 0.15f * modifier;

            if ( modifier >= 1.f ) damage_mod = 0.15f;
            else if ( modifier >= 0.85f ) damage_mod = 0.1333f;
            else if ( modifier >= 0.7f ) damage_mod = 0.1167f;
            else if ( modifier >= 0.55f ) damage_mod = 0.1f;
            else if ( modifier >= 0.4f ) damage_mod = 0.0833f;
            else if ( modifier >= 0.25f ) damage_mod = 0.0667f;
            else if ( modifier >= 0.1f ) damage_mod = 0.05f;
            else return 0.f;

            return raw_damage * damage_mod;
        }

        auto can_cast_q( ) -> bool{
            return m_slot_q->get_manacost( ) <= g_local->mana
                && ( m_slot_q->is_ready( ) || m_slot_q->cooldown_expire - *g_time <= g_features->orbwalker->
                    get_ping( ) );
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
            {
                auto raw_damage = m_q_damage[ m_slot_q->level ] + g_local->bonus_attack_damage( ) * 0.5f;
                //std::cout << "RAW DAMAGE: " << raw_damage << std::endl;

                float passive_damage{ };

                if ( target->is_hero( ) && g_features->buff_cache->get_buff(
                    target->index,
                    ct_hash( "sennapassivemarker" )
                ) ) {
                    const auto hp_modifier  = std::min( g_local->level * 0.01f, 0.1f );
                    const auto bonus_damage = target->health * hp_modifier;

                    passive_damage += bonus_damage;
                }

                passive_damage += g_local->attack_damage( ) * 0.2f;

                raw_damage += passive_damage;

                return helper::calculate_damage(
                    raw_damage + get_cutdown_damage( raw_damage, target->max_health ),
                    target->index,
                    true
                );
            }
            case ESpellSlot::w:
                return helper::calculate_damage(
                    m_w_damage[ m_slot_w->level ] + g_local->bonus_attack_damage( ) * 0.7f,
                    target->index,
                    true
                );
            case ESpellSlot::r:
            {
                const auto raw_damage = m_r_damage[ m_slot_r->level ] + g_local->bonus_attack_damage( ) * 1.15f +
                    g_local->ability_power( ) * 0.7f;

                return helper::calculate_damage(
                    raw_damage + get_cutdown_damage( raw_damage, target->max_health ),
                    target->index,
                    true
                );
            }
            }
            return 0.f;
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
            {
                return g_features->orbwalker->get_attack_cast_delay( ) * 0.8f + g_features->orbwalker->get_ping( );
            }
            case ESpellSlot::w:
            {
                const auto tt   = 0.25f + ( g_local->position.dist_to( target->position ) - 70.f ) / 1200.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.25f + ( g_local->position.dist_to( pred.value( ) ) - 70.f ) / 1200.f;
            }
            case ESpellSlot::r:
            {
                const auto tt   = 1.f + g_local->position.dist_to( target->position ) / 20000.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 1.f + g_local->position.dist_to( pred.value( ) ) / 20000.f;
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
        bool  m_passive_logged{ };

        // silent baseult
        bool    m_baseult_active{ };
        int16_t m_target_index{ };
        float   m_baseult_start_time{ };
        float   m_predicted_cast_time{ };

        // soul tracking
        std::vector< senna_minion_t > m_souls{ };
        int current_souls{};

        // w insta snare
        bool    m_force_q{ };
        int16_t m_q_target_index{ };

        std::vector<unsigned> m_ignored_particles{};

        // debug
        sdk::math::Polygon edge1_poly{ };
        sdk::math::Polygon edge2_poly{ };
        Vec3               poly_start{ };
        Vec3               poly_end{ };
        Vec3               poly2_start{ };
        Vec3               poly2_end{ };
        float              extra_size{ };

        std::vector< float > m_q_damage = { 0.f, 30.f, 65.f, 100.f, 135.f, 170.f };
        std::vector< float > m_w_damage = { 0.f, 70.f, 115.f, 160.f, 205.f, 250.f };
        std::vector< float > m_r_damage = { 0.f, 250.f, 400.f, 550.f };

        float m_q_range{ 600.f };
        float m_q_max_range{ 1300.f };

        float m_w_range{ 1300.f };
        float m_e_range{ 425.f };
    };
}
