#include "pch.hpp"

#include "orbwalker.hpp"

#include "buff_cache.hpp"
#include "entity_list.hpp"
#include "evade.hpp"
#include "prediction.hpp"
#include "target_selector/target_selector.hpp"
#include "tracker.hpp"
#if enable_new_lua
#include "../lua-v2/state.hpp"
#endif
#include "../sdk/globals.hpp"
#include "../utils/c_function_caller.hpp"
#include "..\utils\memory_holder.hpp"
#include "../utils/input.hpp"
#include "champion_modules/module.hpp"
#include "../sdk/game/pw_hud.hpp"
#include "../sdk/game/ai_manager.hpp"
// #include "../sdk/game/buff.hpp"
#include "../sdk/game/hud_manager.hpp"

#include "../sdk/game/spell_cast_info.hpp"
#include "../sdk/game/spell_info.hpp"
#include "../sdk/game/spell_data.hpp"
// #include "../sdk/game/unit_info_component.hpp"
#if enable_new_lua
#include "../lua-v2/cancelable.hpp"
#endif
#include "../utils/directory_manager.hpp"
#include "../utils/path.hpp"

#include "../sdk/game/buff_info.hpp"
#include "../sdk/game/render_manager.hpp"

namespace features {
    constexpr float HOLD_RADIUS{ 65.f };

#if enable_lua
    Orbwalker::OnSpellCastedT::OnSpellCastedT( const i32 slot, const std::string& name, const i16 target ):
        spell_slot( slot ),
        spell_name( name ){ if ( auto t = g_entity_list.get_by_index( target ).get( ) ) object = t; }
#endif

