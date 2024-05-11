#pragma once
#include <cstdint>
#include <d3dx9math.h>

#include "..\..\utils\memory_holder.hpp"

namespace sdk::game {
    class RenderManager {
    public:
        char    pad_0000[ 12 ]; //0x0000
        int32_t width; //0x000C
        int32_t height; //0x0010

        auto get_width( ) const -> int32_t{ return width; }
        auto get_height( ) const -> int32_t{ return height; }

        auto get_view_projection_matrix( D3DXMATRIX* matrix ) const -> void;
        auto get_valid_view_projection_matrix( ) const -> D3DXMATRIX;
    };
}

extern utils::MemoryHolder< sdk::game::RenderManager > g_render_manager;
