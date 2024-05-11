#pragma once
#include <memory>

namespace features {
    class CalledFromWrongThread final : public std::runtime_error {
    public:
        auto what( ) const -> char const* override{ return "CalledFromWrongThread"; }
        ~CalledFromWrongThread( ) override = default;

        CalledFromWrongThread( ): std::runtime_error( "CalledFromWrongThread" ){
        }
    };

    class Threading {
    public:
        [[nodiscard]] auto is_feature_thread( ) const -> bool;
        [[nodiscard]] auto is_render_thread( ) const -> bool;

        std::thread::id feature_thread{ };
        std::thread::id render_thread{ };
    };
}

extern std::unique_ptr< features::Threading > g_threading;
