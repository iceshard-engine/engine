#pragma once
#include <core/build/build.hxx>
#include <fmt/format.h>

namespace core::debug::detail
{

//! \brief Checks if the given value is true and sends the given condition and message on failure.
void assert_internal(bool value, std::string_view condition, std::string_view filename, int fileline, std::string_view message, fmt::format_args arguments) noexcept;

} // namespace core

// Undefine other assert macros
#undef assert

// Defines a helper macro for assert checks.
#define mx_assert(condition, message, ...) \
    if constexpr(!core::build::is_release) \
    { \
        ::core::debug::detail::assert_internal(true == (condition), #condition, __FILE__, __LINE__, message, fmt::make_format_args(__VA_ARGS__)); \
    }
