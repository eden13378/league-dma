#include "pch.hpp"
#include  "crash_detector.hpp"

#include "registry.hpp"
#include "web_client.hpp"
#include "../globals.hpp"
#include "../version.hpp"



namespace crash_detector {
    auto get_sys_op_type( ) noexcept -> std::expected< double, const char* >{
        try {
            auto              ret = 0.0;
            NTSTATUS (WINAPI *RtlGetVersion)( LPOSVERSIONINFOEXW );
            OSVERSIONINFOEXW  osInfo;

            *( FARPROC* )&RtlGetVersion = GetProcAddress( GetModuleHandleA( "ntdll" ), "RtlGetVersion" );

            if ( NULL != RtlGetVersion ) {
                osInfo.dwOSVersionInfoSize = sizeof( osInfo );
                RtlGetVersion( &osInfo );
                ret = ( double )osInfo.dwMajorVersion;
            }
            return ret;
        } catch ( ... ) { return std::unexpected( "could not get sys op type" ); }
    }

    auto check( ) noexcept -> std::expected< void, const char* >{
        try {
            std::string version;
            HKEY        key;

            const auto res = RegOpenKeyExA(
                HKEY_LOCAL_MACHINE,
                "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                0,
                KEY_READ,
                &key
            );


            if ( !registry::get_string_reg_key( key, "CurrentBuildNumber", version, "0" ) ) return { };

            const auto hwid_short_form = user::c_hwid_system( ).get_hwid_short_form( );
            auto       path            = std::format( ( "C:\\{}\\{}" ), hwid_short_form, "run.lock" );
            if ( !std::filesystem::exists( path ) ) {
                std::ofstream lock( path );
#if __BETA
        lock << "beta_";
#endif
                lock << release_version << std::endl;
                lock.flush( );
                lock.close( );
                return { };
            }

            nlohmann::json body;

            user::c_hwid_system hwid;

            std::ifstream     lock( path );
            std::stringstream ss;
            ss << lock.rdbuf( );
            lock.close( );

#if enable_auth
            if ( app->logger ) app->logger->error( "crash detected v: {}", version );

            body[ "content" ] = std::format(
                "crash detected for user {} cpu {} windows {} version {}",
                g_user ? g_user->get_username( ) : "unknown",
                hwid.get_hwid( ).cpu.name,
                version,
                ss.str( )
            );

            WebClient( ).post(
                _(
                    "https://discord.com/api/webhooks/1079073586104516608/YQdXCm2QIF5z2gphbEulmOCO-3gcM0_Vucqq9CATvoU8XLduiKXIQu4dFM3aLbLqH6On?name=test"
                ),
                body.dump( )
            );

            if ( app && app->sentry ) { app->sentry->track_exception( "crashed last time" ); }
#endif
        } catch ( ... ) { return std::unexpected( "error checking for crash" ); }
        return { };
    }

    auto clean( ) noexcept -> void{
        try {
            debug_fn_call( )
            const auto hwid_short_form = user::c_hwid_system( ).get_hwid_short_form( );
            const auto path            = std::format( ( "C:\\{}\\{}" ), hwid_short_form, "run.lock" );
            if ( !std::filesystem::exists( path ) ) return;

            std::filesystem::remove( path );
        } catch ( ... ) {
            // 
        }
    }
}
