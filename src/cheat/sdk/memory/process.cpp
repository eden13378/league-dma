#include "pch.hpp"

#include "process.hpp"

#include <algorithm>
#include <locale>

#include "memory.hpp"

#include "../../DMALibrary/Memory/Memory.h"

namespace sdk::memory {
    void get_all_windows_from_process_id( DWORD dwProcessID, std::vector< HWND >& vhWnds ){
        // find all hWnds (vhWnds) associated with a process id (dwProcessID)
        try {
            HWND hCurWnd = NULL;
            do {
                hCurWnd = FindWindowEx( NULL, hCurWnd, NULL, NULL );
                DWORD dwProcID = 0;
                GetWindowThreadProcessId( hCurWnd, &dwProcID );
                if ( dwProcID == dwProcessID ) vhWnds.push_back( hCurWnd );  // add the found hCurWnd to the vector
            }
            while ( hCurWnd != NULL );
        }
        catch ( ... ) {
            return;
        }
    }

    Process::Process( const std::string& process_name ){
        m_process_name = process_name;
        DMA.Init( m_process_name, true, true );
    }

    Process::~Process( ){
        DMA.Shutdown( );
    }

    auto Process::cache_modules( ) -> void{
        try {
            m_modules.clear( );

            const auto modules = DMA.GetModuleList( m_process_name );

            for ( auto& module : modules ) {
                if ( !m_modules.contains( rt_hash( module.data( ) ) ) ) {
                    const auto mod = DMA.GetModuleInformation( module );

                    m_modules[ rt_hash( module.data( ) ) ] =
                        std::make_shared< Module >( mod.base_address, mod.base_size, rt_hash( module.data( ) ) );
                }
            }
        }
        catch ( ... ) {
        }
    }

    auto Process::inject_dll( const char* path ) const -> bool{
        try {
            if ( !path ) return false;

            const SIZE_T len = strlen( path );
            void* allocated = VirtualAllocEx( get_handle( ), nullptr, len, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE );
            if ( !allocated ) return false;

            if ( !app->memory->write( reinterpret_cast< uintptr_t >( allocated ), ( void* )path, len ) ) {
                VirtualFreeEx( get_handle( ), allocated, len, MEM_DECOMMIT );

                return false;
            }

            const auto k32 = get_module( ct_hash( "kernel32.dll" ) );
            if ( k32 == nullptr ) {
                VirtualFreeEx( get_handle( ), allocated, len, MEM_DECOMMIT );

                return false;
            }

            const void* load_lib = reinterpret_cast< void* >( GetProcAddress(
                reinterpret_cast< HMODULE >( k32->get_base( ) ),
                "LoadLibraryA"
            ) );
            if ( !load_lib ) {
                VirtualFreeEx( get_handle( ), allocated, len, MEM_DECOMMIT );

                return false;
            }

            const HANDLE thread = CreateRemoteThread(
                get_handle( ),
                nullptr,
                0,
                reinterpret_cast< LPTHREAD_START_ROUTINE >( const_cast< void* >( load_lib ) ),
                allocated,
                0,
                nullptr
            );
            if ( thread == INVALID_HANDLE_VALUE ) {
                VirtualFreeEx( get_handle( ), allocated, len, MEM_DECOMMIT );

                return false;
            }

            DWORD result;
            WaitForSingleObject( thread, INFINITE );
            if ( !GetExitCodeThread( thread, &result ) ) {
                CloseHandle( thread );
                VirtualFreeEx( get_handle( ), allocated, len, MEM_DECOMMIT );

                return false;
            }

            if ( !result ) {
                CloseHandle( thread );
                VirtualFreeEx( get_handle( ), allocated, len, MEM_DECOMMIT );

                return false;
            }

            CloseHandle( thread );
            VirtualFreeEx( get_handle( ), allocated, len, MEM_DECOMMIT );

            return true;
        }
        catch ( ... ) {
            return false;
        }
    }

    auto Process::is_running( ) const -> bool{
        try {
            DWORD exit_code;
            if ( GetExitCodeProcess( get_handle( ), &exit_code ) ) return exit_code == STILL_ACTIVE;

            return false;
        }
        catch ( ... ) {
            return false;
        }
    }
}
