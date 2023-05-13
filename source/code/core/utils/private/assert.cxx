/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/assert.hxx>
#include <ice/string/string.hxx>
#include <ice/os/windows.hxx>
#include <ice/mem_allocator_host.hxx>

#include "log_internal.hxx"
#include "log_buffer.hxx"

#include <ctime>
#include <fmt/format.h>
#include <fmt/chrono.h>


#undef assert

namespace ice::detail
{

    static constexpr ice::String LogFormat_AssertLineHeader = "{:%T} [ASRT] ASSERT";
    static constexpr ice::String LogFormat_AssertCondition = "{}({}) : {} > Assertion failed! `{}`\n";
    static constexpr ice::String LogFormat_AssertMessage = "{}({}) : {} | ";

    void assert(
        ice::String condition,
        ice::String message,
        fmt::format_args args,
        ice::detail::LogLocation location
    ) noexcept
    {
        ice::detail::assert_fn(
            condition,
            message,
            ice::move(args),
            location
        );
    }

    void default_assert_fn(
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
            fmt_string(LogFormat_AssertLineHeader),
            fmt::localtime(std::time(nullptr))
        );

        if (LogState::minimal_header_length < format_result.size)
        {
            LogState::minimal_header_length = static_cast<ice::u32>(format_result.size);
        }

        fmt::string_view log_header{ &header_buffer_raw[0], format_result.size };

        static ice::HostAllocator host_alloc{};
        detail::LogMessageBuffer final_buffer{ host_alloc, 3000 };

        fmt::vformat_to(
            std::back_inserter(final_buffer),
            fmt_string(LogFormat_AssertCondition),
            fmt::make_format_args(
                location.file,
                location.line,
                log_header,
                condition
            )
        );

        fmt::vformat_to(
            std::back_inserter(final_buffer),
            fmt_string(LogFormat_AssertMessage),
            fmt::make_format_args(
                location.file,
                location.line,
                log_header
            )
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

        fmt::print(stderr, "{}", fmt_string(final_buffer.begin(), final_buffer.end()));

        final_buffer.push_back('\0');

#if ISP_WINDOWS
        OutputDebugStringA(final_buffer.data());
#endif
    }

    void uninitialized_assert_fn(
        ice::String condition,
        ice::String message,
        fmt::format_args args,
        ice::detail::LogLocation location
    ) noexcept
    {
        fmt::vprint(
            stderr,
            fmt_string(message),
            ice::move(args)
        );
    }

} // namespace ice::detail

ice::detail::AssertFn* ice::detail::assert_fn = ice::detail::uninitialized_assert_fn;
