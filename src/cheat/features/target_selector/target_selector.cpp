#include "pch.hpp"

#include "target_selector.hpp"

#include "../buff_cache.hpp"
#include "../entity_list.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../helper.hpp"
#if enable_new_lua
#include "../../lua-v2/state.hpp"
#endif
#include "../utils/input.hpp"
#include "../champion_modules/module.hpp"

namespace features {
    auto TargetSelector::run( ) -> void{
#if enable_lua
        if ( g_lua2 ) {
            g_lua2->execute_locked(
                []( ) -> void{ g_lua2->run_callback( ct_hash( "features.target_selector" ) ); }
            );
        }
#endif

        update_forced_target( );
        update_orbwalker_target( );
    }

    auto TargetSelector::update_orbwalker_target( ) -> void{
        select_target( );
        //nenny_target_selection_mode( );

        const auto primary_target = get_orbwalker_default_target( );
        if ( !primary_target ) {
            m_secondary_target = nullptr;
            return;
        }

        select_target( true );
        //nenny_target_selection_mode( true );

        const auto secondary_target = get_orbwalker_default_target( true );
        if ( !secondary_target ) m_secondary_target = nullptr;
    }

    auto TargetSelector::update_forced_target( ) -> void{
        if ( !g_config->target_selector.selector_left_click_target->get< bool >( ) ) {
            // todo: remove this
            // if ( m_is_target_forced ) m_is_target_forced = false;
            return;
        }

        // if target is invalid or too much time has passed, disable force target
        if ( m_is_target_forced && ( !m_forced_target || !m_forced_target->get( ) || is_bad_target(
            m_forced_target->get( )->index,
            false,
            true
        ) ) ) {
            // NOLINT(readability-redundant-smartptr-get)
            m_is_target_forced = false;
            return;
        }

        if ( g_keybind_system->is_key_down( utils::EKey::lbutton ) ) {
            if ( *g_time - m_last_force_time <= 0.15f ) return;

            const Object* selected_target = nullptr;
            auto          lowest_distance{ std::numeric_limits< float >::max( ) };
            const auto    cursor_pos = g_input->get_cursor_position( );

            for ( const auto hero : g_entity_list.get_enemies( ) ) {
                if ( !hero || is_bad_target( hero->index ) ) continue;

                Vec2 s_pos{ };

                if ( !world_to_screen( hero->position, s_pos ) ) continue;

                if ( s_pos.y > 25.f ) s_pos.y -= 25.f;

                const auto distance = cursor_pos.dist_to( s_pos );

                if ( distance < 100.f && distance < lowest_distance ) {
                    selected_target = hero;
                    lowest_distance = distance;
                }
            }

            if ( selected_target && ( !m_is_target_forced || m_is_target_forced && m_forced_target->get( )->index !=
                selected_target->index ) )
                set_forced_target( selected_target->index );
                // if found no target near cursor and forced target is already selected -> unselect forced target
            else if ( m_is_target_forced ) {
                m_is_target_forced = false;
                m_last_force_time  = *g_time;
                m_forced_target    = nullptr;
            }
        }
    }

    auto TargetSelector::old_target_selection_mode( ) -> void{
        bool          has_target{ };
        bool          kill_found{ };
        bool          restrict_range{ };
        bool          selected_in_range{ };
        int32_t       selected_priority{ };
        auto          selected_aa_to_kill{ std::numeric_limits< int32_t >::max( ) };
        const Object* selected_hero{ };

        for ( const auto enemy : g_entity_list.get_enemies( ) ) {
            if ( !enemy ) continue;
            if ( g_local->position.dist_to( enemy->position ) > 1500.f || is_bad_target( enemy->index ) ) continue;

            bool can_kill{ };

            const auto damage   = helper::get_aa_damage( enemy->index );
            const auto priority = get_target_priority( enemy->champion_name.text );

            if ( g_features->orbwalker->is_attackable( enemy->index ) ) {
                restrict_range = true;
                if ( damage * 3 > enemy->health ) can_kill = true;
            } else if ( restrict_range ) continue;

            if ( priority >= selected_priority && !kill_found || can_kill || !selected_in_range && restrict_range ) {
                if ( has_target && ( selected_in_range || !restrict_range ) ) {
                    if ( priority == selected_priority ) {
                        if ( std::ceil( enemy->health / damage ) >=
                            selected_aa_to_kill )
                            continue;
                    }
                }

                selected_hero = enemy;

                selected_priority   = priority;
                selected_aa_to_kill = static_cast< int32_t >( std::ceil( enemy->health / damage ) );
                selected_in_range   = g_features->orbwalker->is_attackable( enemy->index );

                if ( can_kill ) kill_found = true;
                has_target = true;
            }
        }

        if ( !selected_hero ) {
            if ( m_default_target && is_bad_target( m_default_target->get( )->index ) ) m_default_target = nullptr;

            return;
        }

        m_default_target = &g_entity_list.get_by_index( selected_hero->index );
    }

