#pragma once
#include "../build.hpp"

#if enable_lua
#include "../include/sol/forward.hpp"
#endif

namespace features {
    class ITargetSelector;
}

namespace lua {
    class Lua;
}

namespace features {
    class EntityList2;

    namespace champion_modules {
        class IModule;
    }

    class Tracker;
    class SpellDetector;
    class TargetSelector;
    class Visuals;
    class Evade;
    class Prediction;
    class Orbwalker;
    class BuffCache;
    class Activator;

    class IFeature {
    public:
        virtual ~IFeature( ) = default;
        /**
         * \brief Run feature code.
         */
        virtual auto run( ) -> void{
        }

        virtual auto pre_run( ) -> void{
        }

        virtual auto post_run( ) -> void{
        }

        virtual auto get_name( ) noexcept -> hash_t{ return { }; }

        virtual auto on_draw( ) -> void{
        }

        virtual auto should_run_in_feature_loop( ) -> bool{ return true; }

        virtual auto force_run_sync( ) -> bool{ return false; }
#if enable_lua
        virtual auto initialize_lua( sol::state* state ) -> void{
        }

        auto               set_enabled( const bool enabled ) -> void{ m_disabled = enabled; }
        [[nodiscard]] auto is_enabled( ) const -> bool{ return !m_disabled; }

        static auto  lua_pre_run( ) -> void;
        static auto  lua_post_run( ) -> void;
        virtual auto on_lua_reset( ) -> void;

        virtual auto on_lua_load( ) -> void{
        }

        auto push_pre_run_callback( const sol::function& f ) -> void;
        auto push_post_run_callback( const sol::function& f ) -> void;
#endif
        virtual auto initialize_menu( ) -> void{
        }

#if _DEBUG
        virtual auto get_full_name( ) -> std::string{ return { }; }
#endif

    private:
#if enable_lua
        lua::Callback m_lua_pre_run_callbacks{ };
        lua::Callback m_lua_post_run_callbacks{ };
#endif
        // std::mutex m_lua_mutex;
        bool m_disabled{ false };
    };

    struct Features {
        std::vector< std::shared_ptr< IFeature > >                  all_features{ };
        std::vector< std::shared_ptr< IFeature > >                  list{ };
        std::vector< std::shared_ptr< IFeature > >                  pre_list{ };
        std::vector< std::shared_ptr< champion_modules::IModule > > modules{ };

        std::shared_ptr< Orbwalker > orbwalker;
        std::shared_ptr< Evade >     evade;
        std::shared_ptr< Visuals >   visuals;
        std::shared_ptr< Tracker >   tracker;
        std::shared_ptr< Activator > activator;

        EntityList2* entity_list{ };

        std::shared_ptr< Prediction >      prediction;
        std::shared_ptr< ITargetSelector > target_selector;
        std::shared_ptr< ITargetSelector > default_target_selector;
        std::shared_ptr< SpellDetector >   spell_detector;

        std::shared_ptr< champion_modules::IModule > current_module;

        std::shared_ptr< BuffCache > buff_cache;
        // EntityList2*                 entity_list2;
        // std::shared_ptr< EntityList > entity_list;

        auto get_module_for_champion( hash_t name ) -> std::shared_ptr< champion_modules::IModule >;

        template <typename t>
        auto create_feature( ) -> std::shared_ptr< t >{
            auto ptr = std::make_shared< t >( );
            list.push_back( ptr );
            all_features.push_back( ptr );
            return ptr;
        }

        template <typename t>
        auto create_pre_feature( ) -> std::shared_ptr< t >{
            auto ptr = std::make_shared< t >( );
            pre_list.push_back( ptr );
            all_features.push_back( ptr );
            return ptr;
        }

        template <typename t>
        auto get_by_name( hash_t name ) -> t*{
            const auto found =
                std::ranges::find_if(
                    all_features,
                    [name]( const std::shared_ptr< IFeature >& feature ) -> bool{ return feature->get_name( ) == name; }
                );

            if ( found != list.end( ) ) return reinterpret_cast< t* >( ( *found ).get( ) );

            return nullptr;
        }
    };
}

extern std::unique_ptr< features::Features > g_features;
