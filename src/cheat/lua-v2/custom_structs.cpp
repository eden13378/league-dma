#include "pch.hpp"
#include "custom_structs.hpp"

#include "../sdk/game/buff_data.hpp"
#include "../sdk/game/buff_info.hpp"
#include "../sdk/game/spell_slot.hpp"
#include "../sdk/game/spell_info.hpp"
#include "../sdk/game/spell_data.hpp"
#include "../sdk/game/spell_cast_info.hpp"

namespace lua {
    LuaBuff::LuaBuff( const std::shared_ptr< sdk::game::BuffData >& buff ){
        m_buff_data = buff;

        const auto info = m_buff_data->get_buff_info( );
        if ( !info ) return;

        m_buff_info = info.get( );
    }

    LuaBuff::LuaBuff(
        const std::shared_ptr< sdk::game::BuffData >& buff,
        const std::shared_ptr< sdk::game::BuffInfo >& buff_info
    ){
        m_buff_data = buff;

        m_buff_info = buff_info;
    }

    auto LuaSpellSlot::get_level( ) const -> int32_t{
        if ( !m_spell_slot ) return { };
        return m_spell_slot->level;
    }

    auto LuaSpellSlot::get_cooldown_expire( ) const -> float{
        if ( !m_spell_slot ) return { };
        return m_spell_slot->cooldown_expire;
    }

    auto LuaSpellSlot::get_charges( ) const -> int32_t{
        if ( !m_spell_slot ) return { };
        return m_spell_slot->charges;
    }

    auto LuaSpellSlot::get_final_cooldown_expire( ) const -> float{
        if ( !m_spell_slot ) return { };
        return m_spell_slot->final_cooldown_expire;
    }

    auto LuaSpellSlot::get_cooldown( ) const -> float{
        if ( !m_spell_slot ) return { };
        return m_spell_slot->cooldown;
    }

    auto LuaSpellSlot::get_coefficient( ) const -> float{
        if ( !m_spell_data ) return { };
        return m_spell_data->get_coefficient( );
    }

    auto LuaSpellSlot::get_coefficient2( ) const -> float{
        if ( !m_spell_data ) return { };
        return m_spell_data->get_coefficient2( );
    }

    auto LuaSpellSlot::get_cooldown_time( ) const -> float{
        if ( !m_spell_data ) return { };
        return m_spell_data->get_cooldown_time( );
    }

    auto LuaSpellSlot::get_name( ) const -> std::string{
        if ( !m_spell_data ) return { };
        return m_spell_data->get_name( );
    }

    auto LuaSpellSlot::get_mana_cost( ) const -> sol::as_table_t< std::vector< float > >{
        if ( !m_spell_data ) return { };
        return sol::as_table( m_spell_data->get_mana_cost( ) );
    }

    auto LuaSpellSlot::get_missile_speed( ) const -> float{
        if ( !m_spell_data ) return { };
        return m_spell_data->get_missile_speed( );
    }

    auto LuaSpellSlot::get_channel_duration( ) const -> float{
        if ( !m_spell_data ) return { };
        return m_spell_data->get_channel_duration( );
    }

    auto LuaSpellSlot::get_missile_width( ) const -> float{
        if ( !m_spell_data ) return { };
        return m_spell_data->get_missile_width( );
    }

    auto LuaSpellSlot::get_owner_character( ) const -> std::optional< std::string >{
        if ( !m_spell_info ) return std::nullopt;
        return m_spell_info->owner_character;
    }

    auto LuaSpellSlot::is_ready( ) const -> bool{
        if ( !m_spell_slot ) return { };
        return m_spell_slot->is_ready( );
    }

    auto LuaBuff::is_active( ) const -> bool{
        if ( !m_buff_info ) return { };
        return m_buff_data->is_active( );
    }

    auto LuaBuff::is_hard_cc( ) const -> bool{
        if ( !m_buff_info ) return { };
        return m_buff_data->is_hard_cc( );
    }

