#include "pch.hpp"

#include "object.hpp"


#include "obfuscated.hpp"
#include "../globals.hpp"
#include "pw_hud.hpp"
#include "render_manager.hpp"
#include "unit_info_component.hpp"
#include "../math/math.hpp"
#include "ai_manager.hpp"
#include "spell_info.hpp"
#include "../../security/src/xorstr.hpp"

#include "obfuscated_flag.hpp"
#include "../../features/entity_cache.hpp"
#include "../../features/entity_list.hpp"
#include "../../features/prediction.hpp"

namespace sdk::game {
    auto Object::is_local( ) const -> bool{
        return index == g_local->index;
    }

    auto Object::get_raw_flags( ) const -> std::optional< ObfuscatedFlag >{
        const auto obf_flag = reinterpret_cast< ObfuscatedFlag* >( reinterpret_cast< intptr_t >( this ) + ( 0x40 ) );


        if ( !obf_flag ) return std::nullopt;

        // professional pointer check!!!
        if ( reinterpret_cast< intptr_t >( obf_flag ) < 0x500 ) return std::nullopt;

        return *obf_flag;
    }

    auto Object::get_flags( ) const -> unsigned long{
        // const auto obf_flag = reinterpret_cast< Obfuscated< >* >( reinterpret_cast< intptr_t >( this ) + ( 0x40 ) );
        // if ( !obf_flag ) return 0;
        //
        // return obf_flag->get( );
        auto f = get_raw_flags( );
        if ( !f ) return 0;

        return f->get( );
    }

    auto Object::compare_flags( EObjectTypeFlags compare_flag ) const -> bool{
        return ( get_flags( ) & static_cast< unsigned long >( compare_flag ) ) != 0;
    }

    /*
     * To update this:
     * 1. go to E8 ? ? ? ? F3 0F 10 94 24 ? ? ? ? 4C 8D 4C 24 ? 
     * 2. Scroll up to first line
     * 3. 2nd operand of instruction is offset
     */
    auto Object::get_unit_info_component( ) const -> utils::MemoryHolder< UnitInfoComponent >{
        const auto ptr =
            reinterpret_cast< Obfuscated< unsigned long long >* >(
                reinterpret_cast< intptr_t >( this ) + 0x45B0
            )->get( );

        if ( ptr == 0 ) return { };

        return utils::MemoryHolder< UnitInfoComponent >( ptr );
    }

    auto Object::get_hpbar_position( ) const -> sdk::math::Vec2{
        const auto uic = get_unit_info_component( );
        if ( !uic.is_valid( ) ) return { };

        const auto base_height      = 90.f; // uic->get_health_bar_height( );
        auto hud_modifier = g_pw_hud->get_unit_info_component_offset( );
        auto camera_modififer = g_pw_hud->get_unit_info_component_modifier( );

        if ( this->is_hero( ) ) hud_modifier = get_model_size_modifier( ) * hud_modifier;

        math::Vec2 screen_pos{ };
        auto world_pos = this->position;

        if ( this->is_hero( ) ) world_pos.y += base_height * get_model_size_modifier( );
        else world_pos.y += base_height;

        if ( !world_to_screen( world_pos, screen_pos ) ) return { };

        screen_pos.y -= ( hud_modifier ) * g_render_manager->get_height( ) * 0.00083333335f * base_height;

        return screen_pos;
    }

