#pragma once
#include <cinttypes>

namespace core::datetime
{


    //! \brief A single tick.
    //! \details The resolution is 10ns.
    //! \details Stored value should be considered magic.
    struct tick_type { int64_t value; };

    //! \brief Time duration.
    struct timespan_type { tick_type tick; };

    //! \brief Time point (UTC).
    struct datetime_type { tick_type tick; };

    //! \brief Time point (Local Timezone).
    struct local_datetime_type { tick_type tick; };

    //! \brief Time point (Unix Timestamp).
    struct unix_timestamp { int64_t value; };


    //! \brief Calendar representing a time point.
    //! \details The representation is not specific to UTC or Local, thus a conversion to a time point is not possible.
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
