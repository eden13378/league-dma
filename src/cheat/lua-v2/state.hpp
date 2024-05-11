#pragma once

#include "../build.hpp"

#if enable_new_lua

#include <sol/forward.hpp>
#include <sol/state.hpp>
#include <mutex>

namespace lua {
    auto string_replace( std::string& str, const std::string& from, const std::string& to ) -> void;

    struct LuaScript {
        explicit LuaScript( std::filesystem::path path );

        std::string name{ };
        std::string path{ };
        bool        is_loaded{ };
        bool        is_encrypted{ };
    };

    struct LuaScripts {
        auto did_change( ) const -> bool{ return m_did_change; }

        auto push_script( const LuaScript& script ) -> void{
            scripts.push_back( script );
            m_did_change = true;
        }

        // auto get_scripts( ) -> std::vector< LuaScript >&{ return m_scripts; }

        std::vector< LuaScript > scripts;

    private:
        bool m_did_change{ };
    };

    class LuaState {
    public:
        struct ChampionModule {
            sol::function spell_q{ };
            sol::function spell_w{ };
            sol::function spell_e{ };
            sol::function spell_r{ };
            sol::function on_draw{ };
            sol::function initialize{ };
            hash_t        champion_name{ };
            sol::function get_priorities{ };

            [[nodiscard]] auto is_valid( ) const -> bool{
                return champion_name != 0 && get_priorities && ( spell_q || spell_w || spell_e || spell_r );
            }
        };

    public:
        LuaState( );
        ~LuaState( );

        auto try_run_script( const std::string& code ) -> bool;

        auto run_script( const std::string& code ) -> void;

        auto try_run_file( const std::string& code ) -> bool;

        auto run_file( const std::string& path ) -> void;

        auto execute_locked( const std::function< void( ) >& function ) -> std::expected< void, const char* >;

        enum class ERunLuaCallbackError {
            invalid_lua_state,
            lua_state_dont_match
        };

        /**
         * \brief Call LUA callback by name
         * \param name callback name
         * \return result
         */
        auto run_callback( hash_t name ) -> std::expected< void, ERunLuaCallbackError >;

        /**
         * \brief Call LUA callback by name
         * \param name callback name
         * \param object object to pass to callback
         * \return result
         */
        auto run_callback( hash_t name, sol::object object ) -> std::expected< void, ERunLuaCallbackError >;

        enum class ERegisterCallbackError { callback_invalid };
        
        auto register_callback( hash_t name, sol::function callback ) -> std::expected<void, ERegisterCallbackError>;

        auto setup_globals( ) noexcept -> void;

        [[nodiscard]] auto get_champion_modules( ) -> std::vector< std::shared_ptr< ChampionModule > >{
            // std::lock_guard lock( _lock );
            return m_champion_modules;
        }


        /**
         * \brief Run lua garbage collector
         */
        auto collect_garbage( ) -> void;

        auto run_function( sol::function fn, sol::object arg = sol::nil ) -> sol::object;
        auto get_state( ) -> sol::state&{ return m_state; }

        static auto decrypt_lua( std::string input ) -> std::string;
        static auto encrypt_lua( std::string input ) -> std::string;

        auto should_reset( ) const -> bool{ return m_should_reset; }
        auto set_reload_lua( ) -> void{ m_should_reset = true; }

        static auto load_available_scripts( ) -> void;

        auto get_memory_usage( ) -> size_t{
            std::lock_guard lock( m_mutex );
            return m_state.memory_used( );
        }

    private:
        static auto open_state( ) -> sol::state;

        static auto register_math_types( sol::state& state ) -> void;
        static auto register_sdk_types( sol::state& state ) -> void;
        static auto register_feature_types( sol::state& state ) -> void;
        static auto register_global_tables( sol::state& state ) -> void;
        static auto register_enums( sol::state& state ) -> void;
        static auto register_menu( sol::state& state ) -> void;
        static auto register_config( sol::state& state ) -> void;
        static auto register_lua_types( sol::state& state ) -> void;
        static auto register_lua_only_types( sol::state& state ) -> void;
        static auto register_debug_overlay( sol::state& state ) -> void;

        auto push_champion_module( const std::shared_ptr< ChampionModule > module ) -> void{
            m_champion_modules.push_back( module );
        }

        auto run( sol::load_result& script ) -> void;

        static auto handle_error( const sol::error& ex ) -> void;
        auto        handle_unknown_error(
            const std::source_location& location = std::source_location::current( )
        ) -> void;

    private:
        sol::state                                       m_state;
        std::mutex                                       m_mutex;
        std::map< hash_t, std::vector< sol::function > > m_callbacks{ };
        std::vector< std::shared_ptr< ChampionModule > > m_champion_modules{ };
        bool                                             m_should_reset{ false };
    };
}

extern std::unique_ptr< lua::LuaState > g_lua2;
extern lua_State*                       g_lua_state2;
extern lua::LuaScripts                  g_scripts;

#endif
