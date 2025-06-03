/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/profiler.hxx>
#include <ice/string/string.hxx>
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

    constexpr auto log_make_args() noexcept
    {
        return fmt::make_format_args();
    }

    template<typename... Args>
    constexpr auto log_make_args(Args&&... args) noexcept
    {
        return fmt::make_format_args(args...);
    }

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
                ice::detail::log_make_args(__VA_ARGS__), \
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
                    ice::detail::log_make_args(__VA_ARGS__), \
                    ice::detail::LogLocation{ .file = __FILE__, .line = __LINE__ } \
                ); \
            } \
        } \
    } while(false)

#if IPT_ENABLED

#define IPT_LOG(severity, tag, format, ...) \
    IPT_MESSAGE(format); \
    ICE_LOG(severity, tag, format, __VA_ARGS__)

#define IPT_ZONE_LOG(severity, tag, format, ...) \
    IPT_ZONE_SCOPED; \
    IPT_MESSAGE(format); \
    ICE_LOG(severity, tag, format, __VA_ARGS__)

#else

#define IPT_LOG(severity, tag, format, ...) \
    ICE_LOG(severity, tag, format, __VA_ARGS__)

#define IPT_ZONE_LOG(severity, tag, format, ...) \
    ICE_LOG(severity, tag, format, __VA_ARGS__)

#endif