    auto TargetSelector::select_target( const bool secondary ) -> void{
        if ( g_config->target_selector.selector_mode->get< int >( ) == 1 ) return select_target_new( secondary );
        if ( g_config->target_selector.selector_mode->get< int >( ) == 2 ) {
            return nenny_target_selection_mode(
                secondary
            );
        }

        const Object* target{ };
        bool          restrict_range{ };
        bool          target_in_attackrange{ };
        bool          target_low_health_priority{ };
        int           target_priority{ };
        auto          target_aa_to_kill{ std::numeric_limits< int32_t >::max( ) };
        auto          target_distance{ std::numeric_limits< float >::max( ) };
        bool          target_override_priority{ };
        bool          target_kill_priority{ };
        float         target_health{ };

        const auto is_cassio   = helper::get_current_hero( ) == EHeroes::cassiopeia;
        const auto ignored_nid = secondary ? m_default_target->get( )->network_id : 0;


        for ( const auto enemy : g_entity_list.get_enemies( ) ) {
            if ( !enemy || enemy->network_id == ignored_nid || g_local->position.dist_to( enemy->position ) >= 5000.f ||
                is_bad_target( enemy->index ) )
                continue;

            const auto distance = g_local->position.dist_to( enemy->position );
            const auto damage   = helper::get_aa_damage( enemy->index );
            auto       priority = get_target_priority( enemy->champion_name.text );
            const auto health   = helper::get_real_health(
                enemy->index,
                EDamageType::physical_damage,
                g_features->orbwalker->get_attack_cast_delay( ),
                true
            );
            const auto aa_to_kill = static_cast< int32_t >( std::ceil( health / damage ) );

            bool override_priority{ };
            bool low_health_priority{ };
            bool force_new_target{ };
            bool kill_priority{ };

            if ( g_features->orbwalker->is_attackable( enemy->index, is_cassio ? 700.f : 0.f, !is_cassio ) ) {
                restrict_range = true;
                if ( !target_in_attackrange ) force_new_target = true;
                if ( aa_to_kill <= g_config->target_selector.selector_aa_to_kill_priority->get<
                    int >( ) )
                    low_health_priority = true;

                kill_priority = aa_to_kill < 2;

                switch ( helper::get_current_hero( ) ) {
                case EHeroes::cassiopeia:
                    if ( g_features->buff_cache->get_buff( enemy->index, ct_hash( "cassiopeiaqdebuff" ) )
                        || g_features->buff_cache->get_buff( enemy->index, ct_hash( "cassiopeiawpoison" ) ) )
                        override_priority = true;

                    break;
                case EHeroes::caitlyn:
                    if ( g_features->orbwalker->is_unit_headshottable( enemy->index ) ) override_priority = true;

                    break;
                case EHeroes::tristana:
                    if ( g_features->buff_cache->
                                     get_buff( enemy->index, ct_hash( "tristanaechargesound" ) ) ) {
                        override_priority =
                            true;
                    }
                    break;
                case EHeroes::vayne:
                {
                    const auto buff = g_features->buff_cache->get_buff(
                        enemy->index,
                        ct_hash( "VayneSilveredDebuff" )
                    );
                    if ( buff && buff->stacks( ) == 2 ) override_priority = true;
                    break;
                }
                case EHeroes::senna:
                    if ( g_features->buff_cache->get_buff( enemy->index, ct_hash( "sennapassivemarker" ) ) ) {
                        priority =
                            5;
                    }
                    break;
                case EHeroes::lux:
                    if ( g_features->buff_cache->
                                     get_buff( enemy->index, ct_hash( "LuxIlluminatingFraulein" ) ) ) {
                        override_priority
                            = true;
                    }

                    break;
                default:
                    break;
                }
            } else if ( restrict_range ) continue;
            else if ( distance + 400.f < target_distance ) force_new_target = true;
            else if ( distance > target_distance && target_distance + 400.f <= distance ) continue;

            // if candidate priority is bigger or equal to target priority
            if ( priority >= target_priority && !target_low_health_priority || low_health_priority ||
                force_new_target || override_priority ) {
                // if target priority is same as candidate
                if ( !force_new_target &&
                    ( !override_priority || target_override_priority ) &&
                    ( !kill_priority || target_kill_priority ) &&
                    ( !low_health_priority || target_low_health_priority ) &&
                    priority == target_priority ) {
                    if ( low_health_priority ) {
                        const auto should_skip =
                            target_low_health_priority &&
                            ( !kill_priority || target_kill_priority ) &&
                            ( target_priority > priority || target_priority == priority && ( target_aa_to_kill <
                                aa_to_kill || target_aa_to_kill == aa_to_kill && target_health < health ) );

                        if ( should_skip ) continue;
                    } else {
                        if ( restrict_range ) {
                            if ( target_priority > priority && aa_to_kill + 2 > target_aa_to_kill ) continue;

                            if ( aa_to_kill > target_aa_to_kill ) continue;
                        } else if ( health <= target_health ) continue;
                    }
                }

                // set new target
                target                = enemy;
                target_priority       = priority;
                target_in_attackrange = g_features->orbwalker->is_attackable(
                    enemy->index,
                    is_cassio ? 700.f : 0.f,
                    !is_cassio
                );
                target_aa_to_kill = target_in_attackrange ? aa_to_kill : std::numeric_limits< int32_t >::max( );
                target_distance   = distance;
                target_health     = health;

                target_low_health_priority = low_health_priority;
                target_override_priority   = override_priority;
                target_kill_priority       = kill_priority;
            }
        }

        if ( secondary ) {
            if ( !target ) {
                if ( m_secondary_target && m_secondary_target->is_valid( ) &&
                    is_bad_target( m_secondary_target->get( )->index ) )
                    m_secondary_target = nullptr;

                return;
            }

            m_secondary_target = &g_entity_list.get_by_index( target->index );
            return;
        }

        if ( !target ) {
            if ( m_default_target && m_default_target->is_valid( ) &&
                is_bad_target( m_default_target->get( )->index ) )
                m_default_target = nullptr;

            return;
        }

        m_default_target = &g_entity_list.get_by_index( target->index );
    }

    auto TargetSelector::select_target_new( const bool secondary ) -> void{
        const Object* target{ };
        bool          restrict_range{ };
        bool          target_in_attackrange{ };
        bool          target_low_health_priority{ };
        int           target_priority{ };
        auto          target_aa_to_kill{ std::numeric_limits< int32_t >::max( ) };
        auto          target_distance{ std::numeric_limits< float >::max( ) };
        bool          target_override_priority{ };
        bool          target_kill_priority{ };
        float         target_health{ };

        const auto is_cassio   = helper::get_current_hero( ) == EHeroes::cassiopeia;
        const auto ignored_nid = secondary ? m_default_target->get( )->network_id : 0;


        for ( const auto enemy : g_entity_list.get_enemies( ) ) {
            if ( !enemy || enemy->network_id == ignored_nid || g_local->position.dist_to( enemy->position ) >= 5000.f ||
                is_bad_target( enemy->index ) )
                continue;

            const auto distance = g_local->position.dist_to( enemy->position );
            const auto damage   = helper::get_aa_damage( enemy->index );
            auto       priority = get_target_priority( enemy->champion_name.text );
            const auto health   = helper::get_real_health(
                enemy->index,
                EDamageType::physical_damage,
                g_features->orbwalker->get_attack_cast_delay( ),
                true
            );
            const auto aa_to_kill = static_cast< int32_t >( std::ceil( health / damage ) );

            bool override_priority{ };
            bool low_health_priority{ };
            bool force_new_target{ };
            bool kill_priority{ };

            if ( g_features->orbwalker->is_attackable( enemy->index, is_cassio ? 700.f : 0.f, !is_cassio ) ) {
                restrict_range = true;
                if ( !target_in_attackrange ) force_new_target = true;
                if ( aa_to_kill <= g_config->target_selector.selector_aa_to_kill_priority->get<
                    int >( ) )
                    low_health_priority = true;

                kill_priority = aa_to_kill < 2;

                switch ( helper::get_current_hero( ) ) {
                case EHeroes::cassiopeia:
                    if ( g_features->buff_cache->get_buff( enemy->index, ct_hash( "cassiopeiaqdebuff" ) ) ||
                        g_features->buff_cache->get_buff( enemy->index, ct_hash( "cassiopeiawpoison" ) ) )
                        override_priority = true;

                    break;
                case EHeroes::caitlyn:
                    if ( g_features->orbwalker->is_unit_headshottable( enemy->index ) ) override_priority = true;

                    break;
                case EHeroes::tristana:
                    if (
                        g_features->buff_cache->get_buff( enemy->index, ct_hash( "tristanaechargesound" ) )
                    )
                        override_priority = true;
                    break;
                case EHeroes::vayne:
                {
                    const auto buff =
                        g_features->buff_cache->get_buff( enemy->index, ct_hash( "VayneSilveredDebuff" ) );
                    if ( buff && buff->stacks( ) == 2 ) override_priority = true;
                    break;
                }
                case EHeroes::senna:
                    if ( g_features->buff_cache->get_buff( enemy->index, ct_hash( "sennapassivemarker" ) ) ) {
                        priority =
                            5;
                    }
                    break;
                case EHeroes::lux:
                    if ( g_features->buff_cache->
                                     get_buff( enemy->index, ct_hash( "LuxIlluminatingFraulein" ) ) ) {
                        override_priority
                            = true;
                    }

                    break;
                default:
                    break;
                }
            } else if ( restrict_range ) continue;
            else if ( distance + 400.f < target_distance ) force_new_target = true; // old value was 300
            else if ( distance > target_distance && target_distance + 400.f <= distance ) continue;

            // if candidate priority is bigger or equal to target priority
            if ( priority >= target_priority && aa_to_kill <= target_aa_to_kill + 1 && !target_low_health_priority ||
                low_health_priority ||
                force_new_target || override_priority ) {
                // if target priority is same as candidate
                if ( !force_new_target &&
                    ( !override_priority || target_override_priority ) &&
                    ( !kill_priority || target_kill_priority ) &&
                    ( !low_health_priority || target_low_health_priority ) && priority <= target_priority ) {
                    if ( low_health_priority ) {
                        const auto should_skip = target_low_health_priority &&
                            ( !kill_priority || target_kill_priority ) &&
                            ( target_priority > priority ||
                                target_priority == priority &&
                                ( target_aa_to_kill < aa_to_kill ||
                                    target_aa_to_kill == aa_to_kill && target_health < health ) );

                        if ( should_skip ) continue;
                    } else {
                        if ( restrict_range ) {
                            if ( target_priority > priority && aa_to_kill + 1 > target_aa_to_kill ) continue;

                            if ( aa_to_kill > target_aa_to_kill ) continue;
                        } else if ( aa_to_kill > target_aa_to_kill || aa_to_kill == target_aa_to_kill && priority <
                            target_priority )
                            continue;
                    }
                }

                // set new target
                target                = enemy;
                target_priority       = priority;
                target_in_attackrange =
                    g_features->orbwalker->is_attackable( enemy->index, is_cassio ? 700.f : 0.f, !is_cassio );
                target_aa_to_kill = aa_to_kill;
                target_distance   = distance;
                target_health     = health;

                target_low_health_priority = low_health_priority;
                target_override_priority   = override_priority;
                target_kill_priority       = kill_priority;
            }
        }

        if ( secondary ) {
            if ( !target ) {
                if ( m_secondary_target && m_secondary_target->is_valid( ) &&
                    is_bad_target( m_secondary_target->get( )->index ) )
                    m_secondary_target = nullptr;

                return;
            }

            m_secondary_target = &g_entity_list.get_by_index( target->index );
            return;
        }

        if ( !target ) {
            if ( m_default_target && m_default_target->is_valid( ) &&
                is_bad_target( m_default_target->get( )->index ) )
                m_default_target = nullptr;

            return;
        }

        m_default_target = &g_entity_list.get_by_index( target->index );
    }

