/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/log.hxx>

namespace ice::detail::webasm
{

    void alert_assert(
        ice::String condition,
        ice::String message,
        fmt::format_args args,
        ice::detail::LogLocation location
    ) noexcept;

    void console_message(
        ice::LogSeverity severity,
        ice::LogTag tag,
        ice::String message,
        fmt::format_args args,
        ice::detail::LogLocation location
    ) noexcept;

} // namespace ice
