#pragma once
#include <ice/string_types.hxx>
#include <ice/log_severity.hxx>
#include <ice/log_tag.hxx>
#include <ice/log_formatters.hxx>

namespace ice::detail
{

    struct LogLocation
    {
        ice::String file;
        ice::u32 line;
    };

    void log(
        ice::LogSeverity severity,
        ice::LogTag tag,
        ice::String message,
        fmt::format_args args,
        ice::detail::LogLocation location
    ) noexcept;

} // namespace ice::detail

#if defined ICE_LOG
#error "Found existing definition of 'ICE_LOG'! Please do not redefine this macro!"
#endif

#define ICE_LOG(severity, tag, format, ...) \
    do{ \
        if constexpr(severity <= ice::lowest_compiled_log_severity) \
        { \
            ice::detail::log( \
                severity, \
                ice::detail::get_tag(tag), \
                format, \
                fmt::make_format_args(__VA_ARGS__), \
                ice::detail::LogLocation{ .file = __FILE__, .line = __LINE__ } \
            ); \
        } \
    } while(false)

#define ICE_LOG_IF(enable_condition, severity, tag, format, ...) \
    do{ \
        if constexpr(severity <= ice::lowest_compiled_log_severity) \
        { \
            if (bool(enable_condition) == true) \
            { \
                ice::detail::log( \
                    severity, \
                    ice::detail::get_tag(tag), \
                    format, \
                    fmt::make_format_args(__VA_ARGS__), \
                    ice::detail::LogLocation{ .file = __FILE__, .line = __LINE__ } \
                ); \
            } \
        } \
    } while(false)
