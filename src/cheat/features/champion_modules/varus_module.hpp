#pragma once
#include "module.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"
#include "../evade.hpp"

namespace features::champion_modules {
    class varus_module final : public IModule {
    public:
        virtual ~varus_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "varus_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Varus" ); }

        auto initialize( ) -> void override{ m_priority_list = { w_spell, q_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "varus" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto spellclear = navigation->add_section( _( "spellclear" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );
            const auto misc       = navigation->add_section( _( "misc" ) );

            //q_settings->checkbox( _( "enable" ), g_config->varus.q_enabled );
            q_settings->checkbox( _( "enable (?)" ), g_config->varus.q_enabled )->set_tooltip(
                _( "AP Auto draw range will show when AP > bonus AD" )
            );
            q_settings->checkbox( _( "slow prediction" ), g_config->varus.q_slow_prediction );
            q_settings->select(
                _( "hitchance" ),
                g_config->varus.q_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );
            q_settings->slider_int( _( "AP auto threshold range" ), g_config->varus.q_ap_draw, 575, 1200, 25 );

            e_settings->checkbox( _( "enable" ), g_config->varus.e_enabled );
            e_settings->checkbox( _( "multihit" ), g_config->varus.e_multihit );
            e_settings->select(
                _( "hitchance" ),
                g_config->varus.e_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            r_settings->checkbox( _( "enable" ), g_config->varus.r_enabled );
            r_settings->checkbox( _( "only if can spread" ), g_config->varus.r_only_if_can_spread );
            r_settings->select(
                _( "hitchance" ),
                g_config->varus.r_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            misc->checkbox( _( "prefer e over q" ), g_config->varus.prefer_e_over_q );

            drawings->checkbox( _( "draw q range" ), g_config->varus.q_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->varus.e_draw_range );
            drawings->checkbox( _( "draw r range" ), g_config->varus.r_draw_range );

            spellclear->checkbox( _( "use e to laneclear" ), g_config->varus.e_spellclear );
            spellclear->slider_int( _( "minimum mana %" ), g_config->varus.spellclear_min_mana, 10, 90, 1 );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->varus.q_draw_range->get< bool >( ) &&
                !g_config->varus.e_draw_range->get< bool >( ) &&
                !g_config->varus.r_draw_range->get< bool >( ) ||
                g_local->is_dead( )
            )
                return;


            g_local.update( );

            if ( g_config->varus.q_draw_range->get< bool >( ) && g_local->spell_book.get_spell_slot( ESpellSlot::q )->
                                                                          level > 0 ) {
                g_render->circle_3d(
                    g_local->position,
                    Color( 31, 88, 255, 255 ),
                    m_q_range,
                    Renderer::outline,
                    80,
                    2.f
                );
            }

            if ( g_local->bonus_attack_damage( ) < g_local->ability_power( ) && g_config->varus.q_draw_range->get<
                    bool >( ) && g_config->varus.q_ap_draw_range->get< bool >( ) && g_local->spell_book.
                get_spell_slot( ESpellSlot::q )->level > 0 ) {
                g_render->circle_3d(
                    g_local->position,
                    Color( 31, 88, 255, 255 ),
                    static_cast< float >( g_config->varus.q_ap_draw->get< int >( ) ),
                    Renderer::outline,
                    80,
                    2.f
                );
            }

            if ( g_config->varus.e_draw_range->get< bool >( ) && g_local->spell_book.get_spell_slot( ESpellSlot::e )->
                                                                          level > 0 ) {
                g_render->circle_3d(
                    g_local->position,
                    Color( 31, 88, 255, 255 ),
                    m_e_range,
                    Renderer::outline,
                    80,
                    2.f
                );
            }

            if ( g_config->varus.r_draw_range->get< bool >( ) && g_local->spell_book.get_spell_slot( ESpellSlot::r )->
                                                                          level > 0 ) {
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

        auto run( ) -> void override{
            initialize_spell_slots( );
            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:
                m_target = CHolder::from_object( g_features->target_selector->get_default_target( ) );

                if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "VarusQLaunch" ) ) ) {
                    spell_q( );
                    return;
                } else if ( *g_time - m_q_cast_begin > 0.2f ) m_channeling_q = false;

                if ( spell_r( ) ) return;
            //if ( spell_e( ) ) return;
                if ( g_local->bonus_attack_damage( ) < g_local->ability_power( ) ) {
                    spell_w( );
                    spell_ap_e( );
                } else {
                    spell_q( );
                    spell_e( );
                }
                return;
            //if ( spell_q( ) ) return;
                break;
            case Orbwalker::EOrbwalkerMode::laneclear:
                if ( g_config->varus.e_spellclear->get< bool >( ) && laneclear_e( ) ) return;
                break;
            default: ;
            }
        }

    private:
        //VarusWDebuff
        auto spell_q( ) -> bool override{
            if ( !g_config->varus.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 1.f ) return false;

            const auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "VarusQLaunch" ) );

            if ( !buff ) {
                if ( *g_time - m_q_cast_begin <= 0.2f || !m_slot_q->is_ready( true ) ) return false;

                const auto target = g_features->target_selector->get_spell_specific_target( 1500.f );
                if ( !target ) return false;

                const auto pred = g_features->prediction->predict( target->index, 1500.f, 1900.f, 70.f, 0.f );

                if ( !pred.valid ) {
                    m_channeling_q = false;
                    return false;
                }

                if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                    g_features->orbwalker->set_cast_time( 0.025f );
                    m_q_cast_begin = *g_time;
                    m_channeling_q = true;
                    return true;
                }

                return false;
            }

            m_q_cast_begin = 0.f;
            m_channeling_q = true;

            auto bonus_range = static_cast< int32_t >( std::floor(
                ( *g_time - buff->buff_data->start_time ) / 0.25f
            ) * 140.f );

            if ( bonus_range > 700 ) bonus_range = 700;

            const auto target = g_features->target_selector->get_spell_specific_target(
                1500.f,
                [this]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); }
            );
            if ( !target ) return false;

            const auto can_kill = target->health + target->total_health_regen <= get_spell_damage(
                ESpellSlot::q,
                target
            );

            auto hitchance =
                !can_kill && g_config->varus.q_slow_prediction->get< bool >( ) &&
                bonus_range != 700 &&
                g_config->varus.q_hitchance->get< int >( ) < 3
                    ? 3
                    : g_config->varus.q_hitchance->get< int >( );
            const auto pred = g_features->prediction->predict(
                target->index,
                m_q_base_range + bonus_range,
                1900.f,
                70.f,
                0.f
            );

            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( hitchance ) ) return false;

            if ( release_chargeable( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->set_cast_time( 0.f );
                m_last_q_time  = *g_time;
                m_channeling_q = false;
                return true;
            }
            return false;
        }

        //AP combo is QW -> E -> QW
        auto spell_w( ) -> bool override{
            if ( !g_config->varus.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.2f ) return false;
            const auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "VarusQLaunch" ) );
            //const auto target = g_features->target_selector->get_default_target();
            const Object* target         = { };
            auto          highest_stacks = 0;
            auto          current_stacks = 0;
            float         lowest_hp      = 0;

            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || g_features->target_selector->is_bad_target( enemy->index ) || !enemy->is_alive( ) ||
                    !g_features->buff_cache->get_buff( enemy->index, ct_hash( "VarusWDebuff" ) ) )
                    continue;
                current_stacks = g_features->buff_cache->get_buff( enemy->index, ct_hash( "VarusWDebuff" ) )->stacks( );
                //check if there is anyone with any stacks that is killable before anything
                if ( enemy->health < get_stack_damage( enemy, current_stacks ) ) {
                    target = enemy;
                    break;
                }
                //kill check is done, now check and make sure that if anyone is within our range, that we make sure they get to 3 stacks and then we cast to them
                if ( enemy->dist_to_local( ) < g_config->varus.q_ap_draw->get< int >( ) )
                    if ( current_stacks ==
                        3.f )
                        target = enemy;
                //if no one is inside of our range stack threshold then find whoever has the highest stacks and cast to them
                if ( current_stacks > highest_stacks ) {
                    highest_stacks = current_stacks;
                    target         = enemy;
                } else if ( current_stacks == highest_stacks ) if ( target->health < enemy->health ) continue;
                target = enemy;
                break;
            }

            //we now have our target, now we need to cast
            if ( !buff ) {
                if ( *g_time - m_q_cast_begin <= 0.2f || !m_slot_q->is_ready( true ) ) return false;
                if ( !target ) return false;
                const auto pred = g_features->prediction->predict( target->index, 1500.f, 1900.f, 70.f, 0.f );

                if ( !pred.valid ) {
                    m_channeling_q = false;
                    return false;
                }
                if ( !m_slot_w->is_ready( ) || *g_time - m_last_w_time <= 0.2f ) return false;
                if ( m_slot_w->get_usable_state( ) != 1 ) {
                    if ( cast_spell( ESpellSlot::w ) ) {
                        //g_features->orbwalker->set_cast_time(0.f);
                        m_last_w_time = *g_time;
                    }
                }
                if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                    g_features->orbwalker->set_cast_time( 0.025f );
                    m_q_cast_begin = *g_time;
                    m_channeling_q = true;
                    return true;
                }

                return false;
            }

            m_q_cast_begin = 0.f;
            m_channeling_q = true;

            auto bonus_range = static_cast< int32_t >( std::floor(
                ( *g_time - buff->buff_data->start_time ) / 0.25f
            ) * 140.f );

            if ( bonus_range > 700 ) bonus_range = 700;

            auto hitchance =
                g_config->varus.q_slow_prediction->get< bool >( ) &&
                bonus_range != 700 &&
                g_config->varus.q_hitchance->get< int >( ) < 3
                    ? 3
                    : g_config->varus.q_hitchance->get< int >( );

            const auto pred = g_features->prediction->predict(
                target->index,
                m_q_base_range + bonus_range,
                1900.f,
                70.f,
                0.f
            );

            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( hitchance ) ) return false;
            if ( !m_slot_w->is_ready( ) || *g_time - m_last_w_time <= 0.2f ) return false;
            if ( m_slot_w->get_usable_state( ) != 1 ) {
                if ( cast_spell( ESpellSlot::w ) ) {
                    //g_features->orbwalker->set_cast_time(0.f);
                    m_last_w_time = *g_time;
                }
            }

            if ( release_chargeable( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->set_cast_time( 0.f );
                m_last_q_time  = *g_time;
                m_channeling_q = false;
                return true;
            }
            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->varus.e_enabled->get< bool >( ) ||
                m_channeling_q || *g_time - m_last_e_time <= 0.4f ||
                !m_slot_e->is_ready( true )
            )
                return false;

            if ( g_config->varus.e_multihit->get< bool >( ) ) {
                const auto multihit = get_multihit_position( m_e_range, 0.f, 150.f, 0.75f, false );

                if ( multihit.hit_count > 1 ) {
                    if ( cast_spell( ESpellSlot::e, multihit.position ) ) {
                        m_last_e_time = *g_time;
                        g_features->orbwalker->set_cast_time( 0.25f );
                        return true;
                    }
                }
            }

            const auto target = g_features->target_selector->get_spell_specific_target(
                m_e_range,
                [this]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::e, unit ); },
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::e, unit ); }
            );
            if ( !target ) return false;

            auto hitchance = g_config->varus.prefer_e_over_q->get< bool >( ) && m_slot_q->is_ready( true )
                                 ? 0
                                 : g_config->varus.e_hitchance->get< int >( );

            const auto pred = g_features->prediction->predict( target->index, m_e_range, 0.f, 150.f, 0.75f, { }, true );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( hitchance )
                && !will_kill(
                    target,
                    [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::e, unit ); }
                ) )
                return false;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                m_last_e_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.25f );
                return true;
            }

            return false;
        }

        //only want to use AP E if they are within our circle, they have 3 stacks, and our Q is not ready.
        //IF they are outside of our circle we want to find the enemy with stacks
        auto spell_ap_e( ) -> bool{
            if ( !g_config->varus.e_enabled->get< bool >( ) ||
                m_channeling_q || *g_time - m_last_e_time <= 0.4f ||
                !m_slot_e->is_ready( true ) ||
                *g_time - m_last_q_time < 0.3f
            )
                return false;

            const auto buff   = g_features->buff_cache->get_buff( g_local->index, ct_hash( "VarusQLaunch" ) );
            const auto target = g_features->target_selector->get_default_target( );

            if ( !target ) return false;
            if ( !buff ) {
                const auto w_buff = g_features->buff_cache->get_buff( target->index, ct_hash( "VarusWDebuff" ) );

                if ( target->dist_to_local( ) > g_config->varus.q_ap_draw->get< int >( ) ) {
                    const Object* target         = { };
                    auto          highest_stacks = 0;
                    auto          current_stacks = 0;
                    float         lowest_hp      = { };

                    for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                        if ( !enemy || g_features->target_selector->is_bad_target( enemy->index ) || !g_features->
                            buff_cache->get_buff( enemy->index, ct_hash( "VarusWDebuff" ) ) )
                            continue;

                        current_stacks = g_features->buff_cache->get_buff(
                            enemy->index,
                            ct_hash( "VarusWDebuff" )
                        )->stacks( );

                        if ( current_stacks > highest_stacks ) {
                            highest_stacks = current_stacks;
                            target         = enemy;
                        } else if ( current_stacks == highest_stacks ) if ( target->health < enemy->health ) continue;

                        target = enemy;
                        break;
                    }
                }

                if ( !w_buff ) return false;
                if ( w_buff->stacks( ) < 3.f ) return false;

                const auto pred = g_features->prediction->predict( target->index, m_e_range, 0, 150.f, 0.75f );
                if ( !pred.valid ) return false;

                if ( ( int )pred.hitchance < g_config->varus.e_hitchance->get< int >( ) || *g_time - m_last_e_time <=
                    0.2f || !m_slot_e->is_ready( true ) || ( m_slot_q->is_ready( ) && m_slot_w->is_ready( ) ) ) {
                    return
                        false;
                }

                auto hitchance = g_config->varus.prefer_e_over_q->get< bool >( ) && m_slot_q->is_ready( true )
                                     ? 0
                                     : g_config->varus.e_hitchance->get< int >( );

                if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( hitchance ) && !will_kill(
                    target,
                    [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::e, unit ); }
                ) )
                    return false;

                if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                    m_last_e_time = *g_time;
                    g_features->orbwalker->set_cast_time( 0.25f );
                    return true;
                }

                return false;
            }
            return { };
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->varus.r_enabled->get< bool >( ) || *g_time - m_last_r_time <= 0.1f || !m_slot_r->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_spell_specific_target(
                m_r_range,
                [this]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::r, unit ); },
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::r, unit ); }
            );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_r_range, 1500.f, 120.f, 0.25f );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->varus.r_hitchance->
                get< int >( ) ) )
                return false;

            if ( g_config->varus.r_only_if_can_spread->get< bool >( ) ) {
                auto closest_spreadable{ std::numeric_limits< float >::max( ) };
                bool spread_assured{ };
                int  count{ };

                for ( const auto hero : g_entity_list.get_enemies( ) ) {
                    if ( !hero ) continue;
                    if ( hero->index == target->index || hero->position.dist_to( pred.position ) > 675.f || g_features->
                        target_selector->is_bad_target( hero->index ) )
                        continue;

                    count++;

                    if ( !spread_assured && hero->position.dist_to( pred.position ) < closest_spreadable ) {
                        closest_spreadable = hero->position.dist_to( pred.position );
                        if ( hero->movement_speed * 1.5f <= 700.f - hero->position.
                                                                          dist_to(
                                                                              pred.position
                                                                          ) )
                            spread_assured = true;
                    }
                }

                if ( !spread_assured && count < 2 ) return false;
            }

            if ( cast_spell( ESpellSlot::r, pred.position ) ) {
                m_last_r_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.25f );
                return true;
            }

            return false;
        }

        auto laneclear_e( ) -> bool{
            if ( !m_slot_e->is_ready( true ) ||
                *g_time - m_last_e_time <= 0.4f ||
                g_local->mana < g_local->max_mana / 100.f * g_config->varus.spellclear_min_mana->get< int >( )
            )
                return false;

            const auto farm_pos = get_best_laneclear_position( m_e_range, 150.f );
            if ( farm_pos.value < 3 ) return false;

            if ( cast_spell( ESpellSlot::e, farm_pos.position ) ) {
                m_last_e_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.25f );

                return true;
            }

            return false;
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->attack_damage( ) * m_q_ad_modifier[ m_slot_q->level ],
                    target->index,
                    true
                );
            case ESpellSlot::e:
                return helper::calculate_damage(
                    m_e_damage[ get_slot_e( )->level ] + g_local->bonus_attack_damage( ) * 0.9f,
                    target->index,
                    true
                );
            case ESpellSlot::r:
                return helper::calculate_damage(
                    m_r_damage[ get_slot_r( )->level ] + g_local->ability_power( ),
                    target->index,
                    false
                );
            }

            return 0.f;
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
            {
                const auto tt   = g_local->position.dist_to( target->position ) / 1900.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return g_local->position.dist_to( pred.value( ) ) / 1900.f;
            }
            case ESpellSlot::e:
                return 0.75f;
            case ESpellSlot::r:
            {
                const auto tt   = 0.25f + g_local->position.dist_to( target->position ) / 1500.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.25f + g_local->position.dist_to( pred.value( ) ) / 1500.f;
            }
            }

            return IModule::get_spell_travel_time( slot, target );
        }

        auto get_stack_damage( Object* target, const int stacks ) const -> float{
            const auto damage_per_stack = ( m_w_stack_modifier[ get_slot_w( )->level ] + g_local->ability_power( ) / 100
                *
                0.025f ) * target->max_health;
            auto       damage_modifier = 0.f;
            const auto missing_hp      = target->max_health - target->health;
            if ( g_local->level < 4 ) {
                damage_modifier = m_w_scaling_damage[ 1 ];
                debug_log( "below 4" );
            } else if ( !( g_local->level < 4 ) && !( g_local->level >= 7 ) ) {
                debug_log( "at or above 4 and below 7" );
                damage_modifier = m_w_scaling_damage[ 2 ];
            } else if ( !( g_local->level < 7 ) && !( g_local->level >= 10 ) ) {
                debug_log( "at or above 7 and below 10" );
                damage_modifier = m_w_scaling_damage[ 3 ];
            } else if ( !( g_local->level < 10 ) && !( g_local->level >= 13 ) ) {
                debug_log( "at or above 10 and below 13" );
                damage_modifier = m_w_scaling_damage[ 4 ];
            } else damage_modifier = m_w_scaling_damage[ 5 ];

            return helper::calculate_damage(
                damage_per_stack * stacks + missing_hp * damage_modifier,
                target->index,
                false
            );
        }

    private:
        // q specific stuff
        bool  m_channeling_q{ };
        float m_q_cast_begin{ };
        float m_q_base_range{ 895.f };

        std::array< float, 6 > m_q_damage      = { 0.f, 10.f, 46.67f, 83.33f, 120.f, 156.67f };
        std::array< float, 6 > m_q_ad_modifier = { 0.f, 0.8333f, 0.8667f, 0.9f, 0.9333f, 0.9667f };

        std::array< float, 6 > m_w_stack_modifier = { 0.f, 0.03f, 0.035f, 0.04f, 0.045f, 0.05f };
        std::array< float, 6 > m_w_scaling_damage = { 0.f, 0.06f, 0.08f, 0.10f, 0.12f, 0.14f };

        std::array< float, 6 > m_e_damage = { 0.f, 60.f, 100.f, 140.f, 180.f, 220.f };

        std::array< float, 6 > m_r_damage = { 0.f, 150.f, 200.f, 250.f };

        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_r_time{ };

        float m_q_range{ 1595.f };
        float m_e_range{ 925.f };
        float m_r_range{ 1300.f };

        bool m_is_killable{ };
    };
}
