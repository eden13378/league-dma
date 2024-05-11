#include "pch.hpp"

#include "unit_info_component.hpp"

#include "object.hpp"
#include <iostream>

namespace sdk::game {
    auto UnitInfoComponent::get_character_records_ptr( ) const -> uintptr_t{
        const auto char_data = *reinterpret_cast< uintptr_t* >( reinterpret_cast< uintptr_t >( this ) + 0x28 );
        if ( !char_data ) return 0;

        const auto char_data_common = app->memory->read< uintptr_t >( char_data + 0x8 );
        if ( !char_data_common.has_value( ) || !*char_data_common ) return 0;

        auto crp = app->memory->read< uintptr_t >( *char_data_common + 0x28 );

        if ( !crp ) return 0;

        return *crp;
    }

    auto UnitInfoComponent::get_health_bar_height( ) const -> float{
        const auto char_records = get_character_records_ptr( );
        if ( !char_records ) return 0.f;

        std::cout << "Charrecodrds: 0x" << std::hex << char_records << std::endl;

        auto hbh = app->memory->read< float >( char_records + 0xC4 );

        if ( !hbh.has_value( ) ) return 0.f;

        return *hbh;
    }

    auto UnitInfoComponent::get_champion_id( ) const -> unsigned{
        const auto char_records = get_character_records_ptr( );
        if ( !char_records ) return 0;

        auto ci = app->memory->read< unsigned >( char_records + 0x808 );

        if ( !ci ) return 0;

        return *ci;
    }

    auto UnitInfoComponent::get_base_ms( ) const -> float{
        const auto char_records = get_character_records_ptr( );
        if ( !char_records ) return 0.f;

        auto bm = app->memory->read< float >( char_records + 0x248 );

        if ( !bm ) return 0.f;

        return *bm;
    }
}