    auto Orbwalker::run( ) -> void{
#if enable_lua
        if ( g_lua2 ) {
            g_lua2->execute_locked( []( ) -> void{ g_lua2->run_callback( ct_hash( "features.orbwalker" ) ); } );
        }
#endif

        update_orb_mode( );
        update_color_state( );

        if ( m_cast_duration > *g_time && m_cast_duration - *g_time >= 5.f ) {
            debug_log(
                "[ !ALERT! ] Monstrous cast duration: {} | diff: {} | t: {}",
                m_cast_duration,
                m_cast_duration - *g_time,
                *g_time
            );

            m_cast_duration = 0.f;
        }

        if ( !should_run( ) ) return;

        // std::cout << "Q: " << std::hex << g_local->spell_book.get_spell_slot( ESpellSlot::q ).get_address( )
        //          << std::endl;

        update_autospacing( );
        update_next_attack_times( );
        update_ignored( );
        update_aa_missile( );

        update_last_hittable_glow( );

        if ( !m_did_debug_print ) {
            debug_log( "aimgr: {:x}", g_local->get_ai_manager().get_address() );
            debug_log( "unitcomponentinfo: {:x}", g_local->get_unit_info_component().get_address() );

            debug_log( "local index: {} network_id: {} team: {}", g_local->index, g_local->network_id, g_local->team );

            // auto spell = g_local->spell_book.get_spell_slot( ESpellSlot::q );
            // if ( spell ) std::cout << "Local SpellQ: " << std::hex << spell.get_address( ) << std::endl;

            m_did_debug_print = true;
        }

        //get_automatic_position();

        // developer_thing( );

        auto sci = g_local->spell_book.get_spell_cast_info( );

        // dont run orbwalker if local recalling
        if ( sci && static_cast< ESpellSlot >( sci->slot ) == ESpellSlot::recall ) {
            m_mode = EOrbwalkerMode::recalling;
            return;
        }

        // TODO: THIS IS THE AWARENESS ONLY CHANGE ONLY
        // return;

        auto aimgr = g_local->get_ai_manager( );
        // if ( aimgr && aimgr->is_moving && aimgr->is_dashing ) std::cout << "dash speed: " << aimgr->dash_speed <<
        // std::endl;

        /*if (aimgr && aimgr->path_end.dist_to(last_path_end) > 5.f)
        {


            std::cout << "[ PATH ] Updated to " << aimgr->path_end.x << ", " << aimgr->path_end.y << ", "
                      << aimgr->path_end.z << std::endl;


            last_path_end = aimgr->path_end;
        }

        auto turget = g_features->target_selector->get_default_target();
        if (turget && turget->spell_book.get_spell_cast_info())
        {
            auto spellcast = turget->spell_book.get_spell_cast_info();
            if (!spellcast) return;

            std::cout << turget->get_name() << " casting " << spellcast->get_spell_name()
                      << " | cooldown: " << spellcast->get_spell_info()->get_spell_data()->get_cooldown_duration(1)
                      << std::endl;



        }*/

        //print_local_buffs();
#if __DEBUG
        if ( GetAsyncKeyState( 0x39 ) ) print_target_buffs( );
        if ( GetAsyncKeyState( 0x37 ) ) print_local_buffs( );

        if (g_config->misc.exploit_enabled->get<bool>() && *g_time - m_last_move_time > 0.1f && !m_pressed &&
            (GetAsyncKeyState(0x31) || GetAsyncKeyState(0x32) || GetAsyncKeyState(0x33)))
        {
            return;

            Vec3 pos = { 11448, 42.5728, 5602 }; //{ 8472, -69.4714, 5908 };


            //4324, -71.2406, 10206
            //6874, 51.6295, 3158

            GetAsyncKeyState(0x33);
            GetAsyncKeyState(0x32);

            if (GetAsyncKeyState(0x33))
                pos = { 4574, 56.5184, 12056 };
            else if (GetAsyncKeyState(0x32))
                pos = { 12256, 51.7294, 5040 }; //{ 12254, 51.7223, 5054 };

            if (g_input->issue_order_move(pos, true)) {
                m_last_position = pos;
                m_last_move_time = *g_time;
            }


            

            //8472, -69.4714, 5908
            //8472, -53.7527, 5770

            return;

            auto slot = ESpellSlot::q;

            switch ( g_config->misc.exploit_slot->get< int >( ) ) {
            case 1:
                slot = ESpellSlot::w;
                break;
            case 2:
                slot = ESpellSlot::e;
                break;
            case 3:
                slot = ESpellSlot::r;
                break;
            default:
                break;
            }

            Vec3 extended_position{ };
            bool argless{ };

            switch ( g_config->misc.exploit_type->get< int >( ) ) {
            case 0:
                extended_position = { 0.f, 0.f, 0.f };
                break;
            case 1:
                extended_position = Vec3( 5000000.f, 5000000.f, 5000000.f );
                break;
            case 2:
                argless = true;
                break;
            default:
                break;
            }

            if ( argless ) g_input->cast_spell( slot );
            else g_input->cast_spell( slot, extended_position );


            std::cout << "[ Exploit ] Casted spell at [ " << extended_position.x << ", " << extended_position.y << ", "
                << extended_position.z << " ] | Argless: " << argless << std::endl;


            m_pressed = true;
        } else if ( m_pressed && !GetAsyncKeyState( 0x32 ) ) m_pressed = false;

#endif

        // run autocombo module
        if ( g_features->current_module ) g_features->current_module->run( );

        //TrundleTrollSmash

        switch ( helper::get_current_hero( ) ) {
        case EHeroes::janna:
        case EHeroes::malzahar:
            if ( sci && sci->slot == 3 ) return;
            break;
        case EHeroes::akshan:
        {
            auto spell = g_local->spell_book.get_spell_slot( ESpellSlot::e );
            if ( spell && rt_hash( spell->get_name( ).data( ) ) != ct_hash( "AkshanE" ) ) return;

            break;
        }
        case EHeroes::kog_maw:
            if ( m_kogmaw_in_passive && g_local
                ->
                attack_range > 0.f
            )
                m_kogmaw_in_passive = false;
            break;
        case EHeroes::riven:
            if ( std::get< 0 >( m_riven_aa_reset ) ) {
                if ( !g_features->buff_cache->get_buff( g_local->index, ct_hash( "riventricleavesoundone" ) ) ) {
                    std::get< 0 >( m_riven_aa_reset ) = false;
                }
            } else if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "riventricleavesoundone" ) ) ) {
                reset_aa_timer( );
                std::get< 0 >( m_riven_aa_reset ) = true;
            }

            if ( std::get< 1 >( m_riven_aa_reset ) ) {
                if ( !g_features->buff_cache->get_buff( g_local->index, ct_hash( "riventricleavesoundtwo" ) ) ) {
                    std::get< 1 >( m_riven_aa_reset ) = false;
                }
            } else if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "riventricleavesoundtwo" ) ) ) {
                reset_aa_timer( );
                std::get< 1 >( m_riven_aa_reset ) = true;
            }

            if ( std::get< 2 >( m_riven_aa_reset ) ) {
                if ( !g_features->buff_cache->get_buff( g_local->index, ct_hash( "riventricleavesoundthree" ) ) ) {
                    std::get< 2 >( m_riven_aa_reset ) = false;
                }
            } else if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "riventricleavesoundthree" ) ) ) {
                reset_aa_timer( );
                std::get< 2 >( m_riven_aa_reset ) = true;
            }


            break;
        case EHeroes::ashe:
            if ( m_attack_reset ) {
                if ( !g_features->buff_cache->get_buff( g_local->index, ct_hash( "AsheQAttack" ) ) ) {
                    m_attack_reset =
                        false;
                }
            } else if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "AsheQAttack" ) ) ) {
                reset_aa_timer( );
                m_attack_reset = true;
            }
            break;
        case EHeroes::trundle:
        {
            if ( m_attack_reset ) {
                if ( !g_features->buff_cache->
                                  get_buff( g_local->index, ct_hash( "TrundleTrollSmash" ) ) )
                    m_attack_reset = false;
            } else if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "TrundleTrollSmash" ) ) ) {
                reset_aa_timer( );
                m_attack_reset = true;
            }
            break;
        }
        case EHeroes::vayne:
            if ( m_attack_reset ) {
                if ( !g_features->buff_cache->get_buff( g_local->index, ct_hash( "vaynetumblebonus" ) ) ) {
                    m_attack_reset =
                        false;
                }
            } else if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "vaynetumblebonus" ) ) ) {
                reset_aa_timer( );
                m_attack_reset = true;
            }
            break;
        case EHeroes::xerath:
            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "XerathLocusOfPower2" ) ) ) return;
            break;
        case EHeroes::blitzcrank:
            if ( m_attack_reset ) {
                if ( !g_features->buff_cache->get_buff( g_local->index, ct_hash( "PowerFist" ) ) ) {
                    m_attack_reset =
                        false;
                }
            } else if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "PowerFist" ) ) ) {
                reset_aa_timer( );
                m_attack_reset = true;
            }
            break;
        case EHeroes::zeri:
            if ( m_attack_reset ) {
                if ( !g_features->buff_cache->get_buff( g_local->index, ct_hash( "ZeriESpecialRounds" ) ) ) {
                    m_attack_reset =
                        false;
                }
            } else if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "ZeriESpecialRounds" ) ) ) {
                reset_aa_timer( );
                m_attack_reset = true;
            }
            break;
        case EHeroes::jax:
            if ( m_attack_reset ) {
                if ( !g_features->buff_cache->get_buff( g_local->index, ct_hash( "JaxEmpowerTwo" ) ) ) {
                    m_attack_reset =
                        false;
                }
            } else if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "JaxEmpowerTwo" ) ) ) {
                reset_aa_timer( );
                m_attack_reset = true;
            }
            break;
        case EHeroes::kayle:
            if ( m_attack_reset ) {
                if ( !g_features->buff_cache->get_buff( g_local->index, ct_hash( "KayleE" ) ) ) m_attack_reset = false;
            } else if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "KayleE" ) ) ) {
                reset_aa_timer( );
                m_attack_reset = true;
            }
            break;
        case EHeroes::kassadin:
            if ( m_attack_reset ) {
                if ( !g_features->buff_cache->get_buff( g_local->index, ct_hash( "NetherBlade" ) ) ) {
                    m_attack_reset =
                        false;
                }
            } else if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "NetherBlade" ) ) ) {
                reset_aa_timer( );
                m_attack_reset = true;
            }
            break;
        case EHeroes::dr_mundo:
            if ( m_attack_reset ) {
                if ( !g_features->buff_cache->get_buff( g_local->index, ct_hash( "DrMundoE" ) ) ) {
                    m_attack_reset =
                        false;
                }
            } else if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "DrMundoE" ) ) ) {
                reset_aa_timer( );
                m_attack_reset = true;
            }
            break;
        case EHeroes::karthus:
            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "KarthusDeathDefiedBuff" ) ) ) return;
            break;
        case EHeroes::lucian:
        {
            auto mgr = g_local->get_ai_manager( );
            if ( mgr ) {
                if ( m_was_dashing && !mgr->is_dashing ) m_was_dashing = false;
                else if ( !m_was_dashing && mgr->is_dashing && mgr->is_moving ) {
                    m_was_dashing = true;
                    reset_aa_timer( );
                }
            }
            break;
        }
        case EHeroes::viktor:
            if ( m_attack_reset ) {
                if ( !g_features->buff_cache->
                                  get_buff(
                                      g_local->index,
                                      ct_hash( "ViktorPowerTransferReturn" )
                                  ) )
                    m_attack_reset = false;
            } else if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "ViktorPowerTransferReturn" ) ) ) {
                reset_aa_timer( );
                m_attack_reset = true;
            }
            break;
        case EHeroes::sivir:
            if ( m_attack_reset ) {
                if ( !g_features->buff_cache->get_buff( g_local->index, ct_hash( "SivirWMarker" ) ) ) {
                    m_attack_reset =
                        false;
                }
            } else if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "SivirWMarker" ) ) ) {
                reset_aa_timer( );
                m_attack_reset = true;
            }
            break;
        case EHeroes::jhin:
            if ( sci && sci->slot == 3 ) return;
            break;
        case EHeroes::kindred:
            if ( m_attack_reset ) {
                if ( !g_features->buff_cache->get_buff( g_local->index, ct_hash( "kindredqasbuff" ) ) ) {
                    m_attack_reset =
                        false;
                }
            } else if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "kindredqasbuff" ) ) ) {
                reset_aa_timer( );
                m_attack_reset = true;
            }
            break;
        case EHeroes::belveth:
            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "BelvethE" ) ) ) return;
            break;
        case EHeroes::olaf:
            if ( m_attack_reset ) {
                if ( !g_features->buff_cache->
                                  get_buff( g_local->index, ct_hash( "OlafFrenziedStrikes" ) ) )
                    m_attack_reset = false;
            } else if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "OlafFrenziedStrikes" ) ) ) {
                reset_aa_timer( );
                m_attack_reset = true;
            }
            break;
        case EHeroes::sett:
            if ( m_attack_reset ) {
                if ( !g_features->buff_cache->get_buff( g_local->index, ct_hash( "SettQ" ) ) ) m_attack_reset = false;
            } else if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "SettQ" ) ) ) {
                reset_aa_timer( );
                m_attack_reset = true;
            }
            break;
        case EHeroes::sion:
            if ( sci && ( sci->slot == 3 || sci->slot == 0 ) ) return;
            break;
        case EHeroes::darius:
            if ( m_attack_reset ) {
                if ( !g_features->buff_cache->
                                  get_buff( g_local->index, ct_hash( "DariusNoxianTacticsONH" ) ) ) {
                    m_attack_reset =
                        false;
                }
            } else if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "DariusNoxianTacticsONH" ) ) ) {
                reset_aa_timer( );
                m_attack_reset = true;
            }
            break;
        case EHeroes::illaoi:
            if ( m_attack_reset ) {
                if ( !g_features->buff_cache->get_buff( g_local->index, ct_hash( "IllaoiW" ) ) ) m_attack_reset = false;
            } else if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "IllaoiW" ) ) ) {
                reset_aa_timer( );
                m_attack_reset = true;
            }
            break;
        case EHeroes::yorick:
            if ( m_attack_reset ) {
                if ( !g_features->buff_cache->get_buff( g_local->index, ct_hash( "yorickqbuff" ) ) ) {
                    m_attack_reset =
                        false;
                }
            } else if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "yorickqbuff" ) ) ) {
                reset_aa_timer( );
                m_attack_reset = true;
            }
            break;
        case EHeroes::caitlyn:
            update_overrides( );
            break;
        case EHeroes::kalista:
        {
            if ( g_config->kalista.jump_exploit->get< bool >( ) && is_movement_disabled( ) ) {
                if ( !sci && !in_attack( ) && !m_should_move && !m_allow_move && !m_sent_move
                    || m_mode == EOrbwalkerMode::combo && GetAsyncKeyState( VK_CONTROL ) ) {
                    m_last_move_time = *g_time + 0.3f;
                    allow_movement( true );
                    //std::cout << "Reallowed movement\n";
                }
            }

            break;
        }
        case EHeroes::aurelion_sol:
        {
            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "AurelionSolQ" ) ) ) return;

            auto slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
            if ( slot && rt_hash( slot->get_name( ).data( ) ) == ct_hash( "AurelionSolWToggle" ) ) return;


            break;
        }
        case EHeroes::fiora:
            if ( m_attack_reset ) {
                if ( !g_features->buff_cache->get_buff( g_local->index, ct_hash( "FioraE" ) ) ) m_attack_reset = false;
            } else if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "FioraE" ) ) ) {
                reset_aa_timer( );
                m_attack_reset = true;
            }
            break;
        default:
            break;
        }

        update_debuff_state( );

        //if (g_features->evade->is_active()) m_should_fast_move = true;

        switch ( m_mode ) {
        case EOrbwalkerMode::combo:
        {
            auto target = get_special_target( );
            
            if ( !target ) {
                target = get_orbwalker_target( );

                if ( !target || !is_attackable( target->index ) && !g_features->target_selector->is_forced( ) ) {
                    target = get_special_target_low_priority( );

                    if ( helper::get_current_hero( ) == EHeroes::kalista && g_config->kalista.combo_attack_unit->get<
                            bool >( )
                        && ( !target || !is_attackable( target->index ) ) )
                        target = get_laneclear_target( true );
                }
            }

            if(target) {
                debug_log( "orbwalker has target" );
            }

            run_orbwalker( target );
            break;
        }
        case EOrbwalkerMode::lasthit:
            run_orbwalker( get_lasthit_target( ) );
            break;
        case EOrbwalkerMode::freeze:
            run_orbwalker( get_freeze_target( ) );
            break;
        case EOrbwalkerMode::laneclear:
        {
            Object* target;
            if ( can_attack_turret( ) ) {
                target = get_lasthit_target( );
                if ( !target ) target = get_turret_target( );
            } else {
                target = get_laneclear_target( g_input->is_key_pressed( utils::EKey::control ) );

                if ( !target && !helper::is_position_under_turret( g_local->position ) ) {
                    target =
                        get_orbwalker_target( );
                }
            }

            run_orbwalker( target );
            break;
        }
        case EOrbwalkerMode::harass:
        {
            auto target = get_xayah_passive_harass_target( );

            if ( !target ) {
                target = g_input->is_key_pressed( utils::EKey::control ) ? get_freeze_target( ) : get_lasthit_target( );

                if ( !target ) target = get_orbwalker_target( );
            }

            run_orbwalker( target );
            break;
        }
        case EOrbwalkerMode::flee:
            if ( can_move( ) ) send_move_input( );
            break;
        default:
            break;
        }

        if ( *g_time - m_last_data_update_time > 0.25f || g_local->attack_speed != m_last_attack_speed ) {
            m_last_attack_speed = g_local->attack_speed;
            m_attack_cast_delay = g_function_caller->attack_cast_delay( );
            m_attack_delay      = g_function_caller->attack_delay( );

            if ( m_ping > 0 && m_ping != m_last_ping ) {
                m_pings.push_back( { m_ping, *g_time } );
                m_last_ping = m_ping;
            }

            m_ping = g_function_caller->ping( );

            m_last_data_update_time = *g_time;
        }
    }

    auto Orbwalker::send_attack( const uintptr_t network_id ) -> bool{
        // Function is used in LUA, message @tore if you change args
        if ( !g_input->issue_order_attack( network_id ) ) return false;

        m_in_attack        = true;
        m_last_attack_time = *g_time;
        disable_movement_until( *g_time + 0.016f );

#if __DEBUG
       // std::cout << "[ orbwalker ] ordered Autoattack | " << *g_time << std::endl;

#endif

        if ( helper::get_current_hero( ) == EHeroes::kalista && g_config->kalista.jump_exploit->get< bool >( ) ) {
            m_should_move = true;
            allow_movement( false );
        }

        return true;
    }

    auto Orbwalker::should_attack_target( const Object* target ) const -> bool{
        if ( m_in_attack || *g_time < m_last_attack_time + m_attack_delay || g_features->evade->is_active( ) ||
            in_action( ) )
            return false;

        if ( g_local->position.dist_to( target->position ) > g_local->attack_range + 90.f ) return false;

        return true;
    }

    auto Orbwalker::can_attack( const int16_t index ) const -> bool{
        // Function is used in LUA, message @tore if you change args
        if ( !is_autoattack_allowed( ) || *g_time - m_last_attack_time <= 0.02f || g_features->evade->is_active( true ) ||
            m_in_attack || m_restrict_attacks )
            return false;

        if ( helper::get_current_hero( ) == EHeroes::caitlyn && is_unit_headshottable( index ) ) return true;

        if ( in_action( ) && m_cast_duration > *g_time + get_ping( ) || m_movement_impaired || m_blinded ||
            m_polymorph )
            return false;

        if (helper::get_current_hero() != EHeroes::kalista) {

            const auto aimgr = g_local->get_ai_manager();
            if (aimgr) {

                if (aimgr->is_dashing && aimgr->is_moving && g_local->position.dist_to(aimgr->path_end) > aimgr->dash_speed * get_ping( ) )
                    return false;
            }
        }

        bool override_attack{ };
        auto is_attack_ready = *g_time + ( g_features->evade->is_active( )  ? get_ping() / 2.f : get_ping( ) / 2.f + 0.066f ) >= m_last_attack_time + m_attack_delay;

        switch ( helper::get_current_hero( ) ) {
        case EHeroes::kaisa:
            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "KaisaE" ) ) ||
                g_features->buff_cache->get_buff( g_local->index, ct_hash( "KaisaEStealth" ) ) )
                return false;
            break;
        case EHeroes::xerath:
            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "XerathArcanopulseChargeUp" ) ) ) {
                return
                    false;
            }
            break;
        case EHeroes::varus:
            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "VarusQLaunch" ) ) ) return false;
            break;
        case EHeroes::pyke:
            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "PykeQ" ) ) ) return false;
            break;
        case EHeroes::graves:
            if ( !g_features->buff_cache->get_buff( g_local->index, ct_hash( "gravesbasicattackammo1" ) ) &&
                !g_features->buff_cache->get_buff( g_local->index, ct_hash( "gravesbasicattackammo2" ) ) )
                return false;

            if ( g_local->bonus_attack_speed > 1.5f ) {
                is_attack_ready = *g_time > m_last_attack_time + m_attack_delay *
                    0.4f;
            } else is_attack_ready = *g_time > m_last_attack_time + m_attack_delay * 0.6f;


            break;
        case EHeroes::jhin:
            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "JhinPassiveReload" ) ) ) return false;
            break;
        case EHeroes::xayah:
            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "XayahR" ) ) ) return false;
            break;
        case EHeroes::nilah:
            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "NilahR" ) ) ) return false;
            break;
        case EHeroes::tristana:
            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "TristanaW" ) ) ) return false;
            break;
        case EHeroes::darius:
            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "dariusqcast" ) ) ) return false;
            break;
        case EHeroes::garen:
            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "GarenE" ) ) ) return false;
            break;
        case EHeroes::urgot:
            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "UrgotW" ) ) ) return false;
            break;
        case EHeroes::caitlyn:
            if ( is_unit_headshottable( index ) ) override_attack = true;
            break;
        case EHeroes::vladimir:
            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "VladimirE" ) ) ) return false;
            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "VladimirSanguinePool" ) ) ) return false;
            break;
        case EHeroes::samira:
            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "SamiraR" ) ) ||
                g_features->buff_cache->get_buff( g_local->index, ct_hash( "SamiraW" ) ) )
                return false;

            if ( samira_can_override_attack( index ) ) override_attack = true;
            break;
        case EHeroes::kalista:
        {
            auto aimgr = g_local->get_ai_manager( );
            if ( aimgr && aimgr->is_dashing && aimgr->is_moving ) return false;

            is_attack_ready = *g_time > m_last_attack_time + m_attack_delay;
            break;
        }
        case EHeroes::zoe:
        {
            const auto target = g_entity_list.get_by_index( index );
            if ( target && target->is_hero( ) ) {
                if ( g_config->zoe.aa_passive_check->get< bool >( )
                    && !g_features->buff_cache->get_buff( g_local->index, ct_hash( "zoepassivesheenbuff" ) ) )
                    return false;
            }

            break;
        }
        case EHeroes::miss_fortune:
            is_attack_ready = *g_time > m_last_attack_time + m_attack_delay;
            break;
        case EHeroes::kog_maw:
        {
            const auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "KogMawIcathianSurprise" ) );
            if ( buff ) {
                const auto time_left = buff->buff_data->end_time - *g_time;

                if ( time_left > 3.25f + get_ping( ) / 2.f || time_left - m_attack_cast_delay < 2.9f +
                    get_ping( ) )
                    return false;


                return true;
            }
            break;
        }
        default:
            break;
        }

        return is_attack_ready || override_attack;
    }

    auto Orbwalker::run_orbwalker( const Object* target ) -> void{
        if ( target ) {
            auto aa_allowed{ true };

            if ( helper::get_current_hero( ) == EHeroes::zeri ) {
                aa_allowed = g_config->zeri.allow_aa_on_fullcharge->get< bool >( ) &&
                    g_features->buff_cache->get_buff( g_local->index, ct_hash( "ZeriQPassiveReady" ) ) ||
                    g_config->zeri.allow_aa_on_low_hp_enemy->get< bool >( ) && ( target->is_lane_minion( ) || target->
                        is_jungle_monster( ) || target->is_hero( ) && helper::get_real_health(
                            target->index,
                            EDamageType::magic_damage,
                            m_attack_cast_delay,
                            true
                        ) < helper::get_aa_damage( target->index, false ) ) ||
                    target->is_minion( ) && target->is_misc_minion( ) || target->is_plant( ) || target->is_barrel( );

                if ( g_config->zeri.dont_aa_minion_on_full_charge->get< bool >( ) && !target->is_plant( ) && !target->
                    is_ward( ) &&
                    g_features->buff_cache->get_buff( g_local->index, ct_hash( "ZeriQPassiveReady" ) ) &&
                    ( target->is_lane_minion( ) || target->is_jungle_monster( ) ) )
                    aa_allowed = false;
            }

            if ( target->is_hero( ) && g_config->orbwalker.disable_aa_level->get< int >( ) < g_local->level ) {
                aa_allowed = false;
            }

           // debug_log( "can_attack: {} | is_attackable: {}", can_attack( target->index ), is_attackable(target->index) );

            

            if ( aa_allowed && can_attack( target->index ) && is_attackable( target->index ) &&
                send_attack( target->network_id ) ) {
                m_attack_damage = helper::get_aa_damage( target->index, helper::get_current_hero( ) != EHeroes::zeri );
                m_last_target_index = target->index;

               // CreateAnimation(g_local->position, target->position, Color(255, 255, 255), 0.5f, 2.f);

                //std::cout << "[ orbwalker: attack ] predicted damage: " << m_attack_damage << std::endl;

                if ( m_is_overridden ) clear_target_override( );

                return;
            }
        }

        send_move_input( );
    }

    auto Orbwalker::get_orbwalker_target( ) -> Object*{
        if ( m_is_overridden ) return get_overridden_target( );

        auto target = g_features->target_selector->get_default_target( );

        if ( g_features->target_selector->is_forced( ) ) {
            const auto default_target = g_features->target_selector->get_orbwalker_default_target( );

            if ( false && default_target  ) {
                // if forced target is out of aa range and default target is inside aa range -> attack default instead of forced
                if ( target->index != default_target->index && g_local->position.dist_to( target->position ) > g_local->
                    attack_range + 90.f
                    && g_local->position.dist_to( default_target->position ) <= g_local->attack_range + 90.f )
                    target = default_target;
            }
        }

        return target;
    }

    auto Orbwalker::get_overridden_target( ) -> Object*{
        if ( !m_is_overridden ) return { };

        if ( m_override_expire_time < *g_time ) {
            clear_target_override( );
            //std::cout << "[ orb ] cleared target override due to expire\n";
            return { };
        }

        auto& target = g_entity_list.get_by_index( m_override_target_index );

        if ( !target ) return { };

        target.update( );

        return target.get( );
    }


    auto Orbwalker::get_special_target( ) -> Object*{
        if ( m_is_overridden ) return get_overridden_target( );

        Object* target{ };
        bool    target_found{ };
        bool    is_gangplank{ };
        auto    barrel_lasthit_time{ 4.f };
        bool    is_local_gangplank{ };


        if ( helper::get_current_hero( ) == EHeroes::gangplank ) {
            is_local_gangplank = true;

            if ( g_local->level >= 13 ) barrel_lasthit_time = 1.f;
            else if ( g_local->level >= 7 ) barrel_lasthit_time = 2.f;
        } else {
            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || rt_hash( enemy->champion_name.text ) != ct_hash( "Gangplank" ) ) continue;

                is_gangplank = true;

                if ( enemy->level >= 13 ) barrel_lasthit_time = 1.f;
                else if ( enemy->level >= 7 ) barrel_lasthit_time = 2.f;

                break;
            }
        }

        if ( is_local_gangplank ) {
            for ( const auto obj : g_entity_list.get_enemy_minions( ) ) {
                if ( !obj || obj->is_dead( ) || obj->is_invisible( ) || !is_attackable( obj->index ) ||
                    is_ignored( obj->index ) || !obj->is_barrel( ) || obj->health <= 2.f )
                    continue;

                // auto attack_time = m_attack_cast_delay + get_ping( ) / 2.f;
                // if ( m_autoattack_missile_speed > 0 ) {
                //     attack_time += g_local->position.dist_to( obj->position ) /
                //         m_autoattack_missile_speed;
                // }

                const auto buff = g_features->buff_cache->get_buff( obj->index, ct_hash( "gangplankebarrelactive" ) );
                if ( !buff || !buff->buff_data ) continue;

                const auto time_alive         = *g_time - buff->buff_data->start_time;
                const auto time_till_killable = barrel_lasthit_time - time_alive;

                const auto health_tick_delay = barrel_lasthit_time / 2.f;
                const auto next_health_tick  = time_alive > health_tick_delay
                                                   ? time_alive + health_tick_delay * 2.f
                                                   : time_alive + health_tick_delay;
                const auto simulated_health = *g_time + m_attack_delay + m_attack_cast_delay * 2.f + get_ping( ) >=
                                              next_health_tick
                                                  ? obj->health - 1.f
                                                  : obj->health;

                if ( simulated_health < 2.f || barrel_lasthit_time <= m_attack_delay + m_attack_cast_delay +
                    get_ping( ) )
                    break;

                if ( time_till_killable > m_attack_cast_delay + get_ping( ) * 2.f ) {
                    target = obj;
                    break;
                }
            }
        } else if ( is_gangplank ) {
            for ( const auto obj : g_entity_list.get_enemy_minions( ) ) {
                if ( !obj || !obj->is_barrel( ) || obj->is_dead( ) || obj->is_invisible( ) ||
                    !is_attackable( obj->index ) || is_ignored( obj->index ) || obj->health > 2 )
                    continue;

                const auto owner = g_entity_list.get_by_index( obj->get_owner_index( ) );
                if ( !owner || owner->team == g_local->team ) continue;


                auto attack_time = m_attack_cast_delay + get_ping( ) / 2.f;
                if ( m_autoattack_missile_speed > 0 ) {
                    attack_time += g_local->position.dist_to( obj->position ) /
                        m_autoattack_missile_speed;
                }

                if ( obj->health == 2.f ) {
                    auto buffs = obj->buff_manager.get_all( );

                    for ( auto data : buffs ) {
                        if ( !data ) continue;

                        auto info = data->get_buff_info( );
                        if ( !info ||
                            rt_hash( info->get_name().c_str(  ) ) != ct_hash( "gangplankebarrelactive" ) &&
                            rt_hash( info->get_name().c_str(  ) ) != ct_hash( "gangplankebarrellife" ) )
                            continue;


                        const auto time_alive = *g_time - data->start_time;

                        // debug_log( "buff: {} alive time: {}", info->name, time_alive );

                        if ( time_alive + attack_time >= barrel_lasthit_time ) {
                            target       = obj;
                            target_found = true;
                        }

                        break;
                    }

                    if ( target_found ) break;

                    continue;
                }

                target = obj;
                break;
            }
        }

        return target;
    }

    auto Orbwalker::get_special_target_low_priority( ) -> Object*{
        if ( m_is_overridden ) return get_overridden_target( );

        if ( helper::get_current_hero( ) == EHeroes::illaoi ) {
            std::vector< float > healths{ };

            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->is_dead( ) || enemy->dist_to_local( ) > 3000.f ) continue;

                healths.push_back( enemy->max_health );
            }

            for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
                if ( !minion || minion->dist_to_local( ) > g_local->attack_range + 200.f || minion->get_owner_index( )
                    != g_local->index || minion->is_normal_minion( )
                    || minion->get_selectable_flag( ) != 1 )
                    continue;

                bool found{ };

                for ( const auto max_hp : healths ) {
                    if ( max_hp == minion->max_health ) {
                        found = true;
                        break;
                    }
                }

                if ( !found ) continue;

                return minion;
            }
        }

        if ( helper::get_current_hero( ) == EHeroes::senna ) return get_senna_soul_target( );

        return { };
    }

    auto Orbwalker::get_lasthit_target( ) -> Object*{
        if ( m_is_overridden ) return get_overridden_target( );

        Object*             target{ };
        Object::EMinionType type{ };
        bool                should_prepare{ };
        bool                lasthittable_found{ };
        bool                has_target{ };

        bool target_require_spellfarm{ };

        // cassiopeia specific
        const auto is_cassiopeia = helper::get_current_hero( ) == EHeroes::cassiopeia;
        const auto has_e_ready   = is_cassiopeia && g_local->spell_book.get_spell_slot( ESpellSlot::e )->is_ready( );
        const auto e_level       = g_local->spell_book.get_spell_slot( ESpellSlot::e )->level;

        // todo: we should use std::sort here
        for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
            if ( !minion ||
                minion->is_dead( ) ||
                minion->is_invisible( ) ||
                !minion->is_normal_minion( ) ||
                !g_config->orbwalker.attack_plants->get< bool >( ) &&
                minion->is_plant( ) ||
                !is_attackable( minion->index )
            )
                continue;

            if ( support_limiter_active( ) && minion->is_lane_minion( ) ) continue;

            auto travel_time = get_aa_missile_speed( minion ) <= 0.f
                                   ? 0.f
                                   : g_features->prediction->get_server_position( g_local->index ).dist_to(
                                       minion->position
                                   ) / get_aa_missile_speed( minion );
            if ( g_config->orbwalker.lasthit_predict_ping->get< bool >( ) ) travel_time += get_ping( ) / 2.f + 0.033f;

            const auto lasthit_health = g_features->prediction->predict_minion_health(
                minion->index,
                m_attack_cast_delay + travel_time
            );
            if ( lasthit_health <= 0.f ) continue;

            const auto aa_damage = helper::get_aa_damage(
                minion->index,
                helper::get_current_hero( ) != EHeroes::zeri
            );

            bool spell_damage_included{ };

            if ( is_cassiopeia && g_config->cassiopeia.e_integrate_lasthitting_in_orbwalker->get< bool >( ) &&
                has_e_ready && ( minion->is_lane_minion( ) || minion->is_jungle_monster( ) ) ) {
                auto additional_damage = 48.f + 4.f * static_cast< float >( g_local->level ) + g_local->ability_power( )
                    * 0.1f;

                const auto is_poisoned =
                    !!g_features->buff_cache->get_buff( minion->index, ct_hash( "cassiopeiaqdebuff" ) ) ||
                    !!g_features->buff_cache->get_buff( minion->index, ct_hash( "cassiopeiawpoison" ) );

                const std::vector< float > e_bonus_damage = { 0.f, 20.f, 40.f, 60.f, 80.f, 100.f };

                if ( is_poisoned ) additional_damage += e_bonus_damage[ e_level ] + g_local->ability_power( ) * 0.6f;

                const auto spellfarm_damage = aa_damage + helper::calculate_damage(
                    additional_damage,
                    minion->index,
                    false
                );

                const auto e_delay = 0.125f + g_local->position.dist_to( minion->position ) / 2500.f +
                    g_features->orbwalker->get_ping( ) / 2.f;

                const auto future_spellfarm_health = e_delay <= travel_time
                                                         ? lasthit_health
                                                         : g_features->prediction->predict_minion_health(
                                                             minion->index,
                                                             m_attack_cast_delay + e_delay,
                                                             false
                                                         );


                spell_damage_included = future_spellfarm_health < spellfarm_damage && future_spellfarm_health > 0.f;
            }

            const auto minion_type = minion->get_minion_type( );
            const auto delay       = m_attack_cast_delay + travel_time + get_ping( ) * 0.5f;

            if ( is_in_turret_range( minion ) ) {
                const auto turret = get_nearest_turret( minion );
                if ( !turret ) continue;

                const auto damage_per_shot = get_turret_shot_damage( minion );
                if ( damage_per_shot <= 0.f ) continue;

                const auto health_before_death = minion->health - std::floor( minion->health / damage_per_shot ) *
                    damage_per_shot;

                if ( m_attack_delay + m_attack_cast_delay <= 1.575f ) {
                    if ( health_before_death > aa_damage && g_features->prediction->is_object_turret_target( minion ) &&
                        !
                        lasthittable_found || lasthit_health <= aa_damage ) {
                        if ( lasthit_health <= aa_damage ) lasthittable_found = true;
                        target     = minion;
                        type       = minion_type;
                        has_target = true;
                        continue;
                    }
                } else {
                    const auto turret_target      = get_turret_current_target( turret );
                    const auto next_turret_target = get_turret_next_target( turret );

                    const auto turret_has_next_target = !lasthittable_found && next_turret_target && next_turret_target
                        ->
                        network_id == minion->network_id;
                    const auto will_current_target_die_before_next_attack = turret_target && g_features->prediction->
                        predict_health( turret_target, delay ) - get_turret_shot_damage( turret_target ) <= 0.f;

                    if ( lasthit_health <= aa_damage ) {
                        lasthittable_found = true;
                        should_prepare     = false;
                        target             = minion;
                        type               = minion_type;

                        target_require_spellfarm = spell_damage_included;
                    } else if ( !should_prepare && !will_current_target_die_before_next_attack &&
                        turret_has_next_target )
                        should_prepare = true;
                }

                continue;
            }

            if ( lasthit_health < aa_damage && ( !has_target || has_target && type > minion_type ) ) {
                target             = minion;
                type               = minion_type;
                has_target         = true;
                lasthittable_found = true;

                target_require_spellfarm = false;
            }

            if ( spell_damage_included && ( !has_target || has_target && type > minion_type ) ) {
                target             = minion;
                type               = minion_type;
                has_target         = true;
                lasthittable_found = true;

                target_require_spellfarm = true;
            }
        }

        if ( should_prepare ) {
            const auto turret = get_nearest_turret( g_local.get( ) );
            if ( !turret ) return nullptr;

            auto next_target = get_turret_next_target( turret );
            if ( !next_target ) return nullptr;

            auto turret_shot_damage = get_turret_shot_damage( next_target );
            auto aa_damage = helper::get_aa_damage( next_target->index, helper::get_current_hero( ) != EHeroes::zeri );

            auto health_before_death = next_target->health - std::floor( next_target->health / turret_shot_damage ) *
                turret_shot_damage;
            if ( health_before_death > aa_damage && health_before_death - aa_damage <= aa_damage ) return next_target;

            next_target = get_turret_next_target( turret, next_target->network_id );
            if ( !next_target ) return nullptr;

            turret_shot_damage = get_turret_shot_damage( next_target );
            aa_damage = helper::get_aa_damage( next_target->index, helper::get_current_hero( ) != EHeroes::zeri );

            health_before_death = next_target->health - std::floor( next_target->health / turret_shot_damage ) *
                turret_shot_damage;
            if ( health_before_death > aa_damage && health_before_death - aa_damage <= aa_damage ) return next_target;
        }

        if ( target && can_attack( ) ) {
            if ( target_require_spellfarm ) add_spellfarm_target( target->index );

            m_latest_lasthit_index = target->index;
        }

        return target;
    }

    auto Orbwalker::get_senna_soul_target( ) const -> Object*{
        if ( helper::get_current_hero( ) != EHeroes::senna || m_mode == EOrbwalkerMode::combo && GetAsyncKeyState(VK_CONTROL) // in fullcombo
            || !g_config->senna.combo_collect_souls->get<bool >( ) )
            return { };

        Object* target{ };

        for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
            if ( !minion || minion->is_dead( ) || !minion->is_senna_minion( ) || !
                is_attackable( minion->index ) )
                continue;

            target = minion;
            break;
        }

        return target;
    }

    auto Orbwalker::get_xayah_passive_harass_target( ) const -> Object*{
        if ( !g_config->xayah.passive_harass->get< bool >( ) || helper::get_current_hero( ) != EHeroes::xayah ) {
            return
                nullptr;
        }

        const auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "XayahPassiveActive" ) );
        if ( !buff || buff->buff_data->end_time - *g_time <= m_attack_cast_delay + get_ping( ) ) return nullptr;

        Object* minion_target{ nullptr };

        auto       local_position = g_local->position;
        const auto local_pred     = g_features->prediction->predict_default( g_local->index, get_ping( ) / 2.f, false );
        if ( local_pred ) local_position = *local_pred;

        for ( auto i = 0; i < 2; i++ ) {
            if ( g_features->target_selector->is_forced( ) && i > 0 ) break;

            const auto target = g_features->target_selector->get_default_target( i > 0 );
            if ( !target ) break;

            auto travel_time = m_attack_cast_delay + local_position.dist_to( target->position ) / 4000.f + get_ping( );
            auto pred        = g_features->prediction->predict_default( target->index, travel_time );
            if ( !pred ) continue;

            travel_time = m_attack_cast_delay + local_position.dist_to( *pred ) / 4000.f + get_ping( );
            pred        = g_features->prediction->predict_default( target->index, travel_time );
            if ( !pred || local_position.dist_to( *pred ) > 975.f ) continue;

            const auto compensation = 50.f / target->movement_speed;
            pred = g_features->prediction->predict_default( target->index, travel_time - compensation );
            if ( !pred ) continue;

            for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
                if ( !minion ||
                    minion->is_dead( ) ||
                    minion->is_invisible( ) ||
                    !minion->is_normal_minion( ) ||
                    !g_config->orbwalker.attack_plants->get< bool >( ) &&
                    minion->is_plant( ) ||
                    !is_attackable( minion->index )
                )
                    continue;

                auto minion_pred =
                    g_features->prediction->predict_default( minion->index, m_attack_cast_delay + get_ping( ) );
                if ( !minion_pred ) continue;

                auto minion_position = *minion_pred;

                auto rect = sdk::math::Rectangle(
                    local_position,
                    local_position.extend( minion_position, 1000.f ),
                    60.f
                );
                auto poly = rect.to_polygon( );

                if ( poly.is_outside( *pred ) ) continue;

                minion_target = minion;
                break;
            }

            if ( !minion_target ) continue;

            break;
        }

        return minion_target;
    }

    auto Orbwalker::get_freeze_target( ) -> Object*{
        if ( m_is_overridden ) return get_overridden_target( );

        if ( support_limiter_active( ) ) return nullptr;

        Object*             target{ };
        Object::EMinionType type{ };
        bool                has_target{ };

        // todo: we should use std::sort here
        for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
            if ( !minion || minion->is_dead( ) || !minion->is_visible( ) ||
                !minion->is_lane_minion( ) ||
                is_ignored( minion->index ) ||
                !is_attackable( minion->index ) )
                continue;

            auto travel_time = get_aa_missile_speed( minion ) <= 0.f
                                   ? 0.f
                                   : g_local->position.dist_to( minion->position ) / get_aa_missile_speed(
                                       minion
                                   );

            if ( g_config->orbwalker.lasthit_predict_ping->get< bool >( ) ) travel_time += get_ping( ) / 2.f + 0.033f;

            const auto health_on_aa = g_features->prediction->predict_minion_health(
                minion->index,
                m_attack_cast_delay + travel_time
            );
            const auto health_on_delayed_aa = g_features->prediction->predict_minion_health(
                minion->index,
                m_attack_cast_delay + travel_time + 0.5f
            );
            const auto minion_type = minion->get_minion_type( );

            if ( health_on_aa > 0.f && health_on_delayed_aa <= 25.f && ( !has_target || has_target && type >
                minion_type ) ) {
                target     = minion;
                type       = minion_type;
                has_target = true;
            }
        }

        return target;
    }

    auto Orbwalker::get_recoded_laneclear_target(bool fast_clear) -> Object* {

        Object* target{ };
        Object::EMinionType target_type{};
        float   target_killable{ };
        float   target_will_die_before_next_attack{};
        bool    target_found{};
        float   target_health{};
        float   target_tick_damage{};

        for ( auto minion : g_entity_list.get_enemy_minions() ) {

            if (!minion || minion->dist_to_local() > 1000.f || minion->is_dead() || minion->is_invisible() ||
                !minion->is_normal_minion() || !is_attackable(minion->index) || is_ignored(minion->index) ||
                !g_config->orbwalker.attack_plants->get<bool>() && minion->is_plant() ||
                support_limiter_active() && minion->is_lane_minion())
                continue;

            //Vec3 minion_position = minion->position;
            //auto position = g_features->prediction->predict_default(minion->index, get_ping());
            //if (position) minion_position = position.value();

            auto minion_type = minion->get_minion_type();

            auto travel_time   = get_aa_missile_speed(minion) <= 0.f
                  ? 0.f - get_ping() / 2.f
                  : g_local->position.dist_to(minion->position) / get_aa_missile_speed(minion) - get_ping() / 2.f;

            const auto attack_health =
                g_features->prediction->predict_minion_health(minion->index, m_attack_cast_delay + travel_time);

            if (g_config->orbwalker.lasthit_predict_ping->get<bool>()) travel_time += get_ping() * 0.5f;

            const auto future_health = g_features->prediction->predict_minion_health( minion->index, m_attack_cast_delay + travel_time);
          //  auto possible_health = minion->health -g_features->prediction->simulate_minion_damage(minion->index, m_attack_cast_delay + m_attack_delay + travel_time);

            auto next_aa_health =  minion->health -g_features->prediction->simulate_minion_damage(minion->index, m_attack_cast_delay + m_attack_delay + travel_time);
            auto attack_damage = helper::get_aa_damage(minion->index );

            if (attack_health <= 0.f) continue;

            auto total_tick_damage = g_features->prediction->get_minion_tick_damage( minion->index );

            if ( future_health < attack_damage &&
                (!target_killable || target_type < minion_type ||
                 target_type == minion_type && !target_will_die_before_next_attack && next_aa_health <= 0.f ))
            {

                target        = minion;
                target_health = future_health;
                target_type   = minion_type;
                target_found  = true;
                target_killable = true;
                target_will_die_before_next_attack = next_aa_health <= 0.f;
                continue;
            }

            if ( //! g_features->prediction->should_damage_minion( minion->index, attack_damage, travel_time,
                 //! m_attack_delay )
                 //&& next_aa_health <= attack_damage * 2.f
                !fast_clear && next_aa_health - attack_damage < attack_damage * 0.33f &&
                g_features->prediction->is_minion_in_danger(minion->index)) {

                if ( ( next_aa_health - attack_damage <= 0.f || minion_type == Object::EMinionType::siege ) &&
                    ( !target_killable || target_type < minion_type || 
                        target_type == minion_type && !target_will_die_before_next_attack && next_aa_health <= attack_damage )) {
                    target                             = {};
                    target_type                        = minion_type;
                    target_will_die_before_next_attack = next_aa_health <= attack_damage;
                    target_killable                    = true;
                }

                // target = {};
                // target_killable = true;

                continue;
            }

            if (target_killable) continue;

            if (!target || //total_tick_damage > target_tick_damage || target->health > minion->health && minion_type == target_type ||
                minion_type > target_type || minion_type == Object::EMinionType::jungle && minion->get_monster_priority() > target->get_monster_priority( ) )
            {
                target                   = minion;
                target_health            = future_health;
                target_type              = minion_type;
                target_found             = true;
                target_killable          = false;
                target_tick_damage       = total_tick_damage;
                //safe_delay               = false;
                //monster_priority         = obj->get_monster_priority();
                //target_in_danger         = g_features->prediction->is_minion_in_danger(minion->index);
                //target_require_spellfarm = false;
            }
        }

        return target;
    }

    // todo: fix code style in this function
    // todo: refactor this function so its way smaller
    auto Orbwalker::get_laneclear_target( bool fast_clear ) -> Object*{
        if ( m_is_overridden ) return get_overridden_target( );

        if ( g_config->orbwalker.new_farm_logic->get<bool>( ) && helper::get_current_hero() != EHeroes::zeri )
            return get_recoded_laneclear_target(fast_clear);

        float target_health{ };
        bool  target_lasthittable{ };
        bool  target_found{ };
        bool  target_in_danger{ };
        bool  delay_attack{ };
        bool  safe_delay{ };
        int   safe_count{ };
        float target_next_attack_health{ };

        bool delay_lasthit{ };

        int monster_priority{ };

        bool                target_require_spellfarm{ };
        Object::EMinionType target_type{ };
        Object*             target{ };

        static auto is_zeri{ helper::get_current_hero( ) == EHeroes::zeri };
        auto        is_charged{
            is_zeri ? !!g_features->buff_cache->get_buff( g_local->index, ct_hash( "ZeriQPassiveReady" ) ) : false
        };

        float hard_delay_threshold = g_config->orbwalker.laneclear_priority->get< int >( ) == 2
                                         ? 0.65f
                                         : g_config->orbwalker.laneclear_priority->get< int >( ) == 1
                                               ? 0.85f
                                               : 1.3f;

        float siege_max_delay = g_config->orbwalker.laneclear_priority->get< int >( ) == 2
                                    ? 0.85f
                                    : g_config->orbwalker.laneclear_priority->get< int >( ) == 1
                                          ? 1.5f
                                          : 1.75f;

        float soft_delay_threshold = g_config->orbwalker.laneclear_priority->get< int >( ) == 2
                                         ? 0.6f
                                         : g_config->orbwalker.laneclear_priority->get< int >( ) == 1
                                               ? 0.9f
                                               : 1.3f;


        // cassiopeia specific
        const auto is_cassiopeia = helper::get_current_hero( ) == EHeroes::cassiopeia;
        const auto has_e_ready = is_cassiopeia && g_local->spell_book.get_spell_slot( ESpellSlot::e )->is_ready( true );
        const auto e_level = is_cassiopeia ? g_local->spell_book.get_spell_slot( ESpellSlot::e )->level : 0;

        for ( const auto obj : g_entity_list.get_enemy_minions( ) ) {
            if ( !obj || obj->dist_to_local( ) > 1000.f ||
                obj->is_dead( ) ||
                obj->is_invisible( ) ||
                !obj->is_normal_minion( ) ||
                !is_attackable( obj->index ) ||
                is_ignored( obj->index ) ||
                !g_config->orbwalker.attack_plants->get< bool >( ) && obj->is_plant( ) ||
                support_limiter_active( ) && obj->is_lane_minion( ) )
                continue;

            auto travel_time = get_aa_missile_speed( obj ) <= 0.f
                                   ? 0.f - get_ping( ) / 2.f
                                   : g_local->position.dist_to( obj->position ) / get_aa_missile_speed( obj ) -
                                   get_ping( ) / 2.f;

            const auto basic_attack_damage = helper::get_aa_damage(
                obj->index,
                helper::get_current_hero( ) != EHeroes::zeri
            );
            const auto attack_health = g_features->prediction->predict_minion_health(
                obj->index,
                m_attack_cast_delay + travel_time,
                is_zeri
            );

            if ( g_config->orbwalker.lasthit_predict_ping->get< bool >( ) ) travel_time += get_ping( ) * 0.5f + 0.033f;

            const auto future_health = g_features->prediction->predict_minion_health(
                obj->index,
                m_attack_cast_delay + travel_time,
                is_zeri
            );
            auto possible_health = obj->health - g_features->prediction->simulate_minion_damage(
                obj->index,
                m_attack_cast_delay + m_attack_delay + travel_time
            );

            //const auto next_attack_health = g_features->prediction->predict_minion_health( obj->index, m_attack_cast_delay + m_attack_delay + travel_time, true );
            auto minion_type = obj->get_minion_type( );

            auto is_lasthittable = future_health <= basic_attack_damage;

            float spellfarm_damage{ };
            bool  lasthittable_spellfarm{ };

            if ( is_cassiopeia && g_config->cassiopeia.e_integrate_lasthitting_in_orbwalker->get< bool >( ) &&
                has_e_ready && ( obj->is_lane_minion( ) || obj->is_jungle_monster( ) ) ) {
                auto additional_damage =
                    48.f + 4.f * static_cast< float >( g_local->level ) + g_local->ability_power( ) * 0.1f;

                const auto is_poisoned =
                    !!g_features->buff_cache->get_buff( obj->index, ct_hash( "cassiopeiaqdebuff" ) ) ||
                    !!g_features->buff_cache->get_buff( obj->index, ct_hash( "cassiopeiawpoison" ) );

                if ( is_poisoned ) {
                    const std::vector< float > e_bonus_damage = { 0.f, 20.f, 40.f, 60.f, 80.f, 100.f };

                    additional_damage += e_bonus_damage[ e_level ] + g_local->ability_power( ) * 0.6f;
                }

                spellfarm_damage =
                    basic_attack_damage + helper::calculate_damage( additional_damage, obj->index, false );

                auto e_delay = 0.125f + g_local->position.dist_to( obj->position ) / 2500.f +
                    g_features->orbwalker->get_ping( ) / 2.f;

                const auto future_spellfarm_health = e_delay < travel_time
                                                         ? future_health
                                                         : g_features->prediction->predict_minion_health(
                                                             obj->index,
                                                             m_attack_cast_delay + e_delay,
                                                             false
                                                         );

                lasthittable_spellfarm = future_spellfarm_health < spellfarm_damage && future_spellfarm_health > 0.f;

                if ( lasthittable_spellfarm && ( !delay_lasthit || minion_type >= Object::EMinionType::siege ) &&
                    ( !target_lasthittable ||
                        minion_type > target_type &&
                        ( !target_in_danger && g_features->prediction->is_minion_in_danger( obj->index ) ||
                            minion_type > target_type ||
                            minion_type == target_type && possible_health + 25.f < target_next_attack_health ) ) ) {
                    target                    = obj;
                    target_lasthittable       = true;
                    target_type               = minion_type;
                    safe_delay                = false;
                    delay_attack              = false;
                    delay_lasthit             = false;
                    target_next_attack_health = possible_health;
                    target_require_spellfarm  = true;

                    // if ( delay_lasthit && minion_type >= Object::e_minion_type::siege ) delay_lasthit = false;
                    target_in_danger = g_features->prediction->is_minion_in_danger( obj->index );
                    continue;
                }
            }

            if ( attack_health <= 0.f ) continue;

            if ( is_zeri ) {
                if ( g_config->zeri.dont_aa_minion_on_full_charge->get< bool >( ) && is_charged &&
                    ( target_type < Object::EMinionType::siege || !is_lasthittable ) )
                    continue;

                if ( is_lasthittable && ( !target_lasthittable
                    || ( minion_type > target_type || !target_in_danger && g_features->prediction->
                        is_minion_in_danger( obj->index ) ) ) ) {
                    target              = obj;
                    target_lasthittable = true;
                    target_type         = minion_type;
                    safe_delay          = false;
                    delay_attack        = false;
                    target_in_danger    = g_features->prediction->is_minion_in_danger( obj->index );
                    continue;
                }

                if ( target_lasthittable || !obj->is_misc_minion( ) && !obj->is_plant( ) ) continue;

                if ( !target_found || minion_type > target_type ) {
                    target           = obj;
                    target_health    = obj->health;
                    target_type      = minion_type;
                    target_found     = true;
                    safe_delay       = false;
                    monster_priority = obj->get_monster_priority( );
                    target_in_danger = obj->is_lane_minion( ) && g_features->prediction->is_minion_in_danger(
                        obj->index
                    );
                }

                continue;
            }

            if ( is_lasthittable && ( !delay_lasthit || minion_type >= Object::EMinionType::siege ) &&
                ( !target_lasthittable || minion_type > target_type && ( !target_in_danger && g_features->prediction->
                    is_minion_in_danger( obj->index ) || minion_type >= target_type && possible_health + 10.f <
                    target_next_attack_health ) ) ) {
                target                    = obj;
                target_lasthittable       = true;
                target_type               = minion_type;
                safe_delay                = false;
                delay_attack              = false;
                delay_lasthit             = false;
                target_next_attack_health = possible_health;
                target_require_spellfarm  = false;
                //if ( delay_lasthit && minion_type >= Object::e_minion_type::siege ) delay_lasthit = false;
                target_in_danger = g_features->prediction->is_minion_in_danger( obj->index );
                continue;
            }

            if ( delay_lasthit ) continue;

            if ( !fast_clear && possible_health <= basic_attack_damage * siege_max_delay &&
                g_features->prediction->is_minion_in_danger( obj->index ) &&
                minion_type == Object::EMinionType::siege &&
                ( !target_lasthittable || target_type < minion_type ) ) {
                target           = obj;
                target_type      = minion_type;
                target_in_danger = g_features->prediction->is_minion_in_danger( obj->index );
                delay_lasthit    = true;

                continue;
            }

            if ( target_lasthittable || target_require_spellfarm || delay_attack || delay_lasthit ) continue;

            if ( !fast_clear ) {
                if ( //possible_current_health - basic_attack_damage < basic_attack_damage &&
                    possible_health <= basic_attack_damage * hard_delay_threshold
                    && g_features->prediction->is_minion_in_danger( obj->index ) ) {
                    target                   = { };
                    delay_attack             = true;
                    safe_delay               = false;
                    target_type              = minion_type;
                    target_in_danger         = true;
                    target_require_spellfarm = false;
                    continue;
                }

                if ( possible_health - basic_attack_damage <= basic_attack_damage * soft_delay_threshold &&
                    g_features->prediction->is_minion_in_danger( obj->index ) ) {
                    if ( safe_count > 1 ) {
                        delay_attack = true;
                        safe_delay   = false;
                        target       = { };
                        target_type  = Object::EMinionType::error;
                    } else {
                        target_type = Object::EMinionType::error;
                        safe_delay  = true;
                        safe_count++;
                        target = { };
                    }

                    target_require_spellfarm = false;

                    continue;
                }
            }

            if ( !target_found || target_found && ( minion_type > target_type || monster_priority < obj->
                get_monster_priority( ) || target_health > obj->health && obj->get_monster_priority( ) >=
                monster_priority ) ) {
                target                   = obj;
                target_health            = obj->health;
                target_type              = minion_type;
                target_found             = true;
                safe_delay               = false;
                monster_priority         = obj->get_monster_priority( );
                target_in_danger         = g_features->prediction->is_minion_in_danger( obj->index );
                target_require_spellfarm = false;
            }
        }

        if ( delay_attack || safe_delay || delay_lasthit ) return nullptr;

        if ( !fast_clear && target && is_in_turret_range( target ) ) return get_lasthit_target( );

        if ( target && ( target_lasthittable || target_require_spellfarm ) && can_attack( ) ) {
            if ( target_require_spellfarm ) add_spellfarm_target( target->index );
            m_latest_lasthit_index = target->index;
        }

        return target;
    }

    auto Orbwalker::get_turret_target( ) const -> Object*{
        for ( const auto turret : g_entity_list.get_enemy_turrets( ) ) {
            if ( !turret || !turret->is_turret_object( ) || !is_attackable( turret->index ) ) continue;

            return turret;
        }

        return nullptr;
    }

    auto Orbwalker::on_draw( ) -> void{
        if ( !g_local ) return;

        draw_target_effect( );
        //draw_movement();

#if __DEBUG
        //draw_developer_data();
        //debug_shit( );
        //draw_minion_healthbar();
#endif

        draw_hold_radius( );
        draw_target( );
        draw_turret_last_hit( );

        draw_mode( );
        draw_attack_timer( );

        draw_forced_notification( );

        draw_local_path();
        DrawAnimations();

#if __DEBUG
        developer_draw( );
#endif
    }

    auto Orbwalker::initialize_menu( ) -> void{
        const auto navigation = g_window->push( _( "orbwalker" ), menu_order::orbwalker );
        const auto general    = navigation->add_section( _( "general" ) );
        general->checkbox( _( "enable" ), g_config->orbwalker.toggle );
        general->slider_int( _( "disable aa after level" ), g_config->orbwalker.disable_aa_level, 0, 31 );
        general->select(
            _( "extra attack delay" ),
            g_config->orbwalker.extra_delay,
            { _( "0ms" ), _( "10ms" ), _( "20ms" ), _( "30ms" ), _( "40ms" ), _( "50ms" ) }
        );

        general->checkbox( _( "support mode" ), g_config->orbwalker.support_mode );

        //config_var( ignore_missile_speed, orbwalker, false )

        general->multi_select(
            _( "farm settings " ),
            {
                g_config->orbwalker.lasthit_predict_ping,
                g_config->orbwalker.ignore_missile_speed,
            },
            { _( "Calculate ping" ), _( "Ignore AA speed" ) }
        );

        general->checkbox( _( "increase sensitivity (?)" ), g_config->orbwalker.extrapolate_cursor )
               ->set_tooltip( _( "Makes orbwalk movement feel more responsive" ) );
        //extrapolate_cursor

        const auto modes = navigation->add_section( _( "modes" ) );
        modes->checkbox( _( "combo (hold ctrl for all in)" ), g_config->orbwalker.orbwalker );
        modes->checkbox( _( "last hit (hold ctrl to freeze)" ), g_config->orbwalker.lasthit );
        modes->checkbox( _( "lane clear (hold ctrl to shove)" ), g_config->orbwalker.lane_clear );
        modes->checkbox( _( "harass (hold ctrl to freeze)" ), g_config->orbwalker.harass );
        modes->checkbox( _( "flee" ), g_config->orbwalker.flee );

        const auto target_selector = navigation->add_section( _( "target selector" ) );
        target_selector->select(
            _( "selection logic" ),
            g_config->target_selector.selector_mode,
            { _( "General" ), _( "ADC" ), _( "Nenny" ) }
        );
        target_selector->slider_int(
            _( "prioritize if AA to kill <=" ),
            g_config->target_selector.selector_aa_to_kill_priority,
            0,
            5
        );
        target_selector->checkbox(
            _( "left-click to force target" ),
            g_config->target_selector.selector_left_click_target
        );

        const auto visuals = navigation->add_section( _( "visuals" ) );
        visuals->select(
            _( "draw target mode" ),
            g_config->orbwalker.draw_target_mode,
            { _( "Off" ), _( "Glow" ), _( "Animated" ) }
        );
        visuals->checkbox( _( "draw orbwalker state" ), g_config->orbwalker.draw_mode );
        visuals->checkbox( _( "draw attack timer" ), g_config->orbwalker.draw_attack_timer );
        visuals->checkbox( _( "draw hold radius" ), g_config->orbwalker.draw_hold_radius );
        visuals->checkbox( _( "forced target indicator" ), g_config->orbwalker.draw_leftclick_target_indicator );
        visuals->checkbox( _( "rgb attack range color" ), g_config->orbwalker.draw_rainbow_attack_range );
        visuals->checkbox(_("draw local path"), g_config->orbwalker.draw_path);
        visuals->select(
            _( "local glow" ),
            g_config->orbwalker.glow_preset,
            { _("Off"), _("Divine"), _("Purple"), _("Rainbow"), _("Ares"), _("Zeus"), _("Zeri"), _("Uranium") }
        );

        visuals->checkbox( _( "lasthittable indicator" ), g_config->orbwalker.draw_lasthittable_minions );


        const auto misc = navigation->add_section( _( "misc" ) );
        misc->select(
            _( "delay between moves" ),
            g_config->orbwalker.reaction_time,
            { _( "75ms" ), _( "100ms" ), _( "150ms" ), _( "200ms" ) }
        );

        misc->select(
            _( "laneclear priority" ),
            g_config->orbwalker.laneclear_priority,
            { _( "Lasthits" ), _( "Hybrid" ), _( "Clear speed" ) }
        );

        misc->checkbox( _( "windwall collision check" ), g_config->orbwalker.windwall_check );
        misc->checkbox( _( "attack plants" ), g_config->orbwalker.attack_plants );
        misc->checkbox( _( "use simplified lasthit (?)" ), g_config->orbwalker.ignore_missile_speed )->set_tooltip(
            _( "Enable this if orbwalker is often lasthitting minions too early" )
        );
        misc->checkbox( _( "turrent minion tracker" ), g_config->orbwalker.draw_turret_minion );
        misc->checkbox( _( "^ show current minion aggro" ), g_config->orbwalker.draw_current_minion_tower );
        misc->checkbox( _( "^ show next minion aggro" ), g_config->orbwalker.draw_next_minion_tower );

        const auto advanced = navigation->add_section( _( "advanced" ) );
        advanced->checkbox( _( "melee magnet" ), g_config->orbwalker.melee_magnet );
        advanced->checkbox(_("auto spacing"), g_config->orbwalker.autospacing_toggle);
        advanced->slider_int(_("preferred spacing %"), g_config->orbwalker.spacing_modifier, 10, 80);
        advanced->checkbox(_("new farming logic"), g_config->orbwalker.new_farm_logic);

        //advanced->slider_int(_("GLOW LAYERS"), g_config->orbwalker.glow_layers, 1, 32);
        //advanced->slider_int(_("GLOW SIZE"), g_config->orbwalker.external_glow_size, 1, 10);


        const auto glow = navigation->add_section( _( "glow " ) );
        auto       ar   = glow->checkbox( _( "glow layer 1" ), g_config->orbwalker.enable_first_glow )
                              ->right_click_menu( );
        ar->add_color_picker( _( "COLOR" ), g_config->orbwalker.glow_color );

        auto ar2 =
            glow->checkbox( _( "glow layer 2" ), g_config->orbwalker.enable_second_glow )->right_click_menu( );
        ar2->add_color_picker( _( "COLOR" ), g_config->orbwalker.second_glow_color );

        auto ar3 =
            glow->checkbox( _( "glow layer 3" ), g_config->orbwalker.enable_third_glow )->right_click_menu( );
        ar3->add_color_picker( _( "COLOR" ), g_config->orbwalker.third_glow_color );

        glow->select(
            _( "layer 1 style" ),
            g_config->orbwalker.glow_style,
            { _( "Static" ), _( "Pulse" ), _( "Rainbow" ) }
        );

        glow->slider_int( _( "layer 1 size" ), g_config->orbwalker.glow_size, 0, 20 );
        glow->slider_int( _( "layer 1 diffusion" ), g_config->orbwalker.glow_diffusion, 0, 40 );

        glow->select(
            _( "layer 2 style" ),
            g_config->orbwalker.second_glow_style,
            { _( "Static" ), _( "Pulse" ), _( "Rainbow" ) }
        );
        glow->slider_int( _( "glow 2 size" ), g_config->orbwalker.second_glow_size, 0, 20 );
        glow->slider_int( _( "glow 2 diffusion" ), g_config->orbwalker.second_glow_diffusion, 0, 40 );


        glow->select(
            _( "layer 3 style" ),
            g_config->orbwalker.third_glow_style,
            { _( "Static" ), _( "Pulse" ), _( "Rainbow" ) }
        );
        glow->slider_int( _( "glow 3 size" ), g_config->orbwalker.third_glow_size, 0, 20 );
        glow->slider_int( _( "glow 3 diffusion" ), g_config->orbwalker.third_glow_diffusion, 0, 40 );
    }

    auto lua_run_cancel_check( hash_t action, bool skip_lock ) -> bool{
#if enable_lua
        if ( !g_lua2 ) return false;

        auto cancelable = std::make_shared< lua::CancelableAction >( );

        if ( skip_lock ) {
            g_lua2->run_callback(
                action,
                sol::make_object( g_lua_state2, cancelable )
            );
        } else {
            g_lua2->execute_locked(
                [action, cancelable]( ) -> void{
                    g_lua2->run_callback(
                        action,
                        sol::make_object( g_lua_state2, cancelable )
                    );
                }
            );
        }

        if ( cancelable->should_cancel( ) ) return true;

        return false;
#else
        return false;
#endif
    }

    auto Orbwalker::intl_send_move_input( Vec3 position, bool force, bool should_lock ) -> bool{
        if ( lua_run_cancel_check( ct_hash( "features.orbwalker.send_move_input" ), !should_lock ) ) return false;

        // Function is used in LUA, message @tore if you change args
        if (!force && !can_move())
            return false;
        

        if ( m_last_cursor_position.length( ) <= 0.f ) {
            m_last_cursor_position = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;
        }

        //if ( position.length( ) <= 0.f ) position = g_pw_hud->get_hud_manager( )->cursor_position_clipped;
        Vec3 tether_position{};
        bool is_tethering{};

        bool did_autospace{ };
        if ( !force || helper::get_current_hero( ) == EHeroes::kalista && !g_features->evade->is_active( ) ) {
            //auto post_evade_point = g_features->evade->get_post_evade_point( );

            if (const auto sticky = get_automatic_position(); sticky.has_value())
            {
                position      = sticky.value();
                did_autospace = true;
            }
            else if (g_features->evade->should_tether())
            {
                position = g_features->evade->get_tether_point();
                tether_position = position;
                is_tethering    = true;
            }

            if (!force && !is_tethering && !did_autospace)
            {
                //get_sticky_position( );

                if ( m_kogmaw_in_passive && rt_hash( g_local->champion_name.text )
                    ==
                    ct_hash( "KogMaw" ) && g_local->
                    attack_range == 0.f
                ) {
                    const auto buff = g_features->buff_cache->get_buff(
                        g_local->index,
                        ct_hash( "KogMawIcathianSurprise" )
                    );
                    if ( buff ) {
                        const auto time_left = buff->buff_data->end_time - *g_time;

                        if ( time_left >= 3.3f + get_ping( ) * 2.f ||
                            time_left - m_attack_cast_delay < 2.9f + get_ping( ) ) {
                            const auto magnet = get_kogmaw_magnet( );
                            if ( magnet.has_value( ) ) position = magnet.value( );
                        } else return false;
                    } else {
                        const auto magnet = get_kogmaw_magnet( );
                        if ( magnet.has_value( ) ) position = magnet.value( );
                    }
                }
                else if ( const auto sticky = get_automatic_position( ); sticky.has_value( ) ) {
                    position      = sticky.value( );
                    did_autospace = true;
                } else {
                    const auto magnet = get_magnet_position( );
                    if ( magnet ) position = magnet.value( );
                    else {
                        auto hud = g_pw_hud->get_hud_manager( );
                        if ( !hud ) return false;

                        auto aimgr = g_local->get_ai_manager( );
                        if ( !aimgr ) return false;

                        if ( g_local->position.dist_to( hud->cursor_position_unclipped ) <= HOLD_RADIUS ) {
                            const auto path = aimgr->get_path( );
                            if ( !aimgr->is_moving || path.size( ) == 1 || path.size( ) == aimgr->
                                next_path_node )
                                return false;

                            position = g_local->position;
                            //std::cout << "[ Orbwalker: Holding position ] T: " << *g_time << std::endl;
                        }
                    }
                }
            }
        }

        if ( position.length( ) <= 0.f ) position = get_extrapolated_cursor_position( );

        if ( !g_input->issue_order_move( position, force || m_should_fast_move || did_autospace ) ) return false;

        static std::random_device                   r;
        static std::default_random_engine           e1( r( ) );
        static std::uniform_int_distribution< int > uniform_dist( 0, 30 );

        const auto randomizer = static_cast< float >( uniform_dist( e1 ) ) / 1000.f;

        //bool is_attack_ready = *g_time > m_last_attack_time + m_attack_delay - get_ping( ) * 1.75f;
        // std::cout << "[ orbwalker ] move order | " << *g_time << std::endl;

        did_tether = is_tethering && position == tether_position;

        m_autospacing    = did_autospace;
        m_last_move_time = *g_time + randomizer;
        m_last_position  = position;
        // m_last_move_position = position.length( ) <= 0.f ? cursor : Vec3(0,0,0);

        if ( m_should_fast_move ) {
            m_last_fast_move   = *g_time;
            m_should_fast_move = false;
        }

        return true;
    }

    auto Orbwalker::can_move( ) -> bool{
        // Function is used in LUA, message @tore if you change args

        if ( m_kogmaw_in_passive ) return true;

        if ( g_features->evade->is_active( ) || !is_movement_allowed( ) ||
            m_is_overridden && m_override_require_attack || m_in_attack || m_restrict_movement )
            return false;

        //bool is_attack_ready_soon = *g_time < m_last_attack_time + m_attack_delay - get_ping( ) && *g_time > m_last_attack_time + m_attack_delay - get_ping( ) * 3.f;
        //if ( is_attack_ready_soon ) return false;

        //float next_attack_time = m_last_attack_time + m_attack_delay - get_ping();
        //float next_attack_time_after_move = *g_time + 0.2f;

        //if ( next_attack_time > *g_time - get_ping(  ) * 2.f
        //    && next_attack_time < next_attack_time_after_move ) return false;

        if ( g_features->buff_cache->is_immobile( g_local->index ) ) return false;

        auto ai_manager = g_local->get_ai_manager( );
        if ( !ai_manager ) return false;

        if ( !m_should_fast_move && *g_time - m_last_fast_move > 0.1f ) {
            if ( !ai_manager->is_moving && get_mode( ) !=
                EOrbwalkerMode::none )
                allow_fast_move( );
        }

        float move_delay;
        switch ( g_config->orbwalker.reaction_time->get< int >( ) ) {
        case 0:
            move_delay = 0.05f;
            break;
        case 1:
            move_delay = 0.075f;
            break;
        case 2:
            move_delay = 0.125f;
            break;
        default:
            move_delay = 0.175f;
            break;
        }

        if (m_autospacing) move_delay = 0.04f;

        /*if ( !m_autospacing && !g_features->evade->is_active( ) && get_sticky_position( ).
            has_value( ) )
            m_should_fast_move = true;*/

        const auto allow_move = *g_time > m_last_move_time + move_delay || m_should_fast_move;

        return allow_move;
    }

    auto Orbwalker::update_next_attack_times( ) -> void{
        auto sci = g_local->spell_book.get_spell_cast_info( );

        switch ( helper::get_current_hero( ) ) {
        case EHeroes::viktor:
            if ( !m_attack_reset && g_features->buff_cache->get_buff(
                g_local->index,
                ct_hash( "ViktorPowerTransferReturn" )
            ) )
                m_attack_reset = true;
            break;
        case EHeroes::kalista:
        {
            if ( !g_config->kalista.jump_exploit->get< bool >( ) ) break;

            const auto aimgr = g_local->get_ai_manager( );
            if ( aimgr ) {
                if ( !m_is_dashing ) {
                    if ( aimgr->is_dashing && aimgr->is_moving ) {
                        m_is_dashing = true;

                        //if (*g_time - m_last_attack_server_time < 0.f || sci) std::cout << "[ FLY ]: Success!\n";

                        send_move_input( { }, true );

                        m_sent_move   = false;
                        m_allow_move  = false;
                        m_should_move = false;

                        //std::cout << "DASH: Found after " << *g_time - m_last_attack_server_time << "  VALID: " << m_exploit_valid << std::endl;
                    }
                } else if ( m_is_dashing ) {
                    m_sent_move   = false;
                    m_allow_move  = false;
                    m_should_move = false;

                    if ( !aimgr->is_dashing || !aimgr->is_moving ) {
                        m_is_dashing     = false;
                        m_dash_time      = 0.f;
                        m_last_dash_time = *g_time;
                        m_exploit_valid  = false;
                    }
                }
            }

            break;
        }
        default:
            break;
        }

        if ( !sci ) {
            m_did_print_cast = false;
            if ( m_in_attack && !m_attack_calculated &&
                ( *g_time - m_last_attack_time >= 0.25f || m_last_target_index == 0 ) ) {
                reset_aa_timer( );
                m_in_attack         = false;
                m_attack_damage     = 0;
                m_last_target_index = 0;

                // std::cout << "[ orbwalker ] reset aa timer due to null sci |  " << *g_time << std::endl;
                //g_features->tracker->add_text_popup(g_local->index, "Reset AA timer", color(255, 155, 50));
            } else if ( m_in_attack && m_attack_calculated && helper::get_current_hero( )
                !=
                EHeroes::sett &&
                helper::get_current_hero( ) != EHeroes::akshan
                && m_attack_end_time - 0.1f <= *g_time
            ) {
                m_in_attack            = false;
                m_last_attack_end_time = m_attack_end_time;

                //std::cout << "[ orbwalker ] assumed attack end cause null sci |  " << *g_time << std::endl;
            } else if ( m_in_attack && m_attack_calculated ) {
                // std::cout << "[ orbwalker ] aa ended abruptly. windup end: " << m_attack_end_time << " | T: " << *g_time
                //     << std::endl;

                reset_aa_timer( );
            } else if ( m_attack_calculated && !was_attack_cast ) {
                reset_aa_timer( );
                // std::cout << "[ SCI: Invalid autoattack ] Reset AA time | Windup left: " << m_attack_end_time - *g_time
                //     << " | T: " << *g_time
                //     << std::endl;
            }

            if ( get_mode( ) == EOrbwalkerMode::none && m_attack_calculated && !was_attack_cast ) {
                m_last_attack_time =
                    0;
            }

            was_attack_cast     = false;
            m_attack_calculated = false;

            if ( helper::get_current_hero( ) == EHeroes::kalista ) {
                if ( m_should_move && !m_allow_move && send_move_input( { }, true ) ) {
                    m_should_move = false;
                    m_allow_move  = true;
                }

                if ( ( m_allow_move || m_should_move || m_sent_move ) && *g_time - m_last_attack_ending_time > 0.4f ) {
                    m_allow_move  = false;
                    m_should_move = false;
                    m_sent_move   = false;
                }

                m_valid_attack = false;
            }

            if ( m_jhin_block ) m_jhin_block = false;

            return;
        }

        auto info = sci->get_spell_info( );
        if ( !info ) return;

        auto data = info->get_spell_data( );
        if ( !data ) return;

        if ( !sci->is_autoattack && !sci->is_special_attack ) {

            if ( m_in_attack ) {
                if ( *g_time < m_ignore_spell_expire_time || sci->server_cast_time - *g_time <= get_ping( ) / 2.f && sci->windup_time > 0.1f ) return;

                m_in_attack = false;

                if ( !m_attack_calculated ) {

                    std::cout << "Resetting attack\n";
                    //g_features->tracker->add_text_popup(g_local->index, "Non-AA, resetting", Color(255, 155, 50));
                    reset_aa_timer( );
                }
            }

            if ( m_attack_calculated && !was_attack_cast ) reset_aa_timer( );

            m_attack_calculated = false;
            was_attack_cast     = false;
            m_attack_end_time   = 0.f;

            switch ( rt_hash( data->get_name().c_str() ) ) {
            case ct_hash( "KatarinaE" ):
            case ct_hash( "KatarinaEDagger" ):
                reset_aa_timer( );
                m_cast_duration = sci->server_cast_time;
                break;
            case ct_hash( "SamiraEmpowered" ): // NOT REAL NAME AND NOT FUCKING WORKING
            {
                if ( g_config->samira.passive_orbwalk->get< bool >( ) &&
                    !samira_is_override_logged( sci->get_target_index( ) ) )
                    samira_add_override_instance( sci->get_target_index( ), *g_time + 1.5f );

                break;
            }
            default:

                if ( !m_did_print_cast || data->get_name( ).size( ) != m_last_name.size( ) ) {
                    /* debug_log(
                        "[ {} ] windup: {} start: {}",
                               data->get_name( ),
                               sci->windup_time,
                               sci->start_time
                   );*/

                   //std::cout << "[ " << data->get_name( ) << " ] | start_time: " << sci->start_time << " | sct "
                   //     << sci->server_cast_time << " | end_time: " << sci->end_time << std::endl;

                    // 0.55f
                    // spell debug

                    //ignore_spell_during_attack( sci->server_cast_time );
                    //disable_movement_until( sci->server_cast_time );

                    //if ( rt_hash( data->get_name( ).data( ) ) == ct_hash( "PantheonWEmpoweredAttack" ) ) {
                    //    g_input->cast_spell( ESpellSlot::e, g_pw_hud->get_hud_manager( )->cursor_position_unclipped );
                    //    std::cout << "[ Empowered W-AA ] Order E cast | " << *g_time;
                    //}


                    if ( sci->windup_time > 0.1f ) {
                        m_cast_start = sci->start_position;
                        m_cast_end   = sci->end_position;
                        was_active   = true;
                    }

                    m_did_cast       = true;
                    m_cast_duration  = sci->server_cast_time;
                    m_did_print_cast = true;

                    switch ( rt_hash( data->get_name( ).c_str( ) ) ) {
                    case ct_hash( "XayahQ" ):
                        disable_autoattack_until(
                            sci->server_cast_time + 0.5f * ( 1.f - std::min(
                                g_local->bonus_attack_speed / 2.6f,
                                1.f
                            ) ) - get_ping( )
                        );
                        break;
                    case ct_hash("SennaE"):
                        disable_autoattack_until(sci->server_cast_time - get_ping() / 2.f);
                        break;
                    default:
                        break;
                    }

                    m_last_name = data->get_name( );

                    // local on spellcast callback can trigger here
#if enable_lua
                    auto cast_spell_data = std::make_shared< OnSpellCastedT >(
                        sci->slot,
                        data->get_name( ),
                        sci->get_target_index( )
                    );
                    if ( g_lua2 ) {
                        g_lua2->execute_locked(
                            []( ) -> void{ g_lua2->run_callback( ct_hash( "features.orbwalker.on_spell_casted" ) ); }
                        );
                    }

#endif
                }

                break;
            }

            return;
        }

        if ( m_jhin_block ) return;

        if ( sci->start_time != m_last_attack_time ) {
            if ( sci->start_time < *g_time - 1.f ) {
                m_attack_calculated = true;
                return;
            }

             /*std::cout << "[ orbwalker ] Autoattack started |  server delay: "
                           << static_cast< int >( ( sci->start_time - m_last_attack_ending_time ) * 1000.f )
                      << "ms | MANUAL DELAY: "
                      << static_cast< int >( ( sci->start_time - m_last_attack_time ) * 1000.f )
                      << "ms | last move" << *g_time - m_last_move_time << std::endl;*/

            //int delay = static_cast<int>( ( sci->start_time - m_last_attack_ending_time ) * 1000.f);
            //int ping_delay = static_cast< int >( get_ping( ) * 1000.f );

            //std::cout << "[ SCI: AA ] start time: " << sci->start_time << " | aa delay: " << std::dec << delay << "ms"
            //          << " | PING: " << ping_delay << "ms" << std::endl;

            std::cout << "[ Orbwalker: AA ] New aa detected " << sci->get_spell_name() << std::endl;

            //m_in_attack = true;

            m_last_attack_time        = sci->start_time;
            m_last_attack_index       = sci->get_target_index( );
            m_attack_end_time         = sci->server_cast_time;
            m_last_attack_ending_time = sci->end_time;

            if ( m_last_attack_index <= 0 ) m_last_attack_index = 0;

            

            //m_processing          = true;
            // on local attack callback can trigger here
#if enable_lua
            if ( g_lua2 ) {
                g_lua2->execute_locked(
                    []( ) -> void{ g_lua2->run_callback( ct_hash( "features.orbwalker.on_auto_attacked" ) ); }
                );
            }

#endif
        }

        m_attack_calculated = !was_attack_cast;
        if ( !was_attack_cast ) was_attack_cast = sci->was_autoattack_cast;
        m_autoattack_missile_nid = sci->missile_nid;
       // std::cout << "missile nid: " << std::dec << sci->missile_nid << std::endl;

        auto       attack_finish_time{ sci->server_cast_time };
        const auto atk_name = data->get_name( );
        auto       can_finish_aa{ true };

        switch ( helper::get_current_hero( ) ) {
        case EHeroes::sett:
            if ( rt_hash( atk_name.c_str() ) == ct_hash( "SettBasicAttack" ) || rt_hash( atk_name.c_str() ) == ct_hash(
                    "SettBasicAttack3"
                )
                || rt_hash( atk_name.c_str() ) == ct_hash( "SettQAttack" ) )
                attack_finish_time = sci->server_cast_time + sci->windup_time;

            else if ( rt_hash( atk_name.c_str() ) == ct_hash( "SettBasicAttack2" ) || rt_hash( atk_name.c_str() ) ==
                ct_hash( "SettBasicAttack4" )
                || rt_hash( atk_name.c_str() ) == ct_hash( "SettQAttack2" ) )
                attack_finish_time = sci->server_cast_time;
            break;
        case EHeroes::akshan:
            if ( rt_hash( atk_name.c_str( ) ) == ct_hash( "AkshanBasicAttack" ) ||
                rt_hash( atk_name.c_str( ) ) == ct_hash( "AkshanCritAttack" ) ) {
                attack_finish_time = sci->server_cast_time;
                can_finish_aa      = false;
            } else if ( rt_hash( atk_name.c_str( ) ) == ct_hash( "AkshanPassiveAttack" ) ) {
                attack_finish_time  = sci->server_cast_time;
                m_attack_calculated = true;
            }

            break;
        case EHeroes::jhin:

            if ( g_config->jhin.force_crit->get< bool >( ) && g_local->crit_chance >= 0.55f ) {
                const auto& atk_target = g_entity_list.get_by_index( sci->get_target_index( ) );
                if ( atk_target && atk_target->is_hero( ) ) {
                    if ( rt_hash( atk_name.c_str() ) == ct_hash( "JhinCritAttack" ) ||
                        rt_hash( atk_name.c_str() ) == ct_hash( "JhinPassiveAttack" ) )
                        attack_finish_time = sci->server_cast_time;
                    else {
                        m_in_attack        = false;
                        m_jhin_block       = true;
                        const auto time    = m_attack_delay + get_ping( ) - 0.61f;
                        m_last_attack_time = *g_time - time;
                        return;
                    }
                } else attack_finish_time = sci->server_cast_time;
            } else attack_finish_time = sci->server_cast_time + 0.033f;
            break;
        default:
            break;
        }

        if ( rt_hash( m_last_attack_name.c_str( ) ) != rt_hash( atk_name.c_str( ) ) ) {
            std::cout << "[ AA: " << atk_name
                << " ] " << *g_time << std::endl;
        }

        m_last_attack_name = atk_name;

        if ( g_config->kalista.jump_exploit->get< bool >( ) &&
            helper::get_current_hero( ) == EHeroes::kalista &&
            m_mode != EOrbwalkerMode::none
        ) {
            if ( m_last_attack_start_time < sci->start_time ) {
                if ( send_move_input( { }, true ) ) {
                    allow_movement( false );

                    m_last_attack_start_time  = sci->start_time;
                    m_last_attack_server_time = sci->server_cast_time;
                    m_last_attack_ending_time = sci->end_time;

                    m_allow_move  = false;
                    m_should_move = false;
                    m_sent_move   = true;
                }
            }
        }

        constexpr auto ping_modifier  = 0.5f;
        const auto     ping_reduction = get_ping( ) * ping_modifier;

        //auto attack_target = g_entity_list.get_by_index( sci->get_target_index(  ) );
        //if ( m_kogmaw_in_passive || attack_target && attack_target->is_barrel( ) )
        //    ping_reduction = 0.f;

        if ( m_in_attack && can_finish_aa && *g_time >
            attack_finish_time - ping_reduction //+ g_config->orbwalker.extra_delay->get< int >( ) * 0.01f
        ) {
            m_in_attack            = false;
            m_last_attack_end_time = m_attack_end_time;
            m_attack_end_time      = 0.f;
            was_attack_cast        = true;

            m_should_fast_move = ping_reduction > 0.f && ( helper::get_current_hero( ) != EHeroes::kalista || !g_config
                ->kalista.jump_exploit->get< bool >( ) );

            m_cast_duration = m_attack_end_time;

#if __DEBUG
            // g_features->tracker->add_text_popup(g_local->index, "windup complete", color(50, 200, 25));
            //std::cout << "[ orbwalker ] AA Finished:  " << sci->get_spell_name( ) << " | T: " << *g_time
            //          << " | SCT: " << sci->server_cast_time
            //          << std::endl;
#endif

            //disable_autoattack_until( *g_time + 0.03f );
            disable_movement_until( 0.f );
            allow_fast_move( );

            m_last_attack_name = sci->get_spell_name( );

            const auto target = g_entity_list.get_by_index( m_last_attack_index );
            if ( !target ) return;

            if ( m_spellfarm_target_expire_time > *g_time && target->index == m_spellfarm_target_index ) {
                std::cout << "[ Orbwalker: Spellfarm target confirmed ] | " << *g_time << std::endl;
                m_spellfarm_confirmed          = true;
                m_spellfarm_target_expire_time = *g_time + 0.25f;
                disable_movement_until( m_cast_duration + 0.01f );
            } else if ( m_spellfarm_target_index > 0 ) {
                std::cout << "[ Orbwalker: Spellfarm target Invalid ] | " << *g_time << std::endl;
                m_spellfarm_target_index = 0;
            }

            //if (get_aa_missile_speed(target.get()) > 0.f)
            //    g_features->prediction->add_hero_attack(target->index, helper::get_aa_damage(target->index), sci->server_cast_time, sci->missile_nid, get_aa_missile_speed(target.get()));

            if ( target->is_hero( ) ) {
                m_last_target = ETargetType::hero;

                if ( helper::get_current_hero( ) == EHeroes::caitlyn ) {
                    if ( rt_hash( sci->get_spell_name( ).data( ) ) == ct_hash( "CaitlynPassiveMissile" ) ) {
                        const auto net_buff = g_features->buff_cache->get_buff(
                            target->index,
                            ct_hash( "eternals_caitlyneheadshottracker" )
                        );
                        const auto trap_buff = g_features->buff_cache->get_buff(
                            target->index,
                            ct_hash( "caitlynwsight" )
                        );

                        if ( net_buff && net_buff->buff_data->start_time < sci->start_time - 0.05f &&
                            is_override_allowed( target->index, true ) ) {
                            set_override_cooldown(
                                target->index,
                                true,
                                net_buff->buff_data->end_time
                            );
                        }

                        if ( trap_buff && trap_buff->buff_data->start_time < sci->start_time - 0.05f &&
                            is_override_allowed( target->index, false ) ) {
                            set_override_cooldown(
                                target->index,
                                false,
                                trap_buff->buff_data->end_time
                            );
                        }
                    }

                    disable_autoattack_until( sci->server_cast_time + m_attack_cast_delay );

                    //if ( trap_buff && is_override_allowed( target->index, false ) ) set_override_cooldown( target->index, false, trap_buff->buff_data->end_time );
                    //else if ( net_buff && is_override_allowed( target->index, true ) ) set_override_cooldown( target->index, true, net_buff->buff_data->end_time );
                }

                const auto autoattack_speed = get_aa_missile_speed( target.get( ) );
                if ( autoattack_speed > 0.f ) {
                    g_features->prediction->add_special_attack(
                        target->index,
                        helper::get_aa_damage( target->index, true ),
                        sci->server_cast_time - *g_time + g_local->position.dist_to( target->position ) /
                        get_aa_missile_speed( ),
                        false,
                        ESpellSlot::q,
                        true
                    );
                }
            } else {
                const auto minion_type = target->get_minion_type( );

                switch ( minion_type ) {
                case Object::EMinionType::melee:
                case Object::EMinionType::ranged:
                case Object::EMinionType::siege:
                case Object::EMinionType::super:
                case Object::EMinionType::jungle:
                    m_last_target = ETargetType::minion;
                    break;
                case Object::EMinionType::turret:
                    m_last_target = ETargetType::turret;
                    break;
                case Object::EMinionType::misc:
                case Object::EMinionType::plant:
                    m_last_target = ETargetType::misc;
                    break;
                default:
                    m_last_target = ETargetType::unknown;
                    break;
                }

                const auto attack_successful = m_last_target_index == target->index;

                if ( attack_successful && target->is_normal_minion( ) ) {
                    switch ( minion_type ) {
                    case Object::EMinionType::melee:
                    case Object::EMinionType::ranged:
                    case Object::EMinionType::siege:
                    case Object::EMinionType::super:
                    case Object::EMinionType::jungle:
                    {
                        const auto aa_speed         = get_aa_missile_speed( target.get( ) );
                        const auto attack_cast_time = sci->server_cast_time - *g_time;

                        auto attack_time = attack_cast_time;
                        if ( aa_speed > 0.f ) {
                            auto       missile_traveltime = target->dist_to_local( ) / aa_speed;
                            const auto pred               = g_features->prediction->predict_default(
                                target->index,
                                attack_cast_time + missile_traveltime,
                                false
                            );
                            if ( pred.has_value( ) && pred.value( ).dist_to( target->position ) > 5.f ) {
                                missile_traveltime = g_local->position.dist_to( *pred ) / aa_speed;
                                attack_time        = attack_cast_time + missile_traveltime;
                            } else attack_time = attack_cast_time + missile_traveltime;
                        }

                        const auto health_on_attack_land = g_features->prediction->predict_health(
                            target.get( ),
                            attack_time,
                            false,
                            false,
                            false,
                            true
                        );
                        const auto attack_damage = helper::get_aa_damage(
                            target->index,
                            helper::get_current_hero( ) != EHeroes::zeri
                        );

                        if ( health_on_attack_land <= attack_damage || m_latest_lasthit_index == target->
                            index )
                            ignore_minion( target->index, attack_time + m_attack_cast_delay );
                        else if ( aa_speed > 0.f ) {
                            g_features->prediction->add_special_attack(
                                target->index,
                                attack_damage,
                                attack_time,
                                false,
                                ESpellSlot::q,
                                true
                            );
                        }

                        break;
                    }
                    case Object::EMinionType::plant:
                    {
                        const auto aa_speed    = get_aa_missile_speed( target.get( ) );
                        auto       attack_time = sci->server_cast_time - *g_time;
                        if ( aa_speed > 0.f ) attack_time += target->dist_to_local( ) / aa_speed;

                        if ( target->health - m_attack_damage <= 0.f ) {
                            ignore_minion(
                                target->index,
                                attack_time + 0.5f
                            );
                        }

                        break;
                    }
                    default:
                        break;
                    }

                    m_attack_damage     = 0;
                    m_last_target_index = 0;
                }
            }
        }
        else if ( m_in_attack ) {
            const auto target_index = sci->get_target_index( );
            if ( m_last_attack_index <= 0 || target_index <= 0 || target_index != m_last_attack_index ) return;

            const auto target = g_entity_list.get_by_index( target_index );
            if ( !target || target->is_dead( ) && ( !target->is_turret_object( ) || !target->
                is_turret_attackable( ) ) ) {
                //g_features->tracker->add_text_popup(g_local->index, "Target reset", color(255, 25, 50));

                std::cout << target->get_name( ) << " | AA timer reset, is_dead: " << target->is_dead( ) << std::endl;

                reset_aa_timer( );
                m_in_attack         = false;
                m_attack_damage     = 0;
                m_last_target_index = 0;
            }
        }
    }

    auto Orbwalker::update_aa_missile( ) -> void{
        if ( m_in_attack || m_missile_checked || !should_reset_aa( ) || m_autoattack_missile_nid == 0 ||
            m_autoattack_missile_speed > 0 )
            return;

        for ( const auto missile : g_entity_list.get_ally_missiles( ) ) {
            if ( !missile || missile->network_id != m_autoattack_missile_nid ) continue;

            auto spellinfo = missile->missile_spell_info( );
            if ( !spellinfo ) return;

            auto spelldata = spellinfo->get_spell_data( );
            if ( !spelldata ) return;

            m_autoattack_missile_speed = spelldata->get_missile_speed( );

            if ( m_autoattack_missile_speed == 500.f || helper::get_current_hero( ) ==
                EHeroes::kayle )
                m_autoattack_missile_speed = 0;

            m_missile_checked = true;

            debug_log( "AA Speed: {}", m_autoattack_missile_speed );

            if ( helper::get_current_hero( ) == EHeroes::jinx ) m_autoattack_missile_speed = 2750.f;

            break;
        }
    }

    auto Orbwalker::should_run( ) -> bool{
        if ( !g_config->orbwalker.toggle->get< bool >( ) ) return false;
        if ( g_local->is_dead( ) ) {
            switch ( helper::get_current_hero( ) ) {
            case EHeroes::kog_maw:

                if ( g_local->attack_range == 0.f && !m_kogmaw_in_passive ) {
                    m_kogmaw_in_passive           = true;
                    m_kogmaw_did_death_autoattack = false;
                }

                return g_local->attack_range == 0.f;
            case EHeroes::karthus:
                return g_features->buff_cache->get_buff( g_local->index, ct_hash( "KarthusDeathDefiedBuff" ) );
            default:
                break;
            }

            return false;
        }

        return true;
    }

    auto Orbwalker::is_valid_target( const Object* object ) -> bool{
        if ( !object ) return false;
        if ( object->is_dead( ) || object->is_invisible( ) ) return false;

        return true;
    }

    auto Orbwalker::draw_turret_last_hit( ) const -> void{
        if ( !g_config->orbwalker.draw_turret_minion->get< bool >( ) ) return;

        for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
            if ( !minion ||
                minion->is_dead( ) ||
                minion->is_invisible( ) ||
                !minion->is_normal_minion( ) ||
                !g_config->orbwalker.attack_plants->get< bool >( ) &&
                minion->is_plant( ) ||
                !is_attackable( minion->index )
            )
                continue;

            if ( is_in_turret_range( minion ) ) {
                const auto turret = get_nearest_turret( minion );
                if ( !turret ) continue;
                else {
                    const auto turret_target      = get_turret_current_target( turret );
                    const auto next_turret_target = get_turret_next_target( turret );

                    const auto turret_has_next_target =
                        next_turret_target && next_turret_target->network_id == minion->network_id;

                    const auto turret_has_current_target =
                        turret_target && turret_target->network_id == minion->network_id;

                    if ( turret_has_current_target && g_config->orbwalker.draw_current_minion_tower->get< bool >( ) ) {
                        g_render->circle_3d( turret_target->position, Color( 31, 0, 255, 255 ), 30, 2, 80, 2.f );
                        Vec2 sp{ };
                        if ( !world_to_screen( turret_target->position, sp ) ) return;

                        const auto text = std::format( ( "current" ) );
                        constexpr auto text_size_set = static_cast< float >( 16 );
                        const auto text_size = g_render->get_text_size( text, g_fonts->get_block( ), text_size_set );

                        g_render->text_shadow(
                            { sp.x - ( text_size.x / 4.f ), sp.y + text_size.y },
                            Color( 0, 0, 255, 220 ),
                            g_fonts->get_default_bold( ),
                            text.c_str( ),
                            text_size_set
                        );
                    }

                    //if ( turret_has_current_target && turret->network_id != turret_target->network_id  )

                    if ( turret_has_next_target && g_config->orbwalker.draw_next_minion_tower->get< bool >( ) ) {
                        Vec2 sp{ };
                        if ( !world_to_screen( next_turret_target->position, sp ) ) return;

                        const auto     text          = std::format( ( "next" ) );
                        constexpr auto text_size_set =
                            static_cast< float >( 16 );
                        const auto text_size = g_render->get_text_size( text, g_fonts->get_block( ), text_size_set );
                        g_render->circle_3d( next_turret_target->position, Color( 0, 0, 0, 255 ), 30, 2, 80, 2.f );
                        g_render->text_shadow(
                            { sp.x - ( text_size.x / 4.f ), sp.y + text_size.y },
                            Color( 255, 25, 25, 220 ),
                            g_fonts->get_default_bold( ),
                            text.c_str( ),
                            text_size_set
                        );
                    }
                }
            }
            continue;
        }
        return;
    }

    auto Orbwalker::draw_target_effect( ) -> void{
        if ( g_config->orbwalker.draw_target_mode->get< int >( ) != 2 ) return;

        auto animation_duration = 3.f;
        if ( *g_time - m_effect_animation_time > animation_duration ) m_effect_animation_time = *g_time;

        auto is_forced{ g_features->target_selector->is_forced( ) };


        for ( auto i = 0; i < 2; i++ ) {
            if ( is_forced && i > 0 ) return;

            const auto target = g_features->target_selector->get_default_target( i == 1 );
            if ( !target || target->is_dead( ) || target->is_invisible( ) ) continue;

            auto animation_time{ 1.25f };
            auto transition_time{ 0.25f };
            auto is_primary_target = i == 0;

            auto bounding_radius = g_features->prediction->get_champion_radius( rt_hash( target->champion_name.text ) );

            auto approach_modifier  = std::min( ( *g_time - m_effect_animation_time ) / animation_time, 1.f );
            auto departure_modifier = std::min(
                ( *g_time - m_effect_animation_time - animation_time - transition_time ) / animation_time,
                1.f
            );
            auto early_transition_modifier =
                std::min( ( *g_time - m_effect_animation_time - animation_time ) / transition_time, 1.f );
            auto late_transition_modifier = std::min(
                ( *g_time - m_effect_animation_time - animation_time * 2.f - transition_time ) / transition_time,
                1.f
            );

            auto animation_state{ 1 };
            if ( early_transition_modifier >= 0.f && early_transition_modifier < 1.f ) animation_state = 2;
            else if ( departure_modifier >= 0.f && departure_modifier < 1.f ) animation_state = 3;
            else if ( late_transition_modifier >= 0.f && late_transition_modifier < 1.f ) animation_state = 4;

            float      modifier;
            auto       line_length    = is_primary_target ? 80.f : 60.f;
            const auto floating_area  = is_primary_target ? 25.f : 10.f;
            const auto line_thickness = is_primary_target ? 5.f : 4.f;

            // if ( animation_state > 3 ) return;

            auto draw_color   = i == 0 ? get_pulsing_color( ) : Color( 255, 255, 255, 250 );
            auto forced_color = get_pulsing_color( );

            if ( is_forced ) {
                g_render->line_3d(
                    g_local->position,
                    target->position.extend( g_local->position, bounding_radius ),
                    forced_color,
                    4.f
                );
            }

            auto line_extend_amount = is_primary_target ? 4.f : 0.f;

            switch ( animation_state ) {
            case 1:
            {
                modifier = approach_modifier;

                auto anim_hitbox =
                    sdk::math::Rectangle(
                        Vec3( target->position.x, target->position.y, target->position.z + bounding_radius ),
                        Vec3( target->position.x, target->position.y, target->position.z - bounding_radius ),
                        bounding_radius
                    )
                    .to_polygon( static_cast< int32_t >( floating_area - floating_area * 0.9f * modifier ) );

                for ( auto j = 0; j < static_cast< int32_t >( anim_hitbox.points.size( ) ); j++ ) {
                    auto start = anim_hitbox.points[ j ];
                    auto end   =
                        j == anim_hitbox.points.size( ) - 1 ? anim_hitbox.points[ 0 ] : anim_hitbox.points[ j + 1 ];

                    auto distance = start.dist_to( end );
                    start         = end.extend( start, distance + line_extend_amount );
                    end           = start.extend( end, distance + line_extend_amount );

                    auto box_color = is_forced ? forced_color : draw_color;

                    //if ( is_forced ) line_length = start.dist_to( end );

                    g_render->line_3d( start, start.extend( end, line_length / 2.f ), box_color, line_thickness );
                    g_render->line_3d( end, end.extend( start, line_length / 2.f ), box_color, line_thickness );
                }

                break;
            }
            case 2:
            {
                modifier = early_transition_modifier;

                auto anim_hitbox =
                    sdk::math::Rectangle(
                        Vec3( target->position.x, target->position.y, target->position.z + bounding_radius ),
                        Vec3( target->position.x, target->position.y, target->position.z - bounding_radius ),
                        bounding_radius
                    )
                    .to_polygon( static_cast< int32_t >( floating_area * 0.1f - floating_area * 0.1f * modifier ) );

                for ( auto j = 0; j < static_cast< int32_t >( anim_hitbox.points.size( ) ); j++ ) {
                    auto start = anim_hitbox.points[ j ];
                    auto end   =
                        j == anim_hitbox.points.size( ) - 1 ? anim_hitbox.points[ 0 ] : anim_hitbox.points[ j + 1 ];

                    auto distance = start.dist_to( end );
                    start         = end.extend( start, distance + line_extend_amount );
                    end           = start.extend( end, distance + line_extend_amount );

                    auto box_color = is_forced ? forced_color : draw_color;

                    //if ( is_forced ) line_length = start.dist_to( end );

                    g_render->line_3d( start, start.extend( end, line_length / 2.f ), box_color, line_thickness );
                    g_render->line_3d( end, end.extend( start, line_length / 2.f ), box_color, line_thickness );
                }

                break;
            }
            case 3:
            {
                modifier = departure_modifier;

                auto anim_hitbox =
                    sdk::math::Rectangle(
                        Vec3( target->position.x, target->position.y, target->position.z + bounding_radius ),
                        Vec3( target->position.x, target->position.y, target->position.z - bounding_radius ),
                        bounding_radius
                    )
                    .to_polygon( static_cast< int32_t >( floating_area * 0.9f * modifier ) );

                for ( auto j = 0; j < static_cast< int32_t >( anim_hitbox.points.size( ) ); j++ ) {
                    auto start = anim_hitbox.points[ j ];
                    auto end   =
                        j == anim_hitbox.points.size( ) - 1 ? anim_hitbox.points[ 0 ] : anim_hitbox.points[ j + 1 ];

                    auto distance = start.dist_to( end );
                    start         = end.extend( start, distance + line_extend_amount );
                    end           = start.extend( end, distance + line_extend_amount );

                    auto box_color = is_forced ? forced_color : draw_color;

                    //if ( is_forced ) line_length = start.dist_to( end );

                    g_render->line_3d( start, start.extend( end, line_length / 2.f ), box_color, line_thickness );
                    g_render->line_3d( end, end.extend( start, line_length / 2.f ), box_color, line_thickness );
                }

                break;
            }
            case 4:
            {
                modifier = late_transition_modifier;

                auto anim_hitbox =
                    sdk::math::Rectangle(
                        Vec3( target->position.x, target->position.y, target->position.z + bounding_radius ),
                        Vec3( target->position.x, target->position.y, target->position.z - bounding_radius ),
                        bounding_radius
                    )
                    .to_polygon( static_cast< int32_t >( floating_area * 0.9f + floating_area * 0.1f * modifier ) );

                for ( auto j = 0; j < static_cast< int32_t >( anim_hitbox.points.size( ) ); j++ ) {
                    auto start = anim_hitbox.points[ j ];
                    auto end   =
                        j == anim_hitbox.points.size( ) - 1 ? anim_hitbox.points[ 0 ] : anim_hitbox.points[ j + 1 ];

                    auto distance = start.dist_to( end );
                    start         = end.extend( start, distance + line_extend_amount );
                    end           = start.extend( end, distance + line_extend_amount );

                    auto box_color = is_forced ? forced_color : draw_color;

                    //if ( is_forced ) line_length = start.dist_to( end );

                    g_render->line_3d( start, start.extend( end, line_length / 2.f ), box_color, line_thickness );
                    g_render->line_3d( end, end.extend( start, line_length / 2.f ), box_color, line_thickness );
                }

                break;
            }
            default:
                break;
            }
        }
    }

    auto Orbwalker::draw_target( ) -> void{
        constexpr auto cycle_duration = 0.6f;
        if ( *g_time - m_draw_cycle_time >= cycle_duration * 1.2f ) m_draw_cycle_time = *g_time;

        constexpr auto cycle_length{ 125 };
        constexpr auto cycle_apex{ cycle_length / 2 };
        constexpr auto cycle_step_time = cycle_duration / static_cast< float >( cycle_length );
        const auto cycle_index = static_cast< int >( std::floor( ( *g_time - m_draw_cycle_time ) / cycle_step_time ) );
        constexpr auto alpha_tick = 255 / cycle_apex;
        int lightness;


        if ( cycle_index > cycle_length ) lightness = 0;
        else {
            if ( cycle_index <= cycle_apex ) lightness = alpha_tick * cycle_index;
            else lightness                             = 255 - alpha_tick * ( cycle_index - cycle_apex );
        }

        m_pulsing_color = Color( 255, lightness, lightness, 255 );
    }

    auto Orbwalker::draw_movement() -> void {

        if (m_autospacing)
        {
            for (auto point : m_automatic_points)
                g_render->circle_3d(point, Color(255, 255, 255, 50), 20.f, Renderer::outline | Renderer::filled, -1, 2.f);
        }

        if (g_local->is_dead() || m_in_attack || g_features->evade->is_active( ) )
            return;

        Vec3 goal_position = g_local->position;
        auto time_until_attack = std::clamp(m_attack_delay - (*g_time - m_last_attack_time), 0.f, m_attack_delay);
        auto pred = g_features->prediction->predict_default(g_local->index, time_until_attack);
        if (pred.has_value()) goal_position = pred.value();

        if (did_tether) goal_position = g_local->position.extend(m_last_position, g_local->position.dist_to(m_last_position));
        else if ( time_until_attack <= 0.f ) return;
        
        

       // goal_position = g_local->position.extend(m_last_position, g_local->movement_speed * time_until_attack);

        auto arrow_point = goal_position;
        auto point_color = m_autospacing ? Color( 255, 50, 50 ) : Color(255, 255, 255);

        g_render->line_3d(g_local->position, arrow_point, point_color, 3.f);

        auto arrow_angle = 42.5f;

        auto temp = (g_local->position - arrow_point).rotated_raw(arrow_angle);

        auto arrow_side_left = arrow_point.extend(arrow_point + temp, 30.f);

        temp                  = (g_local->position - arrow_point).rotated_raw(-arrow_angle);
        auto arrow_side_right = arrow_point.extend(arrow_point + temp, 30.f);

        auto arrow_color = point_color;
        auto bg_color    = Color(0, 0, 0, 225);

        auto shadow_depth = 6.f;

        Vec3 shadow_point = { arrow_point };
        shadow_point.y -= shadow_depth;

        Vec3 shadow_left = { arrow_side_left };
        shadow_left.y -= shadow_depth;

        auto shadow_right{ arrow_side_right };
        shadow_right.y -= shadow_depth;

        g_render->line_3d(arrow_point, arrow_side_left, arrow_color, 4.f);
        g_render->line_3d(arrow_point, arrow_side_right, arrow_color, 4.f);
        g_render->line_3d(g_local->position.extend(arrow_point, arrow_point.dist_to(g_local->position) - 1.f),
                          arrow_point.extend(g_local->position, -1.f), arrow_color, 4.f);

    }


    auto Orbwalker::draw_mode( ) const -> void{
        if ( !g_config->orbwalker.draw_mode->get< bool >( ) || g_local->is_dead( ) ) return;

        g_local.update( );
        // auto local = g_local.create_updated_copy( );
        Vec2 sp{ };

        if ( !world_to_screen( g_local->position, sp ) ) return;

        sp.y -= 5;

        auto spacing_color = animate_color( Color( 255, 50, 50 ), EAnimationType::pulse, 4, 255 );

        if ( g_config->orbwalker.autospacing_toggle->get< bool >( ) ) {
            Vec2 tracing_position = {
                sp.x,
                sp.y + ( g_config->orbwalker.draw_attack_timer->get< bool >( ) ? 40.f : 32.f ) * g_config->misc.screen_scaling->get<float>()
            };

            const auto as_text_size = g_render->get_text_size( "SPACING", g_fonts->get_zabel_12px( ), 12 );
            g_render->text_shadow(
                Vec2( tracing_position.x - as_text_size.x / 2.f, tracing_position.y - as_text_size.y ),
                spacing_color,
                g_fonts->get_zabel_12px( ),
                "SPACING",
                12
            );
        }

        std::string mode_text{ "IDLE" };

        switch ( m_mode ) {
        case EOrbwalkerMode::combo:
            mode_text = g_input->is_key_pressed( utils::EKey::control ) ? _( "FULL COMBO" ) : _( "COMBO" );
            break;
        case EOrbwalkerMode::lasthit:
            mode_text = _( "LASTHIT" );
            break;
        case EOrbwalkerMode::freeze:
            mode_text = _( "FREEZE" );
            break;
        case EOrbwalkerMode::laneclear:
            mode_text = g_input->is_key_pressed( utils::EKey::control ) ? _( "FASTCLEAR" ) : _( "LANECLEAR" );
            break;
        case EOrbwalkerMode::harass:
            mode_text = g_input->is_key_pressed( utils::EKey::control ) ? _( "HARASS + FREEZE" ) : _( "HARASS" );
            break;
        case EOrbwalkerMode::flee:
            mode_text = _( "FLEE" );
            break;
        case EOrbwalkerMode::recalling:
            mode_text = _( "RECALL" );
            break;
        default:
            mode_text = _( "IDLE" );
            break;
        }

        const auto current_time = std::chrono::steady_clock::now( );
        auto diff = std::chrono::duration_cast< std::chrono::milliseconds >( current_time - m_last_mode_update_time ).
            count( );

        if ( diff <= 200 ) {
            std::string last_mode_text{ "IDLE" };
            switch ( m_last_mode ) {
            case EOrbwalkerMode::combo:
                last_mode_text = g_input->is_key_pressed( utils::EKey::control ) ? _( "FULL COMBO" ) : _( "COMBO" );
                break;
            case EOrbwalkerMode::lasthit:
                last_mode_text = _( "LASTHIT" );
                break;
            case EOrbwalkerMode::freeze:
                last_mode_text = _( "FREEZE" );
                break;
            case EOrbwalkerMode::laneclear:
                last_mode_text = g_input->is_key_pressed( utils::EKey::control ) ? _( "FASTCLEAR" ) : _( "LANECLEAR" );
                break;
            case EOrbwalkerMode::harass:
                last_mode_text = g_input->is_key_pressed( utils::EKey::control )
                                     ? _( "HARASS + FREEZE" )
                                     : _( "HARASS" );
                break;
            case EOrbwalkerMode::flee:
                last_mode_text = _( "FLEE" );
                break;
            case EOrbwalkerMode::recalling:
                last_mode_text = _( "RECALL" );
                break;
            default:
                last_mode_text = _( "IDLE" );
                break;
            }

            auto modifier      = std::clamp( static_cast< float >( diff ) / 200.f, 0.f, 1.f );
            modifier           = utils::ease::ease_out_expo( modifier );
            auto pixels_offset = 12.f * modifier;

            const auto mode_size      = g_render->get_text_size( mode_text, g_fonts->get_block( ), 8 );
            const auto last_mode_size = g_render->get_text_size( last_mode_text, g_fonts->get_block( ), 8 );

            Vec2 mode_text_position      = { sp.x - mode_size.x / 2.f, sp.y - 12.f + pixels_offset };
            Vec2 last_mode_text_position = { sp.x - last_mode_size.x / 2.f, sp.y + pixels_offset };

            g_render->text_shadow(
                mode_text_position,
                Color::white( ).alpha( 255 * modifier ),
                g_fonts->get_block( ),
                mode_text.data( ),
                8
            );
            g_render->text_shadow(
                last_mode_text_position,
                Color::white( ).alpha( 255 - 255 * modifier ),
                g_fonts->get_block( ),
                last_mode_text.data( ),
                8
            );

            return;
        }

        const auto text_size = g_render->get_text_size( mode_text, g_fonts->get_block( ), 8 );
        g_render->text_shadow(
            Vec2( sp.x - text_size.x / 2.f, sp.y ),
            Color::white( ),
            g_fonts->get_block( ),
            mode_text.data( ),
            8
        );
    }

    auto Orbwalker::is_unit_headshottable( const int16_t index ) const -> bool{
        auto unit = g_entity_list.get_by_index( index );
        if ( !unit || !can_override_attack( index ) ) return false;

        unit.update( );

        //Caitlyn_Base_W_tar_debuf
        //CaitlynWSnare
        //eternals_caitlyneheadshottracker

        bool found_indicator{ };

        for ( const auto particle : g_entity_list.get_enemy_uncategorized( ) ) {
            if ( !particle || particle->position.dist_to( unit->position ) > 200.f ) continue;

            auto name = particle->get_alternative_name( );
            if ( name.find( "Caitlyn" ) == std::string::npos
                || name.find( "Tar_Headshot" ) == std::string::npos )
                continue;

            found_indicator = true;
            break;
        }

        if ( !found_indicator ) return false;

        auto buff = g_features->buff_cache->get_buff( index, ct_hash( "eternals_caitlyneheadshottracker" ) );
        if ( buff && is_override_allowed( index, true ) ) return true;

        buff = g_features->buff_cache->get_buff( index, ct_hash( "caitlynwsight" ) );
        if ( buff && is_override_allowed( index, false ) ) return true;

        return false;
    }

