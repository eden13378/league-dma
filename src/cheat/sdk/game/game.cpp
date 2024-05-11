#include "pch.hpp"

#include "buff.hpp"
#include "spell_book.hpp"
#include "spell_slot.hpp"
#include "buff_data.hpp"
#include "buff_info.hpp"
#include "../globals.hpp"
#include "spell_cast_info.hpp"
#include "spell_info.hpp"
#include "spell_data.hpp"
#include "spell_details.hpp"
#include "../../features/runtime/runtime.hpp"
#if enable_new_lua
#include "../../lua-v2/state.hpp"
#endif
#include "../../security/src/xorstr.hpp"
#if enable_new_lua
#include "../../lua-v2/custom_structs.hpp"
#include "../../lua-v2/lua_def.hpp"
#endif

#if enable_lua
#include <sol/sol.hpp>
#endif

namespace sdk::game {
    auto SpellBook::get_spell_slot( ESpellSlot id ) const -> utils::MemoryHolder< SpellSlot >{
        if ( static_cast< int32_t >( id ) >= static_cast< int32_t >( ESpellSlot::max ) ) return { };

        const auto ptr = *reinterpret_cast< intptr_t* >( reinterpret_cast< intptr_t >( this ) + offsetof(
            SpellBook,
            spell_slot_q
        ) + 0x8 * static_cast< int32_t >( id ) );

        if ( ptr == 0 ) return { };

        auto read = app->memory->read< SpellSlot >( ptr );

        if ( !read.has_value( ) ) {
            return { };
        }

        return { *read, ptr };
    }

    auto SpellBook::get_spell_cast_info( ) const -> utils::MemoryHolder< SpellCastInfo >{
        const auto ptr = reinterpret_cast< intptr_t >( spell_cast_info );

        if ( !ptr ) return { };

        auto sci = utils::MemoryHolder< SpellCastInfo >( ptr );


        // if ( sci ) {
        //     debug_log( "sci: start_time: {} | end_time: {} | server_cast_time: {} | start_position: Vec3({}, {}, {}) | slot: {} | windup_time: {} | total_cast_time: {} | missile_index: {}",
        //                sci->start_time,
        //                sci->end_time,
        //                sci->server_cast_time,
        //                sci->start_position.x,
        //                sci->start_position.y,
        //                sci->start_position.z,
        //                sci->slot,
        //                sci->windup_time,
        //                sci->total_cast_time,
        //                sci->missile_index
        //     );
        // }

        return sci;
    }
#if enable_new_lua
    auto SpellBook::get_spell_slot_raw( ESpellSlot id ) const -> std::optional< lua::LuaSpellSlot >{
        const auto spl = get_spell_slot( id );
        if ( !spl ) return std::nullopt;

        return lua::LuaSpellSlot( spl.get( ) );
    }

    auto SpellBook::get_spell_cast_info_raw( ) const -> std::optional< lua::LuaSpellCastInfo >{
        const auto spl = get_spell_cast_info( );
        if ( !spl ) return std::nullopt;

        return lua::LuaSpellCastInfo( spl.get( ) );
    }
#endif

    auto SpellSlot::get_spell_info( ) const -> utils::MemoryHolder< SpellInfo >{
        if ( !spell_info ) return { };

        return utils::MemoryHolder< SpellInfo >( reinterpret_cast< intptr_t >( spell_info ) );
    }

    auto SpellSlot::get_details( ) const -> utils::MemoryHolder< SpellDetails >{
        if ( !spell_details ) return { };

        return utils::MemoryHolder< SpellDetails >( reinterpret_cast< intptr_t >( spell_details ) );
    }

    auto SpellSlot::get_usable_state( ) const -> int8_t{
        return *reinterpret_cast< int8_t* >( reinterpret_cast< uintptr_t >( this ) + 0xE8 );
    }

    auto SpellSlot::get_active_state( ) const -> int{
        return *reinterpret_cast< int8_t* >( reinterpret_cast< uintptr_t >( this ) + 0x2F );
    }

