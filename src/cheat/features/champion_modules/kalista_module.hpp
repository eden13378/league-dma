#pragma once

#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class kalista_module final : public IModule {
    public:
        virtual ~kalista_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "kalista_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Kalista" ); }

        struct minion_buff {
            Object*  minion{ };
            unsigned network_id{ };
        };

        struct enemy_damage_t {
            int32_t  index{ };
            unsigned network_id{ };

            float damage{ };
            int   damage_percent{ };
        };

        auto initialize( ) -> void override{ m_priority_list = { e_spell, q_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "kalista" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto orbwalker  = navigation->add_section( _( "orbwalker" ) );

            q_settings->checkbox( _( "enable" ), g_config->kalista.q_enabled );
            q_settings->select(
                _( "hitchance" ),
                g_config->kalista.q_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            e_settings->checkbox( _( "enable" ), g_config->kalista.e_enabled );
            e_settings->checkbox( _( "use if can reset cooldown" ), g_config->kalista.e_to_reset_cooldown );
            e_settings->checkbox( _( "has coup de grace rune" ), g_config->kalista.e_coup_de_grace );
            e_settings->checkbox( _( "spellclear" ), g_config->kalista.e_laneclear );

            drawings->checkbox( _( "draw q range" ), g_config->kalista.q_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->kalista.e_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->kalista.dont_draw_on_cooldown );
            drawings->checkbox( _( "draw e damage" ), g_config->kalista.e_draw_damage );

            orbwalker->checkbox( _( "jump exploit" ), g_config->kalista.jump_exploit );
            orbwalker->checkbox( _( "combo attack unit (?)" ), g_config->kalista.combo_attack_unit )->set_tooltip(
                _( "Orbwalker will attack any attackable unit to gapclose in combo" )
            );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->kalista.q_draw_range->get< bool >( ) &&
                !g_config->kalista.e_draw_range->get< bool >( ) &&
                !g_config->kalista.e_draw_damage->get< bool >( ) ||
                g_local->is_dead( )
            )
                return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->kalista.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->kalista.dont_draw_on_cooldown->
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

            if ( g_config->kalista.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->kalista.dont_draw_on_cooldown->
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

            /*auto sci = g_local->spell_book.get_spell_cast_info();
            if(sci)
            {
                std::string text = "atk: " + std::to_string( sci->end_time - *g_time );
                g_render->text_3d(g_local->position, color(52, 192, 235), g_fonts->get_bold(), text.c_str(), 20.f, true);
            }

            auto aimgr = g_local->get_ai_manager();
            auto path = aimgr->get_path();

            vec2 sp{ };
            if (!world_to_screen(g_local->position, sp)) return;

            std::string text = "IsDashing: " + std::to_string(aimgr->is_dashing);
            std::string text2 = "IsMoving: " + std::to_string(aimgr->is_moving);

            g_render->text_shadow({ sp.x, sp.y + 32.f }, color(255, 255, 255), g_fonts->get_bold(), text.c_str(), 20);
            g_render->text_shadow({ sp.x, sp.y + 52.f }, color(255, 255, 0), g_fonts->get_bold(), text2.c_str(), 20);

            if (aimgr->is_moving && path.size() > 1 && aimgr->next_path_node != 0 && path.size() != aimgr->next_path_node)
            {
                vec2 sp_next;

                if (path.size() > 1u)
                {
                    for (int i = aimgr->next_path_node; i < static_cast<int>(path.size()); i++) {
                        if (i == aimgr->next_path_node) {
                            if (!sdk::math::world_to_screen(g_local->position, sp) || !sdk::math::world_to_screen(path[i], sp_next)) break;
                        }
                        else {
                            if (!sdk::math::world_to_screen(path[i - 1], sp) || !sdk::math::world_to_screen(path[i], sp_next)) break;
                        }

                        //g_render->line({ sp.x + 1, sp.y + 1 }, { sp_next.x + 1, sp_next.y + 1 }, color(10, 10, 10, 255), 1.f);
                        g_render->line(sp, sp_next, color(25, 255, 25), 2.f);
                    }

                    if (sdk::math::world_to_screen(path[path.size() - 1], sp))
                    {
                        g_render->filled_circle(sp, color(255, 255, 255), 3.f, 15);
                        //g_render->text_shadow({ sp.x - 5, sp.y }, color(0, 145, 255, 255), g_fonts->get_bold(), "Local", 16);
                    }
                }
            }*/

            if ( !g_config->kalista.e_draw_damage->get< bool >( ) ) return;

            for ( const auto inst : m_enemy_damage ) {
                const auto& enemy = g_entity_list.get_by_index( inst.index );
                if ( !enemy ) continue;

                Vec2 sp{ };
                if ( !world_to_screen( enemy->position, sp ) ) continue;
                const auto bar_length = g_render_manager->get_width( ) * 0.0547f;
                const auto bar_height = static_cast< float >( g_render_manager->get_height( ) ) * 0.0222f;

                auto base_position = enemy->get_hpbar_position( );

                base_position.x -= bar_length * 0.44f;
                base_position.y -= bar_height;

                //g_render->filled_circle(base_position, color::white(), 1.f);
                const auto modifier   = enemy->health / enemy->max_health;
                const auto damage_mod = inst.damage / enemy->max_health;

                const Vec2 box_start{ base_position.x + bar_length * modifier, base_position.y };
                const Vec2 box_size{
                    damage_mod * bar_length > box_start.x - base_position.x
                        ? base_position.x - box_start.x
                        : -( bar_length * damage_mod ),
                    bar_height * 0.5f
                };

                g_render->filled_box( box_start, box_size, Color( 0, 140, 255, 160 ) );
            }
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_e( );
                spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                spell_e( );
                spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::laneclear:
                spell_clear_e( );
                break;
            default:
                break;
            }

            if ( !g_config->kalista.e_draw_damage->get< bool >( ) ) return;

            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) || is_enemy_logged( enemy->network_id ) ||
                    enemy->dist_to_local( ) > m_e_range * 1.25f )
                    continue;

                const auto damage = get_spell_damage( ESpellSlot::r, enemy );
                if ( damage == 0.f ) continue;

                const auto damage_percent = static_cast< int32_t >( std::min(
                    std::floor( damage / enemy->health * 100 ),
                    100.f
                ) );

                m_enemy_damage.push_back( { enemy->index, enemy->network_id, damage, damage_percent } );
            }

            for ( auto& inst : m_enemy_damage ) {
                auto& enemy = g_entity_list.get_by_index( inst.index );

                if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) || enemy->dist_to_local( ) > m_e_range *
                    1.25f ) {
                    remove_enemy( inst.network_id );
                    continue;
                }

                const auto damage = get_spell_damage( ESpellSlot::r, enemy.get( ) );
                if ( damage == 0.f ) {
                    remove_enemy( inst.network_id );
                    continue;
                }


                const auto damage_percent = static_cast< int >( std::min(
                    std::floor( damage / enemy->health * 100 ),
                    100.f
                ) );

                inst.damage         = damage;
                inst.damage_percent = damage_percent;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->kalista.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f
                || g_features->orbwalker->in_attack( ) || !m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            bool low_hitchance{ };
            if ( g_config->kalista.jump_exploit->get< bool >( ) && g_features->orbwalker->
                                                                               is_attackable( target->index ) ) {
                if ( g_features->orbwalker->get_attack_cast_delay( ) <= 0.23f ) return false;

                low_hitchance = g_features->orbwalker->should_reset_aa( );

                if ( !low_hitchance ) return false;
            }

            const auto predicted = g_features->prediction->predict(
                target->index,
                m_q_range,
                2400.f,
                40.f,
                0.25f
            );

            if ( !predicted.valid ||
                predicted.position.length( ) == 0.f ||
                predicted.hitchance < static_cast< Prediction::EHitchance >( g_config->kalista.q_hitchance->get<
                    int >( ) && !low_hitchance ) )
                return false;

            const auto raw_q_damage = m_q_damage[ m_slot_q->level ] + g_local->attack_damage( );

            if ( g_features->prediction->minion_in_line(
                g_local->position,
                predicted.position,
                40.f,
                0,
                raw_q_damage
            ) )
                return false;

            if ( cast_spell( ESpellSlot::q, predicted.position ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{ return false; }

        auto spell_e( ) -> bool override{
            if ( !g_config->kalista.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || m_slot_e->
                get_usable_state( ) == 1 || !m_slot_e->is_ready( true ) )
                return false;

            bool allow_cast{ };

            for ( const auto hero : g_entity_list.get_enemies( ) ) {
                if ( !hero ||
                    g_features->target_selector->is_bad_target( hero->index ) ||
                    g_local->position.dist_to( hero->position ) > m_e_range
                )
                    continue;

                const auto damage = get_spell_damage( ESpellSlot::e, hero );
                if ( damage == 0.f ) continue;

                const auto health = g_features->prediction->predict_health( hero, 0.25f );

                if ( damage >= health || g_config->kalista.e_to_reset_cooldown->get< bool >( ) && will_e_reset( ) ) {
                    allow_cast = true;
                    break;
                }
            }

            if ( !allow_cast ) return false;

            if ( cast_spell( ESpellSlot::e ) ) {
                m_last_e_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_clear_e( ) -> bool{
            if ( !g_config->kalista.e_laneclear->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || m_slot_e->
                get_usable_state( ) == 1
                || !m_slot_e->is_ready( true ) )
                return false;

            int        execute_count{ };
            const auto min_execute = m_slot_e->level >= 4 || GetAsyncKeyState( VK_CONTROL ) ? 1 : 2;

            for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
                if ( !minion ||
                    minion->is_dead( ) ||
                    minion->is_invisible( ) ||
                    minion->dist_to_local( ) > m_e_range ||
                    !minion->is_lane_minion( ) &&
                    !minion->is_jungle_monster( )
                )
                    continue;

                const auto damage = get_spell_damage( ESpellSlot::e, minion );
                if ( damage == 0.f ) continue;

                auto health = g_features->prediction->predict_health( minion, 0.2f );
                if ( minion->is_major_monster( ) ) health = minion->health;

                //don't ask me why but this change to the damage value fixes all of ren too early issues
                if ( damage >= health ) {
                    if ( minion->is_lane_minion( ) ) {
                        const auto type = minion->get_minion_type( );

                        if ( type == Object::EMinionType::super || type ==
                            Object::EMinionType::siege )
                            execute_count += 2;
                        else execute_count++;
                    } else {
                        if ( minion->get_monster_priority( ) > 1 ) execute_count += 2;
                        else execute_count++;
                    }
                }

                if ( execute_count > min_execute ) break;
            }

            if ( execute_count < min_execute ) return false;

            if ( cast_spell( ESpellSlot::e ) ) {
                m_last_e_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::e:
            {
                const auto buff = g_features->buff_cache->get_buff( target->index, ct_hash( "kalistaexpungemarker" ) );
                if ( !buff ) return 0.f;

                auto raw_damage = m_e_base_damage[ m_slot_e->level ] + g_local->attack_damage( ) * 0.7f;

                auto stacks = buff->stacks( ) - 1;

                if ( !target->is_major_monster( ) ) {
                    stacks += g_features->prediction->get_incoming_attack_count(
                        target->index
                    );
                }

                raw_damage += stacks * ( m_e_bonus_damage[ m_slot_e->level ] + g_local->attack_damage( ) *
                    m_e_ad_multiplier[ m_slot_e->level ] );

                if ( g_config->kalista.e_coup_de_grace->get< bool >( ) && target->is_hero( ) && target->health / target
                    ->max_health < 0.4f )
                    raw_damage *= 1.08f;
                else if ( target->is_major_monster( ) ) raw_damage *= 0.5f;

                return helper::calculate_damage( raw_damage, target->index, true );
            }
            case ESpellSlot::r:
            {
                const auto buff = g_features->buff_cache->get_buff( target->index, ct_hash( "kalistaexpungemarker" ) );
                if ( !buff ) return 0.f;

                auto raw_damage = m_e_base_damage[ m_slot_e->level ] + g_local->attack_damage( ) * 0.7f;

                const auto stacks = buff->stacks( ) - 1;

                raw_damage += stacks * ( m_e_bonus_damage[ m_slot_e->level ] + g_local->attack_damage( ) *
                    m_e_ad_multiplier[ m_slot_e->level ] );

                if ( g_config->kalista.e_coup_de_grace->get< bool >( ) && target->is_hero( ) && target->health / target
                    ->max_health < 0.4f )
                    raw_damage *= 1.08f;
                else if ( target->is_major_monster( ) ) raw_damage *= 0.5f;

                return helper::calculate_damage( raw_damage, target->index, true );
            }
            }

            return 0.f;
        }

        auto will_e_reset( ) -> bool{
            for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
                if ( !minion ||
                    minion->is_dead( ) ||
                    minion->is_invisible( ) ||
                    minion->health == minion->max_health ||
                    minion->dist_to_local( ) > m_e_range ||
                    !minion->is_lane_minion( ) &&
                    !minion->is_jungle_monster( )
                )
                    continue;

                const auto spell_damage = get_spell_damage( ESpellSlot::e, minion );

                if ( spell_damage <= minion->health ) continue;

                return true;
            }

            return false;
        }

        auto is_enemy_logged( const unsigned network_id ) const -> bool{
            for ( const auto& inst : m_enemy_damage ) if ( inst.network_id == network_id ) return true;

            return false;
        }

        auto remove_enemy( const unsigned network_id ) -> void{
            const auto to_remove = std::ranges::remove_if(
                m_enemy_damage,
                [&]( const enemy_damage_t& inst ) -> bool{ return inst.network_id == network_id; }
            );

            if ( to_remove.empty( ) ) return;

            m_enemy_damage.erase( to_remove.begin( ), to_remove.end( ) );
        }

    private:
        // damage calc
        std::vector< float > m_e_base_damage   = { 0.f, 20.f, 30.f, 40.f, 50.f, 60.f };
        std::vector< float > m_e_bonus_damage  = { 0.f, 10.f, 16.f, 22.f, 28.f, 34.f };
        std::vector< float > m_e_ad_multiplier = { 0.f, 0.232f, 0.2755f, 0.319f, 0.3625f, 0.406f };

        std::vector< float > m_q_damage = { 0.f, 20.f, 85.f, 150.f, 215.f, 280.f };

        float m_last_q_time{ };
        float m_last_e_time{ };

        // e special
        std::vector< minion_buff >    m_buffed_minions{ };
        std::vector< enemy_damage_t > m_enemy_damage{ };

        float m_q_range{ 1100.f };
        float m_e_range{ 1000.f };
    };
}