#if __DEBUG
    auto Orbwalker::debug_shit( ) -> void{
        // auto sci = g_local->spell_book.get_spell_cast_info( );
        //
        // if ( ( !sci || sci->server_cast_time <= *g_time ) && was_active ) {
        //     for ( auto enemy : g_entity_list.get_enemies( ) ) {
        //         if ( !enemy ) continue;
        //
        //
        //         auto& obj = g_entity_list.get_by_index( enemy->index );
        //         if ( !obj ) continue;
        //
        //         obj.update( );
        //
        //         auto nearest_point = g_features->evade->get_closest_line_point(
        //             m_cast_start,
        //             m_cast_end,
        //             obj->position
        //         );
        //
        //         std::cout << "[ + ] distance was " << obj->position.dist_to( nearest_point ) << std::endl;
        //
        //         was_active = false;
        //         return;
        //     }
        // }
    }
#endif

    auto draw_line_blurred( Vec2 start, Vec2 end, Color color, float thickness, int blur_strength, float blur_size ) {

        for (int o = blur_strength; o >= 0; o--)
        {


           // float modifier = std::clamp(time_on_path / animation_duration, 0.f, 1.f);
           // modifier       = utils::ease::ease_out_expo(modifier);

            int opacity_step = static_cast<int>( std::floor( 100.f / static_cast<float>(blur_strength) ) );
            int opacity      = o == 0 ? 255 : 100 - opacity_step * o;

            //Vec3 animated_position =
            //    m_previous_path_end.extend(path[i], m_previous_path_end.dist_to(path[i]) * modifier);
            //if (o == 0) current_animated_position = animated_position;

            //if (!sdk::math::world_to_screen(animated_position, sp_next)) break;

            float max_blur = blur_size * o;
            float stroke   = o == 0 ? thickness : thickness + max_blur;

            // g_render->line({ sp.x + 1, sp.y + 1 }, { sp_next.x + 1, sp_next.y + 1 }, Color(10, 10, 10,
            // opacity), 1.f);
            auto current_color = Color(color.r, color.g, color.b, opacity);
            g_render->line(start, end, current_color, stroke);
        }

    }

    auto Orbwalker::draw_local_path( ) -> void{

        if (!g_config->orbwalker.draw_path->get<bool>()) return;

         auto aimgr = g_local->get_ai_manager( );
         auto path  = aimgr->get_path( );
         if (path.empty()) return;
    
         /*g_render->circle_3d(
             m_last_position,
             Color( 0, 155, 255, 60 ),
             25.f,
             Renderer::outline | Renderer::filled,
             32,
             2.f
         );*/

        if (path[path.size() - 1].dist_to( m_current_path_end ) > 1.f) {

            m_previous_path_end = m_current_path_end;
            m_current_path_end  = path[path.size() - 1];
            m_last_path_update_time = *g_time;
        }
        
        if ( path.size( ) > 1 ) {
             Vec2 sp_next{ };
             Vec2 sp{ };

            Vec3 current_animated_position{};

            if ( path.size( ) > 1u ) {
                for ( auto i = aimgr->next_path_node; i < static_cast< int >( path.size( ) ); i++ ) {
                
                     if (!sdk::math::world_to_screen(
                        i == aimgr->next_path_node ? g_local->position.extend(path[i], get_bounding_radius( ) ) : path[i - 1], sp) ||
                         !sdk::math::world_to_screen( path[ i ], sp_next ) )
                         break;

                    if (i == path.size() - 1)
                    {
                        float time_on_path = *g_time - m_last_path_update_time;
                         const float animation_duration = 0.2f;

                        //g_render->draw_ellipse(g_local->position, 300, 16, 8.f);
                         float modifier = std::clamp(time_on_path / animation_duration, 0.f, 1.f);
                         modifier       = utils::ease::ease_out_expo(modifier);

                         Vec3 animated_position =
                             m_previous_path_end.extend(path[i], m_previous_path_end.dist_to(path[i]) * modifier);
                         current_animated_position = animated_position;

                         if (!sdk::math::world_to_screen(animated_position, sp_next)) break;

                         g_render->blur_line(sp, sp_next, Color::white(), 3.f, 3 - 2 * modifier, 5.f - 2.f * modifier);

                        break;
                    }
                     

                    g_render->line(sp, sp_next, Color::white(), 3.f);
                }

                if (sdk::math::world_to_screen(current_animated_position, sp))
                {
                     g_render->filled_circle( sp, Color( 255, 255, 255 ), 3.f, 16 );
                     // g_render->text_shadow({ sp.x - 5, sp.y }, Color(0, 145, 255, 255), g_fonts->get_bold(), "Local",
                     // 16);
                }
            }
        }
    }

    auto Orbwalker::draw_attack_timer( ) const -> void{
        if ( !g_config->orbwalker.draw_attack_timer->get< bool >( ) || g_local->is_dead( ) ) return;

        g_local.update( );
        // auto local = g_local.create_updated_copy( );
        Vec2 sp{ };

        if ( !world_to_screen( g_local->position, sp ) ) return;

        const Vec2 box_start{ sp.x - 20.f, sp.y + 15.f };
        const Vec2 box_size{ 40.f, 5.f };

        const auto percent = std::clamp( ( *g_time - m_last_attack_time ) / m_attack_delay, 0.f, 1.f );

        const Vec2 fill_size{ box_size.x * percent, box_size.y };

        g_render->filled_box( box_start, { box_size.x + 2.f, box_size.y + 2.f }, Color( 30, 30, 30, 200 ) );

        if ( m_blinded || m_polymorph ) {
            g_render->filled_box(
                { box_start.x + 1, box_start.y + 1 },
                fill_size,
                Color( 90, 20, 255, 200 )
            );
        } else if ( m_movement_impaired ) {
            g_render->filled_box(
                { box_start.x + 1, box_start.y + 1 },
                fill_size,
                Color( 255, 20, 20, 200 )
            );
        } else if ( m_is_overridden ) {
            g_render->filled_box(
                { box_start.x + 1, box_start.y + 1 },
                fill_size,
                get_pulsing_color( )
            );
        } else if ( in_action( ) ) {
            g_render->filled_box(
                { box_start.x + 1, box_start.y + 1 },
                fill_size,
                Color( 255, 255, 20, 200 )
            );
        } else if ( percent >= 1.f ) {
            g_render->filled_box(
                { box_start.x + 1, box_start.y + 1 },
                fill_size,
                Color( 30, 255, 30, 155 )
            );
        } else g_render->filled_box( { box_start.x + 1, box_start.y + 1 }, fill_size, Color( 255, 255, 255, 175 ) );
    }

    auto Orbwalker::draw_hold_radius( ) const -> void{
        if ( !g_config->orbwalker.draw_hold_radius->get< bool >( ) ) return;

        g_render->circle_3d( g_local->position, Color( 255, 255, 255, 70 ), HOLD_RADIUS, 2, 40, 2.f );
    }

    auto Orbwalker::draw_debug_data( ) const -> void{
        if ( g_local->is_dead( ) ) return;

        Vec2 start_pos{ 200, 300 };

        //g_render->text_shadow(start_pos, can_attack() ? color(75, 255, 75) : color(255, 75, 75), g_fonts->get_bold(), "CanAttack", 32);
        //start_pos.y += 32.f;

        g_render->text_shadow(
            start_pos,
            in_action( ) ? Color( 255, 255, 75 ) : Color( 255, 75, 75 ),
            g_fonts->get_bold( ),
            "InAction",
            32
        );
        start_pos.y += 32.f;

        std::string target_name{ "nullptr" };
        const auto  target = g_features->target_selector->get_default_target( );
        if ( target && target->is_alive( ) ) target_name = target->champion_name.text;

        const auto text = "target: " + target_name;

        g_render->text_shadow( start_pos, Color( 255, 255, 255 ), g_fonts->get_bold( ), text.c_str( ), 32 );
    }

    auto Orbwalker::draw_ignored_targets( ) const -> void{
        for ( const auto inst : m_ignored ) {
            const auto& unit = g_entity_list.get_by_index( inst.index );
            if ( !unit ) continue;

            const auto bounding_radius = g_entity_list.get_bounding_radius( unit->index );

            g_render->circle_3d(
                unit->position,
                Color( 255, 25, 25, 50 ),
                bounding_radius,
                Renderer::outline | Renderer::filled,
                -1,
                2.f
            );
        }
    }

    auto Orbwalker::is_attackable(
        const int16_t index,
        const float   range,
        const bool    edge_range,
        const bool    is_autoattack
    ) const -> bool{
        // Function is used in LUA, message @tore if you change args
        auto unit = g_entity_list.get_by_index( index );
        if ( !unit || !unit->is_hero( ) && unit->is_turret_object( ) && !unit->is_turret_attackable( ) ) return false;

        unit.update( );

        auto target_bounding_radius = unit->get_bounding_radius( );
        auto local_bounding_radius  = get_bounding_radius( ) - 5.f;

        auto target_position = unit->position;
        auto local_position  = g_local->position;
        auto local_server_position = g_features->prediction->get_server_position(g_local->index);

        auto local_aimgr = g_local->get_ai_manager( );
        if ( local_aimgr ) {
            local_aimgr.update( );

            local_server_position = g_features->prediction->get_server_position(g_local->index);
        }

        if ( unit->is_hero( ) ) {
            const auto target_hash = rt_hash( unit->champion_name.text );
            if ( target_hash == ct_hash( "Nilah" ) && g_features->buff_cache->get_buff(
                    unit->index,
                    ct_hash( "NilahW" )
                ) ||
                target_hash == ct_hash( "Jax" ) && g_features->buff_cache->get_buff(
                    unit->index,
                    ct_hash( "JaxCounterStrike" )
                ) ||
                g_features->buff_cache->get_buff( unit->index, ct_hash( "ShenWBuff" ) ) ||
                target_hash == ct_hash( "Samira" ) && get_aa_missile_speed( unit.get( ) ) > 0.f
                && g_features->buff_cache->get_buff( unit->index, ct_hash( "SamiraW" ) ) )
                return false;


            //target_position = g_features->prediction->get_server_position(unit->index);
        }


        if ( *g_time - m_last_move_time <= 0.1f ) {

            local_position = g_local->position.extend(
                m_last_position,
                g_local->movement_speed * (get_ping( ) + 0.033f)
            );
        }
        else {

            local_bounding_radius *= 0.5f;
        }

        static auto current_hero = helper::get_current_hero( );
        
        if ( current_hero == EHeroes::kalista ) {
            auto aimgr = g_local->get_ai_manager( );

            if ( aimgr && aimgr->is_dashing && aimgr->is_moving ) {
                const auto path = aimgr->get_path( );
                if ( path.size( ) == 2 ) local_position = path[ path.size( ) - 1 ];
            }
        }

        if ( g_config->orbwalker.windwall_check->get< bool >( ) && g_features->prediction->is_windwall_active( ) &&
            is_autoattack && get_aa_missile_speed( unit.get( ) ) > 0.f ) {
            auto       after_windup_target_pos{ unit->position };
            const auto pred = g_features->prediction->predict_default( unit->index, m_attack_cast_delay );
            if ( pred.has_value( ) ) after_windup_target_pos = *pred;

            if ( get_aa_missile_speed( unit.get( ) ) > 0.f && g_features->prediction->windwall_in_line(
                local_position,
                after_windup_target_pos
            ) )
                return false;
        }

        if ( range == 0.f ) {
            const auto autoattack_range =
                helper::get_current_hero( ) == EHeroes::zeri ? 500.f : g_local->attack_range;

            if ( unit->is_hero( ) ) {
                if ( helper::get_current_hero( ) == EHeroes::caitlyn && is_unit_headshottable( unit->index ) ) {
                    return
                        local_position.dist_to( target_position ) <= 1300.f;
                }

                if ( helper::get_current_hero( ) == EHeroes::samira && g_config->samira.passive_orbwalk->get< bool >( )
                    && local_position.dist_to( target_position ) <= samira_get_empowered_range( )
                    && samira_can_override_attack( index ) )
                    return true;
            }

            return local_position.dist_to( unit->position ) < autoattack_range + local_bounding_radius + target_bounding_radius &&
                local_server_position.dist_to( unit->position ) < autoattack_range + local_bounding_radius + target_bounding_radius &&
                g_local->position.dist_to( unit->position ) < autoattack_range + local_bounding_radius + target_bounding_radius &&
                local_position.dist_to( target_position ) < autoattack_range + local_bounding_radius + target_bounding_radius &&
                local_server_position.dist_to( target_position ) < autoattack_range + local_bounding_radius + target_bounding_radius &&
                g_local->position.dist_to( target_position ) < autoattack_range + local_bounding_radius + target_bounding_radius;
        }

        if ( edge_range ) {
            return local_position.dist_to( unit->position ) < range + local_bounding_radius + target_bounding_radius &&
                g_local->position.dist_to( unit->position ) < range + local_bounding_radius + target_bounding_radius &&
                local_position.dist_to( target_position ) < range + local_bounding_radius + target_bounding_radius &&
                g_local->position.dist_to( target_position ) < range + local_bounding_radius + target_bounding_radius;
        }

        return local_position.dist_to( unit->position ) < range &&
            g_local->position.dist_to( unit->position ) < range;
    }


    // Function is used in LUA, message @tore if you change args
    auto Orbwalker::set_cast_time( const float time ) -> void{ m_cast_duration = *g_time + time; }

    auto Orbwalker::update_orb_mode( ) -> void{
        if ( g_menugui->is_chat_open( ) || g_menugui->is_shop_open( ) ) {
            m_mode = EOrbwalkerMode::none;
            return;
        }

        if ( g_config->orbwalker.orbwalker->get< bool >( ) ) m_mode = EOrbwalkerMode::combo;
        else if ( g_config->orbwalker.harass->get< bool >( ) ) m_mode = EOrbwalkerMode::harass;
        else if ( g_config->orbwalker.lane_clear->get< bool >( ) ) m_mode = EOrbwalkerMode::laneclear;
        else if ( g_config->orbwalker.lasthit->get< bool >( ) ) {
            m_mode = g_input->is_key_pressed( utils::EKey::control )
                         ? EOrbwalkerMode::freeze
                         : EOrbwalkerMode::lasthit;
        } else if ( g_config->orbwalker.flee->get< bool >( ) ) m_mode = EOrbwalkerMode::flee;
        else m_mode                                                   = EOrbwalkerMode::none;

        if ( m_mode != m_previous_mode ) {
            m_last_mode     = m_previous_mode;
            m_previous_mode = m_mode;

            m_last_mode_update_time = std::chrono::steady_clock::now( );
        }
    }

    auto Orbwalker::can_attack_turret( ) const -> bool{
        for ( const auto turret : g_entity_list.get_enemy_turrets( ) ) {
            if ( !turret ) continue;
            if ( !is_attackable( turret->index ) ) continue;

            std::string name = turret->name.text;
            if ( name.find( _( "Turret" ) ) != std::string::npos && turret->is_dead( ) ||
                name.find( _( "Turret" ) ) == std::string::npos && turret->get_selectable_flag( ) == 0 )
                continue;

            return true;
        }

        return false;
    }

    auto Orbwalker::get_local_server_position( ) const -> Vec3{
        if ( *g_time - m_last_move_time > 0.1f ) return g_local->position;

        return g_local->position.extend( m_last_position, g_local->movement_speed * get_ping( ) * 2.5f );
    }


    auto Orbwalker::ignore_minion( const int16_t index, const float duration ) -> void{
        IgnoredMinion ign;

        ign.index    = index;
        ign.end_time = *g_time + duration + 1.f;

        m_ignored.push_back( ign );
    }

    auto Orbwalker::update_ignored( ) -> void{
        auto to_remove = std::ranges::remove_if(
            m_ignored,
            []( const IgnoredMinion& ign ){ return ign.end_time <= *g_time; }
        );
        if ( to_remove.empty( ) ) return;
        m_ignored.erase( to_remove.begin( ), to_remove.end( ) );
    }

