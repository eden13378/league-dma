#pragma once

#if _DEBUG
#define run_catching( cmd ) try { cmd } catch(InvalidHolderException& e) { app->logger->error("object = null"); }
#define run_catching_lua( cmd ) cmd
#else
#define ___run_catching( cmd, is_lua ) try { cmd } catch(InvalidHolderException& e) {  } catch(InvalidMemoryAddressException& e) {} \
catch(const std::exception& e) { \
if(e.what()) { \
app->logger->error("exception {} {}",_(#cmd), e.what()); \
} else { \
app->logger->error("exception {} {}", _(#cmd), _("unknown")); \
} \
if constexpr (!is_lua) { \
if( std::string( e.what() ).find("device or resource busy") == std::string::npos ) { if(app->sentry) app->sentry->track_exception(e.what()); } \
} \
} catch(...) { \
app->logger->error("{} {}", _(#cmd), _("uncaught exception")); \
}
#define run_catching( cmd ) ___run_catching( cmd, false )
#define run_catching_lua( cmd ) ___run_catching( cmd, true )
#endif
#if _DEBUG && debug_profiling_times
#define profile_func( call, _name, name_hash ) { \
utils::c_timer timer; \
call \
if ( g_features->visuals->debug_profiling_times.find( name_hash ) == g_features->visuals->debug_profiling_times.end( ) ) { \
auto& debug_feature_time = g_features->visuals->debug_profiling_times[ name_hash ]; \
debug_feature_time.time = static_cast< int32_t >( timer.get_ms_since_start( ).count( ) ); \
debug_feature_time.name = _name; \
} \
else { \
g_features->visuals->debug_profiling_times[ name_hash ].update( static_cast< int32_t >( timer.get_ms_since_start( ).count( ) ) ); \
} \
}
#else
#define profile_func( call, name, name_hash ) call
#endif

namespace features {
    class Runtime {
    public:
        virtual ~Runtime( ) = default;

        virtual auto initialize( ) noexcept -> void = 0;
        virtual auto on_draw( ) noexcept -> void = 0;

        virtual auto on_lua_reset( ) noexcept -> void = 0;
        virtual auto on_lua_load( ) noexcept -> void = 0;

        virtual auto start_drawing( ) noexcept -> void = 0;
        virtual auto start_features( ) noexcept -> void = 0;

        auto initialize_champion_module( ) noexcept -> void;

        static auto load_encouragements( ) noexcept -> void;
        static auto load_taunts( ) noexcept -> void;
        static auto run_menu_initializers( ) noexcept -> void;

        static auto run_feature_list(         const std::vector< std::shared_ptr< IFeature > >& list,
        const bool                                        run_threaded = true ) noexcept -> void;

    protected:
        bool m_module_initialized{ };
    };
}