    auto Object::get_ai_manager( ) const -> utils::MemoryHolder< AiManager >{
        // E8 ? ? ? ? 5E 83 78 04 01
        /*  auto data_fields = reinterpret_cast< unsigned char* >( reinterpret_cast< uintptr_t >( this ) + 0x2BD4 );
          if ( !data_fields ) return { };
  
          auto result = *reinterpret_cast< unsigned* >( &data_fields[ 4 * reinterpret_cast< unsigned char* >( this )[
              0x2BDC ] + 0xC ] );
  
          auto large_key = reinterpret_cast< uintptr_t* >( data_fields + 0x4 );
          for ( int i = 0; i < data_fields[ 1 ]; i++ ) *( &result + i ) ^= ~*large_key++;
  
          if ( !data_fields[ 2 ] || 4 - data_fields[ 2 ] >= 4 ) {
              return utils::c_memory_holder< AiManager >( app->memory->read< uintptr_t >( result + 0x8 ) );
          }
  
          auto small_key = &data_fields[ data_fields[ 2 ] ];
          if ( !small_key ) throw std::runtime_error( "invalid ptr in ai manager decryption" );
          for ( int i = 4 - data_fields[ 2 ]; i < 4; i++ ) {
              if ( !( reinterpret_cast< unsigned char* >( &result ) + i ) || !small_key )
                  throw std::exception(
                      "invalid ptr in ai manager decryption"
                  );
              *( reinterpret_cast< unsigned char* >( &result ) + i ) ^= ~*small_key++;
          }*/
        // auto start_ptr = 0x3718 - 8 * 6;
        //
        // for ( int i = 0; i < 20; ++i ) {
        //     const auto ptr = reinterpret_cast< Obfuscated< unsigned long long >* >( reinterpret_cast< uintptr_t >(
        //             this ) +
        //         ( start_ptr + ( i * 4 ) ) )->get( );
        //
        //     if ( ptr == 0 ) continue;
        //
        //     auto mh = utils::MemoryHolder< AiManager >( app->memory->read< uintptr_t >( ptr + 0x10 ) );
        //
        //     if ( mh.get_address( ) != 0 )
        //         debug_log( "FOUND AIMGR: {} | offsett: {:x}", mh.get_address( ), start_ptr + ( i * 4 ) );
        // }

        // debug_log( "offset: {:x}", ( offsetof( Object, ai_manager ) ) );
        
        const auto ptr = reinterpret_cast< Obfuscated< unsigned long long >* >( reinterpret_cast< uintptr_t >( this ) +
            ( offsetof( Object, ai_manager ) ) )->get( );

        // debug_log( "ai_mgr ptr: {:x}", ptr );
        
        if ( ptr == 0 ) return { };

        auto ptr2 = app->memory->read< uintptr_t >( ptr + 0x10 );

        if ( !ptr2.has_value( ) ) return { };

        auto mh = utils::MemoryHolder< AiManager >( *ptr2 );

        // debug_log( "ai mgr: {:x}", mh.get_address(  ) );

        return mh;
    }

    auto Object::is_alive( ) const -> bool{
        if ( rt_hash( champion_name.text ) == ct_hash( "KogMaw" ) ||
            rt_hash( champion_name.text ) == ct_hash( "Sion" ) ||
            rt_hash( champion_name.text ) == ct_hash( "Karthus" )
        )
            return health > 0.f && get_is_dead_flag( ) % 1 == 0;


        // DONTFUCKING CHANGE THIS, the way it was before DIDNT WORK, see #feedback for proof? who cares
        // no one changed it!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

        return health > 0.f && get_is_dead_flag( ) % 2 == 0;
    }

    auto Object::is_enemy( ) const -> bool{
        return team != g_local->team;
    }

    auto Object::is_ally( ) const -> bool{
        return !is_enemy( );
    }

    auto Object::is_zombie( ) const -> bool{
        __int64 v1; // rax
        __int64 v2; // rdx
        unsigned __int64 v3; // r8
        unsigned __int64 v4; // rcx
        unsigned __int8 v5; // r9
        __int64 v6; // rax
        __int64 v8; // [rsp+8h] [rbp+8h]

        const auto a1 = ( uintptr_t )this;
        v1 = *( unsigned __int8* )( a1 + 0x2E );
        v2 = a1 + 0x2A;
        v3 = *( unsigned __int8* )( a1 + 0x2B );
        v4 = 0i64;
        v5 = *( BYTE* )( v1 + v2 + 5 );
        v8 = v5;
        if ( v3 ) {
            do {
                *( &v8 + v4 ) ^= ~*( QWORD* )( v2 + 8 * v4 + 3 );
                ++v4;
            }
            while ( v4 < v3 );
            v5 = v8;
        }
        if ( !*( BYTE* )( v2 + 2 ) ) return v5;
        v6 = 1i64 - *( unsigned __int8* )( v2 + 2 );
        if ( v6 ) return v5;
        do {
            *( BYTE* )( &v8 + v6 ) ^= ~*( BYTE* )( v2 + v6 + 3 );
            ++v6;
        }
        while ( !v6 );
        return ( unsigned __int8 )v8;
    }