#if _DEBUG
    auto Orbwalker::print_local_buffs( ) -> void{
        /* Object *obj{ };
        for ( auto ally : g_entity_list.get_allies(  )) {

            if ( !ally || ally->dist_to_local(  ) > 500.f ) continue;

            obj = ally;
        }

        if ( !obj ) return;*/

        std::cout << "\n\n\n";
        //system( "cls" );

        // debug_log( "target addr: {:x}", g_entity_list->get_by_index( t->index )->get_address( ) );

        for ( auto buff : g_local->buff_manager.get_all( ) ) {
            if ( !buff || !buff->get_buff_info( ) ||
                buff->stack == 0 )
                continue;

            /*debug_log(
                "Buff: {} STACKS: {}  TYPE: {} START: {}  END: {} TIME ACTIVE: {}",
                buff->get_buff_info()->name,
                buff->stack,
                (int)buff->type,
                buff->start_time,
                buff->end_time,
                *g_time - buff->start_time
            );*/

            std::cout << "^ Address: " << std::hex << buff.get_address() << " | " << buff->get_buff_info()->get_name()
                      << std::dec
            << " [ Type: " << (int)buff->type << " ]\n";
                   
        }
    }

    auto Orbwalker::print_target_buffs( ) -> void{
        const auto t = g_features->target_selector->get_default_target( );
        if ( !t ) return;

        std::cout << "\n\n\n";

        // debug_log( "target addr: {:x}", g_entity_list->get_by_index( t->index )->get_address( ) );

        for ( auto buff : t->buff_manager.get_all( ) ) {
            if ( !buff || !buff->get_buff_info( ) || buff->stack == 0 ) continue;

            debug_log(
                "Buff: {} STACKS: {}  TYPE: {} START: {}  END: {} TIME ACTIVE: {}",
                buff->get_buff_info( )->get_name(),
                buff->stack,
                ( int )buff->type,
                buff->start_time,
                buff->end_time,
                *g_time - buff->start_time
            );

            std::cout << "^ Address: " << std::hex << buff.get_address( ) << std::endl;
        }
    }