    auto TargetSelector::nenny_target_selection_mode( const bool secondary ) -> void{
        const Object* target{ };
        bool          restrict_to_aa_range{ };
        bool          restrict_to_champ_range{ };
        bool          target_in_attack_range{ };
        bool          target_in_champ_range{ };
        bool          target_low_health_priority{ };
        int           target_priority{ };
        auto          target_aa_to_kill{ std::numeric_limits< int32_t >::max( ) };
        auto          target_distance{ std::numeric_limits< float >::max( ) };
        bool          target_override_priority{ };
        bool          target_kill_priority{ };
        float         target_health{ };

        const auto is_cassio   = helper::get_current_hero( ) == EHeroes::cassiopeia;
        const auto ignored_nid = secondary ? m_default_target->get( )->network_id : 0;


        for ( const auto enemy : g_entity_list.get_enemies( ) ) {
            if ( !enemy || enemy->network_id == ignored_nid || g_local->position.dist_to( enemy->position ) >= 5000.f ||
                is_bad_target( enemy->index ) )
                continue;

            const auto distance = g_local->position.dist_to( enemy->position );
            const auto damage   = helper::get_aa_damage( enemy->index );
            const auto priority = get_target_priority( enemy->champion_name.text );
            const auto health   = helper::get_real_health(
                enemy->index,
                EDamageType::physical_damage,
                g_features->orbwalker->get_attack_cast_delay( ),
                true
            );
            const auto aa_to_kill = static_cast< int32_t >( std::ceil( health / damage ) );

            bool override_priority{ };
            bool low_health_priority{ };
            bool force_new_target{ };
            bool kill_priority{ };

            if ( g_features->orbwalker->is_attackable( enemy->index, is_cassio ? 700.f : 0.f, !is_cassio ) ) {
                restrict_to_aa_range = true;
                if ( !target_in_attack_range ) force_new_target = true;
                if ( aa_to_kill <= g_config->target_selector.selector_aa_to_kill_priority->get<
                    int >( ) )
                    low_health_priority = true;

                kill_priority = aa_to_kill < 2;

                switch ( helper::get_current_hero( ) ) {
                case EHeroes::cassiopeia:
                    if ( g_features->buff_cache->get_buff( enemy->index, ct_hash( "cassiopeiaqdebuff" ) ) ||
                        g_features->buff_cache->get_buff( enemy->index, ct_hash( "cassiopeiawpoison" ) ) )
                        override_priority = true;

                    break;
                case EHeroes::caitlyn:
                    if ( g_features->orbwalker->is_unit_headshottable( enemy->index ) ) override_priority = true;

                    break;
                case EHeroes::tristana:
                    if ( g_features->buff_cache->
                                     get_buff( enemy->index, ct_hash( "tristanaechargesound" ) ) ) {
                        override_priority =
                            true;
                    }
                    break;
                case EHeroes::vayne:
                {
                    const auto buff =
                        g_features->buff_cache->get_buff( enemy->index, ct_hash( "VayneSilveredDebuff" ) );
                    if ( buff && buff->stacks( ) == 2 ) override_priority = true;
                    break;
                }
                case EHeroes::senna:
                    if ( g_features->buff_cache->
                                     get_buff( enemy->index, ct_hash( "sennapassivemarker" ) ) ) {
                        override_priority =
                            true;
                    }
                    break;
                case EHeroes::lux:
                    if (
                        g_features->buff_cache->get_buff( enemy->index, ct_hash( "LuxIlluminatingFraulein" ) )
                    ) { override_priority = true; }

                    break;
                default:
                    break;
                }
            } else if ( restrict_to_aa_range ) continue;
            else if ( distance < get_champion_range( rt_hash( g_local->champion_name.text ) ) ) {
                restrict_to_champ_range = true;
            } else if ( !target_in_champ_range ) force_new_target = true;
                // need to see if there could be champ specific logic with no attackable enemy(maybe ez check collision?)

            else if ( distance + 400.f < target_distance ) force_new_target = true; // old value was 300
            else if ( distance > target_distance && target_distance + 400.f <= distance ) continue;

            // if candidate priority is bigger or equal to target priority
            if ( priority >= target_priority && !target_low_health_priority || low_health_priority ||
                force_new_target || override_priority ) {
                // if target priority is same as candidate
                if ( !force_new_target && ( !override_priority || target_override_priority ) &&
                    ( !kill_priority || target_kill_priority ) &&
                    ( !low_health_priority || target_low_health_priority ) && priority == target_priority ) {
                    if ( low_health_priority ) {
                        const auto should_skip = target_low_health_priority &&
                            ( !kill_priority || target_kill_priority ) &&
                            ( target_priority > priority ||
                                target_priority == priority &&
                                ( target_aa_to_kill < aa_to_kill ||
                                    target_aa_to_kill == aa_to_kill && target_health < health ) );

                        if ( should_skip ) continue;
                    } else {
                        if ( restrict_to_aa_range ) {
                            if ( target_priority > priority && aa_to_kill + 2 > target_aa_to_kill ) continue;

                            if ( aa_to_kill > target_aa_to_kill ) continue;
                        } else if ( health <= target_health ) continue;
                    }
                }

                // set new target
                target                 = enemy;
                target_priority        = priority;
                target_in_attack_range = g_features->orbwalker->is_attackable(
                    enemy->index,
                    is_cassio ? 700.f : 0.f,
                    !is_cassio
                );
                target_in_champ_range = distance < get_champion_range( rt_hash( g_local->champion_name.text ) );
                target_aa_to_kill     = target_in_attack_range ? aa_to_kill : std::numeric_limits< int32_t >::max( );
                target_distance       = distance;
                target_health         = health;

                target_low_health_priority = low_health_priority;
                target_override_priority   = override_priority;
                target_kill_priority       = kill_priority;
            }
        }

        if ( secondary ) {
            if ( !target ) {
                if ( m_secondary_target && m_secondary_target->is_valid( ) &&
                    is_bad_target( m_secondary_target->get( )->index ) )
                    m_secondary_target = nullptr;

                return;
            }

            m_secondary_target = &g_entity_list.get_by_index( target->index );
            return;
        }

        if ( !target ) {
            if ( m_default_target && m_default_target->is_valid( ) &&
                is_bad_target( m_default_target->get( )->index ) )
                m_default_target = nullptr;

            return;
        }

        m_default_target = &g_entity_list.get_by_index( target->index );
    }

