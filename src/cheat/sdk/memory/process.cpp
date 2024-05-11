#include "pch.hpp"

#include "process.hpp"

#include <algorithm>
#include <locale>

#include "memory.hpp"

namespace sdk::memory {
    void get_all_windows_from_process_id( DWORD dwProcessID, std::vector< HWND >& vhWnds ){
        // find all hWnds (vhWnds) associated with a process id (dwProcessID)
        try {
            HWND hCurWnd = NULL;
            do {
                hCurWnd        = FindWindowEx( NULL, hCurWnd, NULL, NULL );
                DWORD dwProcID = 0;
                GetWindowThreadProcessId( hCurWnd, &dwProcID );
                if ( dwProcID == dwProcessID ) vhWnds.push_back( hCurWnd );  // add the found hCurWnd to the vector
            } while ( hCurWnd != NULL );
        } catch ( ... ) { return; }
    }

    Process::Process( hash_t process_name ){
        try {
            DWORD pid = 0;

            const auto     snapshot = ( CreateToolhelp32Snapshot )( TH32CS_SNAPPROCESS, 0 );
            PROCESSENTRY32 process;
            ZeroMemory( &process, sizeof process );
            process.dwSize = sizeof process;

            if ( ( Process32First )( snapshot, &process ) ) {
                do {
                    if ( std::wstring tmp = process.szExeFile; rt_hash( utf8_encode(tmp).c_str( ) ) == process_name ) {
                        pid = process.th32ProcessID;
                        std::vector< HWND > windows;
                        get_all_windows_from_process_id( pid, windows );

                        m_windows = windows;
                        break;
                    }
                } while ( ( Process32Next )( snapshot, &process ) );
            }

            CloseHandle( snapshot );

            m_pid    = pid;
            m_handle = OpenProcess( PROCESS_ALL_ACCESS, FALSE, pid );
            cache_modules( );
        } catch ( ... ) {
        }
    }

    auto Process::cache_modules( ) -> void{
        try {
            m_modules.clear( );

            MODULEENTRY32W mod;
            mod.dwSize = sizeof( MODULEENTRY32W );

            const auto snap = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE, m_pid );

            if ( !snap ) return;

            if ( Module32FirstW( snap, &mod ) ) {
                do {
                    try {
                        auto ms = utf8_encode( mod.szModule );
                        std::ranges::transform( ms, ms.begin( ), tolower );

                        if ( !m_modules.contains( rt_hash( ms.data( ) ) ) ) {
                            m_modules[ rt_hash( ms.data( ) ) ] =
                                std::make_shared< Module >( mod, rt_hash( ms.data( ) ) );
                        }
                    } catch ( ... ) {
                    }
                } while ( Module32NextW( snap, &mod ) );
            }

            CloseHandle( snap );
        } catch ( ... ) {
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
        } catch ( ... ) { return false; }
    }

    auto Process::is_running( ) const -> bool{
        try {
            DWORD exit_code;
            if ( GetExitCodeProcess( get_handle( ), &exit_code ) ) return exit_code == STILL_ACTIVE;

            return false;
        } catch ( ... ) { return false; }
    }
}
