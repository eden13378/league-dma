#pragma once
#include "feature.hpp"
#include "helper.hpp"
#include "names.hpp"
#if enable_new_lua
#include "../lua-v2/lua_def.hpp"
#endif
#include "../sdk/game/buff_data.hpp"

namespace features {
    class BuffCacheException final : public std::runtime_error {
    public:
        BuffCacheException( ): std::runtime_error{ ( "error in BuffCache" ) }{
        }

        auto what( ) const -> const char* override{ return "error in BuffCache"; }
        ~BuffCacheException( ) override;
    };

    class BuffCache final : public IFeature {
    public:
        struct Cache {
            utils::MemoryHolder< sdk::game::BuffData > buff_data;
            utils::MemoryHolder< sdk::game::BuffInfo > buff_info;
            std::string                                name;
            hash_t                                     name_hash{ };
            int32_t                                    amount{ };
            int32_t                                    alt_amount{ };

            [[nodiscard]] auto stacks( ) const -> int32_t{ return amount > alt_amount ? amount : alt_amount; }
        };

        auto run( ) -> void override;

        auto get_name( ) noexcept -> hash_t override{ return names::buff_cache; }

#if _DEBUG
        auto get_full_name( ) -> std::string override{ return "c_buff_cache"; }
#endif

        auto force_run_sync( ) -> bool override{ return true; }
#if enable_lua
        auto initialize_lua( sol::state* state ) -> void override;
        auto lua_get_buff( const sol::object object_index, const sol::object name ) -> sol::object;
#endif

        auto get_buff( const int16_t object_index, const hash_t name ) -> Cache*{
            // Function is used in LUA, message @tore if you change args

            if ( !run_thread_check( ) ) return { };

            if ( !is_object_cached( object_index ) ) cache_object( object_index );

            if ( const auto cpy = m_cache.find( object_index ); cpy == m_cache.end( ) ) return { };
            const auto& data = m_cache.at( object_index );

            const auto& found = std::ranges::find_if(
                data,
                [name]( const std::unique_ptr< Cache >& cache ) -> bool{ return cache->name_hash == name; }
            );

            if ( found == data.end( ) ) return nullptr;

            return found->get( );
        }

        auto has_buff( const int16_t object_index, const std::vector< hash_t >& buffs ) -> bool{
            if ( !is_object_cached( object_index ) ) cache_object( object_index );

            if ( !run_thread_check( ) ) return { };

            if ( const auto cpy = m_cache.find( object_index ); cpy == m_cache.end( ) ) return false;
            const auto& data = m_cache.at( object_index );

            for ( auto buff : buffs ) {
                const auto found = std::ranges::find_if(
                    data,
                    [&]( const std::unique_ptr< Cache >& cache ) -> bool{ return cache->name_hash == buff; }
                );

                if ( found != data.end( ) ) return true;
            }

            return false;
        }

#if enable_lua
        // ReSharper disable once CppPassValueParameterByConstReference
        auto lua_has_buff_of_type(
            const sol::object                                                                  object_index,
            const sol::object /* ReSharper disable once CppPassValueParameterByConstReference*/type
        ) -> bool{
            lua_arg_check_ct( object_index, int16_t, "number" )
            lua_arg_check_ct( type, int32_t, "number" )

            return has_buff_of_type(
                object_index.as< int16_t >( ),
                static_cast< EBuffType >( type.as< int32_t >( ) )
            );
        }
#endif

        auto has_buff_of_type( const int16_t object_index, const EBuffType type ) -> bool{
            // Function is used in LUA, message @tore if you change args

            if ( !run_thread_check( ) ) return { };

            if ( !is_object_cached( object_index ) ) cache_object( object_index );

            if ( const auto cpy = m_cache.find( object_index ); cpy == m_cache.end( ) ) return false;
            const auto& data = m_cache.at( object_index );

            const auto found = std::ranges::find_if(
                data,
                [type]( const std::unique_ptr< Cache >& cache ) -> bool{
                    return static_cast< EBuffType >( cache->buff_data->type ) == type;
                }
            );

            return found != data.end( );
        }

#if enable_lua
        // ReSharper disable once CppPassValueParameterByConstReference
        auto lua_has_hard_cc( const sol::object object_index ) -> bool{
            lua_arg_check_ct( object_index, int16_t, "number" );
            return has_hard_cc( object_index.as< int16_t >( ) );
        }
#endif

        auto has_hard_cc( const int16_t object_index ) -> bool{
            // Function is used in LUA, message @tore if you change args

            if ( !run_thread_check( ) ) return { };

            if ( !is_object_cached( object_index ) ) cache_object( object_index );

            if ( const auto cpy = m_cache.find( object_index ); cpy == m_cache.end( ) ) return false;
            const auto& data = m_cache.at( object_index );

            const auto found = std::ranges::find_if(
                data,
                []( const std::unique_ptr< Cache >& cache ) -> bool{
                    return static_cast< EBuffType >( cache->buff_data->type ) == EBuffType::stun ||
                        static_cast< EBuffType >( cache->buff_data->type ) == EBuffType::charm ||
                        static_cast< EBuffType >( cache->buff_data->type ) == EBuffType::fear ||
                        static_cast< EBuffType >( cache->buff_data->type ) == EBuffType::knockup ||
                        static_cast< EBuffType >( cache->buff_data->type ) == EBuffType::knockback ||
                        static_cast< EBuffType >( cache->buff_data->type ) == EBuffType::asleep ||
                        static_cast< EBuffType >( cache->buff_data->type ) == EBuffType::suppression ||
                        static_cast< EBuffType >( cache->buff_data->type ) == EBuffType::taunt;
                }
            );

            return found != data.end( );
        }

#if enable_lua
        // ReSharper disable once CppPassValueParameterByConstReference
        auto lua_is_immobile( const sol::object object_index ) -> bool{
            lua_arg_check_ct( object_index, int16_t, "number" );

            return is_immobile( object_index.as< int16_t >( ) );
        }
#endif

