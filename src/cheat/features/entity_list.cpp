#include "pch.hpp"

#include  "entity_list.hpp"

#include "../lua-v2/state.hpp"

features::EntityList2 g_entity_list = features::EntityList2( );

namespace features {
    auto EntityList2::run( ) -> void{
    }

    auto EntityList2::start( ) -> void{
        debug_log( "starting entity list" );
        // startup entity list update thread

        std::thread(
            [this]( ) -> void{
                while ( app && app->should_run( ) ) {
                    if ( !g_time.force_update( ) ) {
                        std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
                        continue;
                    }
                    static auto last_time = 0.f;

                    if ( !g_config->misc.core_entity_list_high_performance_mode->get< bool >( ) &&
                        last_time == *g_time
                    ) {
                        std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
                        continue;
                    }
                    last_time = *g_time;

                    utils::Timer timer;
                    try { update( ); } catch ( std::exception& ex ) {
                        debug_log( "EntityList::update error {}", ex.what( ) );
                    }
                    catch ( ... ) { debug_log( "EntityList::update error unknown error" ); }
#if debug_overlay
                    g_debug_overlay.post_chart_time(
                        _( "EntityList2" ),
                        timer.get_ms_since_start( ).count( ) / 1000000.f
                    );
#endif
                }

                m_running = false;
            }
        ).detach( );
    }

#if enable_lua
    auto EntityList2::on_lua_reset( ) -> void{ throw std::logic_error( "Not implemented" ); }
    auto EntityList2::on_lua_load( ) -> void{ throw std::logic_error( "Not implemented" ); }

    auto EntityList2::get_in_range( const float range ) -> std::vector< Object* >{
        std::vector< Object* > out;
        for ( auto& object : m_objects ) {
            if ( !object.object ) continue;

            if ( object.object->dist_to_local( ) <= range + 300.f ) {
                object.object.update( );
                if ( object.object->dist_to_local( ) <= range ) out.push_back( object.object.get( ) );
            }
        }

        return out;
    }

    auto EntityList2::get_all_table( ) const -> sol::object{
        std::vector< Object* > objs;

        objs.reserve( m_objects.size( ) );

        for ( auto& o : m_objects ) {
            // const auto obj = o.object.get(  );
            objs.push_back( o.object.get( ) );
        }

        return sol::make_object( g_lua_state2, sol::as_table( objs ) );
    }

#endif


    auto EntityList2::update( ) -> void{
        g_objects.force_update( );

        const auto array_size = get_objects_array_size( );


        m_objects_array.resize( std::clamp( array_size, 1000, 5000 ) );

        app->memory->read_amount(
            reinterpret_cast< intptr_t >( g_objects->object_array ),
            m_objects_array.data( ),
            m_objects_array.size( )
        );

        const auto amount_threads = g_config->misc.core_update_threads_->get< int32_t >( );

        if ( !m_objects.data( ) ) return;

        const auto read_object = [&, amount_threads]( const int32_t index ) -> void{
            for ( auto i = static_cast< size_t >( index ); i < m_objects_array.size( ); i += amount_threads ) {
                if ( i >= m_objects.size( ) || i >= m_objects_array.size( ) ) continue;
                try { update_object( static_cast< int32_t >( i ), index ); } catch ( ... ) {
                }
            }
        };

        // if ( g_config->misc.core_fast_update_threads ) {
        //     static auto activeWorkers = 0;
        //     if ( activeWorkers != amount_threads ) {
        //         m_workers.clear( );
        //
        //         for ( auto i = 0; i < amount_threads; ++i ) m_workers.emplace_back( read_object, i );
        //
        //         activeWorkers = amount_threads;
        //     }
        //
        //     for ( auto& worker : m_workers ) worker.work( );
        //     for ( auto& worker : m_workers ) worker.finish( );
        // } else {
        std::vector< std::thread > threads;

        for ( auto i = 0; i < amount_threads; ++i ) threads.emplace_back( read_object, i );
        for ( auto& thread : threads ) thread.join( );
        // }


        std::array<
            std::vector< Object* >,
            static_cast< int32_t >( EObjectCategory::max )
        > new_categorized_objects{ };

        for ( auto i = 0; i < static_cast< int32_t >( EObjectCategory::max ); ++i ) {
            new_categorized_objects[ i ].reserve(
                m_categorized_objects[ i ].size( ) + 10
            );
        }

        for ( auto& object : m_objects ) {
            if ( !object.object ) continue;

            if ( object.should_invalidate ) {
                object.object.invalidate( );
                object.should_invalidate = false;
                continue;
            }

            const auto category_flag = get_category_for_object( object );

            if ( category_flag != EObjectCategory::error ) {
                new_categorized_objects[ static_cast< size_t >( category_flag ) ].push_back( object.object.get( ) );
                object.last_category = category_flag;
            }
        }

        m_mutex.lock( );
        m_categorized_objects = new_categorized_objects;
        m_mutex.unlock( );
    }

