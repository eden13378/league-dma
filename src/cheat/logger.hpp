#pragma once

#include <spdlog/logger.h>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/details/thread_pool.h>


// #include "../utils/path.hpp"

class Logger {
public:
    Logger( ){
        auto hwid = user::c_hwid_system( ).get_hwid_short_form( );

        try {
            const auto log_path = std::format(
                ( "C:\\{}\\log.log" ),
                user::c_hwid_system( ).get_hwid_short_form( )
            );
            if ( std::filesystem::exists( log_path ) ) std::filesystem::remove( log_path );


            const auto console_sink = std::make_shared< spdlog::sinks::stdout_color_sink_mt >( );
            console_sink->set_level( spdlog::level::info );
            console_sink->set_pattern( "[%^%l%$] %v" );

            auto file_sink = std::make_shared< spdlog::sinks::basic_file_sink_mt >( log_path );
            file_sink->set_level( spdlog::level::info );

            std::vector< spdlog::sink_ptr > sinks = { console_sink, file_sink };

            m_logger = std::make_shared< spdlog::async_logger >(
                "logger",
                sinks.begin( ),
                sinks.end( ),
                spdlog::thread_pool( ),
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
    }

    template <typename T>
    void error( const T& msg ){
        if ( !m_logger ) return;
        m_logger->log( spdlog::level::err, msg );
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
    }

    template <typename T>
    void warn( const T& msg ){
        if ( !m_logger ) return;
        m_logger->log( spdlog::level::warn, msg );
    }

    template <typename... Args>
    auto critical( spdlog::format_string_t< Args... > fmt, Args&&... args ) -> void{
        if ( !m_logger ) return;

        m_logger->critical( fmt, std::forward< Args >( args )... );
    }

    template <typename T>
    void critical( const T& msg ){
        if ( !m_logger ) return;
        m_logger->log( spdlog::level::critical, msg );
    }

    template <typename... Args>
    auto debug( spdlog::format_string_t< Args... > fmt, Args&&... args ) -> void{
        if ( !m_logger ) return;

        m_logger->debug( fmt, std::forward< Args >( args )... );
    }

    template <typename T>
    void debug( const T& msg ){
        if ( !m_logger ) return;
        m_logger->log( spdlog::level::debug, msg );
    }

    template <typename... Args>
    auto trace( spdlog::format_string_t< Args... > fmt, Args&&... args ) -> void{
        if ( !m_logger ) return;

        m_logger->trace( fmt, std::forward< Args >( args )... );
    }

    template <typename T>
    void trace( const T& msg ){
        if ( !m_logger ) return;
        m_logger->log( spdlog::level::trace, msg );
    }

private:
    std::shared_ptr< spdlog::async_logger > m_logger{ };
};