    auto Object::dist_to_local( ) const -> float{
        return g_local->position.dist_to( position );
    }

    auto Object::attack_damage( ) const -> float{
        return base_attack + bonus_attack;
    }

    auto Object::bonus_attack_damage( ) const -> float{
        return bonus_attack;
    }

    auto Object::missile_spell_info( ) const -> utils::MemoryHolder< SpellInfo >{
        if ( !missile_spell_info_p ) return { };

        const auto ptr = reinterpret_cast< intptr_t >( missile_spell_info_p );

        auto msi = app->memory->read< SpellInfo >( ptr );

        if ( !msi.has_value( ) ) return { };

        return { *msi, ptr };
    }

    auto Object::get_entity_info( ) const -> utils::MemoryHolder< EntityInfo >{
        const auto ptr = *reinterpret_cast< uintptr_t* >( reinterpret_cast< uintptr_t >( this ) + 0x4AF0 );
        if ( ptr == 0 ) return { };

        //const auto address = app->memory->read< uintptr_t >( ptr );
        //if ( address == 0 ) return {};

        return utils::MemoryHolder< EntityInfo >( ptr );
    }

    auto Object::get_minion_type( ) const -> EMinionType{
        auto name = get_name( );

        if ( name.find( _( "MinionMelee" ) ) != std::string::npos ) return EMinionType::melee;
        if ( name.find( _( "MinionRanged" ) ) != std::string::npos ) return EMinionType::ranged;
        if ( name.find( _( "MinionSiege" ) ) != std::string::npos ) return EMinionType::siege;
        if ( name.find( _( "MinionSuper" ) ) != std::string::npos ) return EMinionType::super;
        if ( is_jungle_monster( ) ) return EMinionType::jungle;
        if ( is_misc_minion( ) ) return EMinionType::misc;
        if ( is_plant( ) ) return EMinionType::plant;
        if ( name.find( _( "Turret" ) ) != std::string::npos ) return EMinionType::turret;
        if ( is_senna_minion( ) ) return EMinionType::misc;

        return EMinionType::error;
    }

    auto Object::is_senna_minion( ) const -> bool{
        if ( rt_hash( g_local->get_name( ).c_str( ) ) != ct_hash( "Senna" ) ) return false;

        return rt_hash( get_name( ).data( ) ) == ct_hash( "SennaSoul" );
    }

    auto Object::is_misc_minion( ) const -> bool{
        auto name = get_name( );

        return name.find( _( "AnnieTibbers" ) ) != std::string::npos ||
            name.find( _( "ApheliosTurret" ) ) != std::string::npos ||
            name.find( _( "ZacRebirthBloblet" ) ) != std::string::npos ||
            name.find( _( "HeimerT" ) ) != std::string::npos ||
            name.find( _( "IvernMinion" ) ) != std::string::npos ||
            name.find( _( "ZyraThornPlant" ) ) != std::string::npos ||
            name.find( _( "ZyraGraspingPlant" ) ) != std::string::npos ||
            name.find( _( "KalistaSpawn" ) ) != std::string::npos ||
            name.find( _( "YorickGhoulMelee" ) ) != std::string::npos ||
            name.find( _( "YorickBigGoul" ) ) != std::string::npos ||
            name.find( _( "YorickWInvisible" ) ) != std::string::npos ||
            name.find( _( "MalzaharVoidling" ) ) != std::string::npos ||
            name.find( _( "IllaoiMinion" ) ) != std::string::npos && get_selectable_flag( ) == 1 ||
            get_ward_type( ) != EWardType::unknown && get_selectable_flag( ) == 1;
    }

    auto Object::is_plant( ) const -> bool{
        const auto name = get_name( );
        if ( get_selectable_flag( ) == 0 ) return false;

        return name.find( "SRU_Plant_Health" ) != std::string::npos ||
            name.find( "SRU_Plant_Vision" ) != std::string::npos ||
            name.find( "SRU_Plant_Satchel" ) != std::string::npos;
    }

