#include "pch.hpp"

#include "helper.hpp"

#include <string>

#include "buff_cache.hpp"
#include "entity_list.hpp"
#include "orbwalker.hpp"
#include "prediction.hpp"
#include "../sdk/globals.hpp"
#include "../sdk/game/spell_cast_info.hpp"
#include "../sdk/game/spell_data.hpp"
#include "../sdk/game/spell_info.hpp"

namespace features::helper {
    auto get_current_hero( ) -> EHeroes{
        const auto name = std::string( g_local->champion_name.text );

        switch ( rt_hash( name.data() ) ) {
        case ct_hash( "Aatrox" ):
            return EHeroes::aatrox;
        case ct_hash( "Ahri" ):
            return EHeroes::ahri;
        case ct_hash( "Akali" ):
            return EHeroes::akali;
        case ct_hash( "Akshan" ):
            return EHeroes::akshan;
        case ct_hash( "Ashe" ):
            return EHeroes::ashe;
        case ct_hash( "AurelionSol" ):
            return EHeroes::aurelion_sol;
        case ct_hash( "Belveth" ):
            return EHeroes::belveth;
        case ct_hash( "Blitzcrank" ):
            return EHeroes::blitzcrank;
        case ct_hash( "Brand" ):
            return EHeroes::brand;
        case ct_hash( "Cassiopeia" ):
            return EHeroes::cassiopeia;
        case ct_hash( "Caitlyn" ):
            return EHeroes::caitlyn;
        case ct_hash( "Corki" ):
            return EHeroes::corki;
        case ct_hash( "Darius" ):
            return EHeroes::darius;
        case ct_hash( "Draven" ):
            return EHeroes::draven;
        case ct_hash( "DrMundo" ):
            return EHeroes::dr_mundo;
        case ct_hash( "Ezreal" ):
            return EHeroes::ezreal;
        case ct_hash( "Fiora" ):
            return EHeroes::fiora;
        case ct_hash( "Gangplank" ):
            return EHeroes::gangplank;
        case ct_hash( "Garen" ):
            return EHeroes::garen;
        case ct_hash( "Graves" ):
            return EHeroes::graves;
        case ct_hash( "Illaoi" ):
            return EHeroes::illaoi;
        case ct_hash( "Irelia" ):
            return EHeroes::irelia;
        case ct_hash( "Janna" ):
            return EHeroes::janna;
        case ct_hash( "Jax" ):
            return EHeroes::jax;
        case ct_hash( "Jinx" ):
            return EHeroes::jinx;
        case ct_hash( "Jhin" ):
            return EHeroes::jhin;
        case ct_hash( "Kaisa" ):
            return EHeroes::kaisa;
        case ct_hash( "Kalista" ):
            return EHeroes::kalista;
        case ct_hash( "Karthus" ):
            return EHeroes::karthus;
        case ct_hash( "Kassadin" ):
            return EHeroes::kassadin;
        case ct_hash( "Katarina" ):
            return EHeroes::katarina;
        case ct_hash( "Kayle" ):
            return EHeroes::kayle;
        case ct_hash( "Kindred" ):
            return EHeroes::kindred;
        case ct_hash( "KogMaw" ):
            return EHeroes::kog_maw;
        case ct_hash( "Leblanc" ):
            return EHeroes::leblanc;
        case ct_hash( "LeeSin" ):
            return EHeroes::lee_sin;
        case ct_hash( "Lucian" ):
            return EHeroes::lucian;
        case ct_hash( "Lux" ):
            return EHeroes::lux;
        case ct_hash( "Malzahar" ):
            return EHeroes::malzahar;
        case ct_hash( "MissFortune" ):
            return EHeroes::miss_fortune;
        case ct_hash( "MonkeyKing" ):
            return EHeroes::monkey_king;
        case ct_hash( "Nilah" ):
            return EHeroes::nilah;
        case ct_hash( "Olaf" ):
            return EHeroes::olaf;
        case ct_hash( "Orianna" ):
            return EHeroes::orianna;
        case ct_hash( "Pyke" ):
            return EHeroes::pyke;
        case ct_hash( "Riven" ):
            return EHeroes::riven;
        case ct_hash( "Ryze" ):
            return EHeroes::ryze;
        case ct_hash( "Samira" ):
            return EHeroes::samira;
        case ct_hash( "Senna" ):
            return EHeroes::senna;
        case ct_hash( "Sett" ):
            return EHeroes::sett;
        case ct_hash( "Shaco" ):
            return EHeroes::shaco;
        case ct_hash( "Sion" ):
            return EHeroes::sion;
        case ct_hash( "Sivir" ):
            return EHeroes::sivir;
        case ct_hash( "Tristana" ):
            return EHeroes::tristana;
        case ct_hash( "Trundle" ):
            return EHeroes::trundle;
        case ct_hash( "Twitch" ):
            return EHeroes::twitch;
        case ct_hash( "TwistedFate" ):
            return EHeroes::twisted_fate;
        case ct_hash( "Urgot" ):
            return EHeroes::urgot;
        case ct_hash( "Yone" ):
            return EHeroes::yone;
        case ct_hash( "Varus" ):
            return EHeroes::varus;
        case ct_hash( "Vayne" ):
            return EHeroes::vayne;
        case ct_hash( "Veigar" ):
            return EHeroes::veigar;
        case ct_hash( "Viktor" ):
            return EHeroes::viktor;
        case ct_hash( "Vladimir" ):
            return EHeroes::vladimir;
        case ct_hash( "Xayah" ):
            return EHeroes::xayah;
        case ct_hash( "Xerath" ):
            return EHeroes::xerath;
        case ct_hash( "Yorick" ):
            return EHeroes::yorick;
        case ct_hash( "Vex" ):
            return EHeroes::vex;
        case ct_hash( "Zed" ):
            return EHeroes::zed;
        case ct_hash( "Zeri" ):
            return EHeroes::zeri;
        case ct_hash( "Ziggs" ):
            return EHeroes::ziggs;
        case ct_hash( "Zoe" ):
            return EHeroes::zoe;
        default:
            return EHeroes::null;
        }
    }

    auto get_champion_hero( const hash_t name_hash ) -> EHeroes{
        switch ( name_hash ) {
        case ct_hash( "Aatrox" ):
            return EHeroes::aatrox;
        case ct_hash( "Ahri" ):
            return EHeroes::ahri;
        case ct_hash( "Akali" ):
            return EHeroes::akali;
        case ct_hash( "Akshan" ):
            return EHeroes::akshan;
        case ct_hash( "Ashe" ):
            return EHeroes::ashe;
        case ct_hash( "AurelionSol" ):
            return EHeroes::aurelion_sol;
        case ct_hash( "Belveth" ):
            return EHeroes::belveth;
        case ct_hash( "Blitzcrank" ):
            return EHeroes::blitzcrank;
        case ct_hash( "Brand" ):
            return EHeroes::brand;
        case ct_hash( "Cassiopeia" ):
            return EHeroes::cassiopeia;
        case ct_hash( "Caitlyn" ):
            return EHeroes::caitlyn;
        case ct_hash( "Corki" ):
            return EHeroes::corki;
        case ct_hash( "Darius" ):
            return EHeroes::darius;
        case ct_hash( "Draven" ):
            return EHeroes::draven;
        case ct_hash( "DrMundo" ):
            return EHeroes::dr_mundo;
        case ct_hash( "Ezreal" ):
            return EHeroes::ezreal;
        case ct_hash( "Fiora" ):
            return EHeroes::fiora;
        case ct_hash( "Gangplank" ):
            return EHeroes::gangplank;
        case ct_hash( "Garen" ):
            return EHeroes::garen;
        case ct_hash( "Graves" ):
            return EHeroes::graves;
        case ct_hash( "Illaoi" ):
            return EHeroes::illaoi;
        case ct_hash( "Irelia" ):
            return EHeroes::irelia;
        case ct_hash( "Janna" ):
            return EHeroes::janna;
        case ct_hash( "Jax" ):
            return EHeroes::jax;
        case ct_hash( "Jinx" ):
            return EHeroes::jinx;
        case ct_hash( "Jhin" ):
            return EHeroes::jhin;
        case ct_hash( "Kaisa" ):
            return EHeroes::kaisa;
        case ct_hash( "Kalista" ):
            return EHeroes::kalista;
        case ct_hash( "Karthus" ):
            return EHeroes::karthus;
        case ct_hash( "Kassadin" ):
            return EHeroes::kassadin;
        case ct_hash( "Katarina" ):
            return EHeroes::katarina;
        case ct_hash( "Kayle" ):
            return EHeroes::kayle;
        case ct_hash( "Kindred" ):
            return EHeroes::kindred;
        case ct_hash( "KogMaw" ):
            return EHeroes::kog_maw;
        case ct_hash( "Leblanc" ):
            return EHeroes::leblanc;
        case ct_hash( "LeeSin" ):
            return EHeroes::lee_sin;
        case ct_hash( "Lucian" ):
            return EHeroes::lucian;
        case ct_hash( "Lux" ):
            return EHeroes::lux;
        case ct_hash( "Malzahar" ):
            return EHeroes::malzahar;
        case ct_hash( "MissFortune" ):
            return EHeroes::miss_fortune;
        case ct_hash( "MonkeyKing" ):
            return EHeroes::monkey_king;
        case ct_hash( "Nilah" ):
            return EHeroes::nilah;
        case ct_hash( "Olaf" ):
            return EHeroes::olaf;
        case ct_hash( "Orianna" ):
            return EHeroes::orianna;
        case ct_hash( "Pyke" ):
            return EHeroes::pyke;
        case ct_hash( "Riven" ):
            return EHeroes::riven;
        case ct_hash( "Ryze" ):
            return EHeroes::ryze;
        case ct_hash( "Samira" ):
            return EHeroes::samira;
        case ct_hash( "Senna" ):
            return EHeroes::senna;
        case ct_hash( "Sett" ):
            return EHeroes::sett;
        case ct_hash( "Shaco" ):
            return EHeroes::shaco;
        case ct_hash( "Sion" ):
            return EHeroes::sion;
        case ct_hash( "Sivir" ):
            return EHeroes::sivir;
        case ct_hash( "Tristana" ):
            return EHeroes::tristana;
        case ct_hash( "Twitch" ):
            return EHeroes::twitch;
        case ct_hash( "TwistedFate" ):
            return EHeroes::twisted_fate;
        case ct_hash( "Urgot" ):
            return EHeroes::urgot;
        case ct_hash( "Yone" ):
            return EHeroes::yone;
        case ct_hash( "Varus" ):
            return EHeroes::varus;
        case ct_hash( "Vayne" ):
            return EHeroes::vayne;
        case ct_hash( "Veigar" ):
            return EHeroes::veigar;
        case ct_hash( "Viktor" ):
            return EHeroes::viktor;
        case ct_hash( "Vladimir" ):
            return EHeroes::vladimir;
        case ct_hash( "Xayah" ):
            return EHeroes::xayah;
        case ct_hash( "Xerath" ):
            return EHeroes::xerath;
        case ct_hash( "Yorick" ):
            return EHeroes::yorick;
        case ct_hash( "Vex" ):
            return EHeroes::vex;
        case ct_hash( "Zed" ):
            return EHeroes::zed;
        case ct_hash( "Zeri" ):
            return EHeroes::zeri;
        case ct_hash( "Ziggs" ):
            return EHeroes::ziggs;
        case ct_hash( "Zoe" ):
            return EHeroes::zoe;
        default:
            return EHeroes::null;
        }
    }

