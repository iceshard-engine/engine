#include <core/debug/utils.hxx>
#include <core/platform/windows.hxx>

namespace core::debug
{

    void debug_message(std::string_view message) noexcept
    {
        OutputDebugString(message.data());
    }

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

    void abort() noexcept
    {
        ::abort();
    }

} // namespace core::debug