    auto SpellSlot::get_spell_effect_value( ) const -> float{
        return *reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( this ) + 0x90 );
    }

    auto SpellSlot::get_name( ) const -> std::string{
        auto info = get_spell_info( );
        if ( !info ) return _( "error" );

        auto data = info->get_spell_data( );
        if ( !data ) return _( "error" );

        return data->get_name( );
    }

    auto SpellSlot::get_cooldown( ) const -> float{
        auto info = get_spell_info( );
        if ( !info ) return 0.f;

        auto data = info->get_spell_data( );
        if ( !data ) return 0.f;

        return data->get_cooldown_duration( this->level );
    }


    auto SpellSlot::is_ready( const bool is_local ) const -> bool{
        if ( level == 0 ) return false;

        if ( is_local ) {
            auto spell_info = get_spell_info( );
            if ( !spell_info ) return false;

            auto spell_data = spell_info->get_spell_data( );
            if ( !spell_data ) return false;

            const auto mana_cost = spell_data->get_mana_cost( );
            if ( mana_cost.empty( ) || mana_cost.size( ) <= level - 1u ) return cooldown_expire <= *g_time;

            if ( mana_cost[ level - 1 ] > g_local->mana ) return false;
        }

        if ( cooldown == 0.f ) return true;

        return cooldown_expire <= *g_time;
    }

    auto SpellSlot::get_manacost( ) const -> float{
        if ( level == 0 ) return 0.f;

        auto spell_info = get_spell_info( );
        if ( !spell_info ) return 0.f;

        auto spell_data = spell_info->get_spell_data( );
        if ( !spell_data ) return 0.f;

        const auto mana_cost = spell_data->get_mana_cost( );
        if ( mana_cost.empty( ) || level >= static_cast< int32_t >( mana_cost.size( ) ) || mana_cost[ level - 1 ] <=
            1.f )
            return 0.f;

        return mana_cost[ level - 1 ];
    }

    auto SpellSlot::get_spell_info_raw( ) const -> std::optional< SpellInfo >{
        const auto spl = get_spell_info( );

        if ( !spl ) return std::nullopt;

        return *spl;
    }

    auto SpellSlot::get_details_raw( ) const -> std::optional< SpellDetails >{
        const auto spl = get_details( );

        if ( !spl ) return std::nullopt;

        return *spl;
    }

    auto Buff::get_buff_data( ) const -> utils::MemoryHolder< BuffData >{
        if ( buff_data == 0 ) return { };

        return utils::MemoryHolder< BuffData >( buff_data );
    }

    auto BuffData::get_buff_info( ) const -> utils::MemoryHolder< BuffInfo >{
        if ( !buff_info ) return { };

        return utils::MemoryHolder< BuffInfo >( reinterpret_cast< intptr_t >( buff_info ) );
    }

    auto BuffData::is_active( ) const -> bool{
        return end_time > *g_time && std::max( stack, count ) > 0 && start_time < *g_time;
    }

    auto BuffData::is_hard_cc( ) const -> bool{
        switch ( type ) {
        case 5:
        case 8:
        case 11:
        case 21:
        case 22:
        case 24:
            return true;
        default:
            return false;
        }
    }

    auto BuffData::is_disabling( ) const -> bool{
        switch ( type ) {
        case 5:
        case 8:
        case 21:
        case 22:
        case 24:
        case 25:
        case 29:
            return true;
        default:
            return false;
        }
    }

    auto BuffData::is_knock_up( ) const -> bool{
        return type == static_cast< unsigned char >( 29 );
    }

    auto BuffData::is_silence( ) const -> bool{
        return type == static_cast< unsigned char >( 7 );
    }

    auto BuffData::is_cripple( ) const -> bool{
        return type == static_cast< unsigned char >( 18 );
    }

    auto BuffData::is_invincible( ) const -> bool{
        return type == static_cast< unsigned char >( 17 ) ||
            type == static_cast< unsigned char >( 16 );
    }

    auto BuffData::is_slow( ) const -> bool{
        return type == static_cast< unsigned char >( 10 );
    }

    auto BuffData::get_senna_passive_attack_damage( ) const -> float{
        return *reinterpret_cast< float* >( reinterpret_cast< intptr_t >( this ) + 0x54 );
    }


    auto BuffData::get_source_index( ) const -> int16_t{
        const auto ptr = *reinterpret_cast< uintptr_t* >( reinterpret_cast< uintptr_t >( this ) + 0x40 );
        if ( ptr == 0 ) return 0;

        const auto address = app->memory->read< uintptr_t >( ptr + 0x8 );
        if ( !address.has_value( ) ) return 0;
        if ( *address == 0 ) return 0;

        auto source_index = app->memory->read< int16_t >( *address + 0x10 );

        if ( !source_index.has_value( ) ) return 0;

        return *source_index;
    }


    auto BuffManager::get_buff_array_end( ) const -> intptr_t{
        auto bae = app->memory->read< intptr_t >( buff_array_end );
        if ( bae ) return 0;
        return *bae;
    }

    auto BuffManager::size( ) const -> size_t{
        const auto delta = buff_array_end - buff_array_start;

        if ( delta == 0 ) return 0;

        return delta / sizeof( BuffData* );
    }

    auto BuffManager::get_all( ) const -> std::vector< utils::MemoryHolder< BuffData > >{
        if ( !buff_array_start ) return { };
        const auto s = size( );

        if ( s > 128 || s < 0 ) return { };

        std::vector< uintptr_t > rawBuffManagerArray( s );
        app->memory->read_amount< uintptr_t >( buff_array_start, rawBuffManagerArray.data( ), s );

        std::vector< utils::MemoryHolder< BuffData > > buffs( rawBuffManagerArray.size( ) / 2 );

        for ( auto i = 0, j = 0; i < rawBuffManagerArray.size( ); i += 2, j++ ) {
            if ( j >= buffs.size( ) ) break;
            buffs[ j ] = utils::MemoryHolder< BuffData >( rawBuffManagerArray[ i ] );
        }

        return buffs;
    }