    auto Object::is_zyra_plant( ) const -> bool{
        const auto name_hash = rt_hash( get_name( ).data( ) );

        return name_hash == ct_hash( "ZyraThornPlant" ) ||
            name_hash == ct_hash( "ZyraGraspingPlant" ) ||
            name_hash == ct_hash( "ZyraSeed" );
    }


    auto Object::is_barrel( ) const -> bool{
        auto name = get_name( );

        return name.find( _( "GangplankBarrel" ) ) != std::string::npos;
    }

    auto Object::is_feather( ) const -> bool{
        return rt_hash( this->name.text ) == ct_hash( "Feather" );
    }

    auto Object::is_jarvan_flag( ) const -> bool{
        auto name = get_name( );

        return name.find( _( "JarvanIVStandard" ) ) != std::string::npos;
    }

    auto Object::is_sand_soldier( ) const -> bool{
        auto name = get_name( );

        return name.find( _( "AzirSoldier" ) ) != std::string::npos;
    }

    auto Object::is_tentacle( ) const -> bool{
        auto name = get_name( );

        return name.find( _( "IllaoiMinion" ) ) != std::string::npos;
    }

    auto Object::is_windwall( ) const -> bool{
        std::string obj_name = name.text;

        return obj_name.find( _( "YasuoWChildMis" ) ) != std::string::npos;
    }

    auto Object::is_spore( ) const -> bool{
        return rt_hash( get_name().c_str() ) == ct_hash( "BelvethSpore" );
    }

    auto Object::is_minion_only_autoattackable( ) const -> bool{
        auto name = get_name( );

        return name.find( _( "JhinTrap" ) ) != std::string::npos ||
            name.find( _( "GangplankBarrel" ) ) != std::string::npos ||
            name.find( _( "NidaleeSpear" ) ) != std::string::npos ||
            name.find( _( "KalistaSpawn" ) ) != std::string::npos ||
            name.find( _( "YorickWInvisible" ) ) != std::string::npos ||
            name.find( _( "IllaoiMinion" ) ) != std::string::npos ||
            is_plant( ) || get_ward_type( ) != EWardType::unknown && get_ward_type( ) != EWardType::shaco_box;
    }

    auto Object::has_special_minion_health( ) const -> bool{
        if ( is_plant( ) ) return true;

        const auto ward_type = get_ward_type( );
        switch ( ward_type ) {
        case EWardType::blue:
        case EWardType::control:
        case EWardType::jhin_trap:
        case EWardType::nidalee_trap:
        case EWardType::normal:
        case EWardType::teemo_shroom:
        case EWardType::zombie:
        case EWardType::fiddlesticks_effigy:
            return true;
        case EWardType::maokai_sproutling:
        case EWardType::shaco_box:
            return false;
        default:
            break;
        }


        auto name = get_name( );

        return name.find( _( "GangplankBarrel" ) ) != std::string::npos ||
            name.find( _( "KalistaSpawn" ) ) != std::string::npos ||
            name.find( _( "YorickWInvisible" ) ) != std::string::npos ||
            name.find( _( "IllaoiMinion" ) ) != std::string::npos ||
            name.find( _( "MalzaharVoidling" ) ) != std::string::npos;
    }


    auto Object::is_lane_minion( ) const -> bool{
        auto name = get_name( );

        return name.find( _( "MinionMelee" ) ) != std::string::npos ||
            name.find( _( "MinionRanged" ) ) != std::string::npos ||
            name.find( _( "MinionSiege" ) ) != std::string::npos ||
            name.find( _( "MinionSuper" ) ) != std::string::npos;
    }

    auto Object::is_jungle_monster( ) const -> bool{
        auto name = get_name( );

        if ( name.find( _( "CrabWard" ) ) != std::string::npos || is_bad_minion( ) ) return false;

        return name.find( _( "Dragon" ) ) != std::string::npos ||
            name.find( _( "Crab" ) ) != std::string::npos ||
            name.find( _( "RiftHerald" ) ) != std::string::npos ||
            name.find( _( "Razorbeak" ) ) != std::string::npos ||
            name.find( _( "SRU_Red" ) ) != std::string::npos ||
            name.find( _( "SRU_Blue" ) ) != std::string::npos ||
            name.find( _( "Krug" ) ) != std::string::npos ||
            name.find( _( "Gromp" ) ) != std::string::npos ||
            name.find( _( "Murkwolf" ) ) != std::string::npos ||
            name.find( _( "Baron" ) ) != std::string::npos;
    }