    auto LuaBuff::is_disabling( ) const -> bool{
        if ( !m_buff_info ) return { };
        return m_buff_data->is_disabling( );
    }

    auto LuaBuff::is_knock_up( ) const -> bool{
        if ( !m_buff_info ) return { };
        return m_buff_data->is_knock_up( );
    }

    auto LuaBuff::is_silence( ) const -> bool{
        if ( !m_buff_info ) return { };
        return m_buff_data->is_silence( );
    }

    auto LuaBuff::is_cripple( ) const -> bool{
        if ( !m_buff_info ) return { };
        return m_buff_data->is_cripple( );
    }

    auto LuaBuff::is_invincible( ) const -> bool{
        if ( !m_buff_info ) return { };
        return m_buff_data->is_invincible( );
    }

    auto LuaBuff::is_slow( ) const -> bool{
        if ( !m_buff_info ) return { };
        return m_buff_data->is_slow( );
    }

    auto LuaBuff::get_source_index( ) -> int16_t{
        if ( !m_buff_data ) return { };
        return m_buff_data->get_source_index( );
    }

    auto LuaBuff::get_name( ) const -> std::optional< std::string >{
        if ( !m_buff_info ) return std::nullopt;
        return m_buff_info->get_name();
    }

    auto LuaBuff::get_type( ) const -> uint8_t{
        if ( !m_buff_data ) return { };
        return m_buff_data->type;
    }

    auto LuaBuff::get_start_time( ) const -> float{
        if ( !m_buff_data ) return { };
        return m_buff_data->start_time;
    }

    auto LuaBuff::get_end_time( ) const -> float{
        if ( !m_buff_data ) return { };
        return m_buff_data->end_time;
    }

    auto LuaBuff::get_end_time2( ) const -> float{
        if ( !m_buff_data ) return { };
        return m_buff_data->end_time2;
    }

    auto LuaBuff::get_alt_amount( ) const -> int32_t{
        if ( !m_buff_data ) return { };
        return m_buff_data->count;
    }

    auto LuaBuff::get_amount( ) const -> int32_t{
        if ( !m_buff_data ) return { };
        return m_buff_data->stack;
    }

    auto LuaBuff::get_max_stack( ) const -> int32_t{
        if ( !m_buff_data ) return { };
        return m_buff_data->max_stack;
    }

    auto LuaBuff::get_get_amount( ) const -> int32_t{
        if ( !m_buff_data ) return { };
        return get_amount( ) > get_alt_amount( ) ? get_amount( ) : get_alt_amount( );
    }

    LuaSpellCastInfo::LuaSpellCastInfo( std::shared_ptr< sdk::game::SpellCastInfo > info ): m_spell_cast_info{ info }{
        auto i = info->get_spell_info( );
        if ( !i ) return;

        m_spell_info = i.get( );

        auto data = i->get_spell_data( );

        if ( !data ) return;

        m_spell_data = data.get( );
    }

    auto LuaSpellCastInfo::get_target_index( ) const -> int16_t{
        if ( !m_spell_cast_info ) return { };
        return m_spell_cast_info->get_target_index( );
    }

    auto LuaSpellCastInfo::get_missile_index( ) const -> int32_t{
        if ( !m_spell_cast_info ) return { };
        return m_spell_cast_info->missile_index;
    }

    auto LuaSpellCastInfo::get_missile_nid( ) const -> uint32_t{
        if ( !m_spell_cast_info ) return { };
        return m_spell_cast_info->missile_nid;
    }

    auto LuaSpellCastInfo::get_start_position( ) const -> sdk::math::Vec3{
        if ( !m_spell_cast_info ) return { };
        return m_spell_cast_info->start_position;
    }

    auto LuaSpellCastInfo::get_end_position( ) const -> sdk::math::Vec3{
        if ( !m_spell_cast_info ) return { };
        return m_spell_cast_info->end_position;
    }

