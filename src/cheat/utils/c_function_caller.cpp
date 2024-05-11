#include "pch.hpp"

#include "c_function_caller.hpp"

#include "../globals.hpp"

#include "../loader/loader.hpp"

typedef struct _PROCESS_INSTRUMENTATION_CALLBACK_INFORMATION {
    ULONG Version;
    ULONG Reserved;
    PVOID Callback;
}         PROCESS_INSTRUMENTATION_CALLBACK_INFORMATION, *PPROCESS_INSTRUMENTATION_CALLBACK_INFORMATION;

namespace utils {
    using fn_load_library = HMODULE(__stdcall*)( const char* );
    using fn_get_proc_address = FARPROC(__stdcall*)( HMODULE, const char* );
    using fn_init = void(__stdcall*)( );

    struct mapper_data {
        fn_load_library     load_lib;
        fn_get_proc_address get_proc_addr;
        unsigned char*      module_base;
    };

    auto c_function_caller::issue_order_move( const sdk::math::Vec3& position ) -> void{
        // app->logger->info( "call issue order move" );
        
        if ( m_initialized == false || m_stub_base <= 0x1000 ) return;

        update_shared_vars( );
        if ( m_shared_vars.function_idx != 0 ) return;

        m_shared_vars.issue_order_position.x = position.x;
        m_shared_vars.issue_order_position.y = position.y;
        m_shared_vars.issue_order_position.z = position.z;
        // app->logger->info( "issue order move" );
        call( 1, m_shared_vars_address );
    }

    auto c_function_caller::issue_order_attack( const unsigned network_id ) -> void{
        if ( m_initialized == false || m_stub_base <= 0x1000 ) return;

        update_shared_vars( );
        if ( m_shared_vars.function_idx != 0 || network_id == 0 ) return;

        m_shared_vars.issue_order_target_nid = network_id;
        app->logger->info( "issue order attack" );
        call( 2, m_shared_vars_address );
    }

    auto c_function_caller::cast_spell(
        const unsigned         slot,
        const sdk::math::Vec3& start,
        const sdk::math::Vec3& end
    ) -> void{
        if ( m_initialized == false || m_stub_base <= 0x1000 ) return;

        update_shared_vars( );
        if ( m_shared_vars.function_idx != 0 ) return;

        m_shared_vars.cast_spell_slot        = slot;
        m_shared_vars.cast_spell_position.x  = start.x;
        m_shared_vars.cast_spell_position.y  = start.y;
        m_shared_vars.cast_spell_position.z  = start.z;
        m_shared_vars.cast_spell_position2.x = end.x;
        m_shared_vars.cast_spell_position2.y = end.y;
        m_shared_vars.cast_spell_position2.z = end.z;

        call( 10, m_shared_vars_address );
    }

    auto c_function_caller::cast_spell( const unsigned slot, const sdk::math::Vec3& position ) -> void{
        if ( m_initialized == false || m_stub_base <= 0x1000 ) return;

        update_shared_vars( );
        if ( m_shared_vars.function_idx != 0 ) return;

        m_shared_vars.cast_spell_slot       = slot;
        m_shared_vars.cast_spell_position.x = position.x;
        m_shared_vars.cast_spell_position.y = position.y;
        m_shared_vars.cast_spell_position.z = position.z;
        call( 8, m_shared_vars_address );
    }

    auto c_function_caller::cast_spell( const unsigned slot, const unsigned network_id ) -> void{
        if ( m_initialized == false || m_stub_base <= 0x1000 ) return;

        update_shared_vars( );
        if ( m_shared_vars.function_idx != 0 || network_id == 0 ) return;

        m_shared_vars.cast_spell_slot = slot;
        m_shared_vars.cast_spell_nid  = network_id;
        call( 9, m_shared_vars_address );
    }

    auto c_function_caller::cast_spell( const unsigned slot ) -> void{
        if ( m_initialized == false || m_stub_base <= 0x1000 ) return;

        update_shared_vars( );
        if ( m_shared_vars.function_idx != 0 ) return;

        m_shared_vars.cast_spell_slot = slot;
        call( 7, m_shared_vars_address );
    }

