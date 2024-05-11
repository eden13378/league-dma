#include "pch.hpp"

#include "target_selector.hpp"

#if enable_lua

#include "lua_target_selector.hpp"

namespace features {
    auto LuaTargetSelector::run( ) -> void{
        // const auto target_selector = g_lua2->get_custom_target_selector( );
        //
        // if ( !target_selector ) return;
        //
        // const auto result = target_selector->run( );
        //
        // if ( !result.valid( ) ) {
        //     const sol::error err = result;
        //     lua_logger->error( "{}", err.what( ) );
        // }
    }

    auto LuaTargetSelector::get_name( ) noexcept -> hash_t{ return ct_hash( "LuaTargetSelector" ); }

    auto LuaTargetSelector::get_default_target( bool secondary ) -> Object*{
        return nullptr;
        // const auto target_selector = g_lua->get_custom_target_selector( );
        // if ( !target_selector ) return g_features->default_target_selector->get_default_target( );
        //
        // const auto result = target_selector->get_default_target( secondary );
        //
        // if ( !result.valid( ) ) {
        //     const sol::error err = result;
        //     lua_logger->error( "{}", err.what( ) );
        //     return nullptr;
        // }
        //
        // if ( result.get_type( ) == sol::type::nil ) return nullptr;
        // const sol::object object = result;
        //
        // if ( !object.is< lua::structs::LuaObject >( ) ) return nullptr;
        //
        // const auto holder = object.as< lua::structs::LuaObject >( ).get_holder( );
        // if ( !holder ) return nullptr;
        //
        // return holder->get( );
    }

    auto LuaTargetSelector::get_orbwalker_default_target( bool secondary ) -> Object*{
        return nullptr;
        // const auto target_selector = g_lua->get_custom_target_selector( );
        // if ( !target_selector ) return g_features->default_target_selector->get_orbwalker_default_target( );
        //
        // const auto result = target_selector->get_orbwalker_default_target( secondary );
        //
        // if ( !result.valid( ) ) {
        //     const sol::error err = result;
        //     lua_logger->error( "{}", err.what( ) );
        //     return nullptr;
        // }
        //
        // if ( result.get_type( ) == sol::type::nil ) return nullptr;
        // const sol::object object = result;
        //
        // if ( !object.is< lua::structs::LuaObject >( ) ) return nullptr;
        //
        // const auto holder = object.as< lua::structs::LuaObject >( ).get_holder( );
        // if ( !holder ) return nullptr;
        //
        // return holder->get( );
    }

    auto LuaTargetSelector::get_forced_target( ) -> Object*{
        return nullptr;
        // const auto target_selector = g_lua->get_custom_target_selector( );
        // if ( !target_selector ) return g_features->default_target_selector->get_forced_target( );
        //
        // const auto result = target_selector->get_forced_target( );
        //
        // if ( !result.valid( ) ) {
        //     const sol::error err = result;
        //     lua_logger->error( "{}", err.what( ) );
        //     return nullptr;
        // }
        //
        // if ( result.get_type( ) == sol::type::nil ) return nullptr;
        // const sol::object object = result;
        //
        // if ( !object.is< lua::structs::LuaObject >( ) ) return nullptr;
        //
        // const auto holder = object.as< lua::structs::LuaObject >( ).get_holder( );
        // if ( !holder ) return nullptr;
        //
        // return holder->get( );
    }

    auto LuaTargetSelector::set_forced_target( int16_t index ) -> bool{
        return { };
        // const auto target_selector = g_lua->get_custom_target_selector( );
        // if ( !target_selector ) return false;
        //
        // const auto result = target_selector->set_forced_target( index );
        //
        // if ( !result.valid( ) ) {
        //     const sol::error err = result;
        //     lua_logger->error( "{}", err.what( ) );
        //     return false;
        // }
        //
        // if ( result.get_type( ) != sol::type::boolean ) return false;
        //
        // const sol::object object = result;
        //
        // return object.as< bool >( );
    }

    auto LuaTargetSelector::set_forced_target( Object* object ) -> bool{
        if ( !object ) return set_forced_target( -1 );
        return set_forced_target( object->index );
    }

    auto LuaTargetSelector::set_forced_target( CHolder* holder ) -> bool{
        if ( !holder ) return set_forced_target( -1 );
        return set_forced_target( holder->get( ) );
    }

    auto LuaTargetSelector::reset_forced_target( ) -> void{
        // const auto target_selector = g_lua->get_custom_target_selector( );
        // if ( !target_selector ) return;
        //
        // const auto result = target_selector->reset_forced_target( );
        //
        // if ( !result.valid( ) ) {
        //     const sol::error err = result;
        //     lua_logger->error( "{}", err.what( ) );
        //     return;
        // }
    }

    auto LuaTargetSelector::is_forced( ) const -> bool{
        // const auto target_selector = g_lua->get_custom_target_selector( );
        // if ( !target_selector ) return false;
        //
        // const auto result = target_selector->is_forced( );
        //
        // if ( !result.valid( ) ) {
        //     const sol::error err = result;
        //     lua_logger->error( "{}", err.what( ) );
        //     return false;
        // }
        //
        // if ( result.get_type( ) != sol::type::boolean ) return false;
        // const sol::object object = result;
        //
        // return object.as< bool >( );
        return { };
    }

    auto LuaTargetSelector::is_bad_target(
        const int16_t index,
        const bool    ignore_dead,
        const bool    ignore_invisible
    ) -> bool{ return g_features->default_target_selector->is_bad_target( index, ignore_dead, ignore_invisible ); }

    auto LuaTargetSelector::get_spell_specific_target(
        const float                                  range,
        const std::function< float( Object* unit ) > get_travel_time,
        const std::function< float( Object* unit ) > get_damage,
        const int                                    damage_type
    ) -> Object*{
        return g_features->default_target_selector->get_spell_specific_target(
            range,
            get_travel_time,
            get_damage,
            damage_type
        );
    }

    auto LuaTargetSelector::get_killsteal_target(
        const float                                  range,
        const std::function< float( Object* unit ) > get_travel_time,
        const std::function< float( Object* unit ) > get_damage,
        const int                                    damage_type,
        const Vec3                                   source_position
    ) -> Object*{
        return g_features->default_target_selector->get_killsteal_target(
            range,
            get_travel_time,
            get_damage,
            damage_type,
            source_position
        );
    }

    auto LuaTargetSelector::get_target_priority( std::string champion_name ) -> int32_t{
        // const auto target_selector = g_lua->get_custom_target_selector( );
        // if ( !target_selector ) return g_features->default_target_selector->get_target_priority( champion_name );
        //
        // const auto result = target_selector->get_target_priority( champion_name );
        //
        // if ( !result.valid( ) ) {
        //     const sol::error err = result;
        //     lua_logger->error( "{}", err.what( ) );
        //     return -1;
        // }
        //
        // if ( result.get_type( ) == sol::type::nil ) return -1;
        // const sol::object object = result;
        //
        // if ( !object.is< int32_t >( ) ) return -1;
        //
        // return object.as< int32_t >( );
        return { };
    }
}
#endif
