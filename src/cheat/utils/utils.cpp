#include "pch.hpp"

#include "utils.hpp"

#if enable_sentry
#define SENTRY_BUILD_STATIC 1
#include <sentry.h>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "version.lib")
#endif

#include "debug_logger.hpp"
// #include "../sdk/g_unload.hpp"

#pragma pack(push,8)
using THREADNAME_INFO = struct tagTHREADNAME_INFO {
    DWORD  dwType; // Must be 0x1000.
    LPCSTR szName; // Pointer to name (in user addr space).
    DWORD  dwThreadID; // Thread ID (-1=caller thread).
    DWORD  dwFlags; // Reserved for future use, must be zero.
};
#pragma pack(pop)

namespace utils {
    auto enable_debug_privilege( ) -> bool{
        try {
            TOKEN_PRIVILEGES tp;
            LUID             luid;
            HANDLE           h_token;
            if ( !OpenProcessToken(
                GetCurrentProcess( ),
                TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                &h_token
            ) ) {
                debug_log( "OpenProcessToken error: {}", GetLastError( ) );
                return false;
            }

            if ( !LookupPrivilegeValueA(
                nullptr,
                "SeDebugPrivilege",
                &luid
            ) ) {
                debug_log( "LookupPrivilegeValue error: {}", GetLastError( ) );
                return false;
            }

            tp.PrivilegeCount             = 1;
            tp.Privileges[ 0 ].Luid       = luid;
            tp.Privileges[ 0 ].Attributes = SE_PRIVILEGE_ENABLED;

            if ( !AdjustTokenPrivileges(
                h_token,
                FALSE,
                &tp,
                sizeof( TOKEN_PRIVILEGES ),
                nullptr,
                nullptr
            ) ) {
                debug_log( "AdjustTokenPrivileges error: {}", GetLastError( ) );
                return false;
            }

            if ( GetLastError( ) == ERROR_NOT_ALL_ASSIGNED ) {
                debug_log( "The token does not have the specified privilege." );
                return false;
            }


            return true;
        } catch ( ... ) { return false; }
    }

    auto get_focused_process_pid( ) -> unsigned long{
        try {
            unsigned long pid;

            GetWindowThreadProcessId( GetForegroundWindow( ), &pid );

            return pid;
        } catch ( ... ) { return 0; }
    }

    auto start_process( const std::string& path, const std::string& args ) -> bool{
        try {
            // additional information
            STARTUPINFOA        si;
            PROCESS_INFORMATION pi;

            // set the size of the structures
            ZeroMemory( &si, sizeof( si ) );
            si.cb = sizeof( si );
            ZeroMemory( &pi, sizeof( pi ) );

            auto cmd = path + std::string( " " ) + args;

            // start the program up
            if ( CreateProcessA(
                    nullptr,
                    static_cast< char* >( cmd.data( ) ),
                    nullptr,
                    nullptr,
                    FALSE,
                    DETACHED_PROCESS,
                    nullptr,
                    nullptr,
                    &si,
                    &pi
                ) == FALSE
            )
                return false;

            // Close process and thread handles.
            CloseHandle( pi.hProcess );
            CloseHandle( pi.hThread );

            return true;
        } catch ( ... ) { return false; }
    }

    auto read_file_to_string( const std::string& path ) -> std::expected< std::string, const char* >{
        try {
            std::ifstream stream;
            stream.open( path );

            std::stringstream ss;
            ss << stream.rdbuf( );

            return ss.str( );
        } catch ( ... ) { return std::unexpected( "error reading file contents" ); }
    }

    auto set_thread_name( const uint32_t thread_id, const char* thread_name ) -> void{
        try {
            const DWORD MS_VC_EXCEPTION = 0x406D1388;
            // DWORD dwThreadID = ::GetThreadId( static_cast<HANDLE>( t.native_handle() ) );

            THREADNAME_INFO info;
            info.dwType     = 0x1000;
            info.szName     = thread_name;
            info.dwThreadID = thread_id;
            info.dwFlags    = 0;

            RaiseException( MS_VC_EXCEPTION, 0, sizeof( info ) / sizeof( ULONG_PTR ), ( ULONG_PTR* )&info );
        } catch ( ... ) {
        }
    }

    auto set_thread_name( std::thread* thread, const char* thread_name ) -> void{
        try {
            const auto thread_id = ::GetThreadId( static_cast< HANDLE >( thread->native_handle( ) ) );
            set_thread_name( thread_id, thread_name );
        } catch ( ... ) {
        }
    }

    auto set_thread_name( std::thread::id thread, const char* thread_name ) -> void{
        set_thread_name( reinterpret_cast< uint32_t >( &thread ), thread_name );
    }

    auto set_current_thread_priority( const int priority ) -> bool{
        try {
            const auto handle = GetCurrentThread( );

            return SetPriorityClass( handle, priority ) == 0;
        } catch ( ... ) { return false; }
    }
}
