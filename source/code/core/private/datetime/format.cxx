#include <core/datetime/format.hxx>
#include <core/datetime/datetime.hxx>
#include <fmt/format.h>

namespace core::datetime
{
namespace detail
{

//! \brief A single format definition.
struct format_definition
{
    //! \brief The format string.
    const char* format_string;

    //! \brief The minimum length of a buffer to fit the result string.
    size_t format_string_length;
};


static constexpr format_definition time_format{ "{h:2}:{m:02}:{s:02}", 6 + 2 };

static constexpr format_definition date_format{ "{Y:4}/{M:02}/{D:02}", 8 + 2 };

static constexpr format_definition datetime_format{ "{Y:4}/{M:02}/{D:02} {h: 2}:{m:02}:{s:02}", 8 + 2 + 1 + 6 + 2 };

static constexpr format_definition timestamp_format{ "{Y:4}-{M:02}-{D:02}-{h:02}-{m:02}-{s:02}", 14 + 5 };

static constexpr format_definition filestamp_format{ "{Y:4}-{M:02}-{D:02}-{h:02}-{m:02}-{s:02}.{u:03}", 14 + 5 + 1 + 3 };

static constexpr format_definition format_definitions[] = {
    time_format,
    date_format,
    datetime_format,
    timestamp_format,
    filestamp_format,
};

template<class TDatetime>
auto format_datetime(TDatetime time, Format format) noexcept -> std::string
{
    const auto definition = detail::format_definitions[static_cast<size_t>(format)];

    auto calendar = core::datetime::make_calendar(time);
    auto args = fmt::make_format_args(
        fmt::arg("Y", calendar.year), fmt::arg("M", calendar.month), fmt::arg("D", calendar.day)
        , fmt::arg("h", calendar.hour), fmt::arg("m", calendar.minute), fmt::arg("s", calendar.second)
        , fmt::arg("u", calendar.microsecond)
    );

    return fmt::vformat(definition.format_string, std::move(args));
}

} // namespace detail

//! \brief Formats the given datetime object.
auto format_datetime(datetime_type time, Format format) noexcept -> std::string
{
    return detail::format_datetime(time, format);
}

//! \brief Formats the given datetime object.
auto format_datetime(local_datetime_type time, Format format) noexcept -> std::string
{
    return detail::format_datetime(time, format);
}

auto format_datetime(char* buffer, size_t size, datetime_type time, Format format) noexcept -> bool
{
    const auto definition = detail::format_definitions[static_cast<size_t>(format)];
    assert(definition.format_string_length <= size);

    auto result = detail::format_datetime(time, format);
    strcpy_s(buffer, size, result.c_str());
    return result.length() <= size;
}

auto format_datetime(char* buffer, size_t size, local_datetime_type time, Format format) noexcept -> bool
{
    const auto definition = detail::format_definitions[static_cast<size_t>(format)];
    assert(definition.format_string_length <= size);

    auto result = detail::format_datetime(time, format);
    strcpy_s(buffer, size, result.c_str());
    return result.length() <= size;
}

} // namespace core::datetime
