#include "pch.hpp"

#include "pw_hud.hpp"
#include "hud_manager.hpp"
#include "../globals.hpp"

namespace sdk::game {
    auto PwHud::get_hud_manager( ) const -> utils::MemoryHolder< HudManager >{
        const auto ptr = *reinterpret_cast< uintptr_t* >( reinterpret_cast< uintptr_t >( this ) + 0x28 );

        if ( !ptr ) return { };

        return utils::MemoryHolder< HudManager >( ptr );
    }

    auto PwHud::get_unit_info_component_offset( ) const -> float{
        const auto hud_camera_ptr = *reinterpret_cast< uintptr_t* >( reinterpret_cast< uintptr_t >( this ) + 0x18 );

        if ( !hud_camera_ptr ) return { };

        const auto camera_config_ptr = app->memory->read< uintptr_t >( g_league_base + offsets::camera_config );

        if ( !camera_config_ptr.has_value( ) || !*camera_config_ptr ) return { };

        const auto hud_camera_modifier    = app->memory->read< float >( hud_camera_ptr + 0x2C0 );
        const auto camera_config_zoom_max = app->memory->read< float >( *camera_config_ptr + 0x28 );

        if ( !hud_camera_modifier.has_value( ) || !camera_config_zoom_max.has_value( ) ) return { };

        return *camera_config_zoom_max / *hud_camera_modifier;
    }

    auto PwHud::get_unit_info_component_modifier() const -> float {
        const auto hud_camera_ptr = *reinterpret_cast<uintptr_t *>(reinterpret_cast<uintptr_t>(this) + 0x18);

        if (!hud_camera_ptr) return {};

         const auto hud_camera_zoom = app->memory->read<float>(hud_camera_ptr + 0x2C0 );
        const auto hud_camera_modifier = app->memory->read<float>(hud_camera_ptr + 0x2B8 );

        return !hud_camera_modifier.has_value() ? 0.f : *hud_camera_zoom / *hud_camera_modifier;
    }


    auto PwHud::set_zoom( const float value ) const -> bool{
        const auto ptr = *reinterpret_cast< uintptr_t* >( reinterpret_cast< uintptr_t >( this ) + 0x18 );

        if ( !ptr ) return false;

        return app->memory->write< float >( ptr + 0x2BC, value );
    }

    auto PwHud::set_fov( const float value ) const -> bool{
        const auto ptr = *reinterpret_cast< uintptr_t* >( reinterpret_cast< uintptr_t >( this ) + 0x18 );

        if ( !ptr ) return false;

        return app->memory->write< float >( ptr + 0x1BC, value );
    }

    auto PwHud::camera_position( ) const -> D3DXVECTOR3{
        auto camera_config_ptr = app->memory->read< uintptr_t >( g_league_base + offsets::r3d_camera );

        if ( !camera_config_ptr.has_value( ) || !*camera_config_ptr ) return { };
        *camera_config_ptr += 0x8;

        auto cp = app->memory->read< D3DXVECTOR3 >( *camera_config_ptr + 0x0 );

        if ( !cp ) return { };

        return *cp;
    }

    auto PwHud::camera_look_at( ) const -> D3DXVECTOR3{
        auto camera_config_ptr = app->memory->read< uintptr_t >( g_league_base + offsets::r3d_camera );

        if ( !camera_config_ptr.has_value( ) || !*camera_config_ptr ) return { };
        *camera_config_ptr += 0x8;

        auto cla = app->memory->read< D3DXVECTOR3 >( *camera_config_ptr + 0x18 );

        if ( !cla ) return { };

        return *cla;
    }
}
