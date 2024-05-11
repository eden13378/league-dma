#pragma once
#include "../sdk/game/object.hpp"
// #include "../build.hpp"
// #include "../sdk/game/object.hpp"

class CHolder {
public:
    enum class EUpdateError {
        invalid_address,
        unknown_exception,
        object_now_invalid
    };

    CHolder( ) = default;

    auto set_index( const int32_t index ) -> void{ m_index = index; }

    /**
     * \brief Try to update current object.
     */
    auto               update_features( ) noexcept -> std::expected< void, EUpdateError >;
    auto               update( ) noexcept -> std::expected< void, CHolder::EUpdateError >;
    [[nodiscard]] auto is_valid( ) const -> bool;
    [[nodiscard]] auto get( ) const -> sdk::game::Object*;
    auto               get_unchecked( ) -> sdk::game::Object*{ return &m_object; }
    [[nodiscard]] auto get_address( ) const -> intptr_t{ return m_address; }

    auto operator->( ) const -> sdk::game::Object*{ return get( ); }
    auto operator*( ) const -> sdk::game::Object&{ return *get( ); }

    auto get_feature_object( ) -> sdk::game::Object*{ return &m_object; }

    auto        set( const intptr_t address ) noexcept -> void;
    auto        invalidate( ) noexcept -> void;
    static auto from_object( const sdk::game::Object* object ) -> CHolder*;

    explicit operator bool( ) const{
        if ( !this ) return false;
        return is_valid( );
    }

private:
    auto should_update_render( ) noexcept -> bool{
        const auto now   = std::chrono::steady_clock::now( );
        const auto delta = std::chrono::duration_cast< std::chrono::microseconds >( now - m_last_updated_render ).
            count( );
        if ( delta <= 500 && delta > 0 ) return false;

        m_last_updated_render = now;
        return true;
    }

private:
    sdk::game::Object m_object{ };
    intptr_t          m_address{ };
    int32_t           m_index{ -1 };
    /**
     * \brief Last time-point when the object was updated from feature thread.
     */
    // std::chrono::time_point< std::chrono::steady_clock > m_last_updated_features{ };
    std::chrono::time_point< std::chrono::steady_clock > m_last_updated_render{ };
};
