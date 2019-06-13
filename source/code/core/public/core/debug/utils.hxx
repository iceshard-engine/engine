#pragma once
#include <core/build/build.hxx>
#include <string_view>

namespace core::debug
{

//! \brief Outputs a message to the debugger.
void debug_message(std::string_view message) noexcept;

//! \brief Checks if a debugger is attached.
bool is_debugger_attached() noexcept;

//! \brief Waits for a debugger to attach.
void wait_for_debugger() noexcept;

//! \brief Forces a breakpoint in code.
void debug_break() noexcept;

} // namespace core::debug
