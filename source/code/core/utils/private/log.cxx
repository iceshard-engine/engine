#include <ice/log.hxx>
#include <ice/memory/memory_globals.hxx>
#include <ice/memory/pointer_arithmetic.hxx>
#include <ice/os/windows.hxx>

#include "log_internal.hxx"
#include "log_buffer.hxx"

#include <ctime>
#include <fmt/format.h>
#include <fmt/chrono.h>


namespace ice::detail
{

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

        char header_buffer_raw[128 + 256];
        fmt::format_to_n_result format_result = fmt::format_to_n(
            header_buffer_raw,
            128,
            "{:%T} [{}] {}{}{}",
            fmt::localtime(std::time(nullptr)),
            detail::severity_value[static_cast<ice::u32>(severity)],
            base_tag_name,
            ice::string::empty(tag_name) || ice::string::empty(base_tag_name) ? "" : " | ",
            tag_name
        );

        if (log_header_size < format_result.size)
        {
            log_header_size = format_result.size;
        }

        fmt::string_view log_header{ &header_buffer_raw[0], format_result.size };

        detail::log_buffer_alloc.clear();
        detail::LogMessageBuffer final_buffer{ detail::log_buffer_alloc, 2000 };

        fmt::vformat_to(
            final_buffer,
            "{: <{}s} > ",
            fmt::make_format_args(log_header, log_header_size)
        );

        fmt::vformat_to(
            final_buffer,
            message,
            ice::move(args)
        );

        char const* last_char = final_buffer.end() - 1;
        if (*last_char != '\n')
        {
            final_buffer.push_back('\n');
        }

        if (severity == LogSeverity::Critical || severity == LogSeverity::Error)
        {
            fmt::print(stderr, make_string(final_buffer.begin(), final_buffer.end()));
        }
        else
        {
            fmt::print(stdout, make_string(final_buffer.begin(), final_buffer.end()));
        }

        final_buffer.push_back('\0');
        OutputDebugString(final_buffer.data());
    }

    void uninitialized_log_fn(
        ice::LogSeverity /*severity*/,
        ice::LogTag /*tag*/,
        ice::String message,
        fmt::format_args args,
        ice::detail::LogLocation /*location*/
    ) noexcept
    {
        fmt::vprint(message, ice::move(args));
    }

} // namespace ice::detail

ice::detail::LogFn* ice::detail::log_fn = ice::detail::uninitialized_log_fn;