#endif

    auto Orbwalker::support_limiter_active( ) -> bool{
        if ( !g_config->orbwalker.support_mode->get< bool >( ) || *g_time >= 1200.f ) return false;

        for ( const auto hero : g_entity_list.get_allies( ) ) {
            if ( !hero || hero->is_dead( ) || hero->dist_to_local( ) >= 1000.f || hero->network_id == g_local->
                network_id )
                continue;

            return true;
        }

        return false;
    }

    auto Orbwalker::is_in_turret_range( const Object* object ) -> bool{
        if ( !object || object->is_dead( ) ) return false;

        const auto turrets = object->team != g_local->team
                                 ? g_entity_list.get_ally_turrets( )
                                 : g_entity_list.get_enemy_turrets( );

        for ( const auto turret : turrets ) {
            if ( !turret || turret->is_dead( ) ) continue;

            const auto     target_radius = g_entity_list.get_bounding_radius( object->index );
            constexpr auto turret_radius = 88.4f;

            if ( turret->position.dist_to( object->position ) > 800.f + turret_radius + target_radius ) continue;

            return true;
        }

        return false;
    }

    auto Orbwalker::is_position_under_turret( const Vec3& position ) -> bool{
        for ( const auto turret : g_entity_list.get_enemy_turrets( ) ) {
            if ( !turret || turret->is_dead( ) ) continue;

            const auto target_radius = g_features->prediction->get_champion_radius(
                rt_hash( g_local->champion_name.text )
            );
            constexpr auto turret_radius = 88.4f;

            if ( turret->position.dist_to( position ) > 800.f + turret_radius + target_radius ) continue;

            return true;
        }

        return false;
    }


    auto Orbwalker::get_nearest_turret( const Object* object ) -> Object*{
        for ( const auto turret : g_entity_list.get_ally_turrets( ) ) {
            if ( !turret || turret->is_dead( ) || turret->position.dist_to( object->position ) > 1000.f ) continue;

            return turret;
        }

        return nullptr;
    }

    // ReSharper disable once CppMemberFunctionMayBeStatic
    auto Orbwalker::get_turret_current_target( const Object* turret ) const -> Object*{
        if ( !turret || turret->is_dead( ) ) return nullptr;

        auto sci = turret->spell_book.get_spell_cast_info( );
        if ( !sci ) return nullptr;

        for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
            if ( minion && minion->index == sci->get_target_index( ) ) return minion;
        }

        return nullptr;
    }

    auto Orbwalker::get_turret_next_target(
        const Object*  turret,
        const uint32_t ignored_nid
    ) const -> Object*{
        if ( !turret || turret->is_dead( ) ) return nullptr;

        Object*             target{ };
        int16_t             ignored_index{ };
        Object::EMinionType target_type{ };
        auto                target_distance = std::numeric_limits< float >::max( );
        bool                target_found{ };

        bool no_target{ };
        int  candidate_count{ };

        if ( auto sci = turret->spell_book.get_spell_cast_info( ) ) ignored_index = sci->get_target_index( );
        else no_target                                                            = true;

        for ( const auto obj : g_entity_list.get_enemy_minions( ) ) {
            if ( !obj ||
                obj->is_dead( ) ||
                !obj->is_visible( ) ||
                obj->get_minion_type( ) == Object::EMinionType::error ||
                !is_in_turret_range( obj )
            )
                continue;

            ++candidate_count;

            if ( ignored_index == obj->index || obj->network_id == ignored_nid ) {
                if ( !target_found ) target = obj;
                continue;
            }

            const auto type     = obj->get_minion_type( );
            const auto distance = obj->position.dist_to( turret->position );

            if ( !target_found ||
                target_found &&
                ( type > target_type || type == target_type && distance < target_distance )
            ) {
                target          = obj;
                target_type     = type;
                target_distance = distance;
                target_found    = true;
            }
        }

        if ( no_target && candidate_count > 1 && target ) {
            ignored_index   = target->index;
            target_type     = Object::EMinionType::error;
            target_distance = std::numeric_limits< float >::max( );
            target_found    = false;
            target          = { };

            for ( const auto obj : g_entity_list.get_enemy_minions( ) ) {
                if ( !obj ||
                    obj->is_dead( ) ||
                    obj->index == ignored_index ||
                    obj->network_id == ignored_nid ||
                    !obj->is_visible( ) ||
                    obj->get_minion_type( ) == Object::EMinionType::error ||
                    !is_in_turret_range( obj )
                )
                    continue;

                const auto type     = obj->get_minion_type( );
                const auto distance = obj->position.dist_to( turret->position );

                if ( !target_found || target_found && ( type > target_type || type == target_type && distance <
                    target_distance ) ) {
                    target          = obj;
                    target_type     = type;
                    target_distance = distance;
                    target_found    = true;
                }
            }
        }

        //if ( !target )

        return target;
    }

    auto Orbwalker::get_turret_shot_damage( const Object* obj ) -> float{
        switch ( obj->get_minion_type( ) ) {
        case Object::EMinionType::melee:
            return obj->max_health * 0.45f;
        case Object::EMinionType::ranged:
            return obj->max_health * 0.7f;
        case Object::EMinionType::siege:
            return obj->max_health * 0.14f;
        case Object::EMinionType::super:
            return obj->max_health * 0.07f;
        default:
            return 0.f;
        }
    }

    auto Orbwalker::get_spellfarm_target_index( ) -> std::optional< int16_t >{
        if ( *g_time > m_spellfarm_target_expire_time || !m_spellfarm_confirmed || m_spellfarm_target_index ==
            0 )
            return std::nullopt;

        return std::make_optional( m_spellfarm_target_index );
    }

    auto Orbwalker::add_spellfarm_target( const int16_t index ) -> void{
        m_spellfarm_target_index       = index;
        m_spellfarm_target_expire_time = *g_time + 1.f;
        m_spellfarm_confirmed          = false;

        std::cout << "Orbwalker: Added spellfarm target | " << *g_time << std::endl;
    }

    auto Orbwalker::get_aa_missile_speed( const Object* target ) const -> float{
        if ( g_config->orbwalker.ignore_missile_speed->get< bool >( ) ) return 0.f;

        // 2750 Jinx Minigun
        // 2000

        switch ( helper::get_current_hero( ) ) {
        case EHeroes::caitlyn:
            return g_features->buff_cache->get_buff( g_local->index, ct_hash( "caitlynpassivedriver" ) )
                       ? 3000.f
                       : 2500.f;
        case EHeroes::jinx:
        {
            const auto spell = g_local->spell_book.get_spell_slot( ESpellSlot::q );
            if ( !spell ) return 2750.f;

            const auto state = spell->get_usable_state( );

            return state == 1 ? 2750.f : 2000.f;
        }
        case EHeroes::samira:
            if ( target && target->dist_to_local( ) < 300.f ) return 0.f;
            break;
        case EHeroes::zoe:
            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "zoepassivesheenbuff" ) ) ) return 0.f;
            break;
        case EHeroes::viktor:
            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "ViktorPowerTransferReturn" ) ) ) {
                return
                    0.f;
            }
        case EHeroes::twitch:
            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "TwitchFullAutomatic" ) ) ) return 4000.f;
            break;
        default:
            break;
        }

        return m_autoattack_missile_speed;
    }

    auto Orbwalker::update_debuff_state( ) -> void{
        m_blinded           = g_features->buff_cache->has_buff_of_type( g_local->index, EBuffType::blind );
        m_polymorph         = g_features->buff_cache->has_buff_of_type( g_local->index, EBuffType::polymorph );
        m_movement_impaired = g_features->buff_cache->has_hard_cc( g_local->index );
    }

    auto Orbwalker::can_override_attack( const int16_t index ) const -> bool{
        if ( index == 0 ) return false;

        return is_override_allowed( index, true ) || is_override_allowed( index, false );
    }


    auto Orbwalker::update_overrides( ) -> void{
        for ( const auto inst : m_trap_overrides ) {
            if ( inst.end_time + 1.f <= *g_time ) {
                debug_log( "cooldown expired for TRAP" );
                remove_override( inst.index, false );
            }
        }

        for ( const auto inst : m_net_overrides ) {
            if ( inst.end_time + 1.f <= *g_time ) {
                debug_log( "cooldown expired for NET" );
                remove_override( inst.index, true );
            }
        }
    }

    auto Orbwalker::remove_override( const int16_t index, const bool is_net ) -> void{
        if ( is_net ) {
            const auto to_remove = std::ranges::remove_if(
                m_net_overrides,
                [&]( const OverrideInstance& inst ) -> bool{ return inst.index == index; }
            );

            if ( to_remove.empty( ) ) return;

            m_net_overrides.erase( to_remove.begin( ), to_remove.end( ) );
        }

        const auto to_remove = std::ranges::remove_if(
            m_trap_overrides,
            [&]( const OverrideInstance& inst ) -> bool{ return inst.index == index; }
        );

        if ( to_remove.empty( ) ) return;

        m_trap_overrides.erase( to_remove.begin( ), to_remove.end( ) );
    }

    auto Orbwalker::is_override_allowed( const int16_t index, const bool is_net ) const -> bool{
        if ( is_net ) {
            for ( const auto inst : m_net_overrides ) if ( inst.index == index ) return false;

            return true;
        }

        for ( const auto inst : m_trap_overrides ) if ( inst.index == index ) return false;

        return true;
    }

    auto Orbwalker::set_override_cooldown( const int16_t index, const bool is_net, const float end_time ) -> void{
        const OverrideInstance inst{ index, end_time };

        if ( is_net ) m_net_overrides.push_back( inst );
        else m_trap_overrides.push_back( inst );

        debug_log(
            "set cooldown for {}",
            is_net ?
            "net" :
            "trap"
        );
    }

    auto Orbwalker::samira_get_empowered_range( ) -> float{
        if ( g_local->level >= 16 ) return 960.f;
        if ( g_local->level >= 12 ) return 882.5f;
        if ( g_local->level >= 8 ) return 805.f;
        if ( g_local->level >= 4 ) return 727.5f;

        return 650.f;
    }


    auto Orbwalker::samira_remove_override( const int16_t index ) -> void{
        const auto to_remove = std::ranges::remove_if(
            m_samira_overrides,
            [&]( const OverrideInstance& inst ) -> bool{ return inst.index == index; }
        );

        if ( to_remove.empty( ) ) return;

        m_samira_overrides.erase( to_remove.begin( ), to_remove.end( ) );
    }

    auto Orbwalker::samira_update_overrides( ) -> void{
        if ( !g_config->samira.passive_orbwalk->get< bool >( ) ) return;

        for ( const auto& inst : m_samira_overrides ) {
            if ( inst.end_time <= *g_time ) {
                samira_remove_override(
                    inst.index
                );
            }
        }
    }

    auto Orbwalker::samira_add_override_instance( const int16_t index, const float end_time ) -> void{
        const OverrideInstance inst{ index, end_time };

        m_samira_overrides.push_back( inst );
    }

    auto Orbwalker::samira_is_override_logged( const int16_t index ) const -> bool{
        if ( !g_config->samira.passive_orbwalk->get< bool >( ) ) return true;

        for ( const auto inst : m_samira_overrides ) if ( inst.index == index ) return true;

        return false;
    }


    auto Orbwalker::samira_can_override_attack( const int16_t index ) const -> bool{
        if ( !g_config->samira.passive_orbwalk->get< bool >( ) ) return false;

        for ( const auto inst : m_samira_overrides ) if ( inst.index == index ) return false;

        const auto& unit = g_entity_list.get_by_index( index );
        if ( !unit || !unit->is_hero( ) ) return false;

        bool found_cc{ };
        for ( const auto buff : g_features->buff_cache->get_all_buffs( index ) ) {
            if ( !buff ) continue;

            const auto type = static_cast< EBuffType >( buff->buff_data->type );

            switch ( type ) {
            case EBuffType::asleep:
            case EBuffType::stun:
            case EBuffType::suppression:
            case EBuffType::knockup:
            case EBuffType::snare:
            case EBuffType::fear:
            case EBuffType::berserk:
            case EBuffType::taunt:
            case EBuffType::charm:
                if ( buff->buff_data->end_time - *g_time < get_ping( ) * 2.f ) continue;

                found_cc = true;
                break;
            default:
                break;
            }

            if ( found_cc ) break;
        }

        return found_cc;
    }

    auto Orbwalker::update_pings( ) -> void{
        remove_outdated_pings( );

        uint32_t highest_ping{ };
        uint32_t lowest_ping{ 9999 };


        for ( const auto inst : m_pings ) {
            if ( inst.ping > highest_ping ) highest_ping = inst.ping;
            else if ( inst.ping <
                lowest_ping )
                lowest_ping = inst.ping;
        }

        m_highest_ping = highest_ping;
    }

    auto Orbwalker::remove_outdated_pings( ) -> void{
        const auto to_remove = std::ranges::remove_if(
            m_pings,
            [&]( const PingInstance& inst ) -> bool{ return *g_time - inst.ping_time > 60.f; }
        );

        if ( to_remove.empty( ) ) return;

        m_pings.erase( to_remove.begin( ), to_remove.end( ) );
    }

    auto Orbwalker::remove_last_hittable_glow( const unsigned network_id ) -> void{
        const auto to_remove =
            std::ranges::remove_if(
                m_last_hittable_units,
                [ & ]( const LastHittableUnit& inst ) -> bool{ return inst.network_id == network_id; }
            );

        if ( to_remove.empty( ) ) return;

        m_last_hittable_units.erase( to_remove.begin( ), to_remove.end( ) );
    }

    auto Orbwalker::is_unit_glowing( const unsigned network_id ) const -> bool{
        for ( const auto inst : m_last_hittable_units ) if ( inst.network_id == network_id ) return true;

        return false;
    }

    auto Orbwalker::update_last_hittable_glow( ) -> void{
        if ( !g_config->orbwalker.draw_lasthittable_minions->get< bool >( ) ) {
            if ( !m_last_hittable_units.empty( ) ) m_last_hittable_units.clear( );
            return;
        }

        for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
            if ( !minion || minion->dist_to_local( ) > 1000.f || minion->is_dead( ) || !minion->is_lane_minion( ) ||
                is_unit_glowing( minion->network_id ) || !is_attackable(
                    minion->index,
                    g_local->attack_range + 300.f
                ) )
                continue;

            auto travel_time = get_aa_missile_speed( minion ) <= 0.f
                                   ? 0.f
                                   : g_local->position.dist_to( minion->position ) / get_aa_missile_speed( minion );
            if ( g_config->orbwalker.lasthit_predict_ping->get< bool >( ) ) travel_time += get_ping( ) / 2.f;

            const auto lasthit_health = g_features->prediction->predict_minion_health(
                minion->index,
                m_attack_cast_delay + travel_time
            );
            if ( lasthit_health <= 0.f ) continue;

            const auto aa_damage = helper::get_aa_damage(
                minion->index,
                helper::get_current_hero( ) != EHeroes::zeri
            );

            if ( lasthit_health > aa_damage ) continue;

            m_last_hittable_units.push_back(
                { minion->index, minion->network_id, false, 0.f, 0.5f, 0, 0, GlowEffect::ShiningGreen }
            );
        }

        //Layer 1: { 62, 157, 31 }
        //Layer 2 : { 213, 255, 43 }
        //Layer 3: { 0, 255, 112 }

        for ( auto& inst : m_last_hittable_units ) {
            auto unit = g_entity_list.get_by_index( inst.index );
            unit.update( );

            if ( !unit || unit->is_dead( ) ) {
                if ( inst.death_time <= 0.f ) {
                    inst.death_time           = *g_time;
                    inst.glow_effect          = GlowEffect::FadingRed;
                    inst.glow_state           = 0;
                    inst.minimum_update_delay = 0.075f;
                    inst.is_glowing           = true;
                }

                if ( !unit || *g_time - inst.death_time > 2.5f ) {
                    remove_last_hittable_glow( inst.network_id );
                    continue;
                }
            }

            if ( inst.is_glowing && *g_time - inst.last_update_time <= inst.minimum_update_delay || !g_function_caller->
                is_glow_queueable( ) )
                continue;

            Color glow_color{ 0, 255, 120 };

            switch ( inst.glow_effect ) {
            case GlowEffect::ShiningGreen:
            {
                if ( inst.glow_state > 1 ) continue;

                if ( inst.glow_state > 0 ) {
                    glow_color = { 200, 225, 200 };

                    g_function_caller->enable_glow(
                        unit->network_id,
                        D3DCOLOR_ARGB( glow_color.a, glow_color.r, glow_color.g, glow_color.b ),
                        0,
                        3,
                        10
                    );

                    inst.glow_state = 2;
                } else {
                    glow_color = { 255, 255, 255 };

                    g_function_caller->enable_glow(
                        unit->network_id,
                        D3DCOLOR_ARGB( glow_color.a, glow_color.r, glow_color.g, glow_color.b ),
                        1,
                        3,
                        0
                    );

                    inst.glow_state = 1;
                }

                break;
            }
            case GlowEffect::FadingRed:
            {
                float modifier = std::clamp( ( *g_time - inst.death_time ) / 2.f, 0.f, 1.f );
                modifier       = utils::ease::ease_out_quad( modifier );

                if ( inst.glow_state > 0 ) {
                    Color animated_color{ 255, 55, 55 };

                    const Color max_pulse_value = {
                        std::max( 255 - animated_color.r, 0 ),
                        std::max( 255 - animated_color.g, 0 ),
                        std::max( 255 - animated_color.b, 0 )
                    };

                    const auto pulse_modifier = 1.f - modifier;

                    const Color pulse_amount = {
                        static_cast< int32_t >( max_pulse_value.r * pulse_modifier ),
                        static_cast< int32_t >( max_pulse_value.g * pulse_modifier ),
                        static_cast< int32_t >( max_pulse_value.b * pulse_modifier )
                    };

                    animated_color.r += pulse_amount.r;
                    animated_color.g += pulse_amount.g;
                    animated_color.b += pulse_amount.b;


                    glow_color = animated_color;

                    g_function_caller->enable_glow(
                        unit->network_id,
                        D3DCOLOR_ARGB( glow_color.a, glow_color.r, glow_color.g, glow_color.b ),
                        1,
                        6 - 3 * modifier,
                        0
                    );

                    inst.glow_state = 0;
                } else {
                    Color animated_color{ 105, 20, 20 };

                    const Color max_pulse_value = {
                        std::max( 255 - animated_color.r, 0 ),
                        std::max( 255 - animated_color.g, 0 ),
                        std::max( 255 - animated_color.b, 0 )
                    };

                    const auto pulse_modifier = 1.f - modifier;

                    const Color pulse_amount = {
                        static_cast< int32_t >( max_pulse_value.r * pulse_modifier ),
                        static_cast< int32_t >( max_pulse_value.g * pulse_modifier ),
                        static_cast< int32_t >( max_pulse_value.b * pulse_modifier )
                    };

                    animated_color.r += pulse_amount.r;
                    animated_color.g += pulse_amount.g;
                    animated_color.b += pulse_amount.b;


                    glow_color = animated_color;

                    g_function_caller->enable_glow(
                        unit->network_id,
                        D3DCOLOR_ARGB( glow_color.a, glow_color.r, glow_color.g, glow_color.b ),
                        0,
                        8 - 2 * modifier,
                        25
                    );

                    inst.glow_state = 1;
                }

                inst.last_update_time = *g_time;

                break;
            }
            default:
                break;
            }

            continue;


            switch ( inst.last_glow_stage ) {
            case 0:

                glow_color = g_config->orbwalker.glow_style->get< int >( ) == 2
                                 ? g_features->tracker->get_rainbow_color( )
                                 : g_config->orbwalker.glow_color->get< Color >( );
                if ( g_config->orbwalker.glow_style->get< int >( ) == 1 ) {
                    glow_color = g_features->orbwalker->animate_color( glow_color, EAnimationType::pulse, 3 );
                }


                g_function_caller->enable_glow(
                    unit->network_id,
                    D3DCOLOR_ARGB( glow_color.a, glow_color.r, glow_color.g, glow_color.b ),
                    0,
                    g_config->orbwalker.glow_size->get< int >( ),
                    g_config->orbwalker.glow_diffusion->get< int >( )
                );

                inst.last_glow_stage = 1;
                break;
            case 1:
                glow_color = g_config->orbwalker.second_glow_style->get< int >( ) == 2
                                 ? g_features->tracker->get_rainbow_color( )
                                 : g_config->orbwalker.second_glow_color->get< Color >( );
                if ( g_config->orbwalker.second_glow_style->get< int >( ) == 1 ) {
                    glow_color = g_features->orbwalker->animate_color( glow_color, EAnimationType::pulse, 3 );
                }


                g_function_caller->enable_glow(
                    unit->network_id,
                    D3DCOLOR_ARGB( glow_color.a, glow_color.r, glow_color.g, glow_color.b ),
                    1,
                    g_config->orbwalker.second_glow_size->get< int >( ),
                    g_config->orbwalker.second_glow_diffusion->get< int >( )
                );

                inst.last_glow_stage  = 2;
                inst.is_glowing       = true;
                inst.last_update_time = *g_time;
                break;
            case 2:
                glow_color = g_config->orbwalker.third_glow_style->get< int >( ) == 2
                                 ? g_features->tracker->get_rainbow_color( )
                                 : g_config->orbwalker.third_glow_color->get< Color >( );
                if ( g_config->orbwalker.third_glow_style->get< int >( ) == 1 ) {
                    glow_color = g_features->orbwalker->animate_color( glow_color, EAnimationType::pulse, 3 );
                }


                g_function_caller->enable_glow(
                    unit->network_id,
                    D3DCOLOR_ARGB( glow_color.a, glow_color.r, glow_color.g, glow_color.b ),
                    2,
                    g_config->orbwalker.third_glow_size->get< int >( ),
                    g_config->orbwalker.third_glow_diffusion->get< int >( )
                );

                inst.last_glow_stage = 0;
                break;
            default:
                break;
            }

            //inst.is_glowing = true;
        }
    }

    auto Orbwalker::draw_minion_healthbar() -> void {

        // minion healthbar size 62

        for (auto minion : g_entity_list.get_enemy_minions()) {

            if (!minion || minion->is_dead() || !minion->is_lane_minion() || minion->dist_to_local() > 1500.f) continue;

            auto bar = minion->get_hpbar_position();

            //g_render->filled_circle(bar, Color(255, 255, 0), 2, 16);

            Vec2 bar_start = { bar.x - 31, bar.y };
            Vec2 bar_end   = { bar_start.x + 60, bar_start.y };

            //g_render->line(bar_start, bar_end, Color::white(), 2);

            Vec3 minion_position = minion->position;
            auto position        = g_features->prediction->predict_default(minion->index, m_attack_cast_delay);
            if (position) minion_position = position.value();

           
            //auto minion_type = minion->get_minion_type();

            auto travel_time = m_attack_cast_delay +
                g_local->position.dist_to(minion_position) / get_aa_missile_speed(minion) + get_ping() / 2.f;
            auto future_health  = g_features->prediction->predict_minion_health(minion->index, travel_time, true);
            auto next_aa_health = g_features->prediction->predict_possible_minion_health(
                minion->index, travel_time + m_attack_delay + get_ping() / 2.f + 0.033f, true);

            float damage = g_features->prediction->simulate_minion_damage( minion->index, travel_time + m_attack_delay );
            float base_modifier = minion->health / minion->max_health;
            float damage_modifier = damage / minion->max_health;
            float damage_width = damage_modifier * 60.f;

            Vec2 health_end = { bar_start.x + 1.f + 60.f * base_modifier, bar_start.y };
            Vec2 health_start = { health_end.x - damage_width, health_end.y };

            if (health_start.x < bar_start.x) health_start.x = bar_start.x;

            g_render->filled_box({ health_start.x, health_start.y - 6 }, { damage_width, 5 }, Color(40, 150, 255, 125),
                                 -1);

            //if (future_health <= helper::get_aa_damage(minion->index))
            //    g_render->box({ bar_start.x - 3.f, bar_start.y - 9 }, { 66.f, 10 }, Color(40, 120, 210, 150),
            //                     -1, 2.f);

            continue;

            if (g_features->prediction->should_damage_minion(minion->index, helper::get_aa_damage(minion->index),
                                                             travel_time, m_attack_delay)) {
                g_render->circle_3d(minion->position, Color(25, 255, 25, 40), minion->get_bounding_radius(),
                                    Renderer::outline | Renderer::filled, 32, 2.f);
            }

        }

    }


    auto Orbwalker::get_bounding_radius( ) -> float{ return g_features->evade->get_bounding_radius( ); }

    auto Orbwalker::should_auto_space( ) const -> bool{
        if ( !g_config->orbwalker.autospacing_toggle->get< bool >( ) ||
            get_mode( ) != EOrbwalkerMode::combo && get_mode( )
            != EOrbwalkerMode::harass )
            return false;

        const auto obj = g_features->target_selector->get_default_target( );
        if ( !obj || obj->attack_range <= 400.f || obj->attack_range >= g_local->attack_range ) return false;

        auto& target = g_entity_list.get_by_index( obj->index );
        if ( !target ) return false;

        target.update( );

        const auto target_bounding_radius = target->get_bounding_radius( );

        const auto local_bounding_radius = g_local->get_bounding_radius( );
        const auto target_attack_range = target->attack_range + target_bounding_radius + local_bounding_radius;
        const auto local_attack_range = g_local->attack_range + target_bounding_radius * 0.33f + local_bounding_radius;

        const auto can_target_attack = target->dist_to_local( ) <= target_attack_range;
        if ( can_target_attack ) return false;

        const auto speed_diff = target->movement_speed > g_local->movement_speed
                                    ? target->movement_speed - g_local->movement_speed
                                    : 0.f;
        const auto prediction_time = 1.f / ( target->movement_speed / ( 60.f + speed_diff ) );

        const auto pred = g_features->prediction->predict_default( target->index, prediction_time + get_ping( ) );
        const auto post_attack_pred = g_features->prediction->predict_default(
            target->index,
            m_attack_cast_delay + get_ping( )
        );
        const auto range_leverage = g_local->attack_range - target->attack_range;

        auto aimgr = target->get_ai_manager( );
        if ( !aimgr ) return false;

        if ( !pred || !post_attack_pred
            || !aimgr->is_moving // if target is standing still
            || g_local->position.dist_to( *pred ) > local_attack_range
            && g_local->position.dist_to( target->position ) > local_attack_range
            || g_local->position.dist_to( *post_attack_pred ) > target_attack_range + range_leverage )
            return false;

        // angle check
        if ( true ) {
            const auto path     = aimgr->get_path( );
            const auto path_end = path[ path.size( ) - 1 ];

            auto       v1             = pred.value( ) - target->position;
            auto       v2             = g_local->position - target->position;
            auto       dot            = v1.normalize( ).dot_product( v2.normalize( ) );
            const auto approach_angle = acos( dot ) * 180.f / 3.14159265358979323846f;

            v1                        = path_end - target->position;
            v2                        = g_local->position - target->position;
            dot                       = v1.normalize( ).dot_product( v2.normalize( ) );
            const auto path_end_angle = acos( dot ) * 180.f / 3.14159265358979323846f;
            if ( approach_angle > 55.f && path_end_angle > 55.f ) return false;
        }

        return true;
    }

    auto Orbwalker::should_autospace_target( const int16_t index ) const -> bool{
        if ( !g_config->orbwalker.autospacing_toggle->get< bool >( ) ||
            get_mode( ) != EOrbwalkerMode::combo && get_mode( )
            != EOrbwalkerMode::harass )
            return false;

        const auto& target = g_entity_list.get_by_index( index );
        if ( !target || !target->is_hero( ) || target->attack_range <= 400.f || target->attack_range >= g_local->
            attack_range )
            return false;

        const auto target_bounding_radius = g_features->prediction->get_champion_radius(
            rt_hash( target->champion_name.text )
        );
        const auto local_bounding_radius = g_features->prediction->get_champion_radius(
            rt_hash( g_local->champion_name.text )
        );
        const auto target_attack_range = target->attack_range + target_bounding_radius + local_bounding_radius;

        const auto can_target_attack = target->dist_to_local( ) <= target_attack_range;
        if ( can_target_attack ) return false;

        const auto aimgr = target->get_ai_manager( );
        if ( !aimgr ) return false;

        const auto speed_diff = target->movement_speed > g_local->movement_speed
                                    ? target->movement_speed - g_local->movement_speed
                                    : 0.f;
        const auto prediction_time = 1.f / ( target->movement_speed / ( 60.f + speed_diff ) );

        const auto range_leverage = g_local->attack_range - target->attack_range;
        const auto pred = g_features->prediction->predict_default( target->index, prediction_time + get_ping( ) );
        const auto post_attack_pred = g_features->prediction->predict_default(
            target->index,
            m_attack_cast_delay + get_ping( )
        );

        if ( !pred || !post_attack_pred
            || pred.value( ).dist_to( target->position ) <= 0.f // if target is standing still
            || !is_attackable( target->index )
            || g_local->position.dist_to( *post_attack_pred ) > target_attack_range + range_leverage )
            return false;

        // angle check
        if ( true ) {
            auto ai_manager = target->get_ai_manager( );
            if ( !ai_manager ) return false;

            const auto path     = ai_manager->get_path( );
            const auto path_end = path[ path.size( ) - 1 ];

            auto       v1             = pred.value( ) - target->position;
            auto       v2             = g_local->position - target->position;
            auto       dot            = v1.normalize( ).dot_product( v2.normalize( ) );
            const auto approach_angle = acos( dot ) * 180.f / 3.14159265358979323846f;

            v1                        = path_end - target->position;
            v2                        = g_local->position - target->position;
            dot                       = v1.normalize( ).dot_product( v2.normalize( ) );
            const auto path_end_angle = acos( dot ) * 180.f / 3.14159265358979323846f;
            if ( approach_angle > 55.f && path_end_angle > 55.f ) return false;
        }

        return true;
    }

    auto Orbwalker::get_autospace_position( ) -> std::optional< Vec3 >{
        if ( !g_config->orbwalker.autospacing_toggle->get< bool >( ) ) return std::nullopt;

        const auto target = g_features->target_selector->get_default_target( );
        if ( !target ) return std::nullopt;

        const auto range_leverage = g_local->attack_range - target->attack_range;
        if ( range_leverage <= 0.f ) return std::nullopt;

        const auto speed_diff = target->movement_speed > g_local->movement_speed
                                    ? target->movement_speed - g_local->movement_speed
                                    : 0.f;
        const auto prediction_time = 1.f / ( target->movement_speed / ( 50.f + speed_diff ) );

        auto aimgr = target->get_ai_manager( );
        if ( !aimgr ) return std::nullopt;

        const auto path = aimgr->get_path( );

        const auto pred = g_features->prediction->predict_default( target->index, prediction_time );
        if ( !pred || !aimgr->is_moving || static_cast< int >( path.size( ) ) <= 1 ) return std::nullopt;

        const auto move_direction = pred.value( ) - target->position;
        const auto base_direction = g_local->position + move_direction;

        const auto direction = ( base_direction - g_local->position ).normalize( ).rotated( -1.61f );

        const auto points = g_render->get_3d_circle_points( g_local->position, 400.f, 25, 180, direction );
        Vec3       best_point{ };

        std::vector< Vec3 > candidates{ };

        for ( auto point : points ) {
            if ( is_wall_in_line( g_local->position, point )
                || g_features->prediction->minion_in_line( g_local->position, point, 1.f )
                || !g_features->evade->is_position_safe( point )
                || is_position_under_turret( point ) )
                continue;

            candidates.push_back( point );

            const auto distance = point.dist_to( pred.value( ) );
            if ( best_point.length( ) > 0.f && distance < best_point.dist_to( pred.value( ) ) ) continue;

            best_point = point;
        }

        if ( best_point.length( ) <= 0.f ) return std::nullopt;

        possible_space_points    = candidates;
        m_autospace_target_index = target->index;

        return std::make_optional( best_point );
    }

    auto Orbwalker::update_autospacing( ) const -> void{
        if ( !m_autospacing || !g_config->orbwalker.autospacing_toggle->get< bool >( ) ) return;

        auto& target = g_entity_list.get_by_index( m_autospace_target_index );
        if ( !target ) return;

        target.update( );
    }

    auto Orbwalker::get_magnet_position( ) -> std::optional< Vec3 >{
        if ( !g_config->orbwalker.melee_magnet->get< bool >( ) || g_local->attack_range > 300.f ) return std::nullopt;

        const auto target = get_orbwalker_target( );
        if ( !target || target->is_invisible( ) || target->is_dead( ) ) return std::nullopt;

        const auto cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;
        if ( target->dist_to_local( ) > 500.f || target->dist_to_local( ) < cursor.dist_to( target->position ) ) {
            return std::nullopt;
        }

        auto pred = g_features->prediction->predict( target->index, 800.f, g_local->movement_speed, 0.f, 0.f );
        if ( !pred.valid ) return std::nullopt;

        return std::make_optional( pred.position );
    }

    auto Orbwalker::get_kogmaw_magnet( ) const -> std::optional< Vec3 >{
        if ( rt_hash( g_local->champion_name.text ) != ct_hash( "KogMaw" ) || !m_kogmaw_in_passive ) {
            return
                std::nullopt;
        }

        const auto    cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;
        const Object* target{ };
        auto          lowest_distance{ FLT_MAX };

        for ( const auto enemy : g_entity_list.get_enemies( ) ) {
            if ( !enemy || enemy->dist_to_local( ) > 600.f ||
                g_features->target_selector->is_bad_target( enemy->index ) )
                continue;

            const auto distance = enemy->position.dist_to( cursor );
            if ( distance > lowest_distance ) continue;

            target          = enemy;
            lowest_distance = distance;
        }

        if ( !target ) return std::nullopt;

        auto pred = g_features->prediction->predict(
            target->index,
            750.f,
            g_local->movement_speed,
            0.f,
            0.025f,
            { },
            false,
            Prediction::include_ping
        );
        if ( !pred.valid ) return std::nullopt;

        return std::make_optional( pred.position );
    }

    auto Orbwalker::get_automatic_position() -> std::optional<Vec3> {

        if (!g_config->orbwalker.autospacing_toggle->get<bool>() || g_local->attack_range < 475.f || m_mode != EOrbwalkerMode::combo && m_mode != EOrbwalkerMode::harass)
            return std::nullopt;

        const auto target = get_orbwalker_target( );
        if (!target || target->attack_range >= g_local->attack_range || !is_attackable( target->index, g_local->attack_range + 75.f, true, true ) ) return std::nullopt;

        const auto v1            = target->position - g_local->position;
        const auto v2            = g_pw_hud->get_hud_manager()->cursor_position_unclipped - g_local->position;
        const auto dot           = v1.normalize().dot_product(v2.normalize());
        const auto current_angle = acos(dot) * 180.f / 3.14159265358979323846f;
        if (current_angle > 140.f) return std::nullopt;

        //std::cout << "angle: " << current_angle << std::endl;

        float attack_range_difference = g_local->attack_range - target->attack_range;
        float duration = attack_range_difference / target->movement_speed;

        float time_until_attack_ready = std::clamp(m_last_attack_time + m_attack_delay - get_ping() / 2.f - *g_time, 0.f, m_attack_delay ) ;
        float sim_duration = duration + get_ping( ) / 2.f; // time_until_attack_ready; //+ get_ping( ) + 0.033f;

        //std::cout << "Duration: " << sim_duration << std::endl;

        auto       target_position = target->position;
        const auto pred            = g_features->prediction->predict_default(target->index, sim_duration);
        if (pred) target_position = pred.value();

        float additional_distance = time_until_attack_ready * g_local->movement_speed;

        auto aimgr = target->get_ai_manager( );
        bool is_target_moving = aimgr && aimgr->is_moving && aimgr->get_path().size() > 1 && aimgr->next_path_node != aimgr->get_path().size();

        float modifier = static_cast<float>(g_config->orbwalker.spacing_modifier->get<int>()) / 100.f;
        float clamped_difference = std::clamp(attack_range_difference * modifier, 0.f, g_local->get_bounding_radius( ) + target->get_bounding_radius() / 2.f);

        float goal_distance = g_local->attack_range + g_local->get_bounding_radius() + target->get_bounding_radius( ) - clamped_difference;
        //+target->get_bounding_radius() / 2.f; //+ additional_distance * 0.25f;

        if (!is_attackable(target->index)) goal_distance -= std::clamp(attack_range_difference * 0.5f, 0.f, g_local->get_bounding_radius());

        float steer_angle = is_target_moving ? 20.f : 30.f;

        const auto start_direction = (g_features->prediction->get_server_position(g_local->index) - target_position)
                                         .normalize()
                                         .rotated_raw(-(steer_angle / 2.f));
        const auto points =
            g_render->get_3d_circle_points(target_position, goal_distance, 10, static_cast<int>( steer_angle ), start_direction);

        m_autospace_target_index = target->index;

        Vec3 local_position = g_features->prediction->get_server_position(g_local->index);

        std::vector<Vec3> valid_points{};

        Vec3 target_serverpos = g_features->prediction->get_server_position(target->index);

        Vec3 best_point{};
        auto lowest_distance{ 99999.f };
        auto       highest_distance{ 0.f };
        const auto cursor = get_extrapolated_cursor_position( );

        for (auto point : points)
        {
            if (point.dist_to(local_position) <= 75.f && is_target_moving)

                // g_config->orbwalker.melee_magnet->get<bool>( ) && point.dist_to( target->position )
                                  // < local_position.dist_to(target->position) && point.dist_to(g_local->position) <=
                                  // 125.f)
            {

                if (local_position.dist_to( target->position ) > point.dist_to( target_position ) ) {

                    const auto point_dist   = point.dist_to(local_position);
                    const auto extend_value = 50.f - point_dist;

                    point = point.extend(target_position, extend_value);

                }
                else
                {

                    const auto point_dist   = point.dist_to(local_position);
                    const auto extend_value = 50.f - point_dist;

                    point = point.extend(target_position, -extend_value);

                }
            }

            if (point.dist_to(local_position) < 75.f || helper::is_position_under_turret(point) ||
                g_navgrid->is_wall(point) || !g_features->evade->is_position_safe(point))
                continue;

            valid_points.push_back(point);

            if ( is_target_moving ) {

                if (local_position.dist_to(target->position) > point.dist_to(target_position)) {

                    const auto distance = point.dist_to(target_serverpos);
                    if (distance > lowest_distance) continue;

                    best_point      = point;
                    lowest_distance = distance;
                    continue;
                }
               

               const auto distance = point.dist_to(target_serverpos);
                if (distance < highest_distance) continue;

                best_point      = point;
                highest_distance = distance;
                continue;
                
            }

            const auto distance = cursor.dist_to(point);
            if (distance > lowest_distance) continue;

            best_point      = point;
            lowest_distance = distance;
            
        }

        m_automatic_points = valid_points;

        return best_point.length() > 0.f ? std::make_optional(best_point) : std::nullopt;
    }


    auto Orbwalker::get_sticky_position( ) -> std::optional< Vec3 >{
        if ( !g_config->orbwalker.autospacing_toggle->get< bool >( ) || g_local->attack_range < 475.f ) {
            return std::nullopt;
        }

        const auto target = get_orbwalker_target( );
        if ( !target || get_mode( ) != EOrbwalkerMode::combo && get_mode( ) != EOrbwalkerMode::harass
            || can_attack( target->index ) && !is_attackable( target->index ) )
            return std::nullopt;

        auto       target_position = target->position;
        const auto pred            = g_features->prediction->predict_movement( target->index, 0.15f );
        if ( pred ) target_position = pred.value( );

        if ( target_position.dist_to( g_local->position ) >= target->dist_to_local( ) ) return std::nullopt;

        const auto preferred_space = g_local->attack_range + g_local->get_bounding_radius( );
        const auto sticky_base     = target_position.extend( g_local->position, preferred_space );

        const auto direction = ( target_position - sticky_base ).normalize( ).rotated_raw( 90.f );

        const auto sticky_first  = sticky_base.extend( sticky_base + direction, 200.f );
        const auto sticky_second = sticky_first.extend( sticky_base, 400.f );

        m_sticky_start = sticky_first;
        m_sticky_end   = sticky_second;

        std::vector< Vec3 > points{ };

        constexpr auto point_distance = 25.f;
        const auto     segment_count  = sticky_first.dist_to( sticky_second ) / point_distance;

        for ( auto a = 0; a <= static_cast< int32_t >( segment_count ); a++ ) {
            const auto extend_distance = point_distance * static_cast< float >( a );

            if ( extend_distance > sticky_first.dist_to( sticky_second ) ) {
                const auto position = sticky_second;
                points.push_back( position );
                break;
            }

            auto position = sticky_first.extend( sticky_second, point_distance * static_cast< float >( a ) );
            points.push_back( position );
        }

        const auto cursor = get_extrapolated_cursor_position( );

        Vec3 best_point{ };
        auto lowest_distance{ 99999.f };

        for ( auto point : points ) {
            if ( point.dist_to( g_local->position ) < 100.f || helper::is_position_under_turret( point ) ||
                g_navgrid->is_wall( point ) )
                continue;

            const auto distance = cursor.dist_to( point );
            if ( distance > lowest_distance ) continue;

            best_point      = point;
            lowest_distance = distance;
        }

        return best_point.length( ) > 0.f ? std::make_optional( best_point ) : std::nullopt;
    }

    auto Orbwalker::is_wall_in_line( const Vec3& start, const Vec3& end ) -> bool{
        constexpr auto check_distance{ 40.f };
        const auto     loop_amount = static_cast< int >( std::ceil( start.dist_to( end ) / check_distance ) );

        for ( auto i = 1; i <= loop_amount; i++ ) {
            auto position = start.extend( end, check_distance * static_cast< float >( i ) );

            if ( g_navgrid->is_wall( position ) ) return true;
        }

        return false;
    }