    /* auto TargetSelector::nenny_target_selection_mode( ) -> void {
        Object* target{ };
        bool    restrict_range{ };
        bool    target_in_attackrange{ };
        bool    target_low_health_priority{ };
        int     target_priority{ };
        int     target_aa_to_kill{ std::numeric_limits< int32_t >::max( ) };
        float   target_distance{ std::numeric_limits< float >::max( ) };

        const bool is_cassio = helper::get_current_hero( ) == e_heroes::cassiopeia;

        for ( const auto enemy : g_entity_list->get_enemies( ) ) {
            if ( !enemy ) continue;
            if ( g_local->position.dist_to( enemy->position ) >= 5000.f || is_bad_target( enemy->index ) ) continue;

            const auto distance   = g_local->position.dist_to( enemy->position );
            const auto damage     = helper::get_aa_damage( enemy->index );
            auto       priority   = get_target_priority( enemy->champion_name.text );
            const auto aa_to_kill = static_cast< int32_t >( std::ceil( enemy->health / damage ) );

            bool low_health_priority{ };
            bool force_new_target{ };

            if ( g_features->orbwalker->is_attackable( enemy->index, is_cassio ? 700.f : 0.f, !is_cassio ) ) {
                restrict_range = true;
                if ( !target_in_attackrange ) force_new_target = true;
                if ( aa_to_kill <= g_config->target_selector.selector_aa_to_kill_priority->get<
                    int >( ) )
                    low_health_priority = true;

                switch ( helper::get_current_hero( ) ) {
                case e_heroes::cassiopeia:
                    if ( g_features->buff_cache->get_buff( enemy->index, ct_hash( "cassiopeiaqdebuff" ) ) ||
                        g_features->buff_cache->get_buff( enemy->index, ct_hash( "cassiopeiawpoison" ) ) ) {
                        priority            = 5;
                        low_health_priority = true;
                    }

                    break;
                case e_heroes::tristana:
                    if ( g_features->buff_cache->get_buff( enemy->index, ct_hash( "tristanaechargesound" ) ) )
                        priority
                            = 5;
                    break;
                case e_heroes::vayne:
                {
                    auto buff = g_features->buff_cache->get_buff( enemy->index, ct_hash( "VayneSilveredDebuff" ) );
                    if ( buff && buff->stacks( ) == 2 ) priority = 5;
                    break;
                }
                case e_heroes::senna:
                    if ( g_features->buff_cache->get_buff( enemy->index, ct_hash( "sennapassivemarker" ) ) )
                        priority =
                            5;
                    break;
                default:
                    break;
                }
            } else if ( restrict_range ) continue;
            else if ( distance + 300.f < target_distance ) force_new_target = true;
            else if ( distance > target_distance && target_distance + 300.f <= distance ) continue;

            // if candidate priority is bigger or equal to target priority
            if ( priority >= target_priority && !target_low_health_priority || low_health_priority ||
                force_new_target ) {
                // if target priority is same as candidate
                if ( !force_new_target &&
                    ( priority == target_priority || low_health_priority && target_low_health_priority ) ) {
                    if ( restrict_range ) {
                        if ( target_priority > priority && aa_to_kill + 2 > target_aa_to_kill ) continue;

                        if ( aa_to_kill > target_aa_to_kill ) continue;
                    } else if ( target->health < enemy->health ) continue;
                }

                // set new target
                target                = enemy;
                target_priority       = priority;
                target_in_attackrange =
                    g_features->orbwalker->is_attackable( enemy->index, is_cassio ? 700.f : 0.f, !is_cassio );
                target_aa_to_kill = target_in_attackrange ? aa_to_kill : std::numeric_limits< int32_t >::max( );
                target_low_health_priority = low_health_priority;
                target_distance = distance;
            }
        }

        if ( !target ) {
            if ( m_default_target && m_default_target->is_valid( ) &&
                is_bad_target( m_default_target->get( )->index ) )
                m_default_target = nullptr;

            return;
        }

        m_default_target = &g_entity_list->get_by_index( target->index );
    }
    */


    auto TargetSelector::get_spell_specific_target(
        const float                                  range,
        const std::function< float( Object* unit ) > get_travel_time,
        const std::function< float( Object* unit ) > get_damage,
        int                                          damage_type
    ) -> Object*{
        // return get_default_target();

        if ( m_forced_target ) {
            const auto forced_target = get_forced_target( );
            if ( forced_target && forced_target->dist_to_local( ) < range && !
                is_bad_target( forced_target->index ) )
                return forced_target;
        }

        auto  target = get_default_target( );
        int   target_priority{ };
        float target_health{ };
        bool  target_found{ };
        bool  target_kill_priority{ };

        for ( const auto enemy : g_entity_list.get_enemies( ) ) {
            if ( !enemy ) continue;
            if ( is_bad_target( enemy->index ) ) continue;

            float travel_time{ };

            if ( get_travel_time != nullptr ) {
                auto pred = g_features->prediction->predict_default( enemy->index, get_travel_time( enemy ) );
                if ( !pred || g_local->position.dist_to( pred.value( ) ) > range ) continue;

                travel_time = get_travel_time( enemy );
            } else if ( enemy->dist_to_local( ) > range ) continue;

            const auto priority      = get_target_priority( enemy->champion_name.text );
            const auto damage        = get_damage ? get_damage( enemy ) : 0.f;
            const auto kill_priority = damage > 0.f
                                           ? damage > helper::get_real_health(
                                               enemy->index,

                                               static_cast< EDamageType >( damage_type ),
                                               travel_time,
                                               true
                                           )
                                           : false;

            if ( !target_found || kill_priority && !target_kill_priority || target_found && target_kill_priority ==
                kill_priority &&
                ( priority > target_priority || priority == target_priority && enemy->health < target_health ) ) {
                target               = enemy;
                target_priority      = priority;
                target_health        = enemy->health;
                target_kill_priority = kill_priority;
                target_found         = true;
            }
        }

        if ( !target_found ) return { };

        return target;
    }

