#pragma once
#include "entity_list.hpp"

#define cached_function( name )         auto name( int32_t index ) -> bool{ \
                                            std::unique_lock lock(m_mutex);\
                                            if ( *g_time - m_cache[ index ].##name.last_update > 0.1f ) { \
                                                m_cache[ index ].##name.value = g_entity_list.get_by_index( index )->$##name( ); \
                                                m_cache[ index ].##name.last_update = *g_time; \
                                            } \
\
                                            return m_cache[ index ].##name.value; \
                                        }


namespace features {
    class EntityCache {
        template <typename T>
        struct CacheEntry {
            T     value{ };
            float last_update{ };
        };

        struct Cache {
            CacheEntry< bool > is_normal_minion{ };
            CacheEntry< bool > is_turret_object{ };
            // CacheEntry< bool > is_normal_minion{ };
        };

    public:
        EntityCache( ){
            m_cache.reserve( 5000 );

            for ( auto i = 0; i < 5000; ++i ) { m_cache.emplace_back( ); }
        }

        cached_function( is_normal_minion )
        cached_function( is_turret_object )

    private:
        std::vector< Cache > m_cache{ };
        std::mutex           m_mutex;
    };
}

extern features::EntityCache g_entity_cache;