        auto is_immobile( const int16_t object_index ) -> bool{
            // Function is used in LUA, message @tore if you change args

            if ( !run_thread_check( ) ) return { };

            if ( !is_object_cached( object_index ) ) cache_object( object_index );

            if ( const auto cpy = m_cache.find( object_index ); cpy == m_cache.end( ) ) return false;
            const auto& data = m_cache.at( object_index );

            const auto found = std::ranges::find_if(
                data,
                []( const std::unique_ptr< Cache >& cache ) -> bool{
                    return static_cast< EBuffType >( cache->buff_data->type ) == EBuffType::stun ||
                        static_cast< EBuffType >( cache->buff_data->type ) == EBuffType::charm ||
                        static_cast< EBuffType >( cache->buff_data->type ) == EBuffType::fear ||
                        static_cast< EBuffType >( cache->buff_data->type ) == EBuffType::knockup ||
                        static_cast< EBuffType >( cache->buff_data->type ) == EBuffType::knockback ||
                        static_cast< EBuffType >( cache->buff_data->type ) == EBuffType::asleep ||
                        static_cast< EBuffType >( cache->buff_data->type ) == EBuffType::snare ||
                        static_cast< EBuffType >( cache->buff_data->type ) == EBuffType::suppression ||
                        static_cast< EBuffType >( cache->buff_data->type ) == EBuffType::taunt;
                }
            );

            return found != data.end( );
        }

#if enable_lua
        // ReSharper disable once CppPassValueParameterByConstReference
        auto lua_can_cast( const sol::object object_index ) -> bool{
            lua_arg_check_ct( object_index, int16_t, "number" );

            return can_cast( object_index.as< int16_t >( ) );
        }
#endif

        auto can_cast( const int16_t object_index ) -> bool{
            // Function is used in LUA, message @tore if you change args

            if ( !run_thread_check( ) ) return { };

            if ( !is_object_cached( object_index ) ) cache_object( object_index );

            if ( const auto cpy = m_cache.find( object_index ); cpy == m_cache.end( ) ) return false;

            const auto& data  = m_cache.at( object_index );
            const auto  found = std::ranges::find_if(
                data,
                []( const std::unique_ptr< Cache >& cache ) -> bool{
                    return static_cast< EBuffType >( cache->buff_data->type ) == EBuffType::stun ||
                        static_cast< EBuffType >( cache->buff_data->type ) == EBuffType::charm ||
                        static_cast< EBuffType >( cache->buff_data->type ) == EBuffType::fear ||
                        static_cast< EBuffType >( cache->buff_data->type ) == EBuffType::knockup ||
                        static_cast< EBuffType >( cache->buff_data->type ) == EBuffType::knockback ||
                        static_cast< EBuffType >( cache->buff_data->type ) == EBuffType::asleep ||
                        static_cast< EBuffType >( cache->buff_data->type ) == EBuffType::suppression ||
                        static_cast< EBuffType >( cache->buff_data->type ) == EBuffType::taunt ||
                        static_cast< EBuffType >( cache->buff_data->type ) == EBuffType::disarm ||
                        static_cast< EBuffType >( cache->buff_data->type ) == EBuffType::berserk ||
                        static_cast< EBuffType >( cache->buff_data->type ) == EBuffType::polymorph ||
                        static_cast< EBuffType >( cache->buff_data->type ) == EBuffType::silence;
                }
            );

            return found == data.end( );
        }

#if enable_lua
        auto lua_has_buff_type(
            const int16_t                   object_index,
            const std::vector< EBuffType >& types,
            const float                     min_duration = 0.f
        ) -> bool{ return has_buff_type( object_index, types, min_duration, true ); }
#endif

        auto has_buff_type(
            const int16_t                   object_index,
            const std::vector< EBuffType >& types,
            const float                     min_duration = 0.f,
            bool                            lua          = false
        ) -> bool;

#if enable_lua
        auto lua_get_all_buffs( sol::object object_index ) -> sol::object;
#endif

        auto get_all_buffs( const int16_t object_index ) -> std::vector< Cache* >{
            // Function is used in LUA, message @tore if you change args
            if ( !run_thread_check( ) ) return { };

            if ( !is_object_cached( object_index ) ) cache_object( object_index );

            if ( const auto cpy = m_cache.find( object_index ); cpy == m_cache.end( ) ) return { };
            std::vector< Cache* > v;
            for ( auto& value : m_cache.at( object_index ) ) v.push_back( value.get( ) );

            return v;
        }

    private:
        auto cache_object( const sdk::game::Object* object ) -> void;

        auto is_object_cached( int16_t object_index ) -> bool;
        auto is_object_cached( const sdk::game::Object* object ) -> bool;

        auto cache_object( int16_t object_index ) -> void;

    public:
        static auto run_thread_check( ) -> bool{
#if __DEBUG
            if ( g_config->misc.use_multi_core_runtime->get< bool >( ) ) {
                if ( !g_threading->is_feature_thread( ) ) {
                    app->logger->error( "don't call BuffCache from render thread" );
                    throw CalledFromWrongThread( );
                }
            }

            return true;
#endif
#if !__DEBUG
            return true;
#endif
        }

    private:
        std::unordered_map< int16_t, std::vector< std::unique_ptr< Cache > > > m_cache;
        std::mutex                                                             m_mutex;
        float                                                                  m_last_cache_time{ };
    };
}
