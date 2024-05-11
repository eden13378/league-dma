#pragma once
#include "key.hpp"
#if enable_lua
#include "../include/sol/forward.hpp"
#endif
#include "../sdk/sdk.hpp"

#include "../sdk/game/spell_slot.hpp"

namespace utils {
    class Input {
    public:
        enum class EKeyState {
            key_down,
            key_up
        };

        enum class EMouseButton {
            left,
            right
        };

        auto        set_cursor_position( Vec2 position, hash_t caller ) -> void;
        static auto get_cursor_position( ) -> Vec2;

#if enable_lua
        static auto get_cursor_position_game( ) -> sol::object;
#endif

        auto send_key_event( EKey key, EKeyState state, hash_t caller ) -> void;
        auto send_mouse_key_event( EMouseButton button, EKeyState state, hash_t caller ) -> void;

        auto process_queue( ) -> void;
        auto add_custom_to_queue( const std::function< void( ) >& fn, hash_t caller ) -> void;

        auto issue_order_move(
            const sdk::math::Vec3& position,
            bool                   bypass_limiter = false,
            bool                   skipLock       = false
        ) -> bool;
        auto issue_order_attack( uint32_t network_id, bool called_from_lua = false ) -> bool;
        auto issue_order_attack( Object* object, bool called_from_lua = false ) -> bool;
        auto cast_spell( ESpellSlot slot, const Vec3& position, bool called_from_lua = false ) -> bool;
        auto cast_spell( ESpellSlot slot, const Vec3& start, const Vec3& end, bool called_from_lua = false ) -> bool;
        auto cast_spell( ESpellSlot slot, Object* object, bool called_from_lua = false ) -> bool;
        auto cast_spell( ESpellSlot slot, uint32_t network_id, bool called_from_lua = false ) -> bool;
        auto cast_spell( ESpellSlot slot, bool called_from_lua = false ) -> bool;
        auto release_chargeable( ESpellSlot slot, const sdk::math::Vec3& position, bool release = true ) -> bool;

#if enable_lua
        auto issue_order_move_lua( const sol::object position ) -> bool;
        auto issue_order_attack_lua(
            /*uintptr_t*/
            const sol::object network_id
        ) -> bool;
        auto cast_spell_lua(
            const sol::object slot,
            /*uintptr_t*/
            const sol::object network_id,
            const sol::object _end
        ) -> sol::object;
        auto release_chargeable_lua( const sol::object slot, const sol::object position ) -> bool;

        // auto set_issue_order_blocked(bool value) -> void;
        // auto set_cast_spell_blocked(bool value) -> void;

        // auto is_movement_blocked( ) -> bool;
        // auto is_casting_blocked( ) -> bool;

        auto lua_block_issue_order( const sol::object should_block ) -> void;
        auto lua_block_cast_spell( const sol::object should_block ) -> void;

        static auto lua_is_issue_order_blocked( ) -> sol::object;
        static auto is_cast_spell_blocked( ) -> sol::object;
#endif

        static auto is_key_pressed( EKey key ) -> bool;

#if enable_lua
        auto is_key_pressed_lua( const sol::object key ) -> bool;
#endif

    public:
        static auto set_cursor_position_run( Vec2 position ) -> void;
        static auto send_key_event_run( EKey key, EKeyState state ) -> void;
        static auto send_mouse_key_event_run( EMouseButton button, EKeyState state ) -> void;

#if enable_lua
        auto set_cursor_position_lua( const sol::object position ) -> void;
#endif

#if enable_lua
        auto send_key_event_lua( const sol::object key, const sol::object state ) -> void;
#endif

#if enable_lua
        auto send_mouse_key_event_lua( const sol::object button, const sol::object state ) -> void;
#endif

    private:
        auto should_run_action( const int32_t time = 17 ) -> bool{
            const auto curtime = std::chrono::steady_clock::now( );
            const auto diff = std::chrono::duration_cast< std::chrono::milliseconds >( curtime - m_last_action_time ).
                count( );
            if ( diff < time ) return false;
            m_last_action_time = curtime;
            return true;
        }

        auto should_run_attack_action( const int32_t time = 17 ) -> bool{
            const auto curtime = std::chrono::steady_clock::now( );
            const auto diff    = std::chrono::duration_cast< std::chrono::milliseconds >(
                curtime - m_last_attack_action_time
            ).count( );
            if ( diff < time ) return false;
            m_last_attack_action_time = curtime;
            return true;
        }

        auto should_run_spell_action( ESpellSlot slot, const int32_t time = 30 ) -> bool{
            const auto curtime = std::chrono::steady_clock::now( );
            const auto diff    = std::chrono::duration_cast< std::chrono::milliseconds >(
                curtime - m_last_spell_cast_action_time.at( static_cast< size_t >( slot ) )
            ).count( );
            if ( diff < time ) return false;

            m_last_spell_cast_action_time.at( static_cast< size_t >( slot ) ) = curtime;
            return true;
        }

    private:
        struct input_cache_t {
            std::function< void( ) > action;
            hash_t                   caller;
        };

        std::vector< input_cache_t >                         m_input_queue;
        bool                                                 m_ready{ true };
        std::chrono::time_point< std::chrono::steady_clock > m_last_action_time{ std::chrono::steady_clock::now( ) };
        std::chrono::time_point< std::chrono::steady_clock > m_last_attack_action_time{
            std::chrono::steady_clock::now( )
        };
        std::array< std::chrono::time_point< std::chrono::steady_clock >, static_cast< size_t >( ESpellSlot::max ) >
        m_last_spell_cast_action_time{ };
        std::map< EKey, bool > m_was_pressed{ };
    };
}

extern std::unique_ptr< utils::Input > g_input;
