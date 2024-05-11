#pragma once
#include <cstdint>
#include <d3d11.h>

namespace overlay {
    auto initialize( ) -> void;
    auto load_texture_from_file(
        const char*                filename,
        ID3D11ShaderResourceView** out_srv,
        int*                       out_width,
        int*                       out_height
    ) -> bool;
    auto load_texture_from_memory(
        const unsigned char*       buffer,
        int32_t                    len,
        ID3D11ShaderResourceView** out_srv,
        int*                       out_width,
        int*                       out_height
    ) -> bool;
}
