#include "pch.hpp"

#include "buff_cache.hpp"

#include "entity_list.hpp"
#if enable_new_lua
#include "../lua-v2/state.hpp"
#endif
#include "../utils/debug_logger.hpp"
#include "../sdk/game/buff.hpp"
#if enable_new_lua
#include "../lua-v2/custom_structs.hpp"
#endif
#include "../sdk/game/buff_info.hpp"

namespace features {
    auto BuffCache::run( ) -> void{
#if enable_lua
        if ( g_lua2 ) {
            g_lua2->execute_locked( []( ) -> void{ g_lua2->run_callback( ct_hash( "features.buff_cache" ) ); } );
        }
#endif

        if ( m_last_cache_time >= *g_time ) return;
        m_cache.clear( );
        m_last_cache_time = *g_time + 0.0020f;
        // 0.0001f seemed way too low since league will never update buffs that fast,
        // even 0.0020 will make sure buffs get cached every league frame for up to 500 fps
    }

#if enable_lua
    auto BuffCache::lua_get_buff( const sol::object object_index, const sol::object name ) -> sol::object{
        lua_arg_check_ct( object_index, int16_t, "number" )
        lua_arg_check_ct( name, std::string, "string" )

        const auto& buff = get_buff( object_index.as< int16_t >( ), rt_hash( name.as<std::string>( ).data( ) ) );
        if ( !buff ) return sol::nil;


        return sol::make_object(
            g_lua_state2,
            lua::LuaBuff( buff->buff_data.get( ), buff->buff_info.get( ) )
        );
    }
#endif
    auto BuffCache::has_buff_type(
        const int16_t                   object_index,
        const std::vector< EBuffType >& types,
        const float                     min_duration,
        bool                            lua
    ) -> bool{
        if ( !run_thread_check( ) ) return { };

        if ( !is_object_cached( object_index ) ) cache_object( object_index );

        if ( const auto cpy = m_cache.find( object_index ); cpy == m_cache.end( ) ) return false;
        const auto& data = m_cache.at( object_index );

        for ( const auto& buff : data ) {
            if ( buff->buff_data->end_time - *g_time < min_duration ) continue;

            const auto buff_type = static_cast< EBuffType >( buff->buff_data->type );

            for ( const auto type : types ) { if ( buff_type == type ) return true; }
        }

        return false;
    }

#if enable_new_lua
    // ReSharper disable once CppPassValueParameterByConstReference
    auto BuffCache::lua_get_all_buffs( const sol::object object_index ) -> sol::object{
        lua_arg_check_ct( object_index, int16_t, "number" )

        const auto index = object_index.as< int16_t >( );
        if ( index < 0 || index >= 5000 ) return sol::nil;

        const auto                 buffs = get_all_buffs( index );
        std::vector< sol::object > buffs_lua;
        buffs_lua.reserve( buffs.size( ) );
        for ( const auto cache : buffs ) {
            buffs_lua.push_back(
                sol::make_object(
                    g_lua_state2,
                    lua::LuaBuff(
                        cache->buff_data.get( ),
                        cache->buff_info.get( )
                    )
                )
            );
        }

        return sol::make_object( g_lua_state2, sol::as_table( buffs_lua ) );
    }
#endif

    auto BuffCache::cache_object( const Object* object ) -> void{
        if ( !object ) return;

        const auto index = object->index;

        if ( index > 5000 || index < 0 ) return;

        std::unique_lock lock( m_mutex );
        const auto       buffs = object->buff_manager.get_all( );

        for ( const auto& data : buffs ) {
            if ( !data || !data->is_active( ) ) continue;
            const auto info = data->get_buff_info( );
            if ( !info ) continue;

            if ( m_cache.find( index ) == m_cache.end( ) )
                m_cache[ index ] =
                    std::vector< std::unique_ptr< Cache > >( );

            m_cache[ index ].push_back( std::make_unique< Cache >( ) );
            const auto cache = m_cache.at( index ).at( m_cache.at( index ).size( ) - 1 ).get( );
            cache->buff_data = data;
            cache->buff_info = info;
            cache->name      = cache->buff_info->get_name();
            cache->amount    = data->
                stack; //static_cast< int32_t >( ( data->amount_end - data->amount_start ) / sizeof( c_buff ) );
            cache->alt_amount = data->count;
            cache->name_hash  = rt_hash( cache->name.data( ) );
        }
    }

    auto BuffCache::is_object_cached( const Object* object ) -> bool{
        if ( !object ) return false;
        return is_object_cached( object->index );
    }

    auto BuffCache::cache_object( const int16_t object_index ) -> void{
        const auto& holder = g_entity_list.get_by_index( object_index );
        if ( !holder ) return;

        cache_object( holder.get( ) );
    }

    auto BuffCache::is_object_cached( const int16_t object_index ) -> bool{
        std::unique_lock lock( m_mutex );
        return m_cache.contains( object_index );
    }
}
