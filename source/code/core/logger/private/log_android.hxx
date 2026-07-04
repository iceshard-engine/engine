/// Copyright 2023 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/log.hxx>

namespace ice::detail::android
{

    void logcat_assert(
        ice::String condition,
        ice::String message,
        fmt::format_args const& args,
        ice::detail::LogLocation const& location
    ) noexcept;

    void logcat_message(
        ice::LogSeverity severity,
        ice::LogTag tag,
        ice::String message,
        fmt::format_args const& args,
        ice::detail::LogLocation const& location
    ) noexcept;

} // namespace ice
