#include "pch.hpp"

#include "logger.hpp"

namespace lua {
}

std::unique_ptr< lua::Logger > lua_logger = std::make_unique< lua::Logger >( );
