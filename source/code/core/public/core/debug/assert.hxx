#pragma once
#include <core/build/build.hxx>
#include <core/debug/utils.hxx>
#include <fmt/format.h>

namespace core::debug::detail
{


//! \brief Outputs the assert message and checks if controlls the application. #todo Get a better description.
//! \returns ture If the application should break.
bool assert_internal(std::string_view condition, std::string_view filename, int fileline, std::string_view message, fmt::format_args arguments) noexcept;


} // namespace core


#define IS_ASSERT(condition, message, ...) \
    do { \
        if constexpr(!core::build::is_release) { \
            if (false == (condition)) { \
                if (::core::debug::detail::assert_internal(#condition, __FILE__, __LINE__, message, fmt::make_format_args(__VA_ARGS__))) { \
                    core::debug::debug_break(); \
                } \
            } \
        } \
    } while(false)

