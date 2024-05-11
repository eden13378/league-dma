#pragma once
#include <cstdint>
#include "..\..\utils\memory_holder.hpp"

namespace sdk::game {
    class BuffData;

    class Buff {
    public:
        [[nodiscard]] auto get_buff_data( ) const -> utils::MemoryHolder< BuffData >;

        intptr_t buff_data;
        intptr_t buff_unknown;
    };
}
