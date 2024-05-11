#pragma once
#include "..\..\utils\memory_holder.hpp"
#include <d3dx9math.h>


namespace sdk::game {
    class HudManager;

    class PwHud {
    public:
        auto get_hud_manager( ) const -> utils::MemoryHolder< HudManager >;
        auto get_unit_info_component_offset( ) const -> float;
        auto get_unit_info_component_modifier() const -> float;
        auto set_zoom( float value ) const -> bool;
        auto set_fov( float value ) const -> bool;

        auto camera_position( ) const -> D3DXVECTOR3;
        auto camera_look_at( ) const -> D3DXVECTOR3;

        char                pad_0000[ 0xC ]; //0x0000
        class c_hud_camera* hud_camera; //0xC
        char                pad_0010[ 0x4 ]; //0x10
        HudManager*         hud_manager; // 0x14
        char                pad_0001[ 40 ]; // 0x0000
    };
}

extern utils::MemoryHolder< sdk::game::PwHud > g_pw_hud;
