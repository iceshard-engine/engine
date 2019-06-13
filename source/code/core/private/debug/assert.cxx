#include <core/debug/assert.hxx>
#include <core/debug/utils.hxx>
#include <fmt/core.h>

#include <vector>


namespace core::debug::detail
{

void get_message_lines(const char* message, std::vector<std::string_view>& lines) noexcept
{
    const char* data_begin = message;
    const char* data_end = data_begin;

    while (*data_end)
    {
        if (*data_end == '\n')
        {
            lines.push_back(std::string_view{ data_begin, static_cast<uint32_t>(data_end - data_begin) });
            data_begin = data_end + 1;
        }
        ++data_end;
    }

    lines.push_back(std::string_view{ data_begin, static_cast<uint32_t>(data_end - data_begin) });
}

bool assert_internal(std::string_view condition, std::string_view filename, int fileline, std::string_view message, fmt::format_args arguments) noexcept
{
    fmt::memory_buffer message_buffer;
    fmt::vformat_to(message_buffer, message, std::move(arguments));
    message_buffer.push_back('\0');

    std::vector<std::string_view> lines;
    get_message_lines(message_buffer.data(), lines);

    fmt::memory_buffer assert_buffer;
    fmt::format_to(assert_buffer, "{}({}) : assert error : condition = {}, {}\n", filename, fileline, condition, lines.front());
    assert_buffer.push_back('\0');

    fmt::print(stderr, assert_buffer.data());
    core::debug::debug_message(assert_buffer.data());

    std::for_each(std::next(lines.begin()), lines.end(), [&](const auto & line) noexcept
        {
            fmt::print(stderr, "> {}\n", line);
        });


    // Abort on release, Break otherwise.
    if constexpr (core::build::is_release)
    {
        core::debug::abort();
    }
    else
    {
        return true;
    }
}

} // namespace core