    auto Object::is_major_monster( ) const -> bool{
        auto name = get_name( );

        return name.find( _( "Dragon" ) ) != std::string::npos ||
            name.find( _( "RiftHerald" ) ) != std::string::npos ||
            name.find( _( "Baron" ) ) != std::string::npos;
    }

    auto Object::$is_normal_minion( ) const -> bool{
        if ( is_bad_minion( ) ) return false;
        auto name = get_name( );

        if ( name.find( _( "CrabWard" ) ) != std::string::npos ) return false;


        return is_lane_minion( ) ||
            is_jungle_monster( ) ||
            is_misc_minion( ) ||
            is_plant( ) ||
            is_senna_minion( );
    }

    auto Object::is_normal_minion( ) const -> bool{
        return g_entity_cache.is_normal_minion( index );
    }

    auto Object::is_main_camp_monster( ) const -> bool{
        auto name = get_name( );

        if ( name.find( _( "CrabWard" ) ) != std::string::npos ) return false;

        const auto hash = rt_hash( name.data() );

        return hash == ct_hash( "Sru_Crab" ) ||
            hash == ct_hash( "SRU_Razorbeak" ) ||
            hash == ct_hash( "SRU_RazorbeakMini" ) ||
            hash == ct_hash( "SRU_Red" ) ||
            hash == ct_hash( "SRU_Blue" ) ||
            hash == ct_hash( "SRU_Krug" ) ||
            hash == ct_hash( "SRU_KrugMini" ) ||
            hash == ct_hash( "SRU_Gromp" ) ||
            hash == ct_hash( "SRU_Murkwolf" ) ||
            hash == ct_hash( "SRU_MurkwolfMini" );
    }

    auto Object::get_monster_priority( ) const -> int32_t{
        auto name = get_name( );

        if ( name.find( _( "CrabWard" ) ) != std::string::npos || is_bad_minion( ) || !is_jungle_monster( ) ) return 0;

        if ( name.find( _( "RazorbeakMini" ) ) != std::string::npos ||
            name.find( _( "MurkwolfMini" ) ) != std::string::npos ||
            name.find( _( "KrugMini" ) ) != std::string::npos )
            return 1;

        return 2;
    }

#if enable_lua
    auto Object::lua_get_bounding_radius( ) const -> sol::object{
        if ( is_hero( ) ) {
            if ( !this->champion_name.text || !this->champion_name.is_valid( ) ) return sol::nil;

            return sol::make_object(
                g_lua_state2,
                features::Prediction::get_champion_radius( rt_hash( this->champion_name.text ) )
            );
        }

        return sol::make_object( g_lua_state2, g_entity_list.get_bounding_radius( this->index ) );
    }
#endif

    auto Object::is_turret_object( ) const -> bool{
        return g_entity_cache.is_turret_object( index );
    }

    auto Object::$is_turret_object( ) const -> bool{
        std::string object_name = this->name.text;

        return get_name( ).find( _( "Turret" ) ) != std::string::npos ||
            object_name.find( _( "Barracks" ) ) != std::string::npos ||
            object_name.find( _( "HQ" ) ) != std::string::npos;
    }

    auto Object::is_turret_attackable( ) const -> bool{
        if ( get_selectable_flag( ) == 0 ) return false;

        std::string object_name = this->name.text;
        if ( object_name.find( _( "Barracks" ) ) != std::string::npos ||
            object_name.find( _( "HQ" ) ) != std::string::npos )
            return true;

        return !is_dead( );
    }