    auto calculate_damage( const float raw_damage, const int16_t target_index, const bool physical_damage ) -> float{
        auto obj = g_entity_list.get_by_index( target_index );
        if ( !obj ) return 0.f;

        obj.update( );

        if ( physical_damage ) {
            auto       actual_armor   = obj->total_armor;
            const auto flat_armor_pen = g_local->get_lethality( ) * ( 0.6f + 0.4f * static_cast< float >( g_local->
                    level ) /
                18.f );

            actual_armor *= g_local->get_armor_penetration_percent( );
            actual_armor -= flat_armor_pen;

            return raw_damage * ( 100.f / ( 100.f + actual_armor ) );
        }

        auto actual_mr = obj->total_mr;

        actual_mr *= g_local->get_magic_penetration_percent( );
        actual_mr -= g_local->get_flat_magic_penetration( );

        return raw_damage * ( 100.f / ( 100.f + actual_mr ) );
    }

    auto process_damage(
        const float   raw_damage,
        const int16_t source_index,
        const int16_t target_index,
        const bool    physical_damage
    ) -> float{
        const auto source = g_entity_list.get_by_index( source_index );
        if ( !source ) return 0.f;

        const auto target = g_entity_list.get_by_index( target_index );
        if ( !target ) return 0.f;

        if ( physical_damage ) {
            auto       actual_armor   = target->total_armor;
            const auto flat_armor_pen = source->get_lethality( ) * ( 0.6f + 0.4f * static_cast< float >( source->
                level ) / 18.f );

            actual_armor *= source->get_armor_penetration_percent( );
            actual_armor -= flat_armor_pen;

            return raw_damage * ( 100.f / ( 100.f + actual_armor ) );
        }

        auto actual_mr = target->total_mr;

        actual_mr *= source->get_magic_penetration_percent( );
        actual_mr -= source->get_flat_magic_penetration( );

        return raw_damage * ( 100.f / ( 100.f + actual_mr ) );
    }

    auto get_champion_autoattack_damage(
        int16_t source_index,
        int16_t target_index,
        float   server_cast_time,
        bool    is_melee,
        bool    on_hit,
        bool    will_crit
    ) -> float{
        auto source = g_entity_list.get_by_index( source_index );
        if ( !source ) return 0.f;

        auto target = g_entity_list.get_by_index( target_index );
        if ( !target ) return 0.f;

        source.update( );
        target.update( );

        if ( !target->is_hero( ) && target->has_special_minion_health( ) ) {
            switch ( target->get_ward_type( ) ) {
            case Object::EWardType::blue:
            case Object::EWardType::control:
            case Object::EWardType::teemo_shroom:
            case Object::EWardType::jhin_trap:
            case Object::EWardType::zombie:
            case Object::EWardType::nidalee_trap:
            case Object::EWardType::normal:
            case Object::EWardType::fiddlesticks_effigy:
                return 1.f;
            default:

                switch ( rt_hash( target->get_name( ).data( ) ) ) {
                case ct_hash( "MalzaharVoidling" ):
                    return 9999.f;
                default:
                    break;
                }

                break;
            }
        }

        auto total_damage = process_damage(
            will_crit ? source->attack_damage( ) * 1.75f : source->attack_damage( ),
            source_index,
            target_index,
            true
        );

        switch ( get_champion_hero( rt_hash( source->champion_name.text ) ) ) {
        case EHeroes::kayle:
        {
            const auto buffed = !!g_features->buff_cache->get_buff( source->index, ct_hash( "KayleE" ) );

            if ( const auto e_spell = source->spell_book.get_spell_slot( ESpellSlot::e ) ) {
                const auto level          = static_cast< float >( e_spell->level );
                auto       passive_damage = 10.f + level * 5.f + source->bonus_attack * .1f + source->ability_power( ) *
                    .25f;
                total_damage += process_damage( passive_damage, source_index, target_index, false );

                if ( buffed ) {
                    auto active_damage = ( 7.f + level ) * ( 2.f / 100.f ) * source->ability_power( );
                    total_damage += process_damage( active_damage, source_index, target_index, false );
                }
            }
            break;
        }
        case EHeroes::kalista:
            // kalista passive makes her attacks deal only 90% of attack damage
            total_damage *= 0.9f;
            break;
        case EHeroes::belveth:
            // belveth passive makes her attacks deal only 75% of attack damage
            total_damage *= 0.75f;
            break;
        case EHeroes::kaisa:
        {
            auto passive_damage{ std::min( 5.f + 1.0588235f * ( source->level - 1 ), 23.f ) };

            auto       ap_modifier{ 0.15f };
            const auto buff = g_features->buff_cache->get_buff( target->index, ct_hash( "kaisapassivemarker" ) );
            if ( buff && buff->buff_data->end_time > server_cast_time ) ap_modifier += 0.025f * buff->stacks( );

            passive_damage += source->ability_power( ) * ap_modifier;

            total_damage += process_damage( passive_damage, source_index, target_index, false );
            break;
        }
        case EHeroes::caitlyn:
            if ( g_features->buff_cache->get_buff( source->index, ct_hash( "caitlynpassivedriver" ) ) ) {
                float damage_modifier{ };

                if ( target->is_hero( ) ) damage_modifier = source->level > 12 ? 1.2f : source->level > 6 ? 0.9f : 0.6f;
                else damage_modifier = source->level > 12 ? 1.2f : source->level > 6 ? 1.15f : 1.1f;

                damage_modifier += std::min( source->crit_chance * 1.3125f, 1.3125f );
                auto extra_damage = source->attack_damage( ) * damage_modifier;

                total_damage += process_damage( extra_damage, source_index, target_index, true );
            }
            break;
        case EHeroes::jinx:
            if ( g_features->buff_cache->get_buff( source->index, ct_hash( "JinxQ" ) ) ) total_damage *= 1.1f;
            break;
        case EHeroes::zeri:
        {
            float base_damage{ };

            if ( g_features->buff_cache->get_buff( source->index, ct_hash( "ZeriQPassiveReady" ) ) ) {
                base_damage = get_zeri_charge_base_damage( source->level );
                base_damage += source->ability_power( ) * 1.1f;

                auto max_health_modifier{ get_zeri_charge_max_health_modifier( source->level ) };

                base_damage += target->max_health * max_health_modifier;
                total_damage = process_damage( base_damage, source_index, target_index, false );
            } else {
                auto execute_threshold = zeri_execute_threshold( );
                auto enemy_hp          = target->is_lane_minion( )
                                             ? g_features->prediction->predict_minion_health(
                                                 target_index,
                                                 server_cast_time - *g_time
                                             )
                                             : target->health;

                if ( enemy_hp <= execute_threshold ) base_damage = 9999.f;
                else {
                    base_damage = 10.f + 15.f / 17.f + ( source->level - 1 ) * ( 0.7025f + 0.0175f * ( source->level -
                        1 ) );
                    base_damage += source->ability_power( ) * 0.03f;
                }

                total_damage = process_damage( base_damage, source_index, target_index, false );
            }
            break;
        }
        case EHeroes::jax:
        {
            float base_damage{ };

            if ( g_features->buff_cache->get_buff( source->index, ct_hash( "JaxEmpowerTwo" ) ) ) {
                if ( auto w_spell = source->spell_book.get_spell_slot( ESpellSlot::w ) ) {
                    switch ( w_spell->level ) {
                    case 1:
                        base_damage = 50.f;
                        break;
                    case 2:
                        base_damage = 85.f;
                        break;
                    case 3:
                        base_damage = 120.f;
                        break;
                    case 4:
                        base_damage = 155.f;
                        break;
                    case 5:
                        base_damage = 190.f;
                        break;
                    default:
                        base_damage = 0.f;
                        break;
                    }

                    base_damage += source->ability_power( ) * 0.6f;

                    total_damage += process_damage( base_damage, source_index, target_index, false );
                }
            }
            break;
        }
        case EHeroes::zoe:
            if ( g_features->buff_cache->get_buff( source->index, ct_hash( "zoepassivesheenbuff" ) ) ) {
                float bonus_dmg{ };
                switch ( source->level ) {
                case 1:
                    bonus_dmg = 16.f;
                    break;
                case 2:
                    bonus_dmg = 20.f;
                    break;
                case 3:
                    bonus_dmg = 24.f;
                    break;
                case 4:
                    bonus_dmg = 28.f;
                    break;
                case 5:
                    bonus_dmg = 32.f;
                    break;
                case 6:
                    bonus_dmg = 36.f;
                    break;
                case 7:
                    bonus_dmg = 42.f;
                    break;
                case 8:
                    bonus_dmg = 48.f;
                    break;
                case 9:
                    bonus_dmg = 54.f;
                    break;
                case 10:
                    bonus_dmg = 60.f;
                    break;
                case 11:
                    bonus_dmg = 66.f;
                    break;
                case 12:
                    bonus_dmg = 74.f;
                    break;
                case 13:
                    bonus_dmg = 82.f;
                    break;
                case 14:
                    bonus_dmg = 90.f;
                    break;
                case 15:
                    bonus_dmg = 100.f;
                    break;
                case 16:
                    bonus_dmg = 110.f;
                    break;
                case 17:
                    bonus_dmg = 120.f;
                    break;
                default:
                    bonus_dmg = 130.f;
                    break;
                }

                total_damage += process_damage( bonus_dmg, source_index, target_index, false );
            }
            break;
        case EHeroes::draven:
            if ( g_features->buff_cache->get_buff( source->index, ct_hash( "DravenSpinningAttack" ) ) ) {
                auto q_spell = source->spell_book.get_spell_slot( ESpellSlot::q );
                if ( !q_spell ) break;

                float base_dmg{ };
                switch ( q_spell->level ) {
                case 1:
                    base_dmg = 40.f + source->bonus_attack_damage( ) * 0.75f;
                    break;
                case 2:
                    base_dmg = 45.f + source->bonus_attack_damage( ) * 0.85f;
                    break;
                case 3:
                    base_dmg = 50.f + source->bonus_attack_damage( ) * 0.95f;
                    break;
                case 4:
                    base_dmg = 55.f + source->bonus_attack_damage( ) * 1.05f;
                    break;
                case 5:
                    base_dmg = 60.f + source->bonus_attack_damage( ) * 1.15f;
                    break;
                default:
                    break;
                }

                total_damage += process_damage( base_dmg, source_index, target_index, true );
            }
            break;
        case EHeroes::akali:
            if ( g_features->buff_cache->get_buff( source->index, ct_hash( "AkaliPWeapon" ) ) ) {
                float base_dmg{ };
                switch ( source->level ) {
                case 0:
                case 1:
                    base_dmg = 35.f;
                    break;
                case 2:
                    base_dmg = 38.f;
                    break;
                case 3:
                    base_dmg = 41.f;
                    break;
                case 4:
                    base_dmg = 44.f;
                    break;
                case 5:
                    base_dmg = 47.f;
                    break;
                case 6:
                    base_dmg = 50.f;
                    break;
                case 7:
                    base_dmg = 53.f;
                    break;
                case 8:
                    base_dmg = 62.f;
                    break;
                case 9:
                    base_dmg = 71.f;
                    break;
                case 10:
                    base_dmg = 80.f;
                    break;
                case 11:
                    base_dmg = 89.f;
                    break;
                case 12:
                    base_dmg = 98.f;
                    break;
                case 13:
                    base_dmg = 107.f;
                    break;
                case 14:
                    base_dmg = 122.f;
                    break;
                case 15:
                    base_dmg = 137.f;
                    break;
                case 16:
                    base_dmg = 152.f;
                    break;
                case 17:
                    base_dmg = 167.f;
                    break;
                default:
                    base_dmg = 182.f;
                    break;
                }

                auto passive_damage =
                    base_dmg + source->bonus_attack_damage( ) * 0.6f + source->ability_power( ) * 0.55f;

                total_damage += process_damage( passive_damage, source_index, target_index, false );
            }
            break;
        case EHeroes::zed:
            if ( target->health <= target->max_health * 0.5f ) {
                // if enemy has passive cooldown buff, dont add extra damage to aa
                if ( target->is_hero( ) && g_features->buff_cache->get_buff(
                    target_index,
                    ct_hash( "zedpassivecd" )
                ) )
                    break;

                float bonus_damage{ };
                if ( source->level >= 17 ) bonus_damage = 0.1f * target->max_health;
                else if ( source->level >= 13 ) bonus_damage = 0.08f * target->max_health;
                else bonus_damage                            = 0.06f * target->max_health;

                total_damage += process_damage( bonus_damage, source_index, target_index, false );
            }
            break;
        case EHeroes::vayne:
        {
            if ( g_features->buff_cache->get_buff( source->index, ct_hash( "vaynetumblebonus" ) ) ) {
                auto spell = source->spell_book.get_spell_slot( ESpellSlot::q );
                if ( !spell ) break;

                auto ad_modifier = 0.55f + 0.05f * spell->level;

                total_damage += process_damage(
                    source->attack_damage( ) * ad_modifier,
                    source_index,
                    target_index,
                    true
                );
            }

            auto buff = g_features->buff_cache->get_buff( target_index, ct_hash( "VayneSilveredDebuff" ) );
            if ( buff && buff->stacks( ) == 2 ) {
                auto spell = source->spell_book.get_spell_slot( ESpellSlot::w );
                if ( !spell ) break;

                auto max_health_percent{ 0.02f + 0.02f * spell->level };
                auto true_damage = max_health_percent * target->max_health;

                if ( true_damage > 200.f && target->is_minion( ) && target->is_jungle_monster( ) ) true_damage = 200.f;

                total_damage += true_damage;
            }
            break;
        }
        case EHeroes::senna:
        {
            if ( target->is_hero( ) && g_features->buff_cache->get_buff(
                target->index,
                ct_hash( "sennapassivemarker" )
            ) ) {
                auto hp_modifier  = std::min( source->level * 0.01f, 0.1f );
                auto bonus_damage = target->health * hp_modifier;

                total_damage += process_damage( bonus_damage, source_index, target_index, true );
            }

            total_damage += process_damage( source->attack_damage( ) * 0.2f, source_index, target_index, true );
            break;
        }
        case EHeroes::varus:
        {
            if ( const auto slot = source->spell_book.get_spell_slot( ESpellSlot::w ) ) {
                auto bonus_damage{ source->ability_power( ) * 0.3f };

                switch ( slot->level ) {
                case 1:
                    bonus_damage += 7.f;
                    break;
                case 2:
                    bonus_damage += 12.f;
                    break;
                case 3:
                    bonus_damage += 17.f;
                    break;
                case 4:
                    bonus_damage += 22.f;
                    break;
                case 5:
                    bonus_damage += 27.f;
                    break;
                default:
                    break;
                }

                total_damage += process_damage( bonus_damage, source_index, target_index, false );
            }


            break;
        }
        case EHeroes::corki:
        {
            auto raw_damage = will_crit ? source->attack_damage( ) * 1.75f : source->attack_damage( );

            total_damage = process_damage( raw_damage * 0.8f, source_index, target_index, false ) +
                process_damage( raw_damage * 0.2f, source_index, target_index, true );
            break;
        }
        case EHeroes::lux:
        {
            const auto buff = g_features->buff_cache->get_buff( target_index, ct_hash( "LuxIlluminatingFraulein" ) );
            if ( buff && buff->buff_data->end_time > server_cast_time ) {
                auto passive_damage = 10.f + 10.f * std::min( source->level, 18 ) + source->ability_power( ) * 0.2f;
                total_damage += process_damage( passive_damage, source_index, target_index, false );
            }

            break;
        }
        default:
            break;
        }

        if ( on_hit ) {
            total_damage += get_champion_onhit_damage(
                source_index,
                target_index,
                is_melee,
                server_cast_time
            );
        }

        return total_damage;
    }

