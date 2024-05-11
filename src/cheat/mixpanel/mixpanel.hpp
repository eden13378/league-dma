#pragma once
#include <string>

namespace mixpanel {
    class Mixpanel {
    public:
        enum class ETrackEventError {
            invalid_type = 1,
            unknown,
            mixpanel_is_null,
            exception_thrown
        };

        class Event {
        public:
            Event(
                std::string                                       name,
                std::vector< std::pair< std::string, std::any > > values
            ): name{
                    std::move( name )
                },
                values{ std::move( values ) }{
            }

            std::string                                       name;
            std::vector< std::pair< std::string, std::any > > values;
        };

        class TrackTimeEvent {
        public:
            explicit TrackTimeEvent( const std::string& event_name, Mixpanel* parent ) : m_event_name{ event_name },
                m_parent{ parent }{ m_start_time = std::chrono::high_resolution_clock::now( ); }

            explicit TrackTimeEvent(
                const std::string&                                       event_name,
                const std::vector< std::pair< std::string, std::any > >& additional_data,
                Mixpanel*                                                parent
            ) : m_event_name{ event_name }, m_additional_values{ additional_data }, m_parent{ parent }{
                m_start_time = std::chrono::high_resolution_clock::now( );
            }

            auto done( ) -> std::expected< void, ETrackEventError >{
                if ( !m_parent ) return std::unexpected( ETrackEventError::mixpanel_is_null );

                const auto delta_time = static_cast< float >( std::chrono::duration_cast< std::chrono::microseconds >(
                    std::chrono::high_resolution_clock::now( ) - m_start_time
                ).count( ) ) / 1000.f;

                m_additional_values.push_back( { "took_time", std::make_any< float >( delta_time ) } );
                m_parent->track_event( m_event_name, m_additional_values );
                return { };
            }

        private:
            std::string                                       m_event_name{ };
            std::vector< std::pair< std::string, std::any > > m_additional_values{ };
            std::chrono::high_resolution_clock::time_point    m_start_time{ };
            Mixpanel*                                         m_parent{ };
        };

        explicit Mixpanel( const std::string& token ): m_token{ token }{
        }


        auto track_event(
            std::string                                       event_name,
            std::vector< std::pair< std::string, std::any > > values
        ) -> std::expected< void, ETrackEventError >;

        auto track_time(
            std::string                                       event_name,
            std::vector< std::pair< std::string, std::any > > values
        ) -> std::shared_ptr< TrackTimeEvent >;

        auto track_events( std::vector< Event > events ) -> void;

        auto set_username( const std::string& username ) -> void{ m_username = username; }
        auto set_user_id( const int32_t user_id ) -> void{ m_user_id = user_id; }

    private:
        auto send( const nlohmann::json& json ) -> void;

        auto send_events( const std::vector< Event >& events ) -> void;

        auto build_event( const Event& event ) -> nlohmann::json;

    private:
        std::string m_token{ };
        std::string m_username{ };
        int32_t     m_user_id{ };
    };
}