    auto Object::get_bounding_radius( ) const -> float{
        if ( this->is_hero( ) ) {
            switch ( rt_hash( this->champion_name.text ) ) {
            case ct_hash( "Amumu" ):
            case ct_hash( "Annie" ):
            case ct_hash( "Fizz" ):
            case ct_hash( "Heimerdinger" ):
            case ct_hash( "Kennen" ):
            case ct_hash( "Poppy" ):
            case ct_hash( "Teemo" ):
            case ct_hash( "Tristana" ):
            case ct_hash( "Veigar" ):
            case ct_hash( "Warwick" ):
            case ct_hash( "Ziggs" ):
                return 55.f;
            case ct_hash( "Ivern" ):
                return 70.f;
            case ct_hash( "Bard" ):
            case ct_hash( "Blitzcrank" ):
            case ct_hash( "Braum" ):
            case ct_hash( "Chogath" ):
            case ct_hash( "Darius" ):
            case ct_hash( "DrMundo" ):
            case ct_hash( "Galio" ):
            case ct_hash( "Gragas" ):
            case ct_hash( "Hecarim" ):
            case ct_hash( "Malphite" ):
            case ct_hash( "Maokai" ):
            case ct_hash( "Mordekaiser" ):
            case ct_hash( "Nasus" ):
            case ct_hash( "Nautilus" ):
            case ct_hash( "Ornn" ):
            case ct_hash( "Renekton" ):
            case ct_hash( "Rumble" ):
            case ct_hash( "Sejuani" ):
            case ct_hash( "Sion" ):
            case ct_hash( "Skarner" ):
            case ct_hash( "TahmKench" ):
            case ct_hash( "Urgot" ):
            case ct_hash( "Volibear" ):
            case ct_hash( "Yorick" ):
            case ct_hash( "Zac" ):
                return 80.f;
            default:
                return 65.f;
            }
        }

        auto name = get_name( );

        if ( is_senna_minion( ) ) return 65.f;

        if ( name.find( _( "Minion" ) ) != std::string::npos ) {
            if ( name.find( _( "MinionMelee" ) ) != std::string::npos ) return 48.f;
            if ( name.find( _( "MinionRanged" ) ) != std::string::npos ) return 48.f;
            if ( name.find( _( "MinionSiege" ) ) != std::string::npos ) return 65.f;
            if ( name.find( _( "MinionSuper" ) ) != std::string::npos ) return 65.f;

            return 0.f;
        }

        std::string object_name = this->name.text;
        if ( name.find( _( "Turret" ) ) != std::string::npos ) return 88.f;
        if ( object_name.find( _( "Barracks" ) ) != std::string::npos ) return 275.f;
        if ( object_name.find( _( "HQ" ) ) != std::string::npos ) return 320.f;

        if ( name.find( _( "SRU_Red" ) ) != std::string::npos ) return 120.f;
        if ( name.find( _( "SRU_Blue" ) ) != std::string::npos ) return 131.f;
        if ( name.find( _( "Crab" ) ) != std::string::npos ) return 43.5f;
        if ( name.find( _( "Gromp" ) ) != std::string::npos ) return 120.f;
        if ( name.find( _( "RazorbeakMini" ) ) != std::string::npos ) return 50.f;
        if ( name.find( _( "Razorbeak" ) ) != std::string::npos ) return 75.f;
        if ( name.find( _( "MurkwolfMini" ) ) != std::string::npos ) return 50.f;
        if ( name.find( _( "Murkwolf" ) ) != std::string::npos ) return 80.f;
        if ( name.find( _( "KrugMiniMini" ) ) != std::string::npos ) return 25.f;
        if ( name.find( _( "KrugMini" ) ) != std::string::npos ) return 50.f;
        if ( name.find( _( "Krug" ) ) != std::string::npos ) return 100.f;
        if ( name.find( _( "Dragon" ) ) != std::string::npos ) return 125.f;
        if ( name.find( _( "Baron" ) ) != std::string::npos ) return 105.f;
        if ( name.find( _( "RiftHerald" ) ) != std::string::npos ) return 110.f;
        //if ( is_barrel( ) ) return 50.f;
        if ( name.find( _( "GangplankBarrel" ) ) != std::string::npos ) {
            return
                50.f; // is_barrel calls get_name again which is not needed rpm call
        }

        return 35.f;
    }

    auto Object::get_name( ) const -> std::string{
        std::array< char, 32 > v{ };

        const auto ptr = reinterpret_cast< intptr_t >( object_name );
        if ( ptr == 0 ) return { };

        app->memory->read_amount( ptr, v.data( ), 32 );

        return std::string( v.data( ) );
    }

