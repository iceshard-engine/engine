/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "log_android.hxx"
#include "log_internal.hxx"
#include "log_buffer.hxx"

#if ISP_ANDROID
#include <ice/mem_allocator_host.hxx>
#include <android/log.h>

namespace ice::detail::android
{

    static constexpr ice::String LogFormat_AssertCondition = "\nAssertion failed! `{}`";

    auto get_base_tag_name(ice::LogTag tag) noexcept -> ice::String
    {
        ice::u64 constexpr base_tag_mask = ~((1llu << 32) - 1);
        ice::LogTag const base_tag = static_cast<LogTag>(
            static_cast<ice::u64>(tag) & base_tag_mask
        );

        switch (base_tag)
        {
        case LogTag::Core:
            return "IceShard::Core";
        case LogTag::Module:
            return "IceShard::Module";
        case LogTag::System:
            return "IceShard::System";
        case LogTag::Engine:
            return "IceShard::Engine";
        case LogTag::Asset:
            return "IceShard::Asset";
        case LogTag::Game:
            return "IceShard::Game";
        default:
            return "";
        }
    }

    constexpr auto logpriority_from_severity(ice::LogSeverity severity) noexcept -> android_LogPriority
    {
        switch (severity)
        {
            // We use "WARN" priority for retail messages.
        case LogSeverity::Retail: return ANDROID_LOG_WARN;
        case LogSeverity::Critical: return ANDROID_LOG_FATAL;
        case LogSeverity::Error: return ANDROID_LOG_ERROR;
        case LogSeverity::Warning: return ANDROID_LOG_WARN;
        case LogSeverity::Info: return ANDROID_LOG_INFO;
        case LogSeverity::Verbose: return ANDROID_LOG_VERBOSE;
        case LogSeverity::Debug:
        default: return ANDROID_LOG_DEBUG;
        }
    }

    constexpr auto bufferid_from_severity(ice::LogSeverity severity) noexcept -> log_id_t
    {
        switch (severity)
        {
        case LogSeverity::Retail:
        case LogSeverity::Critical:
        case LogSeverity::Error:
        case LogSeverity::Warning: return LOG_ID_MAIN;
        case LogSeverity::Info:
        case LogSeverity::Verbose:
        case LogSeverity::Debug:
        default: return LOG_ID_DEFAULT;
        }
    }

    void logcat_assert(
        ice::String condition,
        ice::String message,
        fmt::format_args args,
        ice::detail::LogLocation location
    ) noexcept
    {
        static ice::HostAllocator host_alloc{};
        detail::LogMessageBuffer final_buffer{ host_alloc, 3000 };

        fmt::vformat_to(
            std::back_inserter(final_buffer),
            fmt_string(message),
            ice::move(args)
        );

        fmt::vformat_to(
            std::back_inserter(final_buffer),
            fmt_string(LogFormat_AssertCondition),
            fmt::make_format_args(fmt_string(condition))
        );

        char* last_char = final_buffer.end() - 1;
        if (*last_char == '\n')
        {
            *last_char = '\0';
        }

        final_buffer.push_back('\0');

        __android_log_assert(
            nullptr,
            "IceShard::Assert",
            "%s",
            final_buffer.data()
        );
}

    void logcat_message(
        ice::LogSeverity severity,
        ice::LogTag tag,
        ice::String message,
        fmt::format_args args,
        ice::detail::LogLocation location
    ) noexcept
    {
        // ice::String const tag_name = log_state->tag_name(tag);

        __android_log_message msg{ .struct_size = sizeof(__android_log_message) };
        msg.buffer_id = bufferid_from_severity(severity);
        msg.priority = logpriority_from_severity(severity);
        msg.tag = ice::string::begin(get_base_tag_name(tag));
        msg.file = ice::string::begin(location.file);
        msg.line = location.line;

        if (__builtin_available(android 30, *))
        {
            if (__android_log_is_loggable(__android_log_get_minimum_priority(), msg.tag, msg.priority) != 0)
            {
                detail::log_buffer_alloc.reset();
                detail::LogMessageBuffer final_buffer{ detail::log_buffer_alloc, 2000 };

                fmt::vformat_to(
                    std::back_inserter(final_buffer),
                    fmt_string(message),
                    ice::move(args)
                );

                char* last_char = final_buffer.end() - 1;
                if (*last_char == '\n')
                {
                    *last_char = '\0';
                }
                else
                {
                    final_buffer.push_back('\0');
                }

                msg.message = final_buffer.data();
                __android_log_write_log_message(&msg);
            }
        }
        else
        {
            detail::log_buffer_alloc.reset();
            detail::LogMessageBuffer final_buffer{ detail::log_buffer_alloc, 2000 };

            fmt::vformat_to(
                std::back_inserter(final_buffer),
                fmt_string(message),
                ice::move(args)
            );

            char* last_char = final_buffer.end() - 1;
            if (*last_char == '\n')
            {
                *last_char = '\0';
            }
            else
            {
                final_buffer.push_back('\0');
            }

            __android_log_write(msg.priority, msg.tag, final_buffer.data());
        }
    }

}

#endif
