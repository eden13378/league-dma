#pragma once
#include <cstdint>

#include "..\..\utils\memory_holder.hpp"

#include <sol/sol.hpp>

namespace lua {
    class LuaBuff;
}

namespace sdk::game {
    class BuffData;

    class BuffManager {
    public:
        BuffManager( ) = default;

        auto get_buff_array_end( ) const -> intptr_t;
        auto size( ) const -> size_t;
        auto get_all( ) const -> std::vector< utils::MemoryHolder< BuffData > >;

#if enable_new_lua
        auto get_all_raw( ) const -> sol::as_table_t< std::vector< lua::LuaBuff > >;
#endif

        char     pad_0000[ 16 ]; //0x0000
        intptr_t buff_array_start; //0x0010
        intptr_t buff_array_end; //0x0018
    };
}
