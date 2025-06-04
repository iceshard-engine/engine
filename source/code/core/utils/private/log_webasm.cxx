/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "log_webasm.hxx"
#include "log_internal.hxx"
#include "log_buffer.hxx"

#if ISP_WEBAPP
#include <ice/mem_allocator_host.hxx>

#include <emscripten/console.h>
#include <emscripten.h>
#include <fmt/chrono.h>
#include <fmt/core.h>

namespace ice::detail::webasm
{
    EM_JS(void, call_alert, (char const* message, size_t len), {
        alert(UTF8ToString(message, len));
        throw "Failed assertion!";
    });

    static constexpr fmt::string_view LogFormat_AssertLineHeader = "{:%T} [ASRT] ASSERT";
    static constexpr fmt::string_view LogFormat_AssertCondition = "{} > Assertion failed! `{}`\n";
    static constexpr fmt::string_view LogFormat_AssertMessage = "{} | ";

    static constexpr fmt::string_view LogFormat_AlertLocation = "{}({})\n\n";
    static constexpr fmt::string_view LogFormat_AlertCondition = "Failed condition: `{}`\n\n";

    static constexpr fmt::string_view LogFormat_LogLineHeader = "{:%T} [{}] {}{}{}";
    static constexpr fmt::string_view LogFormat_LogLine = "{: <{}s} > ";

    auto get_tag_name(ice::LogTag tag) noexcept -> ice::String
    {
        switch (tag)
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
        case LogTag::Tool:
            return "Tool";
        case LogTag::None:
            return "";
        default:
            return detail::internal_log_state->tag_name(tag);
        }
    }

    auto get_base_tag_name(ice::LogTag tag) noexcept -> ice::String
    {
        ice::u64 const tag_value = ice::bit_cast<ice::u64>(tag);
        ice::LogTag const tag_base = LogTag(tag_value >> 32);
        return get_tag_name(tag_base);
    }

    void alert_assert(
        ice::String condition,
        ice::String message,
        fmt::format_args args,
        ice::detail::LogLocation location
    ) noexcept
    {
        char header_buffer_raw[128 + 256];
        fmt::format_to_n_result format_result = fmt::format_to_n(
            header_buffer_raw,
            128,
            LogFormat_AssertLineHeader,
            fmt::localtime(std::time(nullptr))
        );

        if (LogState::minimal_header_length < format_result.size)
        {
            LogState::minimal_header_length = static_cast<ice::u32>(format_result.size);
        }

        fmt::string_view log_header{ &header_buffer_raw[0], format_result.size };

        static ice::HostAllocator host_alloc{};
        detail::LogMessageBuffer final_buffer{ host_alloc, 3000 };

        fmt::format_to(
            std::back_inserter(final_buffer),
            LogFormat_AssertCondition,
            log_header,
            fmt_string(condition)
        );

        fmt::format_to(
            std::back_inserter(final_buffer),
            LogFormat_AssertMessage,
            log_header
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
        final_buffer.push_back('\0');

        emscripten_console_error(final_buffer.data());
        final_buffer.clear();

        fmt::format_to(
            std::back_inserter(final_buffer),
            LogFormat_AlertLocation,
            fmt_string(location.file),
            location.line
        );

        fmt::format_to(
            std::back_inserter(final_buffer),
            LogFormat_AlertCondition,
            fmt_string(condition)
        );

        fmt::vformat_to(
            std::back_inserter(final_buffer),
            fmt_string(message),
            ice::move(args)
        );

        call_alert(final_buffer.data(), final_buffer.size());
    }

    void console_message(
        ice::LogSeverity severity,
        ice::LogTag tag,
        ice::String message,
        fmt::format_args args,
        ice::detail::LogLocation location
    ) noexcept
    {
        detail::LogState const* const log_state = detail::internal_log_state;
        if (log_state->tag_enabled(tag) == false)
        {
            return;
        }

        ice::String const base_tag_name = get_base_tag_name(tag);
        ice::String const tag_name = get_tag_name(tag);

        char header_buffer_raw[256];
        fmt::format_to_n_result format_result = fmt::format_to_n(
            header_buffer_raw,
            256,
            LogFormat_LogLineHeader,
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
            LogFormat_LogLine,
            fmt::make_format_args(log_header, LogState::minimal_header_length)
        );

        [[maybe_unused]]
        ice::usize::base_type const header_size = final_buffer.size();

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
        final_buffer.push_back('\0');

        switch(severity)
        {
            using enum LogSeverity;
        case Debug:
        case Verbose:
            emscripten_console_log(final_buffer.data());
            break;
        case Info:
            emscripten_console_log(final_buffer.data());
            break;
        case Retail:
            emscripten_outn(final_buffer.data(), final_buffer.size());
            break;
        case Warning:
            emscripten_console_warn(final_buffer.data());
            break;
        case Error:
        case Critical:
            emscripten_errn(final_buffer.data(), final_buffer.size());
            break;
        default: break;
        }

        // Flush the sink message after pushing the final '0' character
#if ICE_RELEASE == 0
        ice::detail::internal_log_state->flush(
            ice::LogSinkMessage{
                .severity = severity,
                .tag = tag,
                .tag_name = tag_name,
                .message = ice::String{ final_buffer.begin() + header_size, final_buffer.end() - 1 }
            }
        );
#endif
    }
}

#endif
