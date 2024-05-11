#include "pch.hpp"

#include "holder.hpp"

#include "exceptions.hpp"
#include "../features/entity_list.hpp"
#include "../sdk/game/object.hpp"

auto CHolder::update_features( ) noexcept -> std::expected< void, CHolder::EUpdateError >{
    try {
        if ( !m_address ) return std::unexpected( EUpdateError::invalid_address );
        if ( !should_update_render( ) ) return { };

        app->memory->read< sdk::game::Object >( m_address, &m_object );

        if ( !is_valid( ) ) return std::unexpected( EUpdateError::object_now_invalid );

        return { };
    } catch ( ... ) { return std::unexpected( EUpdateError::unknown_exception ); }
}

auto CHolder::update( ) noexcept -> std::expected< void, CHolder::EUpdateError >{ return update_features( ); }

auto CHolder::is_valid( ) const -> bool{
    if ( !( void* )this || !m_address || !m_index ) return false;

    return m_index != -1 && m_index == m_object.index;
}

auto CHolder::get( ) const -> sdk::game::Object*{
    if ( !is_valid( ) ) throw InvalidHolderException( );

    return const_cast< sdk::game::Object* >( &m_object );
}

auto CHolder::set( const intptr_t address ) noexcept -> void{ m_address = address; }

auto CHolder::invalidate( ) noexcept -> void{ m_address = 0; }

auto CHolder::from_object( const sdk::game::Object* object ) -> CHolder*{
    if ( !object ) return nullptr;

    return &g_entity_list.get_by_index( object->index );
}