    auto EntityList2::invalidate_object( const int32_t index ) -> void{
        if ( index <= 0 || !this || index >= static_cast< i32 >( m_objects.size( ) ) ) return;
        auto& object = m_objects[ index ];
        if ( object.last_category ) object.remove_from_category = object.last_category;
        object.should_invalidate = true;
    }

    auto EntityList2::update_object( const int32_t index, const int32_t thread_index ) -> void{
        if ( !this ) {
            debug_log( "entity list this = nullptr" );
            return;
        }
        // if index is equal or smaller than 0 skip
        if ( index <= 0 || index >= static_cast< i32 >( m_objects_array.size( ) ) ) return;

        const auto object_address = m_objects_array.at( index );

        if ( !object_address || index >= static_cast< int32_t >( m_objects.size( ) ) ) return;
        if ( object_address < 500 ) return invalidate_object( index );

        auto& object = m_objects[ index ];

        // Object tmp_object{ };
        m_thread_cache[ thread_index ] = { };

        // debug_log( "object_address: {:x}", object_address );

        if ( !read_object( object_address, &m_thread_cache[ thread_index ] ) ) return invalidate_object( index );

        // debug_log( "index_read {} index expected: {}", m_thread_cache[ thread_index ].index, index );

        if ( m_thread_cache[ thread_index ].index != index /*||
            m_thread_cache[ thread_index ].next_object_index <= 0*/
        )
            return invalidate_object( index );

        object.flags = m_thread_cache[ thread_index ].get_raw_flags( );

        std::memcpy( object.object.get_unchecked( ), &m_thread_cache[ thread_index ], sizeof( Object ) );
        object.object.set( object_address );
        object.object.set_index( index );
        if ( object.object ) {
            // If object is hero get_bounding_radius is faster than caching
            if ( object.object->is_hero( ) ) object.object->get_bounding_radius( );
            else {
                // Caching here saves about 20% cpu time of whole slotted since get_bounding_radius very expensive for non heroes
                const auto namesHash = rt_hash(
                    object.object->get_name( )
                    .append( object.object->name.text )
                    .c_str( )
                );

                if ( m_boundingRadiusCache[ namesHash ] != 0 )
                    object.bounding_radius = m_boundingRadiusCache[
                        namesHash ];
                else {
                    object.bounding_radius             = object.object->get_bounding_radius( );
                    m_boundingRadiusCache[ namesHash ] = object.bounding_radius;
                }
            }
        }
    }

    auto EntityList2::read_object( const intptr_t address, Object* object ) -> bool{
        if ( !address ) return false;

        try {
            if ( !app->memory->read( address, object ) ) {
                if ( !app->memory->read_amount< int32_t >(
                    address,
                    reinterpret_cast< int* >( object ),
                    offsetof( Object, health )
                ) )
                    return false;
            }

            return true;
        } catch ( ... ) { return false; }
    }

    auto EntityList2::get_category_for_object( ObjectT& object ) -> EObjectCategory{
        if ( !object.flags ) return EObjectCategory::error;
        const auto object_flag = object.flags->get( );

        const auto has_flag = [&]( Object::EObjectTypeFlags flag ) -> bool{
            return ( object_flag & static_cast< unsigned long long >( flag ) ) != 0;
        };

        if ( object.object->is_ally( ) ) {
            if ( has_flag( Object::EObjectTypeFlags::hero ) ) return EObjectCategory::ally_hero;
            if ( has_flag( Object::EObjectTypeFlags::building ) ||
                has_flag( Object::EObjectTypeFlags::turret )
            )
                return EObjectCategory::ally_turret;
            if ( has_flag( Object::EObjectTypeFlags::minion ) ) return EObjectCategory::ally_minion;
            if ( has_flag( Object::EObjectTypeFlags::missile ) ) return EObjectCategory::ally_missile;

            return EObjectCategory::enemy_uncategorized;
        }

        if ( has_flag( Object::EObjectTypeFlags::hero ) ) {
            // debug_log( "enemy with address: {:x}", object.object.get_address( ) );
            return EObjectCategory::enemy_hero;
        }
        if ( has_flag( Object::EObjectTypeFlags::building ) ||
            has_flag( Object::EObjectTypeFlags::turret ) )
            return EObjectCategory::enemy_turret;
        if ( has_flag( Object::EObjectTypeFlags::minion ) ) return EObjectCategory::enemy_minion;
        if ( has_flag( Object::EObjectTypeFlags::missile ) ) return EObjectCategory::enemy_missile;

        return EObjectCategory::enemy_uncategorized;
    }

    auto EntityList2::get_objects_array_size( ) -> int32_t{
        return ( g_objects->last_max_max_offset - reinterpret_cast< intptr_t >( g_objects->object_array ) ) /
            sizeof( intptr_t );
    }
}