    auto TargetSelector::get_killsteal_target(
        const float                                  range,
        const std::function< float( Object* unit ) > get_travel_time,
        const std::function< float( Object* unit ) > get_damage,
        int                                          damage_type,
        Vec3                                         source_position
    ) -> Object*{
        Object* target{ };
        int     target_priority{ };
        float   target_health{ };
        bool    target_found{ };
        bool    target_kill_priority{ };

        if ( source_position.length( ) <= 0.f ) source_position = g_local->position;

        for ( const auto enemy : g_entity_list.get_enemies( ) ) {
            if ( !enemy || is_bad_target( enemy->index ) ) continue;

            float traveltime{ };

            if ( get_travel_time != nullptr ) {
                traveltime = get_travel_time( enemy );
                auto pred  = g_features->prediction->predict_default( enemy->index, traveltime, false );
                if ( !pred || source_position.dist_to( pred.value( ) ) > range ) continue;
            } else if ( enemy->position.dist_to( source_position ) > range ) continue;


            const auto priority     = get_target_priority( enemy->champion_name.text );
            const auto damage       = get_damage ? get_damage( enemy ) : 0.f;
            const auto total_health = helper::get_real_health(
                enemy->index,

                static_cast< EDamageType >( damage_type ),
                traveltime,
                true
            );

            const auto kill_priority = damage > total_health;
            if ( !kill_priority ) continue;

            if ( !target_found || kill_priority && !target_kill_priority || target_found && target_kill_priority ==
                kill_priority &&
                ( priority > target_priority || priority == target_priority && enemy->health < target_health ) ) {
                target               = enemy;
                target_priority      = priority;
                target_health        = enemy->health;
                target_kill_priority = kill_priority;
                target_found         = true;
            }
        }

        if ( !target_found ) return { };

        return target;
    }

    auto TargetSelector::is_bad_target(
        const int16_t index,
        const bool    ignore_dead,
        const bool    ignore_invisible
    ) -> bool{
        // Function is used in LUA, message @tore if you change args
        if ( !BuffCache::run_thread_check( ) ) return { };

        const auto& unit = g_entity_list.get_by_index( index );
        if ( !unit ) return true;

        if ( !ignore_dead && unit->is_dead( ) || !ignore_invisible && unit->is_invisible( ) || unit->
            get_selectable_flag( ) != 1 || unit->is_invulnerable( ) )
            return true;

        const auto name_hash = rt_hash( unit->champion_name.text );

        switch ( name_hash ) {
        case ct_hash( "Ekko" ):
            if ( g_features->buff_cache->get_buff( index, ct_hash( "ekkorinvuln" ) ) ) return true;
            break;
        case ct_hash( "Elise" ):
            if ( g_features->buff_cache->get_buff( index, ct_hash( "EliseSpiderE" ) ) ) return true;
            break;
        case ct_hash( "Fizz" ):
            if ( g_features->buff_cache->get_buff( index, ct_hash( "fizzeicon" ) ) ) return true;
            break;
        case ct_hash( "Gwen" ):
            if ( g_features->buff_cache->get_buff( index, ct_hash( "gwenw_gweninsidew" ) ) ) {
                bool inside_gwen_w{ };
                for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
                    if ( !minion || minion->position.dist_to( unit->position ) > 750.f || minion->is_dead( ) || rt_hash(
                            minion->name.text
                        ) != ct_hash( "TestCube" ) ||
                        minion->get_owner_index( ) != unit->index )
                        continue;

                    if ( g_local->position.dist_to( minion->position ) < 450.f ) inside_gwen_w = true;

                    break;
                }

                if ( !inside_gwen_w ) return true;
            }
            break;
        case ct_hash( "Karthus" ):
            if ( g_features->buff_cache->get_buff( index, ct_hash( "KarthusDeathDefiedBuff" ) ) ) return true;
            break;
        case ct_hash( "Kayn" ):
            if ( g_features->buff_cache->get_buff( index, ct_hash( "KaynR" ) ) ) return true;
            break;
        case ct_hash( "KogMaw" ):
            if ( g_features->buff_cache->get_buff( index, ct_hash( "KogMawIcathianSurprise" ) ) ) return true;
            break;
        case ct_hash( "Lissandra" ):
            if ( g_features->buff_cache->get_buff( index, ct_hash( "LissandraRSelf" ) ) ) return true;
            break;
        case ct_hash( "MasterYi" ):
            if ( g_features->buff_cache->get_buff( index, ct_hash( "AlphaStrike" ) ) ) return true;
            break;
        case ct_hash( "Sion" ):
            if ( g_features->buff_cache->get_buff( index, ct_hash( "sionpassivedelay" ) ) ) return true;
            break;
        case ct_hash( "Viego" ):
            if ( g_features->buff_cache->get_buff( index, ct_hash( "viegopassivecasting" ) ) ) return true;
            break;
        case ct_hash( "Vladimir" ):
            if ( g_features->buff_cache->get_buff( index, ct_hash( "VladimirSanguinePool" ) ) ) return true;
            break;
        case ct_hash( "Xayah" ):
            if ( g_features->buff_cache->get_buff( index, ct_hash( "XayahR" ) ) ) return true;
            break;
        case ct_hash( "Yuumi" ):
            if ( g_features->buff_cache->get_buff( index, ct_hash( "YuumiWAttach" ) ) ) return true;
            break;
        case ct_hash( "Zed" ):
            if ( g_features->buff_cache->get_buff( index, ct_hash( "ZedR2" ) ) &&
                *g_time - g_features->buff_cache->get_buff( index, ct_hash( "ZedR2" ) )->buff_data->start_time < 1.f )
                return true;
            break;
        default:
            break;
        }

        //if (g_features->buff_cache->has_buff_of_type(index, e_buff_type::invulnerability))
        //   return true;

        /*g_features->buff_cache->has_buff(
             index,
             {
                 ct_hash( "KayleR" ),
                 ct_hash( "ChronoRevive" ),
                 ct_hash( "TaricR" )
             })
         return true;*/


        if ( unit->get_targetable_flag( ) == 16 && rt_hash( unit->champion_name.text ) == ct_hash( "Tryndamere" ) &&
            unit->health < 100.f )
            return true;

