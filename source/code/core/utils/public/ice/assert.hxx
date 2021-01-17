#pragma once
#include <ice/log.hxx>

namespace ice::detail
{

    void assert(
        ice::String condition,
        ice::String message,
        fmt::format_args args,
        ice::detail::LogLocation location
    ) noexcept;

} // namespace ice

#define ICE_ASSERT(condition, message, ...) \
    do{ \
        if constexpr(ice::LogSeverity::Error <= ice::lowest_compiled_log_severity) \
        { \
            if (!(condition)) \
            { \
                ice::detail::assert( \
                    #condition, \
                    message, \
                    fmt::make_format_args(__VA_ARGS__), \
                    ice::detail::LogLocation{ .file = __FILE__, .line = __LINE__ } \
                ); \
                std::terminate(); \
            } \
        } \
    } while(false)
