#include <ice/assert.hxx>
#include <ice/memory/memory_globals.hxx>
#include <ice/os/windows.hxx>

#include "log_internal.hxx"
#include "log_buffer.hxx"

#include <ctime>
#include <fmt/format.h>
#include <fmt/chrono.h>


#undef assert

namespace ice::detail
{

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
            "{:%T} [ASRT] ASSERT",
            fmt::localtime(std::time(nullptr))
        );

        if (LogState::minimal_header_length < format_result.size)
        {
            LogState::minimal_header_length = static_cast<ice::u32>(format_result.size);
        }

        fmt::string_view log_header{ &header_buffer_raw[0], format_result.size };

        detail::LogMessageBuffer final_buffer{ ice::memory::default_allocator(), 3000 };

        fmt::vformat_to(
            final_buffer,
            "{}({}) : {} > Assertion failed! `{}`\n",
            fmt::make_format_args(
                location.file,
                location.line,
                log_header,
                condition
            )
        );

        fmt::vformat_to(
            final_buffer,
            "{}({}) : {} | ",
            fmt::make_format_args(
                location.file,
                location.line,
                log_header
            )
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

        fmt::print(stderr, make_string(final_buffer.begin(), final_buffer.end()));

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
            message,
            ice::move(args)
        );
    }

} // namespace ice::detail

ice::detail::AssertFn* ice::detail::assert_fn = ice::detail::uninitialized_assert_fn;
