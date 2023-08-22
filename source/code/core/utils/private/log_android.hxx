#pragma once
#include <ice/log.hxx>

namespace ice::detail::android
{

    void logcat_assert(
        ice::String condition,
        ice::String message,
        fmt::format_args args,
        ice::detail::LogLocation location
    ) noexcept;

    void logcat_message(
        ice::LogSeverity severity,
        ice::LogTag tag,
        ice::String message,
        fmt::format_args args,
        ice::detail::LogLocation location
    ) noexcept;

} // namespace ice
