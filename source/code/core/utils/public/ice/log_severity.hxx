#pragma once
#include <ice/base.hxx>

namespace ice
{

    enum class LogSeverity : ice::u32
    {
        Critical = 0x1,
        Retail,

        Error,
        Warning,
        Info,
        Verbose,
        Debug,

        None = 0x0,
        All = 0xffff'ffff
    };

    static constexpr LogSeverity lowest_compiled_log_severity = []() -> ice::LogSeverity
    {
        if constexpr (ice::build::is_debug)
        {
            return LogSeverity::Debug;
        }
        else if constexpr (ice::build::is_release)
        {
            return LogSeverity::Retail;
        }
        else
        {
            return LogSeverity::Verbose;
        }
    }();

} // namespace ice
