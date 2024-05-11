#pragma once
#include <cstdint>


namespace sdk::game {
    class UnitInfoComponent {
    private:
        auto get_character_records_ptr( ) const -> uintptr_t;

    public:
        auto get_health_bar_height( ) const -> float;
        auto get_champion_id( ) const -> unsigned;
        auto get_base_ms( ) const -> float;

        char                               pad_0000[ 0x4 ]; //0x0000
        class c_unit_health_bar_component* unit_health_bar_component; //0x0004
        char                               pad_0008[ 0x20 ]; //0x0008
        class c_char_data_root*            char_data_root; //0x0010
    };
}
