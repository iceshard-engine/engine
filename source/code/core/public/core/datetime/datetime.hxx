#pragma once
#include <core/datetime/types.hxx>

namespace core::datetime
{


    //! \brief Current time (UTC).
    auto now() noexcept -> datetime_type;

    //! \brief Current time (Local Timezone).
    auto now_local() noexcept -> local_datetime_type;


    //! \brief Creates a calendar object from the given time point (UTC).
    auto make_calendar(datetime_type time) noexcept -> calendar_type;

    //! \brief Creates a calendar object from the given time point (Local Timezone).
    auto make_calendar(local_datetime_type time) noexcept -> calendar_type;


    //! \brief Converts a datetime object between representations.
    //! \remarks Supports: \c datetime_type, \c local_datetime_type, \c unix_timestamp
    template<class TTo, class TFrom>
    auto datetime_cast(TFrom from) noexcept -> TTo;


} // namespace core::datetime
