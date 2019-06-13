#include <core/datetime/datetime.hxx>
#include <core/datetime/constants.hxx>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace core::datetime
{
namespace detail
{

auto datetime_to_filetime(datetime_type time) noexcept -> FILETIME
{
    LARGE_INTEGER bigint;
    bigint.QuadPart = (time.tick.value - constant::ticks_to_1601.value) / constant::ticks_per_microsecond.value;

    FILETIME filetime;
    filetime.dwLowDateTime = bigint.LowPart;
    filetime.dwHighDateTime = bigint.HighPart;

    return filetime;
}

auto local_datetime_to_filetime(local_datetime_type time) noexcept -> FILETIME
{
    LARGE_INTEGER bigint;
    bigint.QuadPart = (time.tick.value - constant::ticks_to_1601.value) / constant::ticks_per_microsecond.value;

    FILETIME filetime;
    filetime.dwLowDateTime = bigint.LowPart;
    filetime.dwHighDateTime = bigint.HighPart;

    FILETIME system_time{ 0 };
    LocalFileTimeToFileTime(&filetime, &system_time);

    return system_time;
}

auto filetime_to_datetime(FILETIME filetime) noexcept -> datetime_type
{
    LARGE_INTEGER bigint;
    bigint.LowPart = filetime.dwLowDateTime;
    bigint.HighPart = filetime.dwHighDateTime;

    return { (bigint.QuadPart * constant::ticks_per_microsecond.value) + constant::ticks_to_1601.value };
}

auto filetime_to_local_datetime(FILETIME filetime) noexcept -> local_datetime_type
{
    FILETIME local_time{ 0 };
    FileTimeToLocalFileTime(&filetime, &local_time);

    LARGE_INTEGER bigint;
    bigint.LowPart = local_time.dwLowDateTime;
    bigint.HighPart = local_time.dwHighDateTime;

    return { (bigint.QuadPart * constant::ticks_per_microsecond.value) + constant::ticks_to_1601.value };
}

auto filetime_to_calendar(FILETIME filetime) noexcept -> calendar_type
{
    SYSTEMTIME native_calendar;
    FileTimeToSystemTime(&filetime, &native_calendar);

    calendar_type result{ };
    result.microsecond = native_calendar.wMilliseconds * 1000;
    result.second = native_calendar.wSecond;
    result.minute = native_calendar.wMinute;
    result.hour = native_calendar.wHour;
    result.day = native_calendar.wDay;
    result.month = native_calendar.wMonth;
    result.year = native_calendar.wYear;
    result.day_of_week = native_calendar.wDayOfWeek;
    return result;
}

auto datetime_to_local_datetime(datetime_type time) noexcept -> local_datetime_type
{
    return filetime_to_local_datetime(datetime_to_filetime(time));
}

auto local_datetime_to_datetime(local_datetime_type time) noexcept -> datetime_type
{
    return filetime_to_datetime(local_datetime_to_filetime(time));
}

} // namespace detail


auto now() noexcept -> datetime_type
{
    FILETIME system_time{ 0 };
    GetSystemTimeAsFileTime(&system_time);

    return detail::filetime_to_datetime(system_time);
}

auto now_local() noexcept -> local_datetime_type
{
    FILETIME system_time{ 0 };
    GetSystemTimeAsFileTime(&system_time);

    return detail::filetime_to_local_datetime(system_time);
}

auto make_calendar(datetime_type time) noexcept -> calendar_type
{
    return detail::filetime_to_calendar(detail::datetime_to_filetime(time));
}

auto make_calendar(local_datetime_type time) noexcept -> calendar_type
{
    return detail::filetime_to_calendar(detail::datetime_to_filetime({ time.tick }));
}


/// Casts ///


template<>
auto datetime_cast(datetime_type from) noexcept -> datetime_type
{
    return from;
}

template<>
auto datetime_cast(datetime_type from) noexcept -> local_datetime_type
{
    return detail::datetime_to_local_datetime(from);
}

template<>
auto datetime_cast(datetime_type from) noexcept -> unix_timestamp
{
    return { (from.tick.value - constant::ticks_to_1970.value) / constant::ticks_per_second.value };
}

template<>
auto datetime_cast(local_datetime_type from) noexcept -> local_datetime_type
{
    return from;
}

template<>
auto datetime_cast(local_datetime_type from) noexcept -> datetime_type
{
    return detail::local_datetime_to_datetime(from);
}

template<>
auto datetime_cast(local_datetime_type from) noexcept -> unix_timestamp
{
    return datetime_cast<unix_timestamp>(detail::local_datetime_to_datetime(from));
}

template<>
auto datetime_cast(unix_timestamp from) noexcept -> unix_timestamp
{
    return from;
}

template<>
auto datetime_cast(unix_timestamp from) noexcept -> datetime_type
{
    return { (from.value * constant::ticks_per_second.value) + constant::ticks_to_1970.value };
}

template<>
auto datetime_cast(unix_timestamp from) noexcept -> local_datetime_type
{
    return detail::datetime_to_local_datetime(datetime_cast<datetime_type>(from));
}

} // namespace core::datetime
