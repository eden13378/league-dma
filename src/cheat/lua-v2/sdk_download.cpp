#include "pch.hpp"
#include "sdk_download.hpp"

#include <string>


namespace lua {
    const std::string GIT_DEFAULT_LOCATION = "C:\\Program Files\\Git\\cmd\\git.exe";

    auto SdkDownloader::download_sdk( ) -> void{
        if ( check_git_default_location( ) ) { return clone_sdk( GIT_DEFAULT_LOCATION ); }
        if ( check_git_in_path( ) ) return clone_sdk( "git" );
    }

    auto SdkDownloader::clone_sdk( std::string git_path ) -> void{
        const std::string out_path = std::format(
            ( "C:\\{}\\lua\\types" ),
            user::c_hwid_system( ).get_hwid_short_form( )
        );

        if ( std::filesystem::exists( out_path ) ) {
            exec( std::format( "{} reset --hard", git_path ), out_path );
            exec( std::format( "HEAD && {} pull", git_path ), out_path );
        } else {
            exec( std::format( "{} clone https://github.com/xbtog/lua-sdk {}", git_path, out_path ), std::nullopt );
        }
    }

    auto SdkDownloader::check_git_default_location( ) -> bool{
        if ( std::filesystem::exists( GIT_DEFAULT_LOCATION ) ) return true;

        return false;
    }

    auto SdkDownloader::check_git_in_path( ) -> bool{
        return false;
        // std::string which_cmd = "which";
        // auto        w_cmd     = std::wstring( which_cmd.begin( ), which_cmd.end( ) );
        //
        // SHELLEXECUTEINFO ShExecInfo = { 0 };
        // ShExecInfo.cbSize           = sizeof( SHELLEXECUTEINFO );
        // ShExecInfo.fMask            = SEE_MASK_NOCLOSEPROCESS;
        // ShExecInfo.hwnd             = nullptr;
        // ShExecInfo.lpVerb           = nullptr;
        // ShExecInfo.lpFile           = w_cmd.data( );
        // ShExecInfo.lpParameters     = L"git";
        // ShExecInfo.lpDirectory      = nullptr;
        // ShExecInfo.nShow            = SW_SHOW;
        // ShExecInfo.hInstApp         = nullptr;
        // ShellExecuteEx( &ShExecInfo );
        // WaitForSingleObject( ShExecInfo.hProcess,INFINITE );
        //
        // DWORD exit_code = 1;
        // GetExitCodeProcess( ShExecInfo.hProcess, &exit_code );
        //
        // return exit_code == 0;
    }

    auto SdkDownloader::exec( std::string command, const std::optional< std::string >& working_dir ) -> bool{
        // additional information
        STARTUPINFOA        si;
        PROCESS_INFORMATION pi;

        // set the size of the structures
        ZeroMemory( &si, sizeof( si ) );
        si.cb = sizeof( si );
        ZeroMemory( &pi, sizeof( pi ) );

        // start the program up
        if ( CreateProcessA(
                nullptr,
                static_cast< char* >( command.data( ) ),
                nullptr,
                nullptr,
                FALSE,
                DETACHED_PROCESS,
                nullptr,
                working_dir ? working_dir->data( ) : nullptr,
                &si,
                &pi
            ) == FALSE
        )
            return false;

        // Close process and thread handles.
        CloseHandle( pi.hProcess );
        CloseHandle( pi.hThread );

        return true;
    }
}
