/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/log.hxx>
#include <ice/os/windows.hxx>
#include <ice/string/string.hxx>

#include "log_internal.hxx"
#include "log_buffer.hxx"

#include <ctime>
#include <fmt/format.h>
#include <fmt/chrono.h>


namespace ice::detail
{

    static constexpr ice::String LogFormat_LogLineHeader = "{:%T} [{}] {}{}{}";
    static constexpr ice::String LogFormat_LogLine = "{: <{}s} > ";

    void log(
        ice::LogSeverity severity,
        ice::LogTag tag,
        ice::String message,
        fmt::format_args args,
        ice::detail::LogLocation location
    ) noexcept
    {
        log_fn(
            severity,
            tag,
            message,
            ice::move(args),
            location
        );
    }

    ice::String severity_value[]{
        "",
        "CRIT",
        "HIGH",

        "ERRR",
        "WARN",
        "INFO",
        "VERB",

        "DEBG",
    };

    auto get_base_tag_name(ice::LogTag tag) noexcept -> ice::String
    {
        ice::u64 constexpr base_tag_mask = ~((1llu << 32) - 1);
        ice::LogTag const base_tag = static_cast<LogTag>(
            static_cast<ice::u64>(tag) & base_tag_mask
        );

        switch (base_tag)
        {
        case LogTag::Core:
            return "Core";
        case LogTag::Module:
            return "Module";
        case LogTag::System:
            return "System";
        case LogTag::Engine:
            return "Engine";
        case LogTag::Asset:
            return "Asset";
        case LogTag::Game:
            return "Game";
        default:
            return "";
        }
    }

    void default_log_fn(
        ice::LogSeverity severity,
        ice::LogTag tag,
        ice::String message,
        fmt::format_args args,
        ice::detail::LogLocation location
    ) noexcept
    {
        detail::LogState const* const log_state = detail::internal_log_state;

        ice::String const base_tag_name = detail::get_base_tag_name(tag);
        ice::String const tag_name = log_state->tag_name(tag);

        char header_buffer_raw[256];
        fmt::format_to_n_result format_result = fmt::format_to_n(
            header_buffer_raw,
            256,
            fmt_string(LogFormat_LogLineHeader),
            fmt::localtime(std::time(nullptr)),
            fmt_string(detail::severity_value[static_cast<ice::u32>(severity)]),
            fmt_string(base_tag_name),
            fmt_string(ice::string::empty(tag_name) || ice::string::empty(base_tag_name) ? "" : " | "),
            fmt_string(tag_name)
        );

        if (LogState::minimal_header_length < format_result.size)
        {
            LogState::minimal_header_length = static_cast<ice::u32>(format_result.size);
        }

        fmt::string_view const log_header{ &header_buffer_raw[0], format_result.size };

        detail::log_buffer_alloc.reset();
        detail::LogMessageBuffer final_buffer{ detail::log_buffer_alloc, 2000 };

        fmt::vformat_to(
            std::back_inserter(final_buffer),
            fmt_string(LogFormat_LogLine),
            fmt::make_format_args(log_header, LogState::minimal_header_length)
        );

        fmt::vformat_to(
            std::back_inserter(final_buffer),
            fmt_string(message),
            ice::move(args)
        );

        char const* last_char = final_buffer.end() - 1;
        if (*last_char != '\n')
        {
            final_buffer.push_back('\n');
        }

        if (severity == LogSeverity::Critical || severity == LogSeverity::Error)
        {
            fmt::print(stderr, "{}", fmt_string(final_buffer.begin(), final_buffer.end()));
        }
        else
        {
            fmt::print(stdout, "{}", fmt_string(final_buffer.begin(), final_buffer.end()));
        }

        final_buffer.push_back('\0');

#if ISP_WINDOWS
        OutputDebugStringA(final_buffer.data());
#endif
    }

    void uninitialized_log_fn(
        ice::LogSeverity /*severity*/,
        ice::LogTag /*tag*/,
        ice::String message,
        fmt::format_args args,
        ice::detail::LogLocation /*location*/
    ) noexcept
    {
        fmt::vprint(fmt_string(message), ice::move(args));
        fmt::print("\n");
    }

} // namespace ice::detail

ice::detail::LogFn* ice::detail::log_fn = ice::detail::uninitialized_log_fn;