    auto c_function_caller::release_chargeable(
        const unsigned         slot,
        const sdk::math::Vec3& position,
        const bool             release
    ) -> void{
        if ( m_initialized == false || m_stub_base <= 0x1000 ) return;

        update_shared_vars( );
        if ( m_shared_vars.function_idx != 0 ) return;

        m_shared_vars.cast_spell_slot            = slot;
        m_shared_vars.cast_spell_position.x      = position.x;
        m_shared_vars.cast_spell_position.y      = position.y;
        m_shared_vars.cast_spell_position.z      = position.z;
        m_shared_vars.release_chargeable_release = release;
        call( 6, m_shared_vars_address );
    }

    auto c_function_caller::enable_glow(
        const unsigned nid,
        const unsigned color_argb,
        const int      glow_id,
        const int      size,
        const int      diffusion,
        const bool     remove
    ) -> void{
        if ( m_initialized == false || m_stub_base <= 0x1000 ) return;

        update_shared_vars( );
        if ( m_shared_vars.glow_list_queued_nid != 0 ) return;

        m_shared_vars.glow_list_queued_nid       = nid;
        m_shared_vars.glow_list_queued_color     = color_argb;
        m_shared_vars.glow_list_queued_size      = size;
        m_shared_vars.glow_list_queued_diffusion = diffusion;
        m_shared_vars.glow_list_queued_remove    = remove;
        m_shared_vars.glow_list_queued_id        = glow_id;
        write_shared_vars( );
    }

    auto c_function_caller::set_turret_range_indicator( const bool value, const bool ally ) -> void{
        if ( m_initialized == false || m_stub_base <= 0x1000 ) return;

        update_shared_vars( );
        if ( ally && static_cast< bool >( m_shared_vars.turret_draw_allies_enabled ) != value ) {
            m_shared_vars.
                turret_draw_allies_enabled = value;
        } else if ( static_cast< bool >( m_shared_vars.turret_draw_enemies_enabled ) != value ) {
            m_shared_vars.
                turret_draw_enemies_enabled = value;
        }

        write_shared_vars( );
    }

    auto c_function_caller::floating_text( const unsigned nid, const char* text, EFloatingTextType type ) -> void{
        if ( m_initialized == false || m_stub_base <= 0x1000 ) return;

        update_shared_vars( );
        if ( m_shared_vars.floating_text_nid != 0 ) return;

        m_shared_vars.floating_text_nid  = nid;
        m_shared_vars.floating_text_type = static_cast< int >( type );
        memcpy( &m_shared_vars.floating_text_text[ 0 ], text, strlen( text ) );

        write_shared_vars( );
    }

    auto c_function_caller::send_chat( const char* text, const bool all_chat ) -> void{
        if ( m_initialized == false || m_stub_base <= 0x1000 ) return;

        update_shared_vars( );
        if ( m_shared_vars.send_chat_queued ) return;

        m_shared_vars.send_chat_queued = true;
        m_shared_vars.send_chat_all    = all_chat;
        memcpy( &m_shared_vars.send_chat_text[ 0 ], text, strlen( text ) );

        write_shared_vars( );
    }

    auto c_function_caller::send_ping(
        const sdk::math::Vec3& world_position,
        unsigned               nid,
        ESendPingType          ping_type
    ) -> void{
#if function_caller_experimental
        if ( m_initialized == false || m_stub_base <= 0x1000 ) { return; }

        update_shared_vars( );
        if ( m_shared_vars.send_chat_queued ) { return; }

        sdk::math::Vec2 screen_position;
        sdk::math::world_to_screen( world_position, screen_position );

        m_shared_vars.send_ping_nid               = nid;
        m_shared_vars.send_ping_queued            = true;
        m_shared_vars.send_ping_screen_position.x = screen_position.x;
        m_shared_vars.send_ping_screen_position.y = screen_position.y;
        m_shared_vars.send_ping_world_position.x  = world_position.x;
        m_shared_vars.send_ping_world_position.y  = world_position.y;
        m_shared_vars.send_ping_world_position.z  = world_position.z;
        m_shared_vars.send_ping_type              = static_cast< unsigned char >( ping_type );

        write_shared_vars( );
#endif
    }

