#include "pch.hpp"

#include "runtime.hpp"
#include "../entity_list.hpp"
#include "../feature.hpp"
#include "../target_selector/ITargetSelector.hpp"


#if _DEBUG
#define run_catching( cmd ) try { cmd } catch(InvalidHolderException& e) { app->logger->error("object = null"); } catch(InvalidMemoryAddressException& e) { app->logger->error("invalid memory address"); } catch( std::runtime_error& e ) { app->logger->error("exception {} {}", _(#cmd), e.what()); } catch( std::exception& e ) { app->logger->error("exception {} {}", _(#cmd), e.what()); } catch(...) { app->logger->error("{} {}", _(#cmd), _("uncaught exception")); }
#define run_catching_lua( cmd ) try { cmd } catch(std::exception& ex) { debug_log("lua exception: {}", ex.what()); } catch(...) { debug_log("lua exception: unknown"); }
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

namespace features {
#if enable_lua
    auto run_lua_initializers( sol::state* state ) -> void{
        for ( const auto feature : g_features->all_features )
            run_catching( feature->initialize_lua( state ); )

        features::EntityList2::initialize_lua( state );

        ITargetSelector::static_initialize_lua( state );
    }
#endif
}
