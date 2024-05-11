#pragma once
#include "..\..\utils\memory_holder.hpp"

#include "inventory_slot.hpp"

namespace sdk::game {
    class Inventory {
    public:
        auto get_inventory_slot( const int32_t item_number ) const -> utils::MemoryHolder< InventorySlot >{
            const auto ptr = *reinterpret_cast< intptr_t* >( reinterpret_cast< intptr_t >( this ) + offsetof(
                Inventory,
                item1
            ) + 0x10 + 0x8 * item_number );

            if ( ptr == 0 ) return { };

            auto is = app->memory->read< InventorySlot >( ptr );

            if(!is.has_value(  )) return {}; 

            return {*is, ptr };
        }

        char                 pad_0000[ 18 ];  // 0x0000
        class InventorySlot* item1; //0x0030
        char                 pad_001C[ 268 ]; //0x001C
    };
}
