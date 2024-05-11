#pragma once
#include <memory>

#include "../logger.hpp"
#include "../lua-v2/state.hpp"

#include "../mixpanel/mixpanel.hpp"

#include "../utils/debug_logger.hpp"
#include "../utils/sentry/sentry.hpp"


namespace sdk {
    namespace memory {
        class Memory;
    }
}

namespace runtime {
    class App {
        using UnloadCB = std::function< void( ) >;

    public:
        ~App( ){ if ( this->logger ) this->logger->info( "deleting app" ); }

        auto should_run( ) const -> bool{ return !m_should_unload; }

        auto unload( ) -> void;
        auto add_unload_callback( const UnloadCB& callback ) -> void;

        auto run_unload_callbacks( ) const -> void;

        auto update_lua_scripts( ) -> void;

        auto get_scripts( ) -> std::deque< lua::LuaScript >;

    public:
        auto setup_logger( ) -> void;
        auto setup_sentry( ) -> void;
        auto setup_mixpanel( ) -> void;

    public:
        std::unique_ptr< user::c_user >        user{ };
        std::unique_ptr< sdk::memory::Memory > memory{ };
        std::shared_ptr< Logger >              logger{ };
        std::unique_ptr< mixpanel::Mixpanel >  mixpanel{ };

        int32_t feature_ticks{ 0 };

        std::unique_ptr< utils::Sentry > sentry{ };

    private:
        bool                         m_should_unload{ false };
        std::vector< UnloadCB >      m_unload_callbacks{ };
        std::deque< lua::LuaScript > m_scripts{ };
    };
}

extern std::unique_ptr< runtime::App > app;
