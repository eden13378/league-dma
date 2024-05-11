#pragma once

namespace sdk::game {
    class MenuGui {
    public:
        [[nodiscard]] auto is_chat_open( ) const -> bool{ return menu_state == 5; }

        [[nodiscard]] auto is_shop_open( ) const -> bool{ return menu_state == 6; }

        char    pad_0000[ 0xC ]; //0x0000
        int32_t menu_state; //0x000C
    };
}
