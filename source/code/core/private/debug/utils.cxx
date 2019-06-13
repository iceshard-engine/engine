#include <core/debug/utils.hxx>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace core::debug
{

bool is_debugger_attached() noexcept
{
    return IsDebuggerPresent() == TRUE;
}

void wait_for_debugger() noexcept
{
    while (!is_debugger_attached())
    {
        Sleep(1);
    }
}

void debug_break() noexcept
{
    __debugbreak();
}

void debug_message(std::string_view message) noexcept
{
    OutputDebugString(message.data());
}

} // namespace core::debug