    auto Object::get_alternative_name( ) const -> std::string{
        std::array< char, 32 > v{ };

        const auto ptr = *reinterpret_cast< uintptr_t* >( reinterpret_cast< uintptr_t >( this ) + 0x60 );
        if ( ptr == 0 ) return { };

        app->memory->read_amount( ptr, v.data( ), 32 );

        return std::string( v.data( ) );
    }

    auto Object::get_missile_target_index( ) const -> int16_t{
        const auto ptr = *reinterpret_cast< uintptr_t* >( reinterpret_cast< uintptr_t >( this ) + 0x3E0 );
        if ( ptr == 0 ) return { };

        auto mti = app->memory->read< int16_t >( ptr );

        if ( !mti.has_value( ) ) return { };

        return *mti;
    }

    auto Object::get_owner_index( ) const -> int16_t{
        const auto ptr = *reinterpret_cast< uintptr_t* >( reinterpret_cast< uintptr_t >( this ) + 0x758 );
        if ( ptr == 0 ) return 0;

        const auto address = app->memory->read< uintptr_t >( ptr + 0x10 );
        if ( !address.has_value( ) || *address == 0 ) return 0;

         const auto owner_ptr = app->memory->read<uintptr_t>(*address );
        if (!owner_ptr.has_value() || *owner_ptr == 0) return 0;

        auto oi = app->memory->read<int16_t>(*owner_ptr + 0x10);

        if ( !oi.has_value( ) ) return 0;

        return *oi;
    }

    auto Object::get_particle_source_index( ) const -> int16_t{
        const auto ptr = *reinterpret_cast< uintptr_t* >( reinterpret_cast< uintptr_t >( this ) + 0x2D8 );
        if ( ptr == 0 ) return 0;

        const auto ptr2 = app->memory->read< uintptr_t >( ptr + 0x8 );
        if ( !ptr2.has_value( ) || *ptr2 == 0 ) return 0;

        const auto address = app->memory->read< uintptr_t >( *ptr2 );
        if ( !address.has_value( ) || *address == 0 ) return 0;

        auto psi = app->memory->read< int16_t >( *address + 0x10 );

        if ( !psi.has_value( ) ) return 0;

        return *psi;
    }

    auto Object::get_particle_spawn_time( ) const -> float{
        const auto ptr = *reinterpret_cast< uintptr_t* >( reinterpret_cast< uintptr_t >( this ) + 0x298 );
        if ( ptr == 0 ) return -1.f;

        const auto ptr2 = app->memory->read< uintptr_t >( ptr + 0x28 );
        if ( !ptr2.has_value( ) || *ptr2 == 0 ) return -1.f;

        auto pst = app->memory->read< float >( *ptr2 + 0x108 );

        if ( !pst.has_value( ) ) return 0;

        return *pst;
    }

    auto Object::get_particle_alt_spawn_time( ) const -> float{
        const auto ptr = *reinterpret_cast< uintptr_t* >( reinterpret_cast< uintptr_t >( this ) + 0x298 );
        if ( ptr == 0 ) return -1.f;

        const auto ptr2 = app->memory->read< uintptr_t >( ptr + 0x28 );
        if ( !ptr2.has_value( ) || *ptr2 == 0 ) return -1.f;

        auto past = app->memory->read< float >( *ptr2 + 0x114 );

        if ( !past.has_value( ) ) return -1.f;

        return *past;
    }

    auto Object::get_particle_target_index( ) const -> int16_t{
        const auto ptr = *reinterpret_cast< uintptr_t* >( reinterpret_cast< uintptr_t >( this ) + 0x60 );
        if ( ptr == 0 ) return 0;

        const auto ptr2 = app->memory->read< uintptr_t >( ptr + 0x50 );
        if ( !ptr2.has_value( ) || *ptr2 == 0 ) return 0;

        const auto ptr3 = app->memory->read< uintptr_t >( *ptr2 + 0x8 );
        if ( !ptr3.has_value( ) || *ptr3 == 0 ) return 0;

        const auto address = app->memory->read< uintptr_t >( *ptr3 );
        if ( !address.has_value( ) || *address == 0 ) return 0;

        auto pti = app->memory->read< int16_t >( *address + 0x10 );

        if ( !pti.has_value( ) ) return 0;

        return *pti;
    }