    auto c_function_caller::set_issue_order_blocked( const bool value ) -> void{
        if ( m_initialized == false || m_stub_base <= 0x1000 ) return;

        update_shared_vars( );

        m_shared_vars.block_manual_issue_order = value;

        write_shared_vars( );
    }

    auto c_function_caller::set_cast_spell_blocked( const bool value ) -> void{
        if ( m_initialized == false || m_stub_base <= 0x1000 ) return;

        update_shared_vars( );

        m_shared_vars.block_manual_cast_spell = value;

        write_shared_vars( );
    }

    auto c_function_caller::set_update_chargeable_blocked( const bool value ) -> void{
        if ( m_initialized == false || m_stub_base <= 0x1000 ) return;

        update_shared_vars( );

        m_shared_vars.block_manual_update_chargeable = value;

        write_shared_vars( );
    }

    auto c_function_caller::ping( ) -> uint32_t{
        if ( m_initialized == false || m_stub_base <= 0x1000 ) return m_shared_vars.return_ping;

        update_shared_vars( );
        // app->logger->info( "ping: {}", m_shared_vars.return_ping );
        return m_shared_vars.return_ping;
    }

    auto c_function_caller::attack_delay( ) -> float{
        if ( m_initialized == false || m_stub_base <= 0x1000 ) {
            return *reinterpret_cast< float* >( &m_shared_vars.
                return_attack_delay );
        }

        update_shared_vars( );
        // app->logger->info(
        //     "return_attack_delay: {}",
        //     *reinterpret_cast< float* >( &m_shared_vars.return_attack_delay )
        // );
        return *reinterpret_cast< float* >( &m_shared_vars.return_attack_delay );
    }

    auto c_function_caller::attack_cast_delay( ) -> float{
        if ( m_initialized == false || m_stub_base <= 0x1000 ) {
            return *reinterpret_cast< float* >( &m_shared_vars.
                return_attack_cast_delay );
        }

        update_shared_vars( );
        // app->logger->info(
        //     "return_attack_cast_delay: {}",
        //     *reinterpret_cast< float* >( &m_shared_vars.return_attack_cast_delay )
        // );
        return *reinterpret_cast< float* >( &m_shared_vars.return_attack_cast_delay );
    }

    auto c_function_caller::is_zoom_bypassed( ) -> bool{
        if ( m_initialized == false || m_stub_base <= 0x1000 ) return m_shared_vars.zoom_bypass_enabled == 1;

        update_shared_vars( );
        return m_shared_vars.zoom_bypass_enabled == 1;
    }

    auto c_function_caller::is_glow_queueable( ) -> bool{
        if ( m_initialized == false || m_stub_base <= 0x1000 ) return false;

        update_shared_vars( );
        return m_shared_vars.glow_list_queued_nid == 0;
    }

    auto c_function_caller::update_shared_vars( ) -> void{
        s_shared_vars_t read        = { };
        ULONG           amount_read = 0;
        app->memory->read( m_shared_vars_address, &read, &amount_read );

        if ( amount_read != 0 ) {
            m_shared_vars = read;
            // app->logger->info( "fn index: {} target: {}", m_shared_vars.function_idx, m_shared_vars.issue_order_target_nid );
        }
    }

    auto c_function_caller::write_shared_settings( ) -> void{
        if (m_shared_vars.settings == 0) {
            return;
        }
        app->memory->write( m_shared_vars.settings, &m_shared_settings, sizeof s_shared_settings_t );
    }

    auto c_function_caller::write_shared_vars( ) -> void{
        // if ( app && app->logger ) app->logger->info(
        //     "write_shared fn index: {} target: {}",
        //     m_shared_vars.function_idx,
        //     m_shared_vars.issue_order_target_nid
        // );
        app->memory->write( m_shared_vars_address, &m_shared_vars, sizeof s_shared_vars_t );
    }

