#include "pch.hpp"
#include "mixpanel.hpp"

namespace mixpanel {
    auto Mixpanel::track_event(
        std::string                                       event_name,
        std::vector< std::pair< std::string, std::any > > values
    ) -> std::expected< void, ETrackEventError >{
        try {
            nlohmann::json event;
            event[ "properties" ][ "token" ] = m_token;
            if ( !m_username.empty( ) ) event[ "properties" ][ "distinct_id" ] = m_username;
            if ( m_user_id != 0 ) event[ "properties" ][ "user_id" ] = m_user_id;

            for ( auto& v : values ) {
                const auto hash_code = v.second.type( ).hash_code( );

                if ( hash_code == typeid( int32_t ).hash_code( ) ) {
                    event[ "properties" ][ v.first ] =
                        std::any_cast< int32_t >( v.second );
                } else if ( hash_code == typeid( int64_t ).hash_code( ) ) {
                    event[ "properties" ][ v.first ] =
                        std::any_cast< int64_t >( v.second );
                } else if ( hash_code == typeid( float ).hash_code( ) ) {
                    event[ "properties" ][ v.first ] =
                        std::any_cast< float >( v.second );
                } else if ( hash_code == typeid( double ).hash_code( ) ) {
                    event[ "properties" ][ v.first ] =
                        std::any_cast< double >( v.second );
                } else if ( hash_code == typeid( std::string ).hash_code( ) ) {
                    event[ "properties" ][ v.first ] =
                        std::any_cast< std::string >( v.second );
                } else if ( hash_code == typeid( bool ).hash_code( ) ) {
                    event[ "properties" ][ v.first ] =
                        std::any_cast< bool >( v.second );
                } else return std::unexpected( ETrackEventError::invalid_type );
            }

            event[ "event" ] = event_name;

            nlohmann::json j;
            j.push_back( event );

            std::thread( [j, this]( ) -> void{ send( j ); } ).detach( );
            return { };
        } catch ( std::exception& ex ) {
            if ( app && app->logger ) app->logger->error( "error sending event: {}", ex.what( ) );
            return std::unexpected( ETrackEventError::exception_thrown );
        } catch ( ... ) { return std::unexpected( ETrackEventError::unknown ); }
    }

    auto Mixpanel::track_time(
        std::string                                       event_name,
        std::vector< std::pair< std::string, std::any > > values
    ) -> std::shared_ptr< TrackTimeEvent >{ return std::make_shared< TrackTimeEvent >( event_name, values, this ); }

    auto Mixpanel::track_events( std::vector< Event > events ) -> void{
        std::thread(
            [events, this]( ) -> void{
                std::this_thread::sleep_for( std::chrono::seconds( 10 ) );

                if ( events.size( ) > 50 ) {
                    const auto batches = std::ceil( static_cast< float >( events.size( ) ) / 50.f );

                    for ( int32_t i = 0; i < static_cast< int32_t >( batches ); ++i ) {
                        const auto current_events     = i * 50;
                        const auto last_current_event = std::min(
                            current_events + 50,
                            static_cast< int32_t >( events.size( ) )
                        );

                        const std::vector current(
                            events.begin( ) + current_events,
                            events.begin( ) + last_current_event
                        );

                        send_events( current );
                    }
                    return;
                }

                return send_events( events );
            }
        ).detach( );
    }

    auto Mixpanel::send( const nlohmann::json& json ) -> void{
        CURL* hnd = curl_easy_init( );

        curl_easy_setopt( hnd, CURLOPT_CUSTOMREQUEST, "POST" );
        // curl_easy_setopt( hnd, CURLOPT_WRITEDATA, stdout );
        curl_easy_setopt( hnd, CURLOPT_URL, "https://api-eu.mixpanel.com/track?ip=1&verbose=1" );

        struct curl_slist* headers = NULL;
        headers                    = curl_slist_append( headers, "accept: text/plain" );
        headers                    = curl_slist_append( headers, "content-type: application/json" );
        curl_easy_setopt( hnd, CURLOPT_HTTPHEADER, headers );

        const auto j = json.dump( );
#if __DEBUG
        if ( app && app->logger ) app->logger->info( "Sending json: {}", j );
#endif
        curl_easy_setopt( hnd, CURLOPT_POSTFIELDS, j.data( ) );

        CURLcode ret = curl_easy_perform( hnd );
    }

    auto Mixpanel::send_events( const std::vector< Event >& events ) -> void{
        if ( events.size( ) > 50 ) return;

        nlohmann::json j;

        for ( auto& event : events ) { j.push_back( build_event( event ) ); }

        send( j );
    }

    auto Mixpanel::build_event( const Event& event ) -> nlohmann::json{
        nlohmann::json j_event;
        j_event[ "properties" ][ "token" ] = m_token;
        if ( !m_username.empty( ) ) j_event[ "properties" ][ "distinct_id" ] = m_username;
        if ( m_user_id != 0 ) j_event[ "properties" ][ "user_id" ] = m_user_id;

        for ( auto& v : event.values ) {
            const auto hash_code = v.second.type( ).hash_code( );

            if ( hash_code == typeid( int32_t ).hash_code( ) ) {
                j_event[ "properties" ][ v.first ] =
                    std::any_cast< int32_t >( v.second );
            } else if ( hash_code == typeid( int64_t ).hash_code( ) ) {
                j_event[ "properties" ][ v.first ] =
                    std::any_cast< int64_t >( v.second );
            } else if ( hash_code == typeid( float ).hash_code( ) ) {
                j_event[ "properties" ][ v.first ] =
                    std::any_cast< float >( v.second );
            } else if ( hash_code == typeid( double ).hash_code( ) ) {
                j_event[ "properties" ][ v.first ] =
                    std::any_cast< double >( v.second );
            } else if ( hash_code == typeid( std::string ).hash_code( ) ) {
                j_event[ "properties" ][ v.first ] =
                    std::any_cast< std::string >( v.second );
            } else if ( hash_code == typeid( bool ).hash_code( ) ) {
                j_event[ "properties" ][ v.first ] =
                    std::any_cast< bool >( v.second );
            }
        }

        j_event[ "event" ] = event.name;

        return j_event;
    }
}