    auto LuaSpellCastInfo::get_windup_time( ) const -> float{
        if ( !m_spell_cast_info ) return { };
        return m_spell_cast_info->windup_time;
    }

    auto LuaSpellCastInfo::get_total_cast_time( ) const -> float{
        if ( !m_spell_cast_info ) return { };
        return m_spell_cast_info->total_cast_time;
    }

    auto LuaSpellCastInfo::is_autoattack( ) const -> bool{
        if ( !m_spell_cast_info ) return { };
        return m_spell_cast_info->is_autoattack;
    }

    auto LuaSpellCastInfo::is_special_attack( ) const -> bool{
        if ( !m_spell_cast_info ) return { };
        return m_spell_cast_info->is_special_attack;
    }

    auto LuaSpellCastInfo::was_autoattack_cast( ) const -> bool{
        if ( !m_spell_cast_info ) return { };
        return m_spell_cast_info->was_autoattack_cast;
    }

    auto LuaSpellCastInfo::was_autoattack_cast2( ) const -> bool{
        if ( !m_spell_cast_info ) return { };
        return m_spell_cast_info->was_autoattack_cast2;
    }

    auto LuaSpellCastInfo::get_slot( ) const -> int32_t{
        if ( !m_spell_cast_info ) return { };
        return m_spell_cast_info->slot;
    }

    auto LuaSpellCastInfo::get_server_cast_time( ) const -> float{
        if ( !m_spell_cast_info ) return { };
        return m_spell_cast_info->server_cast_time;
    }

    auto LuaSpellCastInfo::get_end_time( ) const -> float{
        if ( !m_spell_cast_info ) return { };
        return m_spell_cast_info->end_time;
    }

    auto LuaSpellCastInfo::get_start_time( ) const -> float{
        if ( !m_spell_cast_info ) return { };
        return m_spell_cast_info->start_time;
    }

    auto LuaSpellCastInfo::get_owner_character( ) const -> std::optional< std::string >{
        if ( !m_spell_info ) return std::nullopt;
        return m_spell_info->owner_character;
    }

    auto LuaSpellCastInfo::get_coefficient( ) const -> float{
        if ( !m_spell_data ) return { };
        return m_spell_data->get_coefficient( );
    }

    auto LuaSpellCastInfo::get_coefficient2( ) const -> float{
        if ( !m_spell_data ) return { };
        return m_spell_data->get_coefficient2( );
    }

    auto LuaSpellCastInfo::get_cooldown_time( ) const -> float{
        if ( !m_spell_data ) return { };
        return m_spell_data->get_cooldown_time( );
    }

    auto LuaSpellCastInfo::get_name( ) const -> std::string{
        if ( !m_spell_data ) return { };
        return m_spell_data->get_name( );
    }

    auto LuaSpellCastInfo::get_mana_cost( ) const -> std::vector< float >{
        if ( !m_spell_data ) return { };
        return m_spell_data->get_mana_cost( );
    }

    auto LuaSpellCastInfo::get_missile_speed( ) const -> float{
        if ( !m_spell_data ) return { };
        return m_spell_data->get_missile_speed( );
    }

    auto LuaSpellCastInfo::get_channel_duration( ) const -> float{
        if ( !m_spell_data ) return { };
        return m_spell_data->get_channel_duration( );
    }

    auto LuaSpellCastInfo::get_missile_width( ) const -> float{
        if ( !m_spell_data ) return { };
        return m_spell_data->get_missile_width( );
    }

    LuaSpellSlot::LuaSpellSlot( std::shared_ptr< sdk::game::SpellSlot > slot ){
        m_spell_slot = slot;

        auto info = slot->get_spell_info( );

        if ( !info ) return;

        m_spell_info = info.get( );

        auto data = info->get_spell_data( );

        if ( !data ) return;
        m_spell_data = data.get( );
    }
}
