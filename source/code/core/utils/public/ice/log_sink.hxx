#pragma once
#include <ice/module.hxx>
#include <ice/string/string.hxx>

namespace ice
{

    enum class LogSeverity : ice::u32;
    enum class LogTag : ice::u64;

    struct LogSinkMessage
    {
        ice::LogSeverity severity;
        ice::LogTag tag;
        ice::String tag_name;
        ice::String message;
    };

    enum class LogSinkID : uint8_t { Invalid = 0 };

    using LogSinkFn = void(*)(void* userdata, ice::LogSinkMessage const& message) noexcept;

    auto log_module_register_sink(LogSinkFn fn_sink, void* userdata) noexcept -> ice::LogSinkID;
    void log_module_unregister_sink(ice::LogSinkID sinkid) noexcept;

} // namespace ice
