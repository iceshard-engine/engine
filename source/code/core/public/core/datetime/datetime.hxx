#pragma once
#include <core/datetime/types.hxx>

namespace core::datetime
{

//! \brief Returns the current point in time.
auto now() noexcept -> datetime_type;

//! \brief Returns the current point in time.
auto now_local() noexcept -> local_datetime_type;


//! \brief Returns a calendar for the given date time.
auto make_calendar(datetime_type time) noexcept -> calendar_type;

//! \brief Returns a calendar for the given local date time.
auto make_calendar(local_datetime_type time) noexcept -> calendar_type;


//! \brief Converts a datetime object from one type to another.
//! \remarks Supports casts between types: \c datetime_type, \c local_datetime_type, \c unix_timestamp
template<class TTo, class TFrom>
auto datetime_cast(TFrom from) noexcept -> TTo;

} // namespace core::datetime