#if __DEBUG
    auto Orbwalker::developer_thing( ) -> void{
        auto sci = g_local->spell_book.get_spell_cast_info( );
        if ( !sci ) return;

        const auto start = sci->start_position;
        const auto end   = sci->end_position;

        const auto target = g_features->target_selector->get_default_target( );
        if ( !target ) return;

        const auto nearest = g_features->evade->get_closest_line_point( start, end, target->position );

        std::cout << "Target distance to hitbox: " << target->position.dist_to( nearest ) << " | actual overflow: " <<
            target->position.dist_to( nearest ) - 135.f << std::endl;
    }

    auto Orbwalker::developer_draw( ) -> void{
        //g_render->circle_3d( )

        if (!m_autospacing || m_autospace_target_index == 0) return;

        auto target = g_entity_list.get_by_index(m_autospace_target_index);
        if (!target) return;

        const auto preferred_space = g_local->attack_range + g_local->get_bounding_radius() / 3.f;
        const auto sticky_base     = g_local->position.extend(target->position, preferred_space);

        const auto direction = (g_local->position - sticky_base).normalize().rotated_raw(90.f);

        const auto sticky_first  = sticky_base.extend(sticky_base + direction, 150.f);
        const auto sticky_second = sticky_first.extend(sticky_base, 300.f);

        Color line_color =
            target->position.dist_to(g_local->position) < g_local->attack_range ? m_pulsing_color : Color::white();
        //get_sticky_position( );
        g_render->line_3d(sticky_first, sticky_second, line_color, 5.f);
        g_render->line_3d(g_local->position.extend(sticky_base, g_local->get_bounding_radius()), sticky_base,
                          line_color, 5.f);
    }
