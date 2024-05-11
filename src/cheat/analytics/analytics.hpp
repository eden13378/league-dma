#pragma once
#include <deque>
#include <memory>

#include "tracking.hpp"

namespace analytics {
    class Analytics {
        struct TrackingData {
            std::string_view                         name{ };
            std::shared_ptr< tracking::BaseTracker > tracker{ };
            bool                                     should_delete{ };
        };

    public:
        auto push_tracker(
            std::string_view                          name,
            std::shared_ptr< tracking::BaseTracker > tracker
        ) -> tracking::BaseTracker*{
            m_trackers.push_back( TrackingData{ name, std::move( tracker ) } );
            return m_trackers.back( ).tracker.get( );
        }

        auto submit_data( ) -> void{
            nlohmann::json j;

            for ( auto& tracker : m_trackers ) {
                if ( !tracker.tracker->is_ready( ) ) continue;

                j[ "tracker" ].push_back(
                    nlohmann::json{
                        { _( "type" ), tracker.tracker->get_type( ) },
                        { _( "payload" ), tracker.tracker->as_json( ) }
                    }
                );

                tracker.should_delete = true;
            }

            // m_trackers.
        }

    private:
        std::deque< TrackingData > m_trackers{ };
    };

    static auto instance( ) -> Analytics{
        static Analytics analytics;
        return analytics;
    }
}