    auto get_aa_damage( int16_t target_index, bool on_hit ) -> float{
        auto& obj = g_entity_list.get_by_index( target_index );
        if ( !obj ) return 0.f;

        obj.update( );

        if ( obj->has_special_minion_health( ) ) {
            switch ( obj->get_ward_type( ) ) {
            case Object::EWardType::blue:
            case Object::EWardType::control:
            case Object::EWardType::teemo_shroom:
            case Object::EWardType::jhin_trap:
            case Object::EWardType::zombie:
            case Object::EWardType::nidalee_trap:
            case Object::EWardType::normal:
            case Object::EWardType::fiddlesticks_effigy:
                return 1.f;
            default:

                switch ( rt_hash( obj->get_name( ).data( ) ) ) {
                case ct_hash( "MalzaharVoidling" ):
                    return 9999.f;
                default:
                    break;
                }

                break;
            }
        }

        auto calculate_crit{ std::clamp( g_local->crit_chance, 0.f, 1.f ) >= 0.95f };
        if ( obj->is_hero( ) ) calculate_crit = g_local->crit_chance >= 0.75f;

        auto damage = calculate_damage(
            calculate_crit ? g_local->attack_damage( ) * 1.75f : g_local->attack_damage( ),
            obj->index,
            true
        );

        switch ( get_current_hero( ) ) {
        case EHeroes::kayle:
        {
            const auto buffed = static_cast< bool >( g_features->buff_cache->get_buff(
                g_local->index,
                ct_hash( "KayleE" )
            ) );
            auto e_spell = g_local->spell_book.get_spell_slot( ESpellSlot::e );
            damage       = calculate_damage( g_local->attack_damage( ), obj->index, true );

            if ( e_spell ) {
                const auto level = static_cast< float >( e_spell->level );
                damage += calculate_damage(
                    ( 10.f + ( level * 5.f ) ) + ( g_local->bonus_attack * .1f ) + ( g_local->ability_power( ) * .2f ),
                    obj->index,
                    false
                );

                if ( buffed ) {
                    damage += calculate_damage(
                        ( 7.f + level ) * ( 2.f / 100.f ) * g_local->ability_power( ),
                        obj->index,
                        false
                    );
                }
            }
            break;
        }
        case EHeroes::kalista:
            // kalista passive makes her attacks deal only 90% of attack damage
            damage = calculate_damage( g_local->attack_damage( ) * 0.9f, obj->index, true );
            break;
        case EHeroes::belveth:
            // belveth passive makes her attacks deal only 75% of attack damage
            damage = calculate_damage( g_local->attack_damage( ) * 0.75f, obj->index, true );
            break;
        case EHeroes::kaisa:
        {
            float      base_dmg;
            const auto level = g_local->level;

            if ( level < 3 ) base_dmg = 5;
            else if ( level < 6 ) base_dmg = 8;
            else if ( level < 9 ) base_dmg = 11;
            else if ( level < 11 ) base_dmg = 14;
            else if ( level < 14 ) base_dmg = 17;
            else if ( level < 17 ) base_dmg = 20;
            else base_dmg                   = 23;

            base_dmg += g_local->ability_power( ) * 0.1f;

            damage += calculate_damage( base_dmg, obj->index, false );
            break;
        }
        case EHeroes::caitlyn:
            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "caitlynpassivedriver" ) ) ) {
                float damage_modifier{ };

                if ( obj->is_hero( ) ) damage_modifier = g_local->level > 12 ? 1.2f : g_local->level > 6 ? 0.9f : 0.6f;
                else damage_modifier                   = g_local->level > 12 ? 1.2f : g_local->level > 6 ? 1.15f : 1.1f;

                damage_modifier += std::min( g_local->crit_chance * 1.3125f, 1.3125f );
                auto extra_damage = g_local->attack_damage( ) * damage_modifier;

                damage += calculate_damage( extra_damage, obj->index, true );
            }
            break;
        case EHeroes::jinx:
        {
            const auto spell = g_local->spell_book.get_spell_slot( ESpellSlot::q );
            if ( !spell ) break;

            const auto state = spell->get_usable_state( );

            if ( state == 0 ) {
                auto attack_damage = g_local->attack_damage( ) * 1.1f;
                damage = calculate_damage( calculate_crit ? attack_damage * 1.75f : attack_damage, obj->index, true );
            }

            break;
        }
        case EHeroes::zeri:
        {
            float base_damage{ };

            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "ZeriQPassiveReady" ) ) ) {
                base_damage = get_zeri_charge_base_damage( );
                base_damage += g_local->ability_power( ) * 1.1f;

                auto max_health_modifier{ get_zeri_charge_max_health_modifier( ) };

                base_damage += obj->max_health * max_health_modifier;
                damage = calculate_damage( base_damage, obj->index, false );
            } else {
                auto execute_threshold = zeri_execute_threshold( );
                auto enemy_hp          = obj->is_lane_minion( )
                                             ? g_features->prediction->predict_health(
                                                 obj.get( ),
                                                 g_features->orbwalker->get_attack_cast_delay( )
                                             )
                                             : obj->health;


                if ( enemy_hp <= execute_threshold ) base_damage = 9999.f;
                else {
                    base_damage = 10.f + 15.f / 17.f + static_cast< float >( ( g_local->level - 1 ) ) * ( 0.7025f +
                        0.0175f * static_cast
                        < float >( g_local->level - 1 )
                    );
                    base_damage += g_local->ability_power( ) * 0.03f;
                }

                damage = calculate_damage( base_damage, obj->index, false );
            }
            break;
        }
        case EHeroes::jax:
        {
            float base_damage{ };

            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "JaxEmpowerTwo" ) ) ) {
                //auto w_spell = g_local->spell_book.get_spell_slot(e_spell_slot::w);
                if ( auto w_spell = g_local->spell_book.get_spell_slot( ESpellSlot::w ) ) {
                    switch ( w_spell->level ) {
                    case 1:
                        base_damage = 40.f;
                        break;
                    case 2:
                        base_damage = 75.f;
                        break;
                    case 3:
                        base_damage = 110.f;
                        break;
                    case 4:
                        base_damage = 145.f;
                        break;
                    case 5:
                        base_damage = 180.f;
                        break;
                    default: ;
                    }

                    damage += calculate_damage( base_damage + g_local->ability_power( ) * 0.6f, obj->index, false );
                }
            }
            break;
        }
        case EHeroes::zoe:
            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "zoepassivesheenbuff" ) ) ) {
                float bonus_dmg{ };
                switch ( g_local->level ) {
                case 1:
                    bonus_dmg = 16.f;
                    break;
                case 2:
                    bonus_dmg = 20.f;
                    break;
                case 3:
                    bonus_dmg = 24.f;
                    break;
                case 4:
                    bonus_dmg = 28.f;
                    break;
                case 5:
                    bonus_dmg = 32.f;
                    break;
                case 6:
                    bonus_dmg = 36.f;
                    break;
                case 7:
                    bonus_dmg = 42.f;
                    break;
                case 8:
                    bonus_dmg = 48.f;
                    break;
                case 9:
                    bonus_dmg = 54.f;
                    break;
                case 10:
                    bonus_dmg = 60.f;
                    break;
                case 11:
                    bonus_dmg = 66.f;
                    break;
                case 12:
                    bonus_dmg = 74.f;
                    break;
                case 13:
                    bonus_dmg = 82.f;
                    break;
                case 14:
                    bonus_dmg = 90.f;
                    break;
                case 15:
                    bonus_dmg = 100.f;
                    break;
                case 16:
                    bonus_dmg = 110.f;
                    break;
                case 17:
                    bonus_dmg = 120.f;
                    break;
                default:
                    bonus_dmg = 130.f;
                    break;
                }

                damage += calculate_damage( bonus_dmg, obj->index, false );
            }
            break;
        case EHeroes::draven:
            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "DravenSpinningAttack" ) ) ) {
                auto q_spell = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( !q_spell ) break;

                float base_dmg{ };
                switch ( q_spell->level ) {
                case 1:
                    base_dmg = 40.f + g_local->bonus_attack_damage( ) * 0.75f;
                    break;
                case 2:
                    base_dmg = 45.f + g_local->bonus_attack_damage( ) * 0.85f;
                    break;
                case 3:
                    base_dmg = 50.f + g_local->bonus_attack_damage( ) * 0.95f;
                    break;
                case 4:
                    base_dmg = 55.f + g_local->bonus_attack_damage( ) * 1.05f;
                    break;
                case 5:
                    base_dmg = 60.f + g_local->bonus_attack_damage( ) * 1.15f;
                    break;
                default:
                    break;
                }

                damage += calculate_damage( base_dmg, obj->index, true );
            }
            break;
        case EHeroes::akali:
            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "AkaliPWeapon" ) ) ) {
                float base_dmg{ };
                switch ( g_local->level ) {
                case 0:
                case 1:
                    base_dmg = 35.f;
                    break;
                case 2:
                    base_dmg = 38.f;
                    break;
                case 3:
                    base_dmg = 41.f;
                    break;
                case 4:
                    base_dmg = 44.f;
                    break;
                case 5:
                    base_dmg = 47.f;
                    break;
                case 6:
                    base_dmg = 50.f;
                    break;
                case 7:
                    base_dmg = 53.f;
                    break;
                case 8:
                    base_dmg = 62.f;
                    break;
                case 9:
                    base_dmg = 71.f;
                    break;
                case 10:
                    base_dmg = 80.f;
                    break;
                case 11:
                    base_dmg = 89.f;
                    break;
                case 12:
                    base_dmg = 98.f;
                    break;
                case 13:
                    base_dmg = 107.f;
                    break;
                case 14:
                    base_dmg = 122.f;
                    break;
                case 15:
                    base_dmg = 137.f;
                    break;
                case 16:
                    base_dmg = 152.f;
                    break;
                case 17:
                    base_dmg = 167.f;
                    break;
                default:
                    base_dmg = 182.f;
                    break;
                }

                damage += calculate_damage(
                    base_dmg + g_local->bonus_attack_damage( ) * 0.6f + g_local->ability_power( ) * 0.55f,
                    target_index,
                    false
                );
            }
            break;
        case EHeroes::zed:
            if ( obj->health <= obj->max_health * 0.5f ) {
                // if enemy has passive cooldown buff, dont add extra damage to aa
                if ( obj->is_hero( ) && g_features->buff_cache->get_buff(
                    target_index,
                    ct_hash( "zedpassivecd" )
                ) )
                    break;

                float bonus_damage{ };
                if ( g_local->level >= 17 ) bonus_damage = 0.1f * obj->max_health;
                else if ( g_local->level >= 13 ) bonus_damage = 0.08f * obj->max_health;
                else bonus_damage                             = 0.06f * obj->max_health;

                damage += calculate_damage( bonus_damage, target_index, false );
            }

            break;
        case EHeroes::vayne:
        {
            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "vaynetumblebonus" ) ) ) {
                auto spell = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( !spell ) break;

                auto ad_modifier = 0.55f + 0.05f * spell->level;

                damage += calculate_damage( g_local->attack_damage( ) * ad_modifier, target_index, true );
            }

            auto buff = g_features->buff_cache->get_buff( target_index, ct_hash( "VayneSilveredDebuff" ) );
            if ( buff && buff->stacks( ) == 2 ) {
                auto spell = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( !spell ) break;

                auto max_health_percent{ 0.02f + 0.02f * spell->level };
                auto true_damage = max_health_percent * obj->max_health;

                if ( true_damage > 200.f && obj->is_minion( ) && obj->is_jungle_monster( ) ) true_damage = 200.f;

                damage += true_damage;
            }
            break;
        }
        case EHeroes::senna:
        {
            if ( obj->is_hero( ) && g_features->buff_cache->get_buff( obj->index, ct_hash( "sennapassivemarker" ) ) ) {
                auto hp_modifier  = std::min( static_cast< float >( g_local->level ) * 0.01f, 0.1f );
                auto bonus_damage = obj->health * hp_modifier;

                damage += calculate_damage( bonus_damage, target_index, true );
            }

            damage += calculate_damage( g_local->attack_damage( ) * 0.2f, target_index, true );
            break;
        }
        case EHeroes::varus:
        {
            if ( auto slot = g_local->spell_book.get_spell_slot( ESpellSlot::w ) ) {
                auto bonus_damage{ g_local->ability_power( ) * 0.3f };

                switch ( slot->level ) {
                case 1:
                    bonus_damage += 7.f;
                    break;
                case 2:
                    bonus_damage += 12.f;
                    break;
                case 3:
                    bonus_damage += 17.f;
                    break;
                case 4:
                    bonus_damage += 22.f;
                    break;
                case 5:
                    bonus_damage += 27.f;
                    break;
                default:
                    break;
                }

                damage += calculate_damage( bonus_damage, target_index, false );
            }


            break;
        }
        case EHeroes::corki:
        {
            auto raw_damage = calculate_crit ? g_local->attack_damage( ) * 1.75f : g_local->attack_damage( );
            damage          = calculate_damage( raw_damage * 0.8f, obj->index, false ) + calculate_damage(
                raw_damage * 0.2f,
                obj->index,
                true
            );
            break;
        }
        case EHeroes::lux:
            if ( g_features->buff_cache->get_buff( obj->index, ct_hash( "LuxIlluminatingFraulein" ) ) ) {
                damage +=
                    calculate_damage(
                        10.f + 10.f * std::min( g_local->level, 18 ) + g_local->ability_power( ) * 0.2f,
                        target_index,
                        false
                    );
            }

            break;
        default:
            break;
        }

        if ( on_hit ) damage += get_onhit_damage( target_index );

        return damage;
    }

    auto get_champion_onhit_damage(
        const int16_t source_index,
        const int16_t target_index,
        const bool    is_melee,
        const float   server_cast_time
    )
        -> float{
        auto source = g_entity_list.get_by_index( source_index );
        if ( !source ) return 0.f;

        auto target = g_entity_list.get_by_index( target_index );
        if ( !target ) return 0.f;

        source.update( );
        target.update( );

        float total_damage{ };

        switch ( get_champion_hero( rt_hash( source->champion_name.text ) ) ) {
        case EHeroes::irelia:
        {
            const auto buff = g_features->buff_cache->get_buff( source->index, ct_hash( "ireliapassivestacks" ) );
            if ( buff && buff->stacks( ) == 4 ) {
                total_damage += process_damage(
                    7.f + 3.f * source->level + source->bonus_attack_damage( ) * 0.2f,
                    source_index,
                    target_index,
                    false
                );
            }
            break;
        }
        default:
            break;
        }

        const auto buff = g_features->buff_cache->get_buff( source->index, ct_hash( "6672buff" ) );
        if ( buff && buff->stacks( ) == 2 ) total_damage += 60.f + source->bonus_attack_damage( ) * 0.45f;

        bool       is_energized{ };
        const auto energized_buff = g_features->buff_cache->get_buff(
            source->index,
            ct_hash( "itemstatikshankcharge" )
        );
        if ( energized_buff && energized_buff->stacks( ) == 100 ) is_energized = true;

        bool start_item_calculated{ };

        for ( auto i = 1; i < 7; i++ ) // cuz Inventory Slot starts at 1, so we iterate all, 7 is trinket so no check
        {
            auto slot = source->inventory.get_inventory_slot( i );
            if ( !slot ) continue;
            auto item_base = slot->get_base_item( );
            if ( !item_base ) continue;
            auto item_data = item_base->get_item_data( );
            if ( !item_data ) continue;

            switch ( static_cast< EItemId >( item_data->id ) ) {
            default:
                break;
            case EItemId::doran_ring:
            case EItemId::doran_shield:
            case EItemId::tear_of_goddess:
                if ( start_item_calculated || !target->is_lane_minion( ) ) break;

                total_damage += process_damage( 5.f, source_index, target_index, true );
                start_item_calculated = true;
                break;
            case EItemId::nashors_tooth:
                total_damage += process_damage(
                    15.f + source->ability_power( ) * 0.2f,
                    source_index,
                    target_index,
                    false
                );
                break;
            case EItemId::recurve_bow:

                total_damage += process_damage( 15.f, source_index, target_index, true );
                break;
            case EItemId::blade_of_ruined_king:
                if ( target->is_lane_minion( ) ) {
                    const auto predicted_health = g_features->prediction->predict_minion_health(
                        target_index,
                        server_cast_time - *g_time
                    );

                    total_damage += process_damage(
                        predicted_health * ( is_melee ? 0.12f : 0.08f ),
                        source_index,
                        target_index,
                        true
                    );
                } else {
                    total_damage += process_damage(
                        target->health * ( is_melee ? 0.12f : 0.08f ),
                        source_index,
                        target_index,
                        true
                    );
                }

                break;
            case EItemId::divine_sunderer:
                if ( !g_features->buff_cache->get_buff( source->index, ct_hash( "6632buff" ) ) ) break;

                total_damage += process_damage( target->max_health * 0.09f, source_index, target_index, true );
                break;
            case EItemId::sheen:
                if ( !g_features->buff_cache->get_buff( source->index, ct_hash( "sheen" ) ) ) break;

                total_damage += process_damage( source->base_attack, source_index, target_index, true );
                break;
            case EItemId::wits_end:
            {
                const auto dmg = std::min( 15.f + 65.f / 17 * ( source->level - 1 ), 80.f );
                total_damage += process_damage( dmg, source_index, target_index, false );
                break;
            }
            case EItemId::stormrazor:
            case EItemId::rapidfire_cannon:
                if ( !is_energized ) break;

                total_damage += process_damage( 120.f, source_index, target_index, false );
                break;
            case EItemId::kircheis_shard:
                if ( !is_energized ) break;

                total_damage += process_damage( 80.f, source_index, target_index, false );
                break;
            case EItemId::lichbane:
                if ( !g_features->buff_cache->get_buff( source->index, ct_hash( "lichbane" ) ) ) break;

                total_damage += process_damage(
                    source->base_attack * 0.75f + source->ability_power( ) * 0.5f,
                    source_index,
                    target_index,
                    false
                );
                break;
            case EItemId::triforce:
                if ( !g_features->buff_cache->get_buff( source->index, ct_hash( "3078trinityforce" ) ) ) break;

                total_damage += process_damage(
                    source->base_attack * 2.f,
                    source_index,
                    target_index,
                    true
                );
                break;
            case EItemId::essence_reaver:
                if ( !g_features->buff_cache->get_buff( source->index, ct_hash( "3508buff" ) ) ) break;

                total_damage += process_damage(
                    source->base_attack + source->bonus_attack * 0.4f,
                    source_index,
                    target_index,
                    true
                );
                break;
            }
        }

        return total_damage;
    }

    auto get_spell_damage(
        hash_t  spell_name,
        int16_t source_index,
        int16_t target_index,
        float   server_cast_time,
        float   impact_time
    ) -> float{
        switch ( spell_name ) {
        case ct_hash( "AnnieQ" ):
        {
            auto source = g_entity_list.get_by_index( source_index );
            if ( !source ) return 0.f;

            source.update( );

            const auto spell_slot = source->spell_book.get_spell_slot( ESpellSlot::q );
            if ( !spell_slot ) return 0.f;

            float damage{ };
            switch ( spell_slot->level ) {
            case 1:
                damage = 80.f;
                break;
            case 2:
                damage = 115.f;
                break;
            case 3:
                damage = 150.f;
                break;
            case 4:
                damage = 185.f;
                break;
            case 5:
                damage = 220.f;
                break;
            default:
                damage = 80.f;
                break;
            }

            damage += source->ability_power( ) * 0.8f;

            return process_damage( damage, source_index, target_index, false );
        }
        case ct_hash( "BlindingDart" ):
        {
            auto source = g_entity_list.get_by_index( source_index );
            if ( !source ) return 0.f;

            source.update( );

            const auto spell_slot = source->spell_book.get_spell_slot( ESpellSlot::q );
            if ( !spell_slot ) return 0.f;

            float damage{ };
            switch ( spell_slot->level ) {
            case 1:
                damage = 80.f;
                break;
            case 2:
                damage = 125.f;
                break;
            case 3:
                damage = 170.f;
                break;
            case 4:
                damage = 215.f;
                break;
            case 5:
                damage = 260.f;
                break;
            default:
                damage = 80.f;
                break;
            }

            damage += source->ability_power( ) * 0.8f;

            return process_damage( damage, source_index, target_index, false );
        }
        case ct_hash( "BlindMonkRKick" ):
        {
            auto source = g_entity_list.get_by_index( source_index );
            if ( !source ) return 0.f;

            source.update( );

            const auto spell_slot = source->spell_book.get_spell_slot( ESpellSlot::r );
            if ( !spell_slot ) return 0.f;

            float damage{ };
            switch ( spell_slot->level ) {
            case 1:
                damage = 175.f;
                break;
            case 2:
                damage = 400.f;
                break;
            case 3:
                damage = 625.f;
                break;
            default:
                damage = 175.f;
                break;
            }

            damage += source->bonus_attack_damage( ) * 2.f;

            return process_damage( damage, source_index, target_index, true );
        }
        case ct_hash( "BrandE" ):
        {
            auto source = g_entity_list.get_by_index( source_index );
            if ( !source ) return 0.f;

            source.update( );

            const auto spell_slot = source->spell_book.get_spell_slot( ESpellSlot::e );
            if ( !spell_slot ) return 0.f;

            float damage{ };
            switch ( spell_slot->level ) {
            case 1:
                damage = 70.f;
                break;
            case 2:
                damage = 95.f;
                break;
            case 3:
                damage = 120.f;
                break;
            case 4:
                damage = 145.f;
                break;
            case 5:
                damage = 170.f;
                break;
            default:
                damage = 70.f;
                break;
            }

            damage += source->ability_power( ) * 0.45f;

            return process_damage( damage, source_index, target_index, false );
        }
        case ct_hash( "BrandR" ):
        {
            auto source = g_entity_list.get_by_index( source_index );
            if ( !source ) return 0.f;

            source.update( );

            const auto spell_slot = source->spell_book.get_spell_slot( ESpellSlot::r );
            if ( !spell_slot ) return 0.f;

            float damage{ };
            switch ( spell_slot->level ) {
            case 1:
                damage = 100.f;
                break;
            case 2:
                damage = 200.f;
                break;
            case 3:
                damage = 300.f;
                break;
            default:
                damage = 100.f;
                break;
            }

            damage += source->ability_power( ) * 0.25f;

            return process_damage( damage, source_index, target_index, false );
        }
        case ct_hash( "CaitlynR" ):
        {
            auto source = g_entity_list.get_by_index( source_index );
            if ( !source ) return 0.f;

            source.update( );

            const auto spell_slot = source->spell_book.get_spell_slot( ESpellSlot::r );
            if ( !spell_slot ) return 0.f;

            float damage{ };
            switch ( spell_slot->level ) {
            case 1:
                damage = 300.f;
                break;
            case 2:
                damage = 525.f;
                break;
            case 3:
                damage = 750.f;
                break;
            default:
                damage = 300.f;
                break;
            }

            damage += source->bonus_attack_damage( ) * 2.f;

            return process_damage( damage, source_index, target_index, true );
        }
        case ct_hash( "CassiopeiaE" ):
        {
            auto source = g_entity_list.get_by_index( source_index );
            if ( !source ) return 0.f;

            source.update( );

            const auto spell_slot = source->spell_book.get_spell_slot( ESpellSlot::e );
            if ( !spell_slot ) return 0.f;

            auto damage{ std::min( 48.f + 4.f * source->level, 120.f ) };
            damage += source->ability_power( ) * 0.1f;

            if ( g_features->buff_cache->get_buff( target_index, ct_hash( "cassiopeiaqdebuff" ) ) ||
                g_features->buff_cache->get_buff( target_index, ct_hash( "cassiopeiawpoison" ) ) )
                damage += 20.f * spell_slot->level + source->ability_power( ) * 0.6f;

            return process_damage( damage, source_index, target_index, false );
        }
        case ct_hash( "EvelynnE" ):
        {
            auto source = g_entity_list.get_by_index( source_index );
            if ( !source ) return 0.f;

            auto target = g_entity_list.get_by_index( target_index );
            if ( !target ) return 0.f;

            source.update( );
            target.update( );

            const auto spell_slot = source->spell_book.get_spell_slot( ESpellSlot::e );
            if ( !spell_slot ) return 0.f;

            float damage{ };
            switch ( spell_slot->level ) {
            case 1:
                damage = 55.f;
                break;
            case 2:
                damage = 70.f;
                break;
            case 3:
                damage = 85.f;
                break;
            case 4:
                damage = 100.f;
                break;
            case 5:
                damage = 115.f;
                break;
            default:
                damage = 55.f;
                break;
            }

            auto max_health_mod = 0.03f + std::floor( source->ability_power( ) / 100.f ) * 0.015f;

            damage += target->max_health * max_health_mod;

            return process_damage( damage, source_index, target_index, false );
        }
        case ct_hash( "EvelynnE2" ):
        {
            auto source = g_entity_list.get_by_index( source_index );
            if ( !source ) return 0.f;

            auto target = g_entity_list.get_by_index( target_index );
            if ( !target ) return 0.f;

            source.update( );
            target.update( );

            const auto spell_slot = source->spell_book.get_spell_slot( ESpellSlot::e );
            if ( !spell_slot ) return 0.f;

            float damage{ };
            switch ( spell_slot->level ) {
            case 1:
                damage = 75.f;
                break;
            case 2:
                damage = 100.f;
                break;
            case 3:
                damage = 125.f;
                break;
            case 4:
                damage = 150.f;
                break;
            case 5:
                damage = 175.f;
                break;
            default:
                damage = 75.f;
                break;
            }

            auto max_health_mod = 0.04f + std::floor( source->ability_power( ) / 100.f ) * 0.025f;

            damage += target->max_health * max_health_mod;

            return process_damage( damage, source_index, target_index, false );
        }
        case ct_hash( "Frostbite" ):
        {
            auto source = g_entity_list.get_by_index( source_index );
            if ( !source ) return 0.f;

            source.update( );

            const auto spell_slot = source->spell_book.get_spell_slot( ESpellSlot::e );
            if ( !spell_slot ) return 0.f;

            auto damage = 25.f + 25.f * spell_slot->level;
            damage += source->ability_power( ) * 0.6f;

            const auto buff = g_features->buff_cache->get_buff( target_index, ct_hash( "aniviachilled" ) );
            if ( buff && buff->buff_data->end_time > impact_time ) damage *= 2.f;

            return process_damage( damage, source_index, target_index, false );
        }
        case ct_hash( "GangplankQProceed" ):
        {
            // include onhit

            auto source = g_entity_list.get_by_index( source_index );
            if ( !source ) return 0.f;

            source.update( );

            const auto spell_slot = source->spell_book.get_spell_slot( ESpellSlot::q );
            if ( !spell_slot ) return 0.f;

            float damage{ };
            switch ( spell_slot->level ) {
            default:
                damage = 10.f;
                break;
            case 2:
                damage = 40.f;
                break;
            case 3:
                damage = 70.f;
                break;
            case 4:
                damage = 100.f;
                break;
            case 5:
                damage = 130.f;
                break;
            }

            damage += source->attack_damage( );

            return process_damage( damage, source_index, target_index, true );
        }
        case ct_hash( "GangplankQProceedCrit" ):
        {
            // include onhit

            auto source = g_entity_list.get_by_index( source_index );
            if ( !source ) return 0.f;

            source.update( );

            const auto spell_slot = source->spell_book.get_spell_slot( ESpellSlot::q );
            if ( !spell_slot ) return 0.f;

            float damage{ };
            switch ( spell_slot->level ) {
            default:
                damage = 10.f;
                break;
            case 2:
                damage = 40.f;
                break;
            case 3:
                damage = 70.f;
                break;
            case 4:
                damage = 100.f;
                break;
            case 5:
                damage = 130.f;
                break;
            }

            damage += source->attack_damage( );

            return process_damage( damage * 1.75f, source_index, target_index, true );
        }
        case ct_hash( "JhinQ" ):
        {
            auto source = g_entity_list.get_by_index( source_index );
            if ( !source ) return 0.f;

            source.update( );

            const auto spell_slot = source->spell_book.get_spell_slot( ESpellSlot::q );
            if ( !spell_slot ) return 0.f;

            float damage{ };
            float ad_modifier{ };
            switch ( spell_slot->level ) {
            case 1:
                damage = 45.f;
                ad_modifier = 0.35f;
                break;
            case 2:
                damage = 70.f;
                ad_modifier = 0.425f;
                break;
            case 3:
                damage = 95.f;
                ad_modifier = 0.5f;
                break;
            case 4:
                damage = 120.f;
                ad_modifier = 0.575f;
                break;
            case 5:
                damage = 145.f;
                ad_modifier = 0.65f;
                break;
            default:
                damage = 45.f;
                ad_modifier = 0.35f;
                break;
            }

            damage += source->attack_damage( ) * ad_modifier;

            return process_damage( damage, source_index, target_index, true );
        }
        case ct_hash( "KatarinaQ" ):
        {
            auto source = g_entity_list.get_by_index( source_index );
            if ( !source ) return 0.f;

            source.update( );

            const auto spell_slot = source->spell_book.get_spell_slot( ESpellSlot::q );
            if ( !spell_slot ) return 0.f;

            float damage{ };
            switch ( spell_slot->level ) {
            case 1:
                damage = 80.f;
                break;
            case 2:
                damage = 110.f;
                break;
            case 3:
                damage = 140.f;
                break;
            case 4:
                damage = 170.f;
                break;
            case 5:
                damage = 200.f;
                break;
            default:
                damage = 80.f;
                break;
            }

            damage += source->ability_power( ) * 0.35f;

            return process_damage( damage, source_index, target_index, false );
        }
        case ct_hash( "KhazixQ" ):
        case ct_hash( "KhazixQLong" ):
        {
            auto source = g_entity_list.get_by_index( source_index );
            if ( !source ) return 0.f;

            source.update( );

            const auto spell_slot = source->spell_book.get_spell_slot( ESpellSlot::q );
            if ( !spell_slot ) return 0.f;

            float damage{ };
            switch ( spell_slot->level ) {
            case 1:
                damage = 60.f;
                break;
            case 2:
                damage = 85.f;
                break;
            case 3:
                damage = 110.f;
                break;
            case 4:
                damage = 135.f;
                break;
            case 5:
                damage = 160.f;
                break;
            default:
                damage = 60.f;
                break;
            }

            damage += source->bonus_attack_damage( ) * 1.15f;

            return process_damage( damage, source_index, target_index, true );
        }
        case ct_hash( "LeblancQ" ):
        {
            auto source = g_entity_list.get_by_index( source_index );
            if ( !source ) return 0.f;

            source.update( );

            const auto spell_slot = source->spell_book.get_spell_slot( ESpellSlot::q );
            if ( !spell_slot ) return 0.f;

            float damage{ };
            switch ( spell_slot->level ) {
            case 1:
                damage = 65.f;
                break;
            case 2:
                damage = 90.f;
                break;
            case 3:
                damage = 115.f;
                break;
            case 4:
                damage = 140.f;
                break;
            case 5:
                damage = 165.f;
                break;
            default:
                damage = 65.f;
                break;
            }

            damage += source->ability_power( ) * 0.4f;

            auto buff = g_features->buff_cache->get_buff( target_index, ct_hash( "LeblancQMark" ) );
            if ( buff && buff->buff_data->end_time >= impact_time - 0.05f ) {
                float detonation_damage{ };

                switch ( spell_slot->level ) {
                case 1:
                    detonation_damage = 65.f;
                    break;
                case 2:
                    detonation_damage = 90.f;
                    break;
                case 3:
                    detonation_damage = 115.f;
                    break;
                case 4:
                    detonation_damage = 140.f;
                    break;
                case 5:
                    detonation_damage = 165.f;
                    break;
                default:
                    detonation_damage = 65.f;
                    break;
                }

                detonation_damage += g_local->ability_power( ) * 0.4f;
                damage += detonation_damage;
            } else {
                buff = g_features->buff_cache->get_buff( target_index, ct_hash( "LeblancRQMark" ) );
                if ( buff && buff->buff_data->end_time >= impact_time - 0.05f ) {
                    float detonation_damage{ };

                    if ( const auto r_slot = source->spell_book.get_spell_slot( ESpellSlot::r ) ) {
                        switch ( r_slot->level ) {
                        case 1:
                            detonation_damage = 140.f;
                            break;
                        case 2:
                            detonation_damage = 280.f;
                            break;
                        case 3:
                            detonation_damage = 420.f;
                            break;
                        default:
                            detonation_damage = 140.f;
                            break;
                        }

                        detonation_damage += g_local->ability_power( ) * 0.8f;
                        damage += detonation_damage;
                    }
                }
            }

            return process_damage( damage, source_index, target_index, false );
        }
        case ct_hash( "LeblancRQ" ):
        {
            auto source = g_entity_list.get_by_index( source_index );
            if ( !source ) return 0.f;

            source.update( );

            const auto spell_slot = source->spell_book.get_spell_slot( ESpellSlot::r );
            if ( !spell_slot ) return 0.f;

            float damage{ };
            switch ( spell_slot->level ) {
            case 1:
                damage = 70.f;
                break;
            case 2:
                damage = 140.f;
                break;
            case 3:
                damage = 210.f;
                break;
            default:
                damage = 70.f;
                break;
            }

            damage += source->ability_power( ) * 0.4f;

            auto buff = g_features->buff_cache->get_buff( target_index, ct_hash( "LeblancQMark" ) );
            if ( buff && buff->buff_data->end_time >= impact_time - 0.05f ) {
                float detonation_damage{ };

                if ( const auto q_slot = source->spell_book.get_spell_slot( ESpellSlot::q ) ) {
                    switch ( q_slot->level ) {
                    case 1:
                        detonation_damage = 65.f;
                        break;
                    case 2:
                        detonation_damage = 90.f;
                        break;
                    case 3:
                        detonation_damage = 115.f;
                        break;
                    case 4:
                        detonation_damage = 140.f;
                        break;
                    case 5:
                        detonation_damage = 165.f;
                        break;
                    default:
                        detonation_damage = 65.f;
                        break;
                    }

                    detonation_damage += g_local->ability_power( ) * 0.4f;
                    damage += detonation_damage;
                }
            } else {
                buff = g_features->buff_cache->get_buff( target_index, ct_hash( "LeblancRQMark" ) );
                if ( buff && buff->buff_data->end_time >= impact_time - 0.05f ) {
                    float detonation_damage{ };

                    switch ( spell_slot->level ) {
                    case 1:
                        detonation_damage = 140.f;
                        break;
                    case 2:
                        detonation_damage = 280.f;
                        break;
                    case 3:
                        detonation_damage = 420.f;
                        break;
                    default:
                        detonation_damage = 140.f;
                        break;
                    }

                    detonation_damage += g_local->ability_power( ) * 0.8f;
                    damage += detonation_damage;
                }
            }

            return process_damage( damage, source_index, target_index, false );
        }
        case ct_hash( "NullLance" ):
        {
            auto source = g_entity_list.get_by_index( source_index );
            if ( !source ) return 0.f;

            source.update( );

            const auto spell_slot = source->spell_book.get_spell_slot( ESpellSlot::q );
            if ( !spell_slot ) return 0.f;

            float damage{ };
            switch ( spell_slot->level ) {
            case 1:
                damage = 65.f;
                break;
            case 2:
                damage = 95.f;
                break;
            case 3:
                damage = 125.f;
                break;
            case 4:
                damage = 155.f;
                break;
            case 5:
                damage = 185.f;
                break;
            default:
                damage = 65.f;
                break;
            }

            damage += source->ability_power( ) * 0.7f;

            return process_damage( damage, source_index, target_index, false );
        }
        case ct_hash( "SeismicShard" ):
        {
            auto source = g_entity_list.get_by_index( source_index );
            if ( !source ) return 0.f;

            source.update( );

            const auto spell_slot = source->spell_book.get_spell_slot( ESpellSlot::q );
            if ( !spell_slot ) return 0.f;

            float damage{ };
            switch ( spell_slot->level ) {
            case 1:
                damage = 70.f;
                break;
            case 2:
                damage = 120.f;
                break;
            case 3:
                damage = 170.f;
                break;
            case 4:
                damage = 220.f;
                break;
            case 5:
                damage = 270.f;
                break;
            default:
                damage = 70.f;
                break;
            }

            damage += source->ability_power( ) * 0.6f;

            return process_damage( damage, source_index, target_index, false );
        }
        case ct_hash( "SowTheWind" ):
        {
            auto source = g_entity_list.get_by_index( source_index );
            if ( !source ) return 0.f;

            source.update( );

            const auto spell_slot = source->spell_book.get_spell_slot( ESpellSlot::w );
            if ( !spell_slot ) return 0.f;

            float damage{ };
            switch ( spell_slot->level ) {
            case 1:
                damage = 70.f;
                break;
            case 2:
                damage = 100.f;
                break;
            case 3:
                damage = 130.f;
                break;
            case 4:
                damage = 160.f;
                break;
            case 5:
                damage = 190.f;
                break;
            default:
                damage = 70.f;
                break;
            }

            damage += source->ability_power( ) * 0.5f;

            return process_damage( damage, source_index, target_index, false );
        }
        case ct_hash( "TristanaR" ):
        {
            auto source = g_entity_list.get_by_index( source_index );
            if ( !source ) return 0.f;

            source.update( );

            const auto spell_slot = source->spell_book.get_spell_slot( ESpellSlot::r );
            if ( !spell_slot ) return 0.f;

            float damage{ };
            switch ( spell_slot->level ) {
            case 1:
                damage = 300.f;
                break;
            case 2:
                damage = 400.f;
                break;
            case 3:
                damage = 500.f;
                break;
            default:
                damage = 300.f;
                break;
            }

            damage += source->ability_power( );

            return process_damage( damage, source_index, target_index, false );
        }
        case ct_hash( "TwoShivPoison" ):
        {
            auto source = g_entity_list.get_by_index( source_index );
            if ( !source ) return 0.f;

            auto target = g_entity_list.get_by_index( target_index );
            if ( !target ) return 0.f;

            target.update( );
            source.update( );

            const auto spell_slot = source->spell_book.get_spell_slot( ESpellSlot::e );
            if ( !spell_slot ) return 0.f;

            const auto low_health = ( target->health + target->total_health_regen ) / target->max_health < 0.3f;

            float damage{ };
            switch ( spell_slot->level ) {
            case 1:
                damage = low_health ? 105.f : 70.f;
                break;
            case 2:
                damage = low_health ? 142.5f : 95.f;
                break;
            case 3:
                damage = low_health ? 180.f : 120.f;
                break;
            case 4:
                damage = low_health ? 217.5f : 145.f;
                break;
            case 5:
                damage = low_health ? 255.f : 170.f;
                break;
            default:
                damage = low_health ? 105.f : 70.f;
                break;
            }

            damage += low_health
                          ? source->bonus_attack_damage( ) * 1.2f + source->ability_power( ) * 0.9f
                          : source->bonus_attack_damage( ) * 0.8f + source->ability_power( ) * 0.6f;

            return process_damage( damage, source_index, target_index, false );
        }
        case ct_hash( "VeigarR" ):
        {
            auto source = g_entity_list.get_by_index( source_index );
            if ( !source ) return 0.f;

            auto target = g_entity_list.get_by_index( target_index );
            if ( !target ) return 0.f;

            target.update( );
            source.update( );

            const auto spell_slot = source->spell_book.get_spell_slot( ESpellSlot::r );
            if ( !spell_slot ) return 0.f;

            float damage{ };
            switch ( spell_slot->level ) {
            case 1:
                damage = 175.f;
                break;
            case 2:
                damage = 250.f;
                break;
            case 3:
                damage = 325.f;
                break;
            default:
                damage = 175.f;
                break;
            }

            damage += source->ability_power( ) * 0.75f;

            auto missing_health  = 1.f - target->health / target->max_health;
            auto damage_modifier = std::min( missing_health / 0.6667f, 1.f );

            damage *= 1.f + damage_modifier;

            return process_damage( damage, source_index, target_index, false );
        }
        case ct_hash( "VladimirQ" ):
        {
            auto source = g_entity_list.get_by_index( source_index );
            if ( !source ) return 0.f;

            source.update( );

            const auto spell_slot = source->spell_book.get_spell_slot( ESpellSlot::q );
            if ( !spell_slot ) return 0.f;

            float damage{ };
            switch ( spell_slot->level ) {
            case 1:
                damage = 80.f;
                break;
            case 2:
                damage = 100.f;
                break;
            case 3:
                damage = 120.f;
                break;
            case 4:
                damage = 140.f;
                break;
            case 5:
                damage = 160.f;
                break;
            default:
                damage = 80.f;
                break;
            }

            damage += source->ability_power( ) * 0.6f;

            auto buff = g_features->buff_cache->get_buff( source_index, ct_hash( "vladimirqfrenzy" ) );
            if ( buff && buff->buff_data->end_time >= server_cast_time ) damage *= 1.85f;

            return process_damage( damage, source_index, target_index, false );
        }
        case ct_hash( "ViktorPowerTransfer" ):
        {
            auto source = g_entity_list.get_by_index( source_index );
            if ( !source ) return 0.f;

            source.update( );

            const auto spell_slot = source->spell_book.get_spell_slot( ESpellSlot::q );
            if ( !spell_slot ) return 0.f;

            float damage{ };
            switch ( spell_slot->level ) {
            case 1:
                damage = 60.f;
                break;
            case 2:
                damage = 75.f;
                break;
            case 3:
                damage = 90.f;
                break;
            case 4:
                damage = 105.f;
                break;
            case 5:
                damage = 120.f;
                break;
            default:
                damage = 60.f;
                break;
            }

            damage += source->ability_power( ) * 0.4f;

            return process_damage( damage, source_index, target_index, false );
        }
        default:
            return 0.f;
        }
    }

    auto get_onhit_damage( const int16_t target_index ) -> float{
        auto obj = g_entity_list.get_by_index( target_index );
        if ( !obj ) return 0.f;

        obj.update( );
        // const auto target = obj->create_updated_copy( );
        // if ( !target ) return 0.f;

        float damage{ };
        bool  is_melee{ };

        switch ( get_current_hero( ) ) {
        case EHeroes::irelia:
        {
            const auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "ireliapassivestacks" ) );
            if ( buff && buff->stacks( ) == 4 ) {
                damage += calculate_damage(
                    7.f + 3.f * static_cast< float >( g_local->level ) + g_local->bonus_attack_damage( ) * 0.2f,
                    obj->index,
                    false
                );
            }

            is_melee = true;

            break;
        }
        default:
            break;
        }

        const auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "6672buff" ) );
        if ( buff && buff->stacks( ) == 2 ) damage += 60.f + g_local->bonus_attack_damage( ) * 0.45f;

        bool       is_energized{ };
        const auto energized_buff = g_features->buff_cache->get_buff(
            g_local->index,
            ct_hash( "itemstatikshankcharge" )
        );
        if ( energized_buff && energized_buff->stacks( ) == 100 ) is_energized = true;

        bool start_item_calculated{ };

        for ( auto i = 1; i < 7; i++ ) // cuz Inventory Slot starts at 1, so we iterate all, 7 is trinket so no check
        {
            auto slot = g_local->inventory.get_inventory_slot( i );
            if ( !slot ) continue;
            auto item_base = slot->get_base_item( );
            if ( !item_base ) continue;
            auto item_data = item_base->get_item_data( );
            if ( !item_data ) continue;

            switch ( static_cast< EItemId >( item_data->id ) ) {
            default:
                break;
            case EItemId::doran_ring:
            case EItemId::doran_shield:
            case EItemId::tear_of_goddess:
                if ( start_item_calculated || !obj->is_lane_minion( ) ) break;

                damage += calculate_damage( 5.f, target_index, true );
                start_item_calculated = true;
                break;
            case EItemId::nashors_tooth:
                damage += calculate_damage( 15.f + g_local->ability_power( ) * 0.2f, target_index, false );
                break;
            case EItemId::recurve_bow:
                damage += calculate_damage( 15.f, target_index, true );
                break;
            case EItemId::blade_of_ruined_king:
                if ( obj->is_lane_minion( ) ) {
                    const auto predicted_health = g_features->prediction->predict_health(
                        obj.get( ),
                        g_features->orbwalker->get_attack_cast_delay( )
                    );
                    damage += calculate_damage(
                        predicted_health * ( is_melee ? 0.12f : 0.08f ),
                        target_index,
                        true
                    );
                } else {
                    damage += calculate_damage(
                        obj->health * ( is_melee ? 0.12f : 0.08f ),
                        target_index,
                        true
                    );
                }

                break;
            case EItemId::divine_sunderer:
                if ( !g_features->buff_cache->get_buff( g_local->index, ct_hash( "6632buff" ) ) ) break;

                damage += calculate_damage( obj->max_health * 0.09f, target_index, true );
                break;
            case EItemId::sheen:
                if ( !g_features->buff_cache->get_buff( g_local->index, ct_hash( "sheen" ) ) ) break;

                damage += calculate_damage( g_local->base_attack, target_index, true );
                break;
            case EItemId::wits_end:
            {
                const auto dmg = std::min( 15.f + 65.f / 17 * static_cast< float >( g_local->level - 1 ), 80.f );

                damage += calculate_damage( dmg, target_index, false );
                break;
            }
            case EItemId::stormrazor:
            case EItemId::rapidfire_cannon:
                if ( !is_energized ) break;

                damage += calculate_damage( 120.f, target_index, false );
                break;
            case EItemId::kircheis_shard:
                if ( !is_energized ) break;

                damage += calculate_damage( 80.f, target_index, false );
                break;
            case EItemId::lichbane:
                if ( !g_features->buff_cache->get_buff( g_local->index, ct_hash( "lichbane" ) ) ) break;

                damage += calculate_damage(
                    g_local->base_attack * 0.75f + g_local->ability_power( ) * 0.5f,
                    target_index,
                    false
                );
                break;
            case EItemId::triforce:
                if ( !g_features->buff_cache->get_buff( g_local->index, ct_hash( "3078trinityforce" ) ) ) break;

                damage += calculate_damage( g_local->base_attack * 2.f, target_index, true );
                break;
            case EItemId::essence_reaver:
                if ( !g_features->buff_cache->get_buff( g_local->index, ct_hash( "3508buff" ) ) ) break;

                damage += calculate_damage( g_local->base_attack + g_local->bonus_attack * 0.4f, target_index, true );
                break;
            case EItemId::rageknife:
            case EItemId::rageblade:
                damage += calculate_damage( item_base->rageblade_damage, target_index, true );
                break;
            }
        }

        return damage;
    }

    auto get_real_health(
        const int16_t     target_index,
        const EDamageType damage_type,
        const float       delay,
        const bool        predict_ally_damage
    ) -> float{
        auto target = g_entity_list.get_by_index( target_index );
        if ( !target ) return -1.f;

        target.update( );

        auto health = target->health + target->total_health_regen * std::ceil( delay + 1.f );

        switch ( damage_type ) {
        case EDamageType::physical_damage:
            health += target->physical_shield;
            break;
        case EDamageType::magic_damage:
            health += target->magic_shield;
            break;
        default:
            break;
        }

        health += target->shield;


        if ( !target->is_hero( ) ) return health;

        const auto is_melee = target->attack_range < 325.f;

        const std::vector< float > shieldbow_strength = {
            0.f,
            250.f,
            250.f,
            250.f,
            250.f,
            250.f,
            250.f,
            250.f,
            250.f,
            250.f,
            292.22f,
            334.44f,
            376.67f,
            418.89f,
            461.11f,
            503.33f,
            545.56f,
            587.78f,
            630.f
        };

        const auto shieldbow_shield  = shieldbow_strength[ std::min( target->level, 18 ) ];
        const auto hexdrinker_shield = is_melee
                                           ? 100.f + 10.f * std::min( target->level, 18 )
                                           : 75.f + 7.5f * std::min( target->level, 18 );
        const auto maw_of_malmortius_shield = is_melee
                                                  ? 200.f + target->bonus_attack_damage( ) * 2.25f
                                                  : 150.f + target->bonus_attack_damage( ) * 1.6875f;
        const auto steraks_cage_shield = ( target->max_health - 530.f ) *
            0.8f; // incorrect calc, would need bonus_health value

        for ( auto i = 1; i < 7; i++ ) {
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
            case EItemId::hexdrinker:
                if ( damage_type != EDamageType::magic_damage ) continue;

                health += hexdrinker_shield;
                break;
            case EItemId::maw_of_malmortius:
                if ( damage_type != EDamageType::magic_damage ) continue;

                health += maw_of_malmortius_shield;
                break;
            case EItemId::shieldbow:
                health += shieldbow_shield;
                break;
            case EItemId::steraks_cage:
                health += steraks_cage_shield;
                break;
            default:
                break;
            }
        }

        if ( predict_ally_damage ) {
            const auto incoming_damage = g_features->prediction->get_incoming_champion_damage(
                target->index,
                delay,
                true
            );

            if ( incoming_damage > 0.f ) health -= incoming_damage;
        }

        return health;
    }

    auto get_incoming_damage( const int16_t target_index, const float delay ) -> float{
        auto target = g_entity_list.get_by_index( target_index );
        if ( !target ) return 0.f;

        target.update( );
        if ( target->is_dead( ) || target->is_invisible( ) ) return 0.f;

        auto total_damage{ 0.f };

        if ( target->team == g_local->team ) {
            for ( const auto hero : g_entity_list.get_enemies( ) ) {
                if ( !hero || hero->is_dead( ) || hero->is_invisible( ) || hero->position.dist_to( target->position ) >
                    hero->attack_range * 2.f )
                    continue;

                auto sci = hero->spell_book.get_spell_cast_info( );
                if ( !sci || !sci->is_autoattack && !sci->is_special_attack || sci->server_cast_time < *g_time
                    || sci->get_target_index( ) != target_index )
                    continue;

                auto info = sci->get_spell_info( );
                if ( !info ) continue;

                auto data = info->get_spell_data( );
                if ( !data ) continue;

                const auto  name = data->get_name( );
                const bool  will_crit{ name.find( _( "Crit" ) ) != std::string::npos };
                const float autoattack_damage = will_crit ? hero->attack_damage( ) * 1.75f : hero->attack_damage( );

                total_damage += calculate_damage( autoattack_damage, target_index, true );
            }
        } else {
            for ( const auto hero : g_entity_list.get_allies( ) ) {
                if ( !hero || hero->is_dead( ) || hero->is_invisible( ) ||
                    hero->position.dist_to( target->position ) > hero->attack_range * 2.f )
                    continue;

                auto sci = hero->spell_book.get_spell_cast_info( );
                if ( !sci || !sci->is_autoattack && !sci->is_special_attack || sci->server_cast_time < *g_time || sci->
                    server_cast_time > *g_time + delay ||
                    sci->get_target_index( ) != target_index )
                    continue;

                auto info = sci->get_spell_info( );
                if ( !info ) continue;

                auto data = info->get_spell_data( );
                if ( !data ) continue;

                const auto  name = data->get_name( );
                const bool  will_crit{ name.find( _( "Crit" ) ) != std::string::npos };
                const float autoattack_damage = will_crit ? hero->attack_damage( ) * 1.75f : hero->attack_damage( );

                total_damage += calculate_damage( autoattack_damage, target_index, true );
            }
        }

        return total_damage;
    }


    auto is_position_under_turret( const Vec3& position, const bool ally_turret ) -> bool{
        if ( g_local->is_dead( ) || position.length( ) <= 0.f ) return false;

        if ( ally_turret ) {
            for ( const auto turret : g_entity_list.get_ally_turrets( ) ) {
                if ( !turret || turret->is_dead( ) || position.dist_to( turret->position ) > 925.f ) continue;

                return true;
            }
        } else {
            for ( const auto turret : g_entity_list.get_enemy_turrets( ) ) {
                if ( !turret || turret->is_dead( ) || position.dist_to( turret->position ) > 925.f ) continue;

                return true;
            }
        }

        return false;
    }

    auto is_position_near_turret( const Vec3& position, const bool ally_turret, const float nearby_threshold ) -> bool{
        if ( g_local->is_dead( ) || position.length( ) <= 0.f ) return false;

        if ( ally_turret ) {
            for ( const auto turret : g_entity_list.get_ally_turrets( ) ) {
                if ( !turret || turret->is_dead( ) || position.dist_to( turret->position ) > 905.f +
                    nearby_threshold )
                    continue;

                return true;
            }
        } else {
            for ( const auto turret : g_entity_list.get_enemy_turrets( ) ) {
                if ( !turret || turret->is_dead( ) || position.dist_to( turret->position ) > 905.f +
                    nearby_threshold )
                    continue;

                return true;
            }
        }

        return false;
    }


    auto get_nearby_champions_count( const Vec3& position, const bool allies, const float nearby_threshold ) -> int{
        if ( g_local->is_dead( ) || position.length( ) <= 0.f ) return 0;

        int count{ };

        if ( allies ) {
            for ( const auto ally : g_entity_list.get_allies( ) ) {
                if ( !ally || ally->network_id == g_local->network_id || ally->is_dead( )
                    || ally->is_invisible( ) || ally->position.dist_to( position ) > nearby_threshold )
                    continue;

                ++count;
            }
        } else {
            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) ||
                    enemy->position.dist_to( position ) > nearby_threshold )
                    continue;

                ++count;
            }
        }

        return count;
    }

    auto is_wall_in_line( const Vec3& start, const Vec3& end ) -> bool{
        for ( auto i = 1; i <= 6; i++ ) {
            auto temp = start.extend( end, start.dist_to( end ) / 6.f * static_cast< float >( i ) );

            if ( g_navgrid->is_wall( temp ) ) return true;
        }

        return false;
    }

    auto get_missile_hitbox_polygon(Vec3 start, Vec3 end, float radius) -> sdk::math::Polygon {

        if (start.dist_to(end) <= radius) {
            start = end.extend(start, radius );
        }
        else start = start.extend(end, radius / 2.f);

        const auto direction       = (end - start).normalize().rotated(-1.571f);
        const auto start_direction = (end - start).normalize().rotated(1.571f);
        auto       start_points    = g_render->get_3d_circle_points(start, radius, 16, 180, start_direction);
        const auto end_points      = g_render->get_3d_circle_points(end, radius, 16, 180, direction);

        /*auto local_position = g_local->position;

        const auto to_remove = std::ranges::remove_if(start_points,
                                                      [&](const Vec3 &position) -> bool
                                                      { return position.dist_to(g_local->position) > 600.f; });

        if (!to_remove.empty()) start_points.erase(to_remove.begin(), to_remove.end());*/

        auto poly = sdk::math::Circle(start, radius).to_polygon();
        poly.points = start_points;

        std::vector<Vec3> points{};

        constexpr auto point_distance = 30.f;

        auto start_point = start_points.back();
        auto end_point   = end_points.front();

        auto segment_count = start_point.dist_to(end_point) / point_distance;

        for (auto a = 0; a <= segment_count; a++)
        {
            const auto extend_distance = point_distance * static_cast<float>(a);

            if (extend_distance > start_point.dist_to(end_point)) {

                if (end_point.dist_to(g_local->position) > 600.f) continue;

                points.push_back(end_point);
                break;
            }

            auto position = start_point.extend(end_point, point_distance * static_cast<float>(a));
            if (position.dist_to(g_local->position) > 600.f) continue;

            points.push_back(position);
        }

        poly.points.insert(poly.points.end(), points.begin(), points.end());

        poly.points.insert(poly.points.end(), end_points.begin(), end_points.end());

        points.clear();

        start_point = end_points.back();
        end_point   = start_points.front();

        segment_count = start_point.dist_to(end_point) / point_distance;

        for (auto a = 0; a <= segment_count; a++) {

            const auto extend_distance = point_distance * static_cast<float>(a);

            if (extend_distance > start_point.dist_to(end_point)) {

                if (end_point.dist_to(g_local->position) > 600.f) continue;

                points.push_back(end_point);
                break;
            }

            auto position = start_point.extend(end_point, point_distance * static_cast<float>(a));
            if (position.dist_to(g_local->position) > 600.f) continue;

            points.push_back(position);
        }

         poly.points.insert(poly.points.end(), points.begin(), points.end());

        return poly;
    }

    auto zeri_execute_threshold( int level, float ability_power ) -> float{
        float execute_threshold;

        if ( level == 0 ) {
            ability_power = g_local->ability_power( );
            level         = g_local->level;
        }

        switch ( level ) {
        case 1:
            execute_threshold = 60.f;
            break;
        case 2:
            execute_threshold = 63.81f;
            break;
        case 3:
            execute_threshold = 67.81f;
            break;
        case 4:
            execute_threshold = 71.99f;
            break;
        case 5:
            execute_threshold = 76.36f;
            break;
        case 6:
            execute_threshold = 80.91f;
            break;
        case 7:
            execute_threshold = 85.65f;
            break;
        case 8:
            execute_threshold = 90.57f;
            break;
        case 9:
            execute_threshold = 95.68f;
            break;
        case 10:
            execute_threshold = 100.98f;
            break;
        case 11:
            execute_threshold = 106.46f;
            break;
        case 12:
            execute_threshold = 112.12f;
            break;
        case 13:
            execute_threshold = 117.97f;
            break;
        case 14:
            execute_threshold = 124.01f;
            break;
        case 15:
            execute_threshold = 130.23f;
            break;
        case 16:
            execute_threshold = 136.63f;
            break;
        case 17:
            execute_threshold = 143.22f;
            break;
        default:
            execute_threshold = 150.f;
            break;
        }

        execute_threshold += ability_power * 0.18f;

        return execute_threshold;
    }

    auto get_zeri_charge_base_damage( int level ) -> float{
        float charged_damage;

        if ( level == 0 ) level = g_local->level;

        switch ( level ) {
        case 1:
            charged_damage = 90.f;
            break;
        case 2:
            charged_damage = 94.66f;
            break;
        case 3:
            charged_damage = 99.54f;
            break;
        case 4:
            charged_damage = 104.66f;
            break;
        case 5:
            charged_damage = 109.99f;
            break;
        case 6:
            charged_damage = 115.56f;
            break;
        case 7:
            charged_damage = 121.35f;
            break;
        case 8:
            charged_damage = 127.37f;
            break;
        case 9:
            charged_damage = 133.61f;
            break;
        case 10:
            charged_damage = 140.08f;
            break;
        case 11:
            charged_damage = 146.78f;
            break;
        case 12:
            charged_damage = 153.7f;
            break;
        case 13:
            charged_damage = 160.85f;
            break;
        case 14:
            charged_damage = 168.85f;
            break;
        case 15:
            charged_damage = 175.83f;
            break;
        case 16:
            charged_damage = 183.66f;
            break;
        case 17:
            charged_damage = 191.72f;
            break;
        default:
            charged_damage = 200.f;
            break;
        }

        return charged_damage;
    }

    auto get_zeri_charge_max_health_modifier( int level ) -> float{
        float modifier;

        if ( level == 0 ) level = g_local->level;

        switch ( level ) {
        case 1:
            modifier = 0.01f;
            break;
        case 2:
            modifier = 0.0159f;
            break;
        case 3:
            modifier = 0.0221f;
            break;
        case 4:
            modifier = 0.0287f;
            break;
        case 5:
            modifier = 0.0354f;
            break;
        case 6:
            modifier = 0.0425f;
            break;
        case 7:
            modifier = 0.0499f;
            break;
        case 8:
            modifier = 0.0576f;
            break;
        case 9:
            modifier = 0.0655f;
            break;
        case 10:
            modifier = 0.0737f;
            break;
        case 11:
            modifier = 0.0823f;
            break;
        case 12:
            modifier = 0.0911f;
            break;
        case 13:
            modifier = 0.1002f;
            break;
        case 14:
            modifier = 0.1096f;
            break;
        case 15:
            modifier = 0.1192f;
            break;
        case 16:
            modifier = 0.1292f;
            break;
        case 17:
            modifier = 0.1395f;
            break;
        default:
            modifier = 0.15f;
            break;
        }

        return modifier;
    }
}
