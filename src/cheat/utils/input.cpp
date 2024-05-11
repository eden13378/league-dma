#include "pch.hpp"

#include "input.hpp"

#include "c_function_caller.hpp"
#include "../features/names.hpp"
#if enable_lua
#include "../include/sol/sol.hpp"
#include "../sdk/game/hud_manager.hpp"
#endif
#if enable_new_lua
#include "../lua-v2/cancelable.hpp"
#include "../lua-v2/lua_def.hpp"
#include "../lua-v2/state.hpp"
#endif
// #include "../lua/c_lua.hpp"

std::unique_ptr< utils::Input > g_input = std::make_unique< utils::Input >( );

namespace utils {
    auto Input::set_cursor_position( const Vec2 position, const hash_t caller ) -> void{
        if ( !m_ready ) return;
        // std::unique_lock lock( mutex );
        m_input_queue.push_back(
            {
                .action = [position, this]( ) -> void{ set_cursor_position_run( position ); },
                .caller = caller
            }
        );
    }

    auto Input::get_cursor_position( ) -> Vec2{
        POINT point;
        GetCursorPos( &point );

        return { static_cast< float >( point.x ), static_cast< float >( point.y ) };
    }

#if enable_lua
    auto Input::get_cursor_position_game( ) -> sol::object{
        if ( !g_pw_hud ) return sol::nil;
        const auto hud_manager = g_pw_hud->get_hud_manager( );
        if ( !hud_manager ) return sol::nil;

        return sol::make_object( g_lua_state2, hud_manager->cursor_position_unclipped );
    }
#endif

    auto Input::send_key_event( EKey key, EKeyState state, const hash_t caller ) -> void{
        if ( !m_ready ) return;
        // std::unique_lock lock( mutex );
        m_input_queue.push_back(
            {
                .action = [key, state, this]( ) -> void{ send_key_event_run( key, state ); },
                .caller = caller
            }
        );
    }

    auto Input::send_mouse_key_event( EMouseButton button, EKeyState state, const hash_t caller ) -> void{
        if ( !m_ready ) return;
        // std::unique_lock lock( mutex );
        m_input_queue.push_back(
            {
                .action = [button, state, this]( ) -> void{ send_mouse_key_event_run( button, state ); },
                .caller = caller
            }
        );
    }

    auto Input::process_queue( ) -> void{
        // std::unique_lock lock( mutex );
        m_ready = false;

        const auto has_evade_input = std::ranges::find_if(
            m_input_queue,
            []( const input_cache_t& cache ) -> bool{ return cache.caller == features::names::evade; }
        ) != m_input_queue.end( );

        for ( auto& queue : m_input_queue ) {
            // todo: improve this to use a priority list
            if ( has_evade_input && queue.caller != features::names::evade ) continue;
            queue.action( );
        }

        m_input_queue.clear( );
        m_ready = true;
    }

    auto Input::add_custom_to_queue( const std::function< void( ) >& fn, const hash_t caller ) -> void{
        if ( !m_ready ) return;
        // std::unique_lock lock( mutex );
        m_input_queue.push_back( { .action = fn, .caller = caller } );
    }

#if enable_lua
    // ReSharper disable once CppPassValueParameterByConstReference
    auto Input::issue_order_move_lua( const sol::object position ) -> bool{
        lua_arg_check( position, Vec3 )

        return issue_order_move( position.as< Vec3 >( ), false, true );
    }

    // ReSharper disable once CppPassValueParameterByConstReference
    auto Input::issue_order_attack_lua( const sol::object network_id ) -> bool{
        if ( network_id.get_type( ) == sol::type::number ) {
            return issue_order_attack(
                network_id.as< uint32_t >( ),
                true
            );
        }
        if ( network_id.is< Object* >( ) ) {
            const auto target = network_id.as< Object* >( );
            if ( !target ) return false;
            return issue_order_attack( target, true );
        }

        return false;
    }


    auto Input::cast_spell_lua(
        // ReSharper disable CppPassValueParameterByConstReference
        const sol::object slot,
        const sol::object network_id,
        const sol::object _end
        // ReSharper restore CppPassValueParameterByConstReference
    ) -> sol::object{
        lua_arg_check_ct( slot, uint32_t, "number" )

        const auto s = slot.as< uint32_t >( );

        if ( _end.get_type( ) != sol::type::nil ) {
            lua_arg_check( _end, Vec3 )
            lua_arg_check( network_id, Vec3 )
            return sol::make_object(
                g_lua_state2,
                cast_spell( static_cast< ESpellSlot >( s ), network_id.as< Vec3 >( ), _end.as< Vec3 >( ), true )
            );
        }

        if ( network_id.get_type( ) == sol::type::number ) {
            return sol::make_object(
                g_lua_state2,
                cast_spell( static_cast< ESpellSlot >( s ), network_id.as< uint32_t >( ), true )
            );
        }

        if ( network_id.is< Object* >( ) ) {
            const auto target = network_id.as< Object* >( );
            if ( !target ) return sol::make_object( g_lua_state2, false );
            return sol::make_object( g_lua_state2, cast_spell( static_cast< ESpellSlot >( s ), target, true ) );
        }

        if ( network_id.is< Vec3 >( ) ) {
            return sol::make_object(
                g_lua_state2,
                cast_spell( static_cast< ESpellSlot >( s ), network_id.as< Vec3 >( ), true )
            );
        }

        return sol::make_object( g_lua_state2, cast_spell( static_cast< ESpellSlot >( s ), true ) );
    }