#endif

    auto Orbwalker::get_extrapolated_cursor_position( ) -> Vec3{
        if ( !g_config->orbwalker.extrapolate_cursor->get< bool >( ) ) {
            return g_pw_hud->get_hud_manager( )->
                             cursor_position_unclipped;
        }

        const Vec3 cursor_position = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;
        if ( m_last_cursor_position.dist_to( cursor_position ) <= 4.0f ) return cursor_position;

        const Vec3 cursor_direction      = ( cursor_position - m_last_cursor_position ).normalize( );
        const Vec3 extrapolated_position = cursor_position.extend(
            cursor_position + cursor_direction,
            m_last_cursor_position.dist_to( cursor_position ) * 1.f
        );
        m_last_cursor_position = cursor_position;
        if (*g_time - m_last_cursor_time > 2.f)
        {
            m_last_cursor_time = *g_time;
            return cursor_position;
        }

        m_last_cursor_time = *g_time;

        const auto local_position = g_features->prediction->get_server_position(g_local->index);

        return local_position.extend(
            extrapolated_position, local_position.dist_to(cursor_position)
        );
    }



    auto Orbwalker::draw_forced_notification( ) const -> void{
        if ( !g_config->orbwalker.draw_leftclick_target_indicator->get< bool >( ) || !g_features->target_selector->
            is_forced( ) )
            return;

        const auto target = g_features->target_selector->get_default_target( );
        if ( !target ) return;


        std::string champ_name = target->champion_name.text;

        const Vec2 indicator_base{
            static_cast< float >( g_render_manager->get_width( ) ) / 2.f,
            static_cast< float >( g_render_manager->get_height( ) ) * 0.65f
        };

        const Vec2 texture_size{ 40.f, 40.f };

        const Vec2 indicator_data{ indicator_base.x, indicator_base.y + 28.f };
        const Vec2 texture_position{ indicator_base.x, indicator_base.y - texture_size.y * 0.5f };

        const auto box_color = get_pulsing_color( );

        const std::string text      = "TARGET FORCED";
        const auto        text_size = g_render->get_text_size( text, g_fonts->get_zabel( ), 32 );

        const std::string data_text = "You will not target other enemies";

        const auto text_size_data = g_render->get_text_size( data_text, g_fonts->get_bold( ), 16 );

        const Vec2 box_start = { indicator_base.x - text_size.x / 2.f - 3.f, indicator_base.y + 3.f };

        g_render->filled_box( box_start, { text_size.x + 6.f, text_size.y + 12.f }, Color( 20, 20, 20, 90 ) );
        g_render->box( box_start, { text_size.x + 6.f, text_size.y + 12.f }, box_color );

auto champ_texture =             path::join(
                                     { directory_manager::get_resources_path( ), "champions", champ_name, champ_name + "_square.png" }
                                 );
        const auto texture = g_render->load_texture_from_file(
            champ_texture.has_value(  ) ? *champ_texture : ""
        );
        if ( texture ) {
            g_render->image(
                { texture_position.x - texture_size.x / 2, texture_position.y - texture_size.y / 2 },
                texture_size,
                texture
            );


            g_render->box(
                { texture_position.x - texture_size.x / 2, texture_position.y - texture_size.y / 2 },
                texture_size,
                box_color,
                -1
            );
        }

        g_render->text_shadow(
            Vec2( indicator_base.x - text_size.x / 2.f, indicator_base.y ),
            box_color,
            g_fonts->get_zabel( ),
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

    auto Orbwalker::override_target( const int16_t target_index, const bool require_attack ) -> void{
        m_override_target_index   = target_index;
        m_override_expire_time    = *g_time + 0.3f;
        m_is_overridden           = true;
        m_override_require_attack = require_attack;

        //std::cout << "[ override ] overrode attack target, end in: " << m_override_expire_time - *g_time << std::endl;
    }

    auto Orbwalker::update_color_state( ) -> void{
        constexpr auto update_speed = 5000;
        const auto     current_time = std::chrono::steady_clock::now( );
        const auto     diff         = std::chrono::duration_cast< std::chrono::milliseconds >(
            current_time - m_last_color_update_time
        ).count( );
        if ( diff < update_speed ) return;

        m_last_color_update_time = current_time;
    }

    auto Orbwalker::animate_color(
        const Color          base_color,
        const EAnimationType type,
        const int            speed,
        const int            pulse_ceiling
    ) const -> Color{
        auto animated_color{ base_color };

        const auto cycle_time   = static_cast< int32_t >( 5.f / static_cast< float >( speed ) * 1000.f );
        const auto current_time = std::chrono::steady_clock::now( );
        const auto diff         = std::chrono::duration_cast< std::chrono::milliseconds >(
                current_time - m_last_color_update_time
            ).
            count( );

        auto animation_state = static_cast< float >( diff ) / static_cast< float >( cycle_time );
        if ( animation_state > 1.f ) animation_state -= std::floor( animation_state );

        animation_state = utils::ease::ease_out_quad( animation_state );

        switch ( type ) {
        case EAnimationType::pulse:
        {
            const Color max_pulse_value = {
                std::max( pulse_ceiling - base_color.r, 0 ),
                std::max( pulse_ceiling - base_color.g, 0 ),
                std::max( pulse_ceiling - base_color.b, 0 )
            };

            const auto pulse_modifier = 1.f - animation_state;

            const Color pulse_amount = {
                static_cast< int32_t >( max_pulse_value.r * pulse_modifier ),
                static_cast< int32_t >( max_pulse_value.g * pulse_modifier ),
                static_cast< int32_t >( max_pulse_value.b * pulse_modifier )
            };

            animated_color.r += pulse_amount.r;
            animated_color.g += pulse_amount.g;
            animated_color.b += pulse_amount.b;
            break;
        }
        default:
            break;
        }

        return animated_color;
    }

    auto Orbwalker::should_reset_aa( ) const -> bool{
        // Function is used in LUA, message @tore if you change args
        return !m_in_attack && *g_time > m_last_attack_end_time &&
            *g_time <= m_last_attack_end_time + m_attack_cast_delay * 1.25f;
    }

    auto Orbwalker::draw_developer_data( ) -> void{
        Vec2 root_position{ 250.f, 100.f };

        std::string header = "common issues";
        g_render->text_shadow( root_position, Color::red( ), g_fonts->get_zabel( ), header.data( ), 32 );
        auto size = g_render->get_text_size( header, g_fonts->get_zabel( ), 32 );

        Vec2 draw_position = { root_position.x, root_position.y + size.y };

        for ( int i = 0; i < 3; i++ ) {
            std::string value{ };

            switch ( i ) {
            case 0:
                value = "AA missile speed: " + std::to_string( m_autoattack_missile_speed );
                break;
            case 1:
            {
                auto aimgr = g_local->get_ai_manager( );
                if ( !aimgr ) continue;

                value = "Velocity [ " + std::to_string( aimgr->velocity.x ) + ", " + std::to_string( aimgr->velocity.y )
                    +
                    ", " + std::to_string( aimgr->velocity.z ) + " ]";
                break;
            }
            default:
                continue;
            }

            size = g_render->get_text_size( value, g_fonts->get_zabel_16px( ), 16 );
            g_render->text( draw_position, Color::white( ), g_fonts->get_zabel_16px( ), value.data( ), 16 );

            draw_position.y += size.y;
        }
    }

    void Orbwalker::ClearAnimations() {
        const auto to_remove = std::ranges::remove_if(animationList,
                                                      [&](const AnimationInstance &instance) -> bool
                                                      { return instance.start_time + instance.duration < *g_time; });

        if (to_remove.empty()) { return; }

        animationList.erase(to_remove.begin(), to_remove.end());
    }

    void Orbwalker::DrawAnimations() {

         ClearAnimations();

        for (const auto inst : animationList)
        {
            float       modifier     = std::clamp((*g_time - inst.start_time) / inst.duration, 0.f, 1.f);
            const float raw_modifier = modifier;
            modifier                 = 1.f - std::pow(1.f - modifier, 5.f);

            const float radius      = inst.radius;
            const float draw_radius = radius * modifier;

            float thickness      = 3.f + inst.radius;
            float draw_thickness = thickness + (7.f - 7.f * modifier);

            const int alpha = static_cast<int>(255.f - 255.f * raw_modifier);

            Color draw_color = inst.color;
            draw_color.a     = alpha;

            g_render->line_3d(inst.position, inst.second_position, draw_color, draw_thickness);
        }
    }


}