        return false;
    }


    auto TargetSelector::get_target_priority( const std::string champion_name ) -> int32_t{
        auto priority{ 0 };
        switch ( rt_hash( champion_name.data( ) ) ) {
        case ct_hash( "Aatrox" ):
            priority = g_config->target_priorities.aatrox_priority->get< int >( );
            break;

        case ct_hash( "Ahri" ):
            priority = g_config->target_priorities.ahri_priority->get< int >( );
            break;

        case ct_hash( "Akali" ):
            priority = g_config->target_priorities.akali_priority->get< int >( );
            break;

        case ct_hash( "Akshan" ):
            priority = g_config->target_priorities.akshan_priority->get< int >( );
            break;

        case ct_hash( "Alistar" ):
            priority = g_config->target_priorities.alistar_priority->get< int >( );
            break;

        case ct_hash( "Amumu" ):
            priority = g_config->target_priorities.amumu_priority->get< int >( );
            break;

        case ct_hash( "Anivia" ):
            priority = g_config->target_priorities.anivia_priority->get< int >( );
            break;

        case ct_hash( "Annie" ):
            priority = g_config->target_priorities.annie_priority->get< int >( );
            break;

        case ct_hash( "Aphelios" ):
            priority = g_config->target_priorities.aphelios_priority->get< int >( );
            break;

        case ct_hash( "Ashe" ):
            priority = g_config->target_priorities.ashe_priority->get< int >( );
            break;

        case ct_hash( "AurelionSol" ):
            priority = g_config->target_priorities.aurelionson_priority->get< int >( );
            break;

        case ct_hash( "Azir" ):
            priority = g_config->target_priorities.azir_priority->get< int >( );
            break;

        case ct_hash( "Bard" ):
            priority = g_config->target_priorities.bard_priority->get< int >( );
            break;

        case ct_hash( "BelVeth" ):
            priority = g_config->target_priorities.belveth_priority->get< int >( );
            break;

        case ct_hash( "Blitzcrank" ):
            priority = g_config->target_priorities.blitzcrank_priority->get< int >( );
            break;

        case ct_hash( "Brand" ):
            priority = g_config->target_priorities.brand_priority->get< int >( );
            break;

        case ct_hash( "Braum" ):
            priority = g_config->target_priorities.braum_priority->get< int >( );
            break;

        case ct_hash( "Caitlyn" ):
            priority = g_config->target_priorities.caitlyn_priority->get< int >( );
            break;

        case ct_hash( "Camille" ):
            priority = g_config->target_priorities.camille_priority->get< int >( );
            break;

        case ct_hash( "Cassiopeia" ):
            priority = g_config->target_priorities.cassiopeia_priority->get< int >( );
            break;

        case ct_hash( "Chogath" ):
            priority = g_config->target_priorities.chogath_priority->get< int >( );
            break;

        case ct_hash( "Corki" ):
            priority = g_config->target_priorities.corki_priority->get< int >( );
            break;

        case ct_hash( "Darius" ):
            priority = g_config->target_priorities.darius_priority->get< int >( );
            break;

        case ct_hash( "Diana" ):
            priority = g_config->target_priorities.diana_priority->get< int >( );
            break;

        case ct_hash( "Draven" ):
            priority = g_config->target_priorities.draven_priority->get< int >( );
            break;

        case ct_hash( "DrMundo" ):
            priority = g_config->target_priorities.drmundo_priority->get< int >( );
            break;

        case ct_hash( "Ekko" ):
            priority = g_config->target_priorities.ekko_priority->get< int >( );
            break;

        case ct_hash( "Elise" ):
            priority = g_config->target_priorities.elise_priority->get< int >( );
            break;

        case ct_hash( "Evelynn" ):
            priority = g_config->target_priorities.evelynn_priority->get< int >( );
            break;

        case ct_hash( "Ezreal" ):
            priority = g_config->target_priorities.ezreal_priority->get< int >( );
            break;

        case ct_hash( "Fiddlesticks" ):
            priority = g_config->target_priorities.fiddlesticks_priority->get< int >( );
            break;

        case ct_hash( "Fiora" ):
            priority = g_config->target_priorities.fiora_priority->get< int >( );
            break;

        case ct_hash( "Fizz" ):
            priority = g_config->target_priorities.fizz_priority->get< int >( );
            break;

        case ct_hash( "Galio" ):
            priority = g_config->target_priorities.galio_priority->get< int >( );
            break;

        case ct_hash( "Gangplank" ):
            priority = g_config->target_priorities.gangplank_priority->get< int >( );
            break;

        case ct_hash( "Garen" ):
            priority = g_config->target_priorities.garen_priority->get< int >( );
            break;

        case ct_hash( "Gnar" ):
            priority = g_config->target_priorities.gnar_priority->get< int >( );
            break;

        case ct_hash( "Gragas" ):
            priority = g_config->target_priorities.gragas_priority->get< int >( );
            break;

        case ct_hash( "Graves" ):
            priority = g_config->target_priorities.graves_priority->get< int >( );
            break;

        case ct_hash( "Gwen" ):
            priority = g_config->target_priorities.gwen_priority->get< int >( );
            break;

        case ct_hash( "Hecarim" ):
            priority = g_config->target_priorities.hecarim_priority->get< int >( );
            break;

        case ct_hash( "Heimerdinger" ):
            priority = g_config->target_priorities.heimerdinger_priority->get< int >( );
            break;

        case ct_hash( "Illaoi" ):
            priority = g_config->target_priorities.illaoi_priority->get< int >( );
            break;

        case ct_hash( "Irelia" ):
            priority = g_config->target_priorities.irelia_priority->get< int >( );
            break;

        case ct_hash( "Ivern" ):
            priority = g_config->target_priorities.ivern_priority->get< int >( );
            break;

        case ct_hash( "Janna" ):
            priority = g_config->target_priorities.janna_priority->get< int >( );
            break;

        case ct_hash( "JarvanIV" ):
            priority = g_config->target_priorities.jervaniv_priority->get< int >( );
            break;

        case ct_hash( "Jax" ):
            priority = g_config->target_priorities.jax_priority->get< int >( );
            break;

        case ct_hash( "Jayce" ):
            priority = g_config->target_priorities.jayce_priority->get< int >( );
            break;

        case ct_hash( "Jhin" ):
            priority = g_config->target_priorities.jhin_priority->get< int >( );
            break;

        case ct_hash( "Jinx" ):
            priority = g_config->target_priorities.jinx_priority->get< int >( );
            break;

        case ct_hash( "KaiSa" ):
            priority = g_config->target_priorities.kaisa_priority->get< int >( );
            break;

        case ct_hash( "Kalista" ):
            priority = g_config->target_priorities.kalista_priority->get< int >( );
            break;

        case ct_hash( "Karma" ):
            priority = g_config->target_priorities.karma_priority->get< int >( );
            break;

        case ct_hash( "Karthus" ):
            priority = g_config->target_priorities.karthus_priority->get< int >( );
            break;

        case ct_hash( "Kassadin" ):
            priority = g_config->target_priorities.kassadin_priority->get< int >( );
            break;

        case ct_hash( "Katarina" ):
            priority = g_config->target_priorities.katarina_priority->get< int >( );
            break;

        case ct_hash( "Kayle" ):
            priority = g_config->target_priorities.kayle_priority->get< int >( );
            break;

        case ct_hash( "Kayn" ):
            priority = g_config->target_priorities.kayn_priority->get< int >( );
            break;

        case ct_hash( "Kennen" ):
            priority = g_config->target_priorities.kennen_priority->get< int >( );
            break;

        case ct_hash( "KhaZix" ):
            priority = g_config->target_priorities.khazix_priority->get< int >( );
            break;

        case ct_hash( "Kindred" ):
            priority = g_config->target_priorities.kindred_priority->get< int >( );
            break;

        case ct_hash( "Kled" ):
            priority = g_config->target_priorities.kled_priority->get< int >( );
            break;

        case ct_hash( "KogMaw" ):
            priority = g_config->target_priorities.kogmaw_priority->get< int >( );
            break;

        case ct_hash( "KSante" ):
            priority = g_config->target_priorities.ksante_priority->get< int >( );
            break;

        case ct_hash( "LeBlanc" ):
            priority = g_config->target_priorities.leblanc_priority->get< int >( );
            break;

        case ct_hash( "LeeSin" ):
            priority = g_config->target_priorities.leesin_priority->get< int >( );
            break;

        case ct_hash( "Leona" ):
            priority = g_config->target_priorities.leona_priority->get< int >( );
            break;

        case ct_hash( "Lillia" ):
            priority = g_config->target_priorities.lillia_priority->get< int >( );
            break;

        case ct_hash( "Lissandra" ):
            priority = g_config->target_priorities.lissandra_priority->get< int >( );
            break;

        case ct_hash( "Lucian" ):
            priority = g_config->target_priorities.lucian_priority->get< int >( );
            break;

        case ct_hash( "Lulu" ):
            priority = g_config->target_priorities.lulu_priority->get< int >( );
            break;

        case ct_hash( "Lux" ):
            priority = g_config->target_priorities.lux_priority->get< int >( );
            break;

        case ct_hash( "Malphite" ):
            priority = g_config->target_priorities.malphite_priority->get< int >( );
            break;

        case ct_hash( "Malzahar" ):
            priority = g_config->target_priorities.malzahar_priority->get< int >( );
            break;

        case ct_hash( "Maokai" ):
            priority = g_config->target_priorities.maokai_priority->get< int >( );
            break;

        case ct_hash( "MasterYi" ):
            priority = g_config->target_priorities.masteryi_priority->get< int >( );
            break;

        case ct_hash( "Milio" ):
            priority = g_config->target_priorities.milio_priority->get< int >( );
            break;

        case ct_hash( "MissFortune" ):
            priority = g_config->target_priorities.missfortune_priority->get< int >( );
            break;

        case ct_hash( "Mordekaiser" ):
            priority = g_config->target_priorities.mordekaiser_priority->get< int >( );
            break;

        case ct_hash( "Morgana" ):
            priority = g_config->target_priorities.morgana_priority->get< int >( );
            break;

        case ct_hash( "Nami" ):
            priority = g_config->target_priorities.nami_priority->get< int >( );
            break;

        case ct_hash( "Nasus" ):
            priority = g_config->target_priorities.nasus_priority->get< int >( );
            break;

        case ct_hash( "Nautilus" ):
            priority = g_config->target_priorities.nautilus_priority->get< int >( );
            break;

        case ct_hash( "Neeko" ):
            priority = g_config->target_priorities.neeko_priority->get< int >( );
            break;

        case ct_hash( "Nidalee" ):
            priority = g_config->target_priorities.nidalee_priority->get< int >( );
            break;

        case ct_hash( "Nilah" ):
            priority = g_config->target_priorities.nilah_priority->get< int >( );
            break;

        case ct_hash( "Nocturne" ):
            priority = g_config->target_priorities.nocturne_priority->get< int >( );
            break;

        case ct_hash( "Nunu" ):
            priority = g_config->target_priorities.nunu_priority->get< int >( );
            break;

        case ct_hash( "Olaf" ):
            priority = g_config->target_priorities.olaf_priority->get< int >( );
            break;

        case ct_hash( "Orianna" ):
            priority = g_config->target_priorities.orianna_priority->get< int >( );
            break;

        case ct_hash( "Ornn" ):
            priority = g_config->target_priorities.ornn_priority->get< int >( );
            break;

        case ct_hash( "Phanteon" ):
            priority = g_config->target_priorities.pantheon_priority->get< int >( );
            break;

        case ct_hash( "Poppy" ):
            priority = g_config->target_priorities.poppy_priority->get< int >( );
            break;

        case ct_hash( "Pyke" ):
            priority = g_config->target_priorities.pyke_priority->get< int >( );
            break;

        case ct_hash( "Qiyana" ):
            priority = g_config->target_priorities.qiyana_priority->get< int >( );
            break;

        case ct_hash( "Quinn" ):
            priority = g_config->target_priorities.quinn_priority->get< int >( );
            break;

        case ct_hash( "Rakan" ):
            priority = g_config->target_priorities.rakan_priority->get< int >( );
            break;

        case ct_hash( "Rammus" ):
            priority = g_config->target_priorities.rammus_priority->get< int >( );
            break;

        case ct_hash( "RekSai" ):
            priority = g_config->target_priorities.reksai_priority->get< int >( );
            break;

        case ct_hash( "Rell" ):
            priority = g_config->target_priorities.rell_priority->get< int >( );
            break;

        case ct_hash( "RenataGlasc" ):
            priority = g_config->target_priorities.renataglasc_priority->get< int >( );
            break;

        case ct_hash( "Renekton" ):
            priority = g_config->target_priorities.renekton_priority->get< int >( );
            break;

        case ct_hash( "Rengar" ):
            priority = g_config->target_priorities.rengar_priority->get< int >( );
            break;

        case ct_hash( "Riven" ):
            priority = g_config->target_priorities.riven_priority->get< int >( );
            break;

        case ct_hash( "Rumble" ):
            priority = g_config->target_priorities.rumble_priority->get< int >( );
            break;

        case ct_hash( "Ryze" ):
            priority = g_config->target_priorities.ryze_priority->get< int >( );
            break;

        case ct_hash( "Samira" ):
            priority = g_config->target_priorities.samira_priority->get< int >( );
            break;

        case ct_hash( "Sejuani" ):
            priority = g_config->target_priorities.sejuani_priority->get< int >( );
            break;

        case ct_hash( "Senna" ):
            priority = g_config->target_priorities.senna_priority->get< int >( );
            break;

        case ct_hash( "Seraphine" ):
            priority = g_config->target_priorities.seraphine_priority->get< int >( );
            break;

        case ct_hash( "Sett" ):
            priority = g_config->target_priorities.sett_priority->get< int >( );
            break;

        case ct_hash( "Shaco" ):
            priority = g_config->target_priorities.shaco_priority->get< int >( );
            break;

        case ct_hash( "Shen" ):
            priority = g_config->target_priorities.shen_priority->get< int >( );
            break;

        case ct_hash( "Shyvana" ):
            priority = g_config->target_priorities.shyvana_priority->get< int >( );
            break;

        case ct_hash( "Singed" ):
            priority = g_config->target_priorities.singed_priority->get< int >( );
            break;

        case ct_hash( "Sion" ):
            priority = g_config->target_priorities.sion_priority->get< int >( );
            break;

        case ct_hash( "Sivir" ):
            priority = g_config->target_priorities.sivir_priority->get< int >( );
            break;

        case ct_hash( "Skarner" ):
            priority = g_config->target_priorities.skarner_priority->get< int >( );
            break;

        case ct_hash( "Sona" ):
            priority = g_config->target_priorities.sona_priority->get< int >( );
            break;

        case ct_hash( "Soraka" ):
            priority = g_config->target_priorities.soraka_priority->get< int >( );
            break;

        case ct_hash( "Swain" ):
            priority = g_config->target_priorities.swain_priority->get< int >( );
            break;

        case ct_hash( "Sylas" ):
            priority = g_config->target_priorities.sylas_priority->get< int >( );
            break;

        case ct_hash( "Syndra" ):
            priority = g_config->target_priorities.syndra_priority->get< int >( );
            break;

        case ct_hash( "Tahmkench" ):
            priority = g_config->target_priorities.tahmkench_priority->get< int >( );
            break;

        case ct_hash( "Taliyah" ):
            priority = g_config->target_priorities.taliyah_priority->get< int >( );
            break;

        case ct_hash( "Talon" ):
            priority = g_config->target_priorities.talon_priority->get< int >( );
            break;

        case ct_hash( "Taric" ):
            priority = g_config->target_priorities.taric_priority->get< int >( );
            break;

        case ct_hash( "Teemo" ):
            priority = g_config->target_priorities.teemo_priority->get< int >( );
            break;

        case ct_hash( "Thresh" ):
            priority = g_config->target_priorities.thresh_priority->get< int >( );
            break;

        case ct_hash( "Tristana" ):
            priority = g_config->target_priorities.tristana_priority->get< int >( );
            break;

        case ct_hash( "Trundle" ):
            priority = g_config->target_priorities.trundle_priority->get< int >( );
            break;

        case ct_hash( "Tryndamere" ):
            priority = g_config->target_priorities.tryndamere_priority->get< int >( );
            break;

        case ct_hash( "TwistedFate" ):
            priority = g_config->target_priorities.twistedfate_priority->get< int >( );
            break;

        case ct_hash( "Twitch" ):
            priority = g_config->target_priorities.twitch_priority->get< int >( );
            break;

        case ct_hash( "Udyr" ):
            priority = g_config->target_priorities.udyr_priority->get< int >( );
            break;

        case ct_hash( "Urgot" ):
            priority = g_config->target_priorities.urgot_priority->get< int >( );
            break;

        case ct_hash( "Varus" ):
            priority = g_config->target_priorities.varus_priority->get< int >( );
            break;

        case ct_hash( "Vayne" ):
            priority = g_config->target_priorities.vayne_priority->get< int >( );
            break;

        case ct_hash( "Veigar" ):
            priority = g_config->target_priorities.veigar_priority->get< int >( );
            break;

        case ct_hash( "VelKoz" ):
            priority = g_config->target_priorities.velkoz_priority->get< int >( );
            break;

        case ct_hash( "Vex" ):
            priority = g_config->target_priorities.vex_priority->get< int >( );
            break;

        case ct_hash( "Vi" ):
            priority = g_config->target_priorities.vi_priority->get< int >( );
            break;

        case ct_hash( "Viego" ):
            priority = g_config->target_priorities.viego_priority->get< int >( );
            break;

        case ct_hash( "Viktor" ):
            priority = g_config->target_priorities.viktor_priority->get< int >( );
            break;

        case ct_hash( "Vladimir" ):
            priority = g_config->target_priorities.vladimir_priority->get< int >( );
            break;

        case ct_hash( "Volibear" ):
            priority = g_config->target_priorities.volibear_priority->get< int >( );
            break;

        case ct_hash( "Warwick" ):
            priority = g_config->target_priorities.warwick_priority->get< int >( );
            break;

        case ct_hash( "MonkeyKing" ):
            priority = g_config->target_priorities.monkeyking_priority->get< int >( );
            break;

        case ct_hash( "Xayah" ):
            priority = g_config->target_priorities.xayah_priority->get< int >( );
            break;

        case ct_hash( "Xerath" ):
            priority = g_config->target_priorities.xerath_priority->get< int >( );
            break;

        case ct_hash( "XinZhao" ):
            priority = g_config->target_priorities.xinzhao_priority->get< int >( );
            break;

        case ct_hash( "Yasuo" ):
            priority = g_config->target_priorities.yasuo_priority->get< int >( );
            break;

        case ct_hash( "Yone" ):
            priority = g_config->target_priorities.yone_priority->get< int >( );
            break;

        case ct_hash( "Yorick" ):
            priority = g_config->target_priorities.yorick_priority->get< int >( );
            break;

        case ct_hash( "Yuumi" ):
            priority = g_config->target_priorities.yuumi_priority->get< int >( );
            break;

        case ct_hash( "Zac" ):
            priority = g_config->target_priorities.zac_priority->get< int >( );
            break;

        case ct_hash( "Zed" ):
            priority = g_config->target_priorities.zed_priority->get< int >( );
            break;

        case ct_hash( "Zeri" ):
            priority = g_config->target_priorities.zeri_priority->get< int >( );
            break;

        case ct_hash( "Ziggs" ):
            priority = g_config->target_priorities.ziggs_priority->get< int >( );
            break;

        case ct_hash( "Zilean" ):
            priority = g_config->target_priorities.zilean_priority->get< int >( );
            break;

        case ct_hash( "Zoe" ):
            priority = g_config->target_priorities.zoe_priority->get< int >( );
            break;

        case ct_hash( "Zyra" ):
            priority = g_config->target_priorities.zyra_priority->get< int >( );
            break;

        default:
            break;
        }

        if ( priority > 0 ) return priority;

        if ( std::ranges::find( m_low_priority_targets, champion_name ) != m_low_priority_targets.end( ) ) return 1;

        if ( std::ranges::find( m_medium_priority_targets, champion_name ) != m_medium_priority_targets.end( ) ) {
            return
                2;
        }

        if ( std::ranges::find( m_high_priority_targets, champion_name ) != m_high_priority_targets.end( ) ) return 3;

        if ( std::ranges::find( m_highest_priority_targets, champion_name ) != m_highest_priority_targets.
            end( ) )
            return 4;

        return 1;
    }

    auto TargetSelector::get_orbwalker_default_target( const bool secondary ) -> Object*{
        // Function is used in LUA, message @tore if you change args
        if ( secondary ) {
            if ( !m_secondary_target ||
                !m_secondary_target->is_valid( ) ||
                !m_secondary_target->get( )
            )
                return nullptr;

            auto& obj = g_entity_list.get_by_index( m_secondary_target->get( )->index );
            if ( !obj ) return nullptr;
            obj.update( );

            m_secondary_target = &obj;
            return obj.get( );
        }

        if ( !m_default_target || !m_default_target->is_valid( ) || !m_default_target->get( ) ) return nullptr;

        auto& obj = g_entity_list.get_by_index( m_default_target->get( )->index );
        if ( !obj ) return nullptr;
        obj.update( );

        m_default_target = &obj;
        return obj.get( );
    }

    auto TargetSelector::get_forced_target( ) -> Object*{
        // Function is used in LUA, message @tore if you change args
        if ( !m_forced_target || !m_forced_target->get( ) ) return nullptr;

        auto& obj = g_entity_list.get_by_index( m_forced_target->get( )->index );
        if ( !obj ) return m_forced_target->get( );

        obj.update( );
        // auto copy = obj->create_updated_copy( );

        m_forced_target = &obj;
        return obj.get( );
    }

    auto TargetSelector::set_forced_target( const int16_t index ) -> bool{
        auto& obj = g_entity_list.get_by_index( index );
        if ( !obj ) return false;

        return set_forced_target( &obj );
    }

    auto TargetSelector::set_forced_target( Object* object ) -> bool{
        return set_forced_target( CHolder::from_object( object ) );
    }

    auto TargetSelector::set_forced_target( CHolder* holder ) -> bool{
        // Function is used in LUA, message @tore if you change args
        holder->update( );

        if ( holder->get( )->is_dead( ) ) return false;

        m_forced_target    = holder;
        m_last_force_time  = *g_time;
        m_is_target_forced = true;

        return m_is_target_forced;
    }

    auto TargetSelector::reset_forced_target( ) -> void{
        // Function is used in LUA, message @tore if you change args
        m_forced_target    = nullptr;
        m_last_force_time  = 0.f;
        m_is_target_forced = false;
    }

    auto TargetSelector::get_default_target( const bool secondary ) -> Object*{
        // Function is used in LUA, message @tore if you change args
        //if ( m_override_target && m_override_target.value( ) && m_override_target.value( )->is_valid( ) ) return m_override_target.value( )->get( );

        return m_is_target_forced ? get_forced_target( ) : get_orbwalker_default_target( secondary );
    }
}