    auto Input::release_chargeable_lua(
        // ReSharper disable CppPassValueParameterByConstReference
        const sol::object slot,
        const sol::object position
        // ReSharper restore CppPassValueParameterByConstReference
    ) -> bool{
        lua_arg_check_ct( slot, uint32_t, "number" )
        lua_arg_check( position, Vec3 )

        const auto s = slot.as< uint32_t >( );

        return release_chargeable( static_cast< ESpellSlot >( s ), position.as< Vec3 >( ), true );
    }

    // ReSharper disable once CppPassValueParameterByConstReference
    auto Input::lua_block_issue_order( const sol::object should_block ) -> void{
        lua_arg_check_v( should_block, bool )

        g_function_caller->set_issue_order_blocked( should_block.as< bool >( ) );
    }

    // ReSharper disable once CppPassValueParameterByConstReference
    auto Input::lua_block_cast_spell( const sol::object should_block ) -> void{
        lua_arg_check_v( should_block, bool )

        g_function_caller->set_cast_spell_blocked( should_block.as< bool >( ) );
    }

    auto Input::lua_is_issue_order_blocked( ) -> sol::object{
        return sol::make_object( g_lua_state2, g_function_caller->is_movement_blocked( ) );
    }

    auto Input::is_cast_spell_blocked( ) -> sol::object{
        return sol::make_object( g_lua_state2, g_function_caller->is_casting_blocked( ) );
    }
#endif


    auto Input::is_key_pressed( EKey key ) -> bool{ return GetAsyncKeyState( static_cast< int >( key ) ); }

#if enable_lua
    // ReSharper disable once CppPassValueParameterByConstReference
    auto Input::is_key_pressed_lua( const sol::object key ) -> bool{
        lua_arg_check_ct( key, int32_t, "number" )

        return is_key_pressed( static_cast< EKey >( key.as< int32_t >( ) ) );
    }
#endif

    auto lua_run_cancel_check( hash_t action, bool skip_lock ) -> bool{
#if enable_lua
        if ( !g_lua2 ) return false;

        auto cancelable = std::make_shared< lua::CancelableAction >( );

        if ( skip_lock ) {
            g_lua2->run_callback(
                action,
                sol::make_object( g_lua_state2, cancelable )
            );
        } else {
            g_lua2->execute_locked(
                [action, cancelable]( ) -> void{
                    g_lua2->run_callback(
                        action,
                        sol::make_object( g_lua_state2, cancelable )
                    );
                }
            );
        }

        if ( cancelable->should_cancel( ) ) return true;

        return false;
#else
        return false;
#endif
    }

    auto Input::issue_order_move(
        const sdk::math::Vec3& position,
        const bool             bypass_limiter,
        const bool             skipLock
    ) -> bool{
        if ( !bypass_limiter ) { if ( !should_run_action( ) ) return false; } else {
            if ( !should_run_action( 25 ) ) return false;
        }

        if ( lua_run_cancel_check( ct_hash( "local.issue_order_move" ), skipLock ) ) return false;


        g_function_caller->issue_order_move( position );
        return true;
    }

    auto Input::issue_order_attack( uint32_t network_id, const bool called_from_lua ) -> bool{
        if ( !should_run_attack_action( ) ) return false;

        if ( lua_run_cancel_check( ct_hash( "local.issue_order_attack" ), called_from_lua ) ) return false;

        g_function_caller->issue_order_attack( network_id );
        return true;
    }

    auto Input::issue_order_attack( Object* object, const bool called_from_lua ) -> bool{
        if ( !should_run_action( ) ) return false;

        if ( lua_run_cancel_check( ct_hash( "local.issue_order_attack" ), called_from_lua ) ) return false;

        g_function_caller->issue_order_attack( object->network_id );
        return true;
    }

    auto Input::cast_spell( ESpellSlot slot, const Vec3& position, const bool called_from_lua ) -> bool{
        if ( !should_run_spell_action( slot ) ) return false;

        if ( lua_run_cancel_check( ct_hash( "local.cast_spell" ), called_from_lua ) ) return false;

        g_function_caller->cast_spell( static_cast< unsigned >( slot ), position );
        return true;
    }