    auto Object::get_kills( ) const -> int{
        auto kills = app->memory->read< int >( reinterpret_cast< uintptr_t >( this ) + 0x47C8 );

        if ( !kills.has_value( ) ) return 0;

        return *kills;
    }

    auto Object::get_particle_direction( ) const -> math::Vec3{
        const auto ptr = *reinterpret_cast< uintptr_t* >( reinterpret_cast< uintptr_t >( this ) + 0x298 );
        if ( ptr == 0 ) return { };

        //const auto ptr2 = app->memory->read<uintptr_t>(ptr);
        //if (ptr2 == 0) return {};

        auto pd = app->memory->read< math::Vec3 >( ptr + 0x128 );
        if ( !pd.has_value( ) ) return { };

        return *pd;
    }

    auto Object::get_experience( ) const -> float{ // TODO: TORE UPDATE THIS
        return *reinterpret_cast<float *>(reinterpret_cast<uintptr_t>(this) + 0x4DD8);
    }

    auto Object::get_vision_score( ) const -> float{
        // todo: read memory here
        return *reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( this ) + 0x48A8 );
    }

    auto Object::get_history_position( int32_t history_tick ) const -> const math::Vec3&{ //outdate
        if ( history_tick > 10 ) history_tick = 10;

        const uintptr_t offset = 0x37C4 + 0x10 * history_tick;

        return *reinterpret_cast< Vec3* >( reinterpret_cast< uintptr_t >( this ) + offset );
    }

    auto Object::get_history_position_time( int history_tick ) const -> float{ //outdate
        if ( history_tick > 10 ) history_tick = 10;

        const uintptr_t offset = 0x37D0 + 0x10 * history_tick;

        return *reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( this ) + offset );
    }

    auto Object::get_ward_type( ) const -> EWardType{
        const auto name = get_name( );

        if ( name.find( _( "SightWard" ) ) != std::string::npos
            || name.find( _( "YellowTrinket" ) ) != std::string::npos )
            return EWardType::normal;

        const auto hash = rt_hash( name.c_str( ) );

        if ( hash == ct_hash( "JammerDevice" ) ) return EWardType::control;
        if ( hash == ct_hash( "BlueTrinket" ) ) return EWardType::blue;
        if ( hash == ct_hash( "PerksZombieWard" ) ) return EWardType::zombie;
        if ( hash == ct_hash( "TeemoMushroom" ) ) return EWardType::teemo_shroom;
        if ( hash == ct_hash( "JhinTrap" ) ) return EWardType::jhin_trap;
        if ( hash == ct_hash( "NidaleeSpear" ) ) return EWardType::nidalee_trap;
        if ( hash == ct_hash( "ShacoBox" ) ) return EWardType::shaco_box;
        if ( hash == ct_hash( "MaokaiSproutling" ) ) return EWardType::maokai_sproutling;
        if ( hash == ct_hash( "FiddleSticksEffigy" ) ) return EWardType::fiddlesticks_effigy;
        if ( hash == ct_hash( "CaitlynTrap" ) ) return EWardType::caitlyn_trap;


        return EWardType::unknown;
    }

    auto Object::get_base_ms( ) const -> float{
        const auto uic = get_unit_info_component( );
        if ( !uic.is_valid( ) ) return { };

        return uic->get_base_ms( );
    }

    auto Object::is_invulnerable( ) const -> bool{
        const auto flag = get_targetable_flag( );

        switch ( flag ) {
        case 1:
        case 3:
        case 4:
        case 17:
        case 33:
            return true;
        default:
            return false;
        }
    }

    auto Object::is_bad_minion( ) const -> bool{
        auto name = get_name( );

        if ( get_targetable_flag( ) == 1 ||
            get_targetable_flag( ) == 65 && ( name.find( _( "KrugMiniMini" ) ) != std::string::npos || name.
                find( _( "KrugMini" ) ) != std::string::npos ) )
            return true;

        return false;
    }

    auto Object::get_holder( ) const -> CHolder&{
        return g_entity_list.get_by_index( index );
    }
}