#if enable_new_lua
    auto BuffManager::get_all_raw( ) const -> sol::as_table_t< std::vector< lua::LuaBuff > >{
        const auto all = get_all( );
        std::vector< lua::LuaBuff > buffs;
        buffs.reserve( all.size( ) );

        for ( auto& b : all ) {
            buffs.emplace_back( b.get( ) );
        }

        return sol::as_table( buffs );
    }
#endif

    auto CameraConfig::get_zoom( ) -> float{
        const auto ptr = app->memory->read< uintptr_t >(
            app->memory->get_process( )->get_module( ct_hash( "league of legends.exe" ) )->get_base( ) +
            offsets::camera_config
        );

        if ( !ptr.has_value( ) ) return 0.f;

        auto zoom_value = app->memory->read< float >( *ptr + 0x24 );
        if ( !zoom_value.has_value( ) ) return 0.f;

        return *zoom_value;
    }

    auto CameraConfig::set_zoom( const float value ) -> bool{
        const auto ptr = app->memory->read< uintptr_t >(
            app->memory->get_process( )->get_module( ct_hash( "league of legends.exe" ) )->get_base( ) +
            offsets::camera_config
        );

        if ( !ptr.has_value( ) ) return false;


        return app->memory->write< float >( *ptr + 0x24, value );
    }

    auto SpellCastInfo::get_target_index( ) const -> int16_t{
        // todo: DONT ADD THIS TO STRUCT
        const auto ptr = *reinterpret_cast< intptr_t* >( reinterpret_cast< intptr_t >( this ) + 0x100 );

        if ( ptr == 0 ) return -1;

        auto ti = app->memory->read< int16_t >( ptr );

        if ( !ti.has_value( ) ) return -1;

        return *ti;
    }

    auto SpellCastInfo::get_spell_info( ) const -> utils::MemoryHolder< SpellInfo >{
        if ( !spell_info ) return { };

        return utils::MemoryHolder< SpellInfo >( reinterpret_cast< intptr_t >( spell_info ) );
    }

    auto SpellCastInfo::get_spell_name( ) const -> std::string{
        const auto info = get_spell_info( );
        if ( !info ) return _( "error" );

        const auto data = info->get_spell_data( );
        if ( !data ) return _( "error" );

        return data->get_name( );
    }

    auto SpellCastInfo::get_spell_info_raw( ) -> std::optional< SpellInfo >{
        const auto spi = get_spell_info( );

        if ( !spi ) return std::nullopt;

        return *spi;
    }


    auto SpellInfo::get_spell_data( ) const -> utils::MemoryHolder< SpellData >{
        if ( !spell_data ) return { };

        return utils::MemoryHolder< SpellData >( reinterpret_cast< intptr_t >( spell_data ) );
    }

    auto SpellInfo::get_spell_data_raw( ) const -> std::optional< SpellData >{
        const auto sdr = get_spell_data( );

        if ( !sdr ) return std::nullopt;

        return *sdr;
    }

    auto SpellData::get_name( ) const -> std::string{
        std::array< char, 32 > v{ };
        if ( !spell_name ) return { };

        app->memory->read_amount( reinterpret_cast< intptr_t >( spell_name ), v.data( ), 32 );

        return v.data( );
    }

    auto SpellData::get_mana_cost( ) const -> std::vector< float >{
        std::vector< float > ret{ };

        //ret.reserve( 6 );

        for ( int i = 0; i < 6; i++ ) {
            const auto value = *reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( this ) + 0x604 + 0x4 * i );
            ret.push_back( value );
        }

        return ret;
    }

#if enable_lua
    auto Navgrid::lua_is_wall( sol::object position ) const -> sol::object{
        lua_arg_check_ct( position, Vec3, "vec3" )

        return sol::make_object( g_lua_state2, is_wall( position.as< Vec3 >( ) ) );
    }

    auto Navgrid::lua_is_bush( sol::object position ) const -> sol::object{
        lua_arg_check_ct( position, Vec3, "vec3" )

        return sol::make_object( g_lua_state2, is_bush( position.as< Vec3 >( ) ) );
    }

    auto Navgrid::lua_is_river( sol::object position ) const -> sol::object{
        lua_arg_check_ct( position, Vec3, "vec3" )

        return sol::make_object( g_lua_state2, is_river( position.as< Vec3 >( ) ) );
    }

    auto Navgrid::lua_is_building( sol::object position ) const -> sol::object{
        lua_arg_check_ct( position, Vec3, "vec3" )

        return sol::make_object( g_lua_state2, is_building( position.as< Vec3 >( ) ) );
    }
#endif
}
