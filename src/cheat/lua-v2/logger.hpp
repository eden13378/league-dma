#pragma once

#include <spdlog/logger.h>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>

// #include "../utils/path.hpp"

namespace lua {
    class Logger {
    public:
        Logger( ){
            auto hwid = user::c_hwid_system( ).get_hwid_short_form( );

            try {
                const auto log_path = std::format(
                    ( "C:\\{}\\lua_log.log" ),
                    user::c_hwid_system( ).get_hwid_short_form( )
                );
                if ( std::filesystem::exists( log_path ) ) std::filesystem::remove( log_path );

                // m_logger = spdlog::basic_logger_mt< spdlog::async_factory >(
                //     "lua",
                //     log_path
                // );
                // if ( m_logger ) m_logger->flush_on( spdlog::level::info );


                m_thread_pool = std::make_shared< spdlog::details::thread_pool >( 8192, 1 );

                const auto console_sink = std::make_shared< spdlog::sinks::stdout_color_sink_mt >( );
                console_sink->set_level( spdlog::level::info );
                console_sink->set_pattern( "[%^%l%$] %v" );

                auto file_sink = std::make_shared< spdlog::sinks::basic_file_sink_mt >( log_path );
                file_sink->set_level( spdlog::level::info );

                std::vector< spdlog::sink_ptr > sinks = { console_sink, file_sink };

                m_logger = std::make_shared< spdlog::async_logger >(
                    "lua",
                    sinks.begin( ),
                    sinks.end( ),
                    m_thread_pool,
                    spdlog::async_overflow_policy::block
                );

                if ( m_logger ) { m_logger->flush_on( spdlog::level::info ); }
            } catch ( ... ) {
            }
        }

        template <typename... Args>
        auto error( spdlog::format_string_t< Args... > fmt, Args&&... args ) -> void{
            if ( !m_logger ) return;

            m_logger->error( fmt, std::forward< Args >( args )... );
            m_logger->flush( );
        }

        template <typename T>
        void error( const T& msg ){
            if ( !m_logger ) return;

            m_logger->log( spdlog::level::err, msg );
            m_logger->flush( );
        }


        template <typename... Args>
        auto info( spdlog::format_string_t< Args... > fmt, Args&&... args ) -> void{
            if ( !m_logger ) return;

            m_logger->info( fmt, std::forward< Args >( args )... );
        }

        template <typename T>
        void info( const T& msg ){
            if ( !m_logger ) return;

            m_logger->log( spdlog::level::info, msg );
        }

        template <typename... Args>
        auto warn( spdlog::format_string_t< Args... > fmt, Args&&... args ) -> void{
            if ( !m_logger ) return;

            m_logger->warn( fmt, std::forward< Args >( args )... );
            m_logger->flush( );
        }

        template <typename T>
        void warn( const T& msg ){
            if ( !m_logger ) return;

            m_logger->log( spdlog::level::warn, msg );
            m_logger->flush( );
        }

    private:
        std::shared_ptr< spdlog::logger >               m_logger{ };
        std::shared_ptr< spdlog::details::thread_pool > m_thread_pool{ };
    };
}

extern std::unique_ptr< lua::Logger > lua_logger;