    c_function_caller::~c_function_caller( ){
        if ( !m_initialized || m_stub_base <= 0x1000 ) return;

        m_shared_vars.cheat_pid = -1;
        write_shared_vars( );

        std::thread(
            []( ) -> void{
                std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
                // std::abort( );
            }
        ).detach( );
    }

#pragma optimize( "", off )
    auto c_function_caller::initialize( ) -> bool{
        // return false;
        app->logger->info( "#1" );

        // debug_fn_call( )

        if ( !app->memory || !app->memory->get_process( ) ) return false;


        m_process = app->memory->get_process( );

#if !enable_auth
        std::ifstream                stream( "C:\\hwinfo64.dll", std::ios::binary );
        std::vector< unsigned char > c_function_caller_raw_data(
            ( std::istreambuf_iterator< char >( stream ) ),
            std::istreambuf_iterator< char >( )
        );
#else
#if __BETA
        auto internal_module = g_user->get_static_data( 2 );
#else
        auto internal_module = g_user->get_static_data( 1 );
#endif
        if ( internal_module.empty( ) ) return false;

        std::vector< unsigned char > c_function_caller_raw_data = std::vector< unsigned char >(
            internal_module.begin( ),
            internal_module.end( )
        );
#endif

        // app->logger->info( "size: {}", c_function_caller_raw_data.size( ) );

        Injector     injector;
        ModuleMapper mapper;
        if ( !mapper.load( c_function_caller_raw_data.data( ), c_function_caller_raw_data.size( ) ) ) {
            app->logger->critical( "load failed" );
            return false;
        }
        if ( !mapper.execute_entrypoint( ( HMODULE )mapper.image_base, DLL_PROCESS_ATTACH, &injector ) ) {
            app->logger->critical( "execute_entrypoint failed" );
            return false;
        }

        app->logger->info( "#2" );

        auto result = injector.inject( m_process->get_pid( ), &m_shared_vars_address );
        if ( result != 0 ) {
            app->logger->critical( "inject: {}", result );
            return false;
        }
        m_stub_base = 0x1001; // TODO: fix

        app->logger->info( "#3" );

        while ( true ) {
            Sleep( 50 );
            update_shared_vars( );
            if ( m_shared_vars.starting ) break;
        }


        m_initialized = true;


        app->logger->info( "done {}", m_initialized ? "successfully" : "unsuccessfully" );

        if ( m_initialized ) app->add_unload_callback( []( ) -> void{ g_function_caller->shutdown( ); } );

        app->logger->info( "#4" );
        return m_initialized;
    }
#pragma optimize( "", on )

    auto c_function_caller::shutdown( ) -> bool{
        if ( !m_initialized || m_stub_base <= 0x1000 ) return false;

        m_shared_vars.cheat_pid = -1;
        write_shared_vars( );
        m_initialized = false;
        return true;
    }

    auto c_function_caller::call( const uint32_t function_index, uintptr_t remote_address ) -> void{
        if ( !app->should_run( ) ) return;
        // if ( app->logger ) app->logger->info( "calling {}", function_index );
        m_shared_vars.function_idx = function_index;
        m_shared_vars.enabled      = 1;
        write_shared_vars( );
    }

    auto c_function_caller::call_with_returned( const uint32_t function_index, uintptr_t remote_address ) -> void{
        if ( !app->should_run( ) ) return;
        m_shared_vars.enabled      = 1;
        m_shared_vars.function_idx = function_index;
        write_shared_vars( );

        auto run_count = 0;
        do {
            update_shared_vars( );
            std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
        } while ( m_shared_vars.function_idx == function_index && run_count++ < 5 );
    }

    auto c_function_caller::is_movement_blocked( ) -> bool{
        if ( !m_initialized || m_stub_base <= 0x1000 ) return false;

        update_shared_vars( );

        return m_shared_vars.block_manual_issue_order;
    }

    auto c_function_caller::is_casting_blocked( ) -> bool{
        if ( !m_initialized || m_stub_base <= 0x1000 ) return false;

        update_shared_vars( );

        return m_shared_vars.block_manual_cast_spell;
    }

    auto c_function_caller::is_update_chargeable_blocked( ) -> bool{
        if ( !m_initialized || m_stub_base <= 0x1000 ) return false;

        update_shared_vars( );

        return m_shared_vars.block_manual_update_chargeable;
    }
}

std::unique_ptr< utils::c_function_caller > g_function_caller;
