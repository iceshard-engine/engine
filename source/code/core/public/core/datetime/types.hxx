#pragma once
#include <cinttypes>

namespace core::datetime
{

//! \brief A single tick.
//! \remarks The resolution is 10ns.
//! \remarks A stored value is context specific.
struct tick_type { int64_t value; };

//! \brief Represents a relative time duration.
struct timespan_type { tick_type tick; };

//! \brief A specific point in time.
//! \remarks The time point is absolute. (UTC)
struct datetime_type { tick_type tick; };

//! \brief A specific point in time.
//! \remarks The time point is relative to the current time zone.
struct local_datetime_type { tick_type tick; };

//! \brief A specific point in time.
//! \remarks The resolution is 1s.
struct unix_timestamp { int64_t value; };


//! \brief A specific point in time.
//! \remarks Representation is splited into specific components.
//! \remarks A calendar is context specific and should be only used for simple formating purposes.
struct calendar_type
{
    int32_t microsecond;

    int32_t second;
    int32_t minute;
    int32_t hour;

    int32_t day;
    int32_t month;
    int32_t year;

    int32_t day_of_week;
};

} // namespace core::datetime
