#pragma once
#include "base_item.hpp"

namespace sdk::game {
    class InventorySlot {
    public:
        auto get_base_item( ) const -> utils::MemoryHolder< BaseItem >{
            if ( !base_item ) return { };

            return utils::MemoryHolder< BaseItem >( reinterpret_cast< intptr_t >( base_item ) );
        }

        auto get_base_item_raw( ) const -> std::optional< BaseItem >{
            const auto bi = get_base_item( );
            if ( !bi ) return std::nullopt;
            return *bi;
        }

        unsigned char   count; // 0x0
        char            pad_0001[ 3 ]; // 0x0001
        float           update_time; //0x0004
        int32_t         stacks; //0x0008
        char            pad_000C[ 4 ]; // 0x000C
        class BaseItem* base_item; //0x0010
    };
}
