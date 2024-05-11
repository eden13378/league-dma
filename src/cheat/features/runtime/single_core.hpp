#pragma once
#include "runtime2.hpp"

namespace features {
    class SingleCoreRuntime final : public Runtime {
    public:
        ~MultiCoreRuntime( ) override = default;

        auto initialize( ) noexcept -> void override;
        auto on_draw( ) noexcept -> void override;
        auto on_lua_reset( ) noexcept -> void override;
        auto on_lua_load( ) noexcept -> void override;

        auto start_drawing( ) noexcept -> void override;
        auto start_features( ) noexcept -> void override;
    };
}
