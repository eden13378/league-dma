#include "pch.hpp"

#include "tracking.hpp"

#include "analytics.hpp"

namespace analytics::tracking {
    auto track_duration( std::string_view event_name ) -> DurationTracker*{
        return reinterpret_cast< DurationTracker* >( instance( ).
            push_tracker( event_name, std::make_shared< DurationTracker >( ) ) );
    }

    auto track_average_duration( std::string_view event_name, size_t count ) -> AverageDurationTracker*{
        return reinterpret_cast< AverageDurationTracker* >( instance( ).
            push_tracker( event_name, std::make_shared< AverageDurationTracker >( count ) ) );
    }
}
