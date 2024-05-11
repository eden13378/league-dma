#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../../sdk/game/render_manager.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class twitch_module final : public IModule {
    public:
        virtual ~twitch_module( ) = default;

        struct enemy_damage_t {
            int32_t  index{ };
            unsigned network_id{ };

            float damage{ };
            int   damage_percent{ };
        };

        auto get_name( ) -> hash_t override{ return ct_hash( "twitch_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Twitch" ); }

        auto initialize( ) -> void override{ m_priority_list = { e_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "twitch" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );

            q_settings->checkbox( _( "enable (?)" ), g_config->twitch.q_enabled )->set_tooltip(
                "will be used to increase attackspeed in combo"
            );

            w_settings->checkbox( _( "enable" ), g_config->twitch.w_enabled );
            w_settings->checkbox( _( "only if can reset aa" ), g_config->twitch.w_aa_reset );
            w_settings->select(
                _( "hitchance" ),
                g_config->twitch.w_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            e_settings->checkbox( _( "enable" ), g_config->twitch.e_enabled );
            //e_settings->checkbox( _( "predict damage" ), g_config->twitch.e_predict_damage );
            e_settings->select(_("damage prediction"), g_config->twitch.e_damage_prediction_mode,
                               { _("Off"), _("Poison damage"), _("All damage") });
            e_settings->checkbox( _( "has coup de grace rune" ), g_config->twitch.e_coup_de_grace_rune );
            e_settings->select( _( "prefer" ), g_config->twitch.e_mode, { _( "Killsteal" ), _( "Faster E" ) } );
            e_settings->checkbox( _( "spellclear e" ), g_config->twitch.e_spellclear );

            drawings->checkbox( _( "draw q duration" ), g_config->twitch.q_draw_time_left );
            drawings->checkbox( _( "draw e damage" ), g_config->twitch.e_draw_damage );
            drawings->checkbox( _( "draw r range" ), g_config->twitch.r_draw_range );
            drawings->checkbox( _( "draw r duration" ), g_config->twitch.r_draw_duration );
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) return;

            // const auto obj = g_entity_list->get_by_index( g_local->index );
            // if ( !obj ) return;
            // auto local = obj->create_copy( );
            // local.update( );
            g_local.update( );

            if ( g_config->twitch.r_draw_range->get< bool >( ) ) {
                auto slot = g_local->spell_book.get_spell_slot( ESpellSlot::r );
                if ( slot && slot->level > 0 && slot->is_ready( true ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 173, 47, 68, 255 ),
                        g_local->attack_range + 65.f + 300.f,
                        Renderer::outline,
                        70,
                        3.f
                    );
                }
            }


            Vec2 sp{ };
            if ( g_config->twitch.r_draw_duration->get< bool >( ) && m_r_active && m_r_end_time > *g_time &&
                world_to_screen( g_local->position, sp ) ) {
                Vec2 base_position = { sp.x + 50.f, sp.y - 35.f };

                auto modifier =
                    1.f - std::max( 1.f - ( m_r_end_time - *g_time ) / ( m_r_end_time - m_r_start_time ), 0.f );
                if ( modifier > 1.f ) modifier = 1.f;
                else if ( modifier < 0.f ) modifier = 0.f;

                std::string text = " ";
                auto        data = std::to_string( m_r_end_time - *g_time );
                data.resize( 3 );

                text += data + "s";

                auto size       = g_render->get_text_size( text, g_fonts->get_zabel_16px( ), 16 );
                auto dummy_size = g_render->get_text_size( _( " 3.5s" ), g_fonts->get_zabel_16px( ), 16 );

                g_render->text_shadow(
                    base_position,
                    Color( 255, 255, 255 ),
                    g_fonts->get_zabel_16px( ),
                    text.data( ),
                    16
                );

                float bar_width  = dummy_size.x + 5.f;
                auto  bar_height = 6.f;

                Vec2 bar_position = { base_position.x, base_position.y + size.y };
                Vec2 bar_size     = { bar_width, bar_height };

                Vec2 background_position = { bar_position.x - 1.f, bar_position.y - 1.f };
                Vec2 background_size     = { bar_size.x + 2.f, bar_size.y + 2.f };

                g_render->filled_box( background_position, background_size, Color( 10, 10, 10, 155 ) );


                Vec2 progress_size = Vec2{ bar_size.x * modifier, bar_size.y };

                g_render->filled_box( bar_position, progress_size, Color( 255, 255, 25 ) );
            }

            if ( g_config->twitch.e_draw_damage->get< bool >( ) ) {
                for ( const auto inst : m_enemy_damage ) {
                    auto enemy = g_entity_list.get_by_index( inst.index );
                    if ( !enemy ) continue;

                    enemy.update( );

                    if ( enemy->is_dead( ) || enemy->is_invisible( ) || !
                        world_to_screen( enemy->position, sp ) )
                        continue;

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

                    auto       damage      = inst.damage;
                    auto       modifier    = enemy->health / enemy->max_health;
                    auto       damage_mod  = damage / enemy->max_health;
                    const auto is_killable = damage > enemy->health;

                    Vec2 box_start{ base_position.x + bar_length * modifier + buffer, base_position.y - buffer };
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
                            ? g_features->orbwalker->animate_color(
                                Color( 255, 50, 75 ),
                                EAnimationType::pulse,
                                10
                            ).alpha( 180 )
                            : Color( 40, 150, 255, 180 )
                    );
                }
            }

            if ( m_q_expire_time == 0.f || m_q_expire_time < *g_time || !sdk::math::world_to_screen(
                g_local->position,
                sp
            ) )
                return;

            const auto time_left = m_q_expire_time - *g_time;
            const auto output    = fmt::format( "{:.1f}s", time_left );

            sp.x += 32;
            sp.y -= 32;

            if ( time_left < 1.f ) {
                g_render->text_shadow(
                    sp,
                    Color( 255, 0, 0, 255 ),
                    g_fonts->get_zabel( ),
                    output.c_str( ),
                    32
                );
            } else if ( time_left < 3.f ) {
                g_render->text_shadow(
                    sp,
                    Color( 255, 255, 0, 255 ),
                    g_fonts->get_zabel( ),
                    output.c_str( ),
                    32
                );
            } else g_render->text_shadow( sp, Color::white( ), g_fonts->get_zabel( ), output.c_str( ), 32 );
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            if ( g_config->twitch.q_draw_time_left->get< bool >( ) ) {
                const auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "TwitchHideInShadows" ) );
                if ( buff ) m_q_expire_time = buff->buff_data->end_time;
                else m_q_expire_time        = 0.f;
            }

            update_ult( );

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ||
                g_features->orbwalker->in_attack( ) )
                return;

            spell_e( );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_q( );
                spell_w( );
                break;
            case Orbwalker::EOrbwalkerMode::laneclear:
                spellclear_e( );
                break;
            default:
                break;
            }

            if ( !g_config->twitch.e_draw_damage->get< bool >( ) ) return;

            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) || is_enemy_logged( enemy->network_id ) ||
                    enemy->dist_to_local( ) > m_e_range * 1.25f )
                    continue;

                const auto damage = get_spell_damage( ESpellSlot::e, enemy );
                if ( damage == 0.f ) continue;

                //damage += get_passive_damage( enemy );

                auto damage_percent = static_cast< int32_t >( std::min(
                    std::floor( damage / enemy->health * 100 ),
                    100.f
                ) );
                if ( damage_percent > 999 ) damage_percent = 999;

                m_enemy_damage.push_back( { enemy->index, enemy->network_id, damage, damage_percent } );
            }

            for ( auto& inst : m_enemy_damage ) {
                auto& enemy = g_entity_list.get_by_index( inst.index );

                if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) || enemy->dist_to_local( ) > m_e_range *
                    1.25f ) {
                    remove_enemy( inst.network_id );
                    continue;
                }

                const auto damage = get_spell_damage( ESpellSlot::e, enemy.get( ) );
                if ( damage == 0.f ) {
                    remove_enemy( inst.network_id );
                    continue;
                }

                //damage += get_passive_damage( enemy.get( ) );

                auto damage_percent = static_cast< int32_t >( std::floor( damage / enemy->health * 100.f ) );
                if ( damage_percent > 999 ) damage_percent = 999;

                inst.damage         = damage;
                inst.damage_percent = damage_percent;
            }

            //c_module::run( );

            // E Speed: 3000
            // R Speed: 4000
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->twitch.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 1.5f || !m_slot_q->is_ready(
                    true
                )
                || m_slot_q->get_usable_state( ) == 1 )
                return false;

            const auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "twitchhideinshadowsbuff" ) );
            if ( buff && buff->buff_data->end_time - *g_time > 1.f ) return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || !g_features->orbwalker->is_attackable( target->index ) || !g_features->orbwalker->
                should_reset_aa( ) )
                return false;

            if ( cast_spell( ESpellSlot::q ) ) {
                m_last_q_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->twitch.w_enabled->get< bool >( ) ||
                *g_time - m_last_w_time <= .4f ||
                !m_slot_w->is_ready( true ) ||
                g_features->orbwalker->in_attack( ) ||
                g_local->mana - m_slot_w->get_manacost( ) < m_slot_e->get_manacost( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || g_config->twitch.w_aa_reset->get< bool >( ) && !g_features->orbwalker->should_reset_aa( )
                || g_features->buff_cache->get_buff( target->index, ct_hash( "TwitchDeadlyVenom" ) ) )
                return false;

            if ( g_features->orbwalker->is_attackable( target->index )
                && !g_features->orbwalker->should_reset_aa( ) && g_features->orbwalker->get_next_possible_aa_time( ) <=
                *g_time + 0.3f )
                return false;

            const auto predicted = g_features->prediction->predict( target->index, m_w_range, 1400.f, 0.f, 0.25f );

            if ( !predicted.valid ||
                ( !g_features->orbwalker->should_reset_aa( ) || !g_features->orbwalker->is_attackable( target->index ) )
                &&
                predicted.hitchance < static_cast< Prediction::EHitchance >( g_config->twitch.w_hitchance->get<
                    int >( ) )
            )
                return false;


            if ( cast_spell( ESpellSlot::w, predicted.position ) ) {
                m_last_w_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->twitch.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready(
                true
            ) )
                return false;

            bool allow_cast{ };
            bool can_killsteal{ };

            for ( const auto hero : g_entity_list.get_enemies( ) ) {
                if ( !hero || hero->dist_to_local( ) > m_e_range || g_features->target_selector->is_bad_target(
                    hero->index
                ) )
                    continue;

                auto damage = get_spell_damage( ESpellSlot::e, hero );
                if ( damage == 0.f ) continue;

                const auto travel_time = 0.25f + hero->dist_to_local( ) / 3000.f;
                const auto health      = helper::get_real_health(
                    hero->index,
                    EDamageType::physical_damage,
                    travel_time,
                    g_config->twitch.e_damage_prediction_mode->get<int>() == 2
                );

                if ( damage > health ) {
                    allow_cast    = true;
                    can_killsteal = true;
                    break;
                }

                if ( allow_cast || g_features->orbwalker->get_mode( ) != Orbwalker::EOrbwalkerMode::combo ) continue;

                if (g_config->twitch.e_damage_prediction_mode->get<int>() > 0 &&
                    (hero->dist_to_local() > 1000.f || g_config
                    ->twitch.e_mode->get< int >( ) == 1 ) ) {
                    damage += get_passive_damage( hero );

                    if ( damage > health ) {
                        can_killsteal = false;
                        allow_cast    = true;
                    }
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

        auto spellclear_e( ) -> void{
            if ( !g_config->twitch.e_spellclear->get< bool >( ) || *g_time - m_last_e_time <= 0.5f || !m_slot_e->
                is_ready( true ) )
                return;

            int  executable_count{ };
            bool has_jungle_monster{ };

            for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
                if ( !minion || minion->is_dead( ) || minion->is_invisible( ) || minion->dist_to_local( ) > m_e_range
                    || !minion->is_lane_minion( ) && !minion->is_jungle_monster( ) )
                    continue;

                const auto damage = get_spell_damage( ESpellSlot::e, minion );
                if ( damage <= minion->health ) continue;

                ++executable_count;
                if ( !has_jungle_monster ) has_jungle_monster = minion->is_jungle_monster( );
            }

            if ( executable_count <= 2 && !has_jungle_monster ) return;

            if ( cast_spell( ESpellSlot::e ) ) m_last_e_time = *g_time;
        }

        auto get_poison_stacks( int16_t index ) -> int {
            const auto buff = g_features->buff_cache->get_buff( index, ct_hash("TwitchDeadlyVenom"));
            if (!buff || buff->buff_data->end_time - *g_time <= 0.225f) return 0.f;

            auto stacks = buff->stacks();

            if (stacks < 6)
            {
                const auto incoming_attacks = g_features->prediction->get_incoming_attack_count(index);
                stacks                      = static_cast<int32_t>(fmin(stacks + incoming_attacks, 6));
            }

            return stacks;
        }

        auto spell_r( ) -> bool override{ return false; }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::e:
            {
                const auto buff = g_features->buff_cache->get_buff( target->index, ct_hash( "TwitchDeadlyVenom" ) );
                if ( !buff || buff->buff_data->end_time - *g_time <= 0.225f ) return 0.f;

                auto stacks = buff->stacks( );

                if ( stacks < 6 ) {
                    const auto incoming_attacks = g_features->prediction->get_incoming_attack_count( target->index );
                    stacks                      = static_cast< int32_t >( fmin( stacks + incoming_attacks, 6 ) );
                }

                auto raw_damage = m_e_base_damage[ m_slot_e->level ] + ( m_e_stack_damage[ m_slot_e->level ] + g_local
                    ->bonus_attack_damage( ) * 0.35f ) * stacks;
                auto ap_damage = g_local->ability_power( ) * 0.35f * stacks;

                if ( target->is_hero( ) && g_config->twitch.e_coup_de_grace_rune->get< bool >( ) && target->health /
                    target->max_health < 0.4f ) {
                    raw_damage *= 1.08f;
                    ap_damage *= 1.08f;
                }

                auto total_damage = helper::calculate_damage( std::floor( raw_damage ), target->index, true );
                total_damage += helper::calculate_damage( std::floor( ap_damage ), target->index, false );

                //std::cout << "phys: " << helper::calculate_damage(raw_damage, target->index, true) << std::endl;
                //std::cout << "ap: " << helper::calculate_damage(ap_damage, target->index, false) << std::endl;

                return total_damage;
            }
            default:
                break;
            }

            return 0.f;
        }

    private:
        static auto get_passive_damage( const Object* object ) -> float{
            const auto buff = g_features->buff_cache->get_buff( object->index, ct_hash( "TwitchDeadlyVenom" ) );
            if ( !buff || buff->buff_data->end_time - *g_time <= 0.225f ) return 0.f;

            int32_t stack_damage;

            if ( g_local->level < 5 ) stack_damage = 1;
            else if ( g_local->level < 9 ) stack_damage = 2;
            else if ( g_local->level < 13 ) stack_damage = 3;
            else if ( g_local->level < 17 ) stack_damage = 4;
            else stack_damage                            = 5;

            int32_t tick_count{ };

            for ( auto i = 1; i <= 6; i++ ) {
                if ( buff->buff_data->end_time - *g_time < static_cast< float >( i ) ) {
                    tick_count = i;
                    break;
                }
            }

            if ( tick_count > 0 && g_features->prediction->get_incoming_attack_count( object->index ) > 0 ) {
                tick_count =
                    6;
            }

            if ( tick_count <= 0 ) return 0.f;

            const auto damage_per_tick = stack_damage * buff->stacks( ) + g_local->ability_power( ) * ( 0.03f * buff->
                stacks( ) );

            return std::floor( damage_per_tick ) * tick_count - object->total_health_regen * tick_count;
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

        auto update_ult( ) -> void{
            if ( !m_r_active ) {
                const auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "TwitchFullAutomatic" ) );
                if ( buff ) {
                    m_r_active     = true;
                    m_r_start_time = buff->buff_data->start_time;
                    m_r_end_time   = buff->buff_data->end_time;
                }
            } else {
                const auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "TwitchFullAutomatic" ) );
                if ( buff && buff->buff_data->end_time > m_r_end_time ) {
                    m_r_start_time = buff->buff_data->start_time;
                    m_r_end_time   = buff->buff_data->end_time;
                }

                if ( *g_time > m_r_end_time || !buff ) m_r_active = false;
            }
        }

    private:
        // damage calc
        std::array< float, 6 > m_e_base_damage  = { 0.f, 20.f, 30.f, 40.f, 50.f, 60.f };
        std::array< float, 6 > m_e_stack_damage = { 0.f, 15.f, 20.f, 25.f, 30.f, 35.f };

        // r tracking
        bool  m_r_active{ };
        float m_r_start_time{ };
        float m_r_end_time{ };

        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_q_expire_time{ };

        // damage draw
        std::vector< enemy_damage_t > m_enemy_damage{ };

        float m_w_range{ 950.f };
        float m_e_range{ 1200.f };
    };
}
