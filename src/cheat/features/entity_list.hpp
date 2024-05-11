#pragma once

#include "feature.hpp"

#define entity_list_getter_new(name, type)         auto get_##name##( ) -> std::vector< Object* >{ \
                                                       std::unique_lock lock(m_mutex); \
                                                       auto v = std::vector(m_categorized_objects[ static_cast< int >( EObjectCategory::##type## ) ]); \
                                                       return v; \
                                                   }

#define entity_list_getter_lua_new(name, type) auto get_##name##_lua( ) const -> sol::object { \
    auto t_list = m_categorized_objects[ static_cast< int >( EObjectCategory::##type## ) ]; \
    std::vector< sol::user< Object* >> objs; objs.reserve(t_list.size()); for ( auto o : t_list ) { objs.push_back( sol::make_user( o ) ); } \
    return sol::make_object(g_lua_state2, sol::as_table(t_list));  \
}
//
// uto& t_list = m_categorized_objects[ static_cast< int >( EObjectCategory::enemy_hero ) ];
// std::vector< sol::user< Object* > > objs;
// objs.reserve( t_list.size( ) );
// for ( auto o : t_list ) { objs.push_back( sol::make_user( o ) ); }
// return sol::as_table( objs );

namespace features {
    struct WorkerThread {
        std::function< void( int ) > f;
        int                          arg;

        WorkerThread( ) :
            arg( 0 ){
        }

        explicit WorkerThread( WorkerThread&& x ) noexcept{
            f   = x.f;
            arg = x.arg;
        }

        WorkerThread( std::function< void( int ) > fnc, const int a ) : f( std::move( fnc ) ), arg( a ){
        }

        ~WorkerThread( ){
            std::cout << "worker thread destroyed" << std::endl;

            stop_thread( );
        }

        inline auto stop_thread( ) -> void{
            exiting.store( true );
            work( );
            finish( );
            std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
            has_work.store( false );
            if ( m_thread.joinable( ) ) m_thread.join( );
        }

        inline auto work( ) -> void{ has_work.store( true ); }

        inline auto finish( ) const -> void{
            while ( has_work.load( ) ) {
            }
        }

    private:
        std::atomic< bool > has_work{ false };

        std::atomic< bool > exiting{ false };
        std::atomic< bool > thread_started{ false };

        std::thread m_thread = std::thread(
            [ this ]{
                thread_started.store( true );
                while ( app->should_run( ) ) {
                    while ( !has_work.load( ) ) {
                        if ( exiting.load( ) ) return;
                        std::this_thread::yield( );
                    }

                    if ( f ) f( arg );

                    has_work.store( false );
                }
            }
        );
    };

    constexpr auto entity_list_max_objects = 5000;

    class EntityList2 final {
        enum class EObjectCategory {
            ally_hero,
            ally_turret,
            ally_minion,
            ally_missile,
            ally_uncategorized,
            enemy_hero,
            enemy_turret,
            enemy_minion,
            enemy_missile,
            enemy_uncategorized,
            error,
            max
        };

        struct ObjectT {
            CHolder object{ };

            std::optional< EObjectCategory > last_category{ };
            std::optional< EObjectCategory > remove_from_category{ };
            bool                             should_invalidate{ };
            std::optional< ObfuscatedFlag >  flags{ };
            float                            bounding_radius{ };
        };

    public:
        EntityList2( ){
            m_objects.reserve( entity_list_max_objects );
            for ( auto i = 0; i < entity_list_max_objects; ++i ) {
                m_objects.emplace_back( );
                m_objects.back( ).object.set_index( i );
            }
        }

        ~EntityList2( ){
            debug_log( "entity list is being destroyed" );

            while ( m_running ) std::this_thread::sleep_for( std::chrono::milliseconds( 15 ) );

            if ( app )
                debug_log( "should_run: {} feature tick: {}", app->should_run( ), app->feature_ticks );
        }

        static auto run( ) -> void;
        auto        start( ) -> void;

#if enable_lua
        static auto initialize_lua( sol::state* state ) -> void;
        static auto on_lua_reset( ) -> void;
        static auto on_lua_load( ) -> void;
#endif

    public:
        auto get_by_network_id( const uint32_t network_id ) -> CHolder&{
            for ( auto& object : m_objects ) {
                if ( !object.object ) continue;
                if ( object.object->network_id == network_id ) return object.object;
            }

            return m_invalid_object;
        }

        [[nodiscard]] auto get_by_network_id_raw( const uint32_t network_id ) const -> Object*{
            for ( auto& object : m_objects ) {
                if ( !object.object ) { continue; }
                if ( object.object->network_id == network_id ) return object.object.get( );
            }

            return nullptr;
        }

        auto get_by_index( const int32_t index ) -> CHolder&{
            if ( !is_index_valid( index ) ) return m_invalid_object;
            return m_objects[ index ].object;
        }

        [[nodiscard]] auto get_by_index_raw( const int32_t index ) const -> Object*{
            if ( !is_index_valid( index ) ) return nullptr;

            auto& obj = m_objects[ index ].object;
            if ( !obj ) return nullptr;

            return obj.get( );
        }

        [[nodiscard]] auto get_bounding_radius( const int32_t index ) const -> float{
            if ( !is_index_valid( index ) ) return -1.f;
            return m_objects[ index ].bounding_radius;
        }

        entity_list_getter_new( enemies, enemy_hero )
        entity_list_getter_new( allies, ally_hero )
        entity_list_getter_new( enemy_turrets, enemy_turret )
        entity_list_getter_new( ally_turrets, ally_turret )
        entity_list_getter_new( ally_minions, ally_minion )
        entity_list_getter_new( enemy_minions, enemy_minion )
        entity_list_getter_new( ally_uncategorized, ally_uncategorized )
        entity_list_getter_new( enemy_uncategorized, enemy_uncategorized )
        entity_list_getter_new( ally_missiles, ally_missile )
        entity_list_getter_new( enemy_missiles, enemy_missile )

        auto get_in_range( float range ) -> std::vector< Object* >;
#if enable_new_lua
        auto get_in_range_table( const float range ) -> sol::as_table_t< std::vector< Object* > >{
            return sol::as_table( get_in_range( range ) );
        }
#endif

#if enable_lua
        auto get_enemies2_lua( ) const -> sol::as_table_t< std::vector< sol::user< Object* > > >{
            auto& t_list = m_categorized_objects[ static_cast< int >( EObjectCategory::enemy_hero ) ];
            std::vector< sol::user< Object* > > objs;
            objs.reserve( t_list.size( ) );
            for ( auto o : t_list ) { objs.push_back( sol::make_user( o ) ); }
            return sol::as_table( objs );
        }

        entity_list_getter_lua_new( enemies, enemy_hero )
        entity_list_getter_lua_new( allies, ally_hero )
        entity_list_getter_lua_new( enemy_turrets, enemy_turret )
        entity_list_getter_lua_new( ally_turrets, ally_turret )
        entity_list_getter_lua_new( ally_minions, ally_minion )
        entity_list_getter_lua_new( enemy_minions, enemy_minion )
        entity_list_getter_lua_new( ally_uncategorized, ally_uncategorized )
        entity_list_getter_lua_new( enemy_uncategorized, enemy_uncategorized )
        entity_list_getter_lua_new( ally_missiles, ally_missile )
        entity_list_getter_lua_new( enemy_missiles, enemy_missile )

        auto get_all_table( ) const -> sol::object;
#endif

    private:
        static auto is_index_valid( const int32_t index ) -> bool{ return index >= 0 && index < 5000; }

        // ReSharper disable once CppMemberFunctionMayBeStatic
        auto is_index_valid_raw( const int32_t index ) -> bool{ return is_index_valid( index ); }

        auto update( ) -> void;

        auto        update_object( int32_t index, int32_t thread_index ) -> void;
        static auto read_object( intptr_t address, Object* object ) -> bool;

        static auto get_category_for_object( ObjectT& object ) -> EObjectCategory;

        auto invalidate_object( int32_t index ) -> void;

        static auto get_objects_array_size( ) -> int32_t;

    private:
        std::vector< ObjectT > m_objects{ };
        std::array<
            std::vector< Object* >,
            static_cast< int32_t >( EObjectCategory::max )
        >                           m_categorized_objects{ };
        utils::Dynamic< intptr_t >  m_objects_array{ };
        CHolder                     m_invalid_object{ };
        std::map< unsigned, float > m_boundingRadiusCache;
        std::mutex                  m_mutex;
        std::array< Object, 8 >     m_thread_cache{ };
        bool                        m_running{ true };
        std::vector< WorkerThread > m_workers{ };
    };
}

extern features::EntityList2 g_entity_list;
