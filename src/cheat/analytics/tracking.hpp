#pragma once
#include <chrono>
#include <string_view>

#include "../security/src/xorstr.hpp"
#include <nlohmann/json.hpp>

namespace analytics::tracking {
    struct BaseTracker {
        virtual      ~BaseTracker( ) = default;
        virtual auto as_json( ) -> nlohmann::json{ return nlohmann::json{ }; }
        virtual auto get_type( ) -> std::string{ return "base"; }

        auto is_ready( ) const -> bool{ return m_ready; }

    protected:
        bool m_ready{ };
    };

    struct DurationTracker final : BaseTracker {
        ~DurationTracker( ) override = default;
        DurationTracker( ){ m_start = std::chrono::high_resolution_clock::now( ); }

        auto as_json( ) -> nlohmann::json override{
            return nlohmann::json{
                { _( "start" ), m_start.time_since_epoch( ).count( ) },
                { _( "end" ), m_end.time_since_epoch( ).count( ) },
                { _( "duration" ), std::chrono::duration_cast< std::chrono::milliseconds >( m_end - m_start ).count( ) }
            };
        }

        auto get_type( ) -> std::string override{ return _( "duration" ); }

        auto done( ) -> void{
            m_end   = std::chrono::high_resolution_clock::now( );
            m_ready = true;
        }

    private:
        std::chrono::time_point< std::chrono::high_resolution_clock > m_start;
        std::chrono::time_point< std::chrono::high_resolution_clock > m_end;
    };

    struct ProcessCpuUsageTracker final : BaseTracker {
        ~ProcessCpuUsageTracker( ) override = default;

        ProcessCpuUsageTracker( ){
            FILETIME creation_time, exit_time, kernel_time, user_time;
            auto     t = GetProcessTimes( GetCurrentProcess( ), &creation_time, &exit_time, &kernel_time, &user_time );

            if(!t) return;

            // todo: implement this

            // user_time.dwHighDateTime
        }

        auto as_json( ) -> nlohmann::json override{
            // return nlohmann::json{
            //     { _( "start" ), m_start.time_since_epoch( ).count( ) },
            //     { _( "end" ), m_end.time_since_epoch( ).count( ) },
            //     { _( "duration" ), std::chrono::duration_cast< std::chrono::milliseconds >( m_end - m_start ).count( ) }
            // };
            return {};
        }

        auto get_type( ) -> std::string override{ return _( "duration" ); }

        auto done( ) -> void{
            // m_end   = std::chrono::high_resolution_clock::now( );
            m_ready = true;
        }

    private:
        float m_general_usage{ };
        float m_process_usage{ };
        // std::chrono::time_point< std::chrono::high_resolution_clock > m_start;
        // std::chrono::time_point< std::chrono::high_resolution_clock > m_end;
    };

    struct AverageDurationTracker : BaseTracker {
    private:
        struct Entry {
            Entry( ): m_start( std::chrono::high_resolution_clock::now( ) ){
            }

            auto as_json( ) const -> nlohmann::json{
                return nlohmann::json{
                    { _( "start" ), m_start.time_since_epoch( ).count( ) },
                    { _( "end" ), m_end.time_since_epoch( ).count( ) },
                    {
                        _( "duration" ),
                        std::chrono::duration_cast< std::chrono::milliseconds >( m_end - m_start ).count( )
                    }
                };
            }

            auto is_ready( ) const -> bool{ return m_ready; }

            auto done( ) -> void{
                m_end   = std::chrono::high_resolution_clock::now( );
                m_ready = true;
            }

        private:
            std::chrono::time_point< std::chrono::high_resolution_clock > m_start{ };
            std::chrono::time_point< std::chrono::high_resolution_clock > m_end{ };
            bool                                                          m_ready{ };
        };

    public:
        ~AverageDurationTracker( ) override = default;
        explicit AverageDurationTracker( size_t count ): m_count( count ){ m_entries.emplace_back( ); }

        auto start_new( ) -> void{
            if ( m_entries.size( ) < m_count ) { m_entries.emplace_back( ); } else {
                throw std::runtime_error(
                    _( "AverageDurationTracker::start_new: max count reached" )
                );
            }
        }

        auto done( ) -> void{
            m_entries.back( ).done( );

            if ( m_entries.size( ) >= m_count ) m_ready = true;
        }

        auto get_type( ) -> std::string override{ return _( "average_duration" ); }

        auto as_json( ) -> nlohmann::json override{
            nlohmann::json j;
            for ( auto entry : m_entries ) { j.push_back( entry.as_json( ) ); }

            return j;
        }

    private:
        std::vector< Entry > m_entries;
        size_t               m_count;
    };

    auto track_duration( std::string_view event_name ) -> DurationTracker*;
    auto track_average_duration( std::string_view event_name, size_t count ) -> AverageDurationTracker*;
}