    auto Input::cast_spell( ESpellSlot slot, const Vec3& start, const Vec3& end, const bool called_from_lua ) -> bool{
        if ( !should_run_spell_action( slot ) ) return false;

        if ( lua_run_cancel_check( ct_hash( "local.cast_spell" ), called_from_lua ) ) return false;


        g_function_caller->cast_spell( static_cast< unsigned >( slot ), start, end );
        return true;
    }

    auto Input::cast_spell( ESpellSlot slot, Object* object, const bool called_from_lua ) -> bool{
        if ( !should_run_spell_action( slot ) ) return false;

        if ( lua_run_cancel_check( ct_hash( "local.cast_spell" ), called_from_lua ) ) return false;

        g_function_caller->cast_spell( static_cast< unsigned >( slot ), object->network_id );
        return true;
    }

    auto Input::cast_spell( ESpellSlot slot, uint32_t network_id, const bool called_from_lua ) -> bool{
        if ( !should_run_spell_action( slot ) ) return false;

        if ( lua_run_cancel_check( ct_hash( "local.cast_spell" ), called_from_lua ) ) return false;


        g_function_caller->cast_spell( static_cast< unsigned >( slot ), network_id );
        return true;
    }

    auto Input::cast_spell( ESpellSlot slot, const bool called_from_lua ) -> bool{
        if ( !should_run_spell_action( slot ) ) return false;

        if ( lua_run_cancel_check( ct_hash( "local.cast_spell" ), called_from_lua ) ) return false;

        g_function_caller->cast_spell( static_cast< unsigned >( slot ) );
        return true;
    }

    auto Input::release_chargeable( ESpellSlot slot, const sdk::math::Vec3& position, const bool release ) -> bool{
        if ( !should_run_action( ) ) return false;

        // if ( lua_run_cancel_check( ct_hash( "local.release_chargeable" ), called_from_lua ) ) return false;


        g_function_caller->release_chargeable( static_cast< unsigned >( slot ), position, release );
        return true;
    }

    auto Input::set_cursor_position_run( const Vec2 position ) -> void{
        SetCursorPos( static_cast< int32_t >( position.x ), static_cast< int32_t >( position.y ) );
    }

    auto Input::send_key_event_run( EKey key, const EKeyState state ) -> void{
        INPUT input;
        input.type           = INPUT_KEYBOARD;
        input.ki.wScan       = MapVirtualKey( static_cast< int32_t >( key ), MAPVK_VK_TO_VSC );
        input.ki.time        = 0;
        input.ki.dwExtraInfo = 0;
        input.ki.wVk         = static_cast< int32_t >( key );
        input.ki.dwFlags     = state == EKeyState::key_down ? 0 : KEYEVENTF_KEYUP;

        SendInput( 1, &input, sizeof( INPUT ) );
    }

    auto Input::send_mouse_key_event_run( const EMouseButton button, const EKeyState state ) -> void{
        int32_t flags;

        if ( state == EKeyState::key_down ) { flags = button == EMouseButton::left ? 0x2 : 0x8; } else {
            flags = button == EMouseButton::left ? 0x4 : 0x10;
        }

        mouse_event( flags, 0, 0, 0, 0 );
    }

#if enable_lua
    auto Input::set_cursor_position_lua(
        // ReSharper disable CppPassValueParameterByConstReference
        const sol::object position
        // ReSharper restore CppPassValueParameterByConstReference
    ) -> void{
        lua_arg_check_v( position, Vec2 )

        set_cursor_position_run( position.as< Vec2 >( ) );
    }

    auto Input::send_key_event_lua(
        // ReSharper disable CppPassValueParameterByConstReference
        const sol::object key,
        const sol::object state
        // ReSharper restore CppPassValueParameterByConstReference
    ) -> void{
        lua_arg_check_ct_v( key, int32_t, "number" )
        lua_arg_check_ct_v( state, int32_t, "number" )

        send_key_event_run(
            static_cast< EKey >( key.as< int32_t >( ) ),
            static_cast< EKeyState >( state.as< int32_t >( ) )
        );
    }


    auto Input::send_mouse_key_event_lua(
        // ReSharper disable CppPassValueParameterByConstReference
        const sol::object button,
        const sol::object state
        // ReSharper restore CppPassValueParameterByConstReference
    ) -> void{
        lua_arg_check_ct_v( button, int32_t, "number" )
        lua_arg_check_ct_v( state, int32_t, "number" )

        send_mouse_key_event_run(
            static_cast< EMouseButton >( button.as< int32_t >( ) ),
            static_cast< EKeyState >( state.as< int32_t >( ) )
        );
    }
#endif
}
