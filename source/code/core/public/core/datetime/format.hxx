#pragma once
#include <core/datetime/types.hxx>
#include <string_view>

namespace core::datetime
{


    //! \brief Enum holding available datetime serialization formats.
    enum class Format
    {
        Time,
        Date,
        DateTime,
        Timestamp,
        FileTimestamp,
        UnixTimestamp = Timestamp,
    };

    //! \brief Formats the given datetime object (UTC).
    auto format_datetime(datetime_type time, Format format) noexcept->std::string;

    //! \brief Formats the given datetime object (Local Timezone).
    auto format_datetime(local_datetime_type time, Format format) noexcept->std::string;

    //! \brief Formats the given datetime object (UTC).
    //! \details Output is written into the given buffer.
    auto format_datetime(char* buffer, size_t size, datetime_type time, Format format) noexcept -> bool;

    //! \brief Formats the given datetime object (Local Timezone).
    //! \details Output is written into the given buffer.
    auto format_datetime(char* buffer, size_t size, local_datetime_type time, Format format) noexcept -> bool;

    //! \brief Formats the given datetime object.
    //! \details Output is written into the given character array.
    template<class TDatetime, size_t Size>
    auto format_datetime(char(&buffer)[Size], TDatetime time, Format format) noexcept -> bool
    {
        return format_datetime(&buffer[0], Size, time, format);
    }


} // namespace core::datetime
