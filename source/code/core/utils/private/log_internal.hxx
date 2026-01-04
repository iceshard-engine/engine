/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/string_types.hxx>
#include <ice/container_types.hxx>
#include <ice/log_tag.hxx>
#include <ice/log_severity.hxx>
#include <ice/log_sink.hxx>
#include <ice/log.hxx>

#include <fmt/format.h>
#include <fmt/chrono.h>

namespace ice::detail
{

    struct LogTagInfo
    {
        ice::HeapString<> name;
        bool enabled;
    };

    class LogState final
    {
    public:
        static ice::u32 minimal_header_length;

        LogState(ice::Allocator& alloc) noexcept;
        ~LogState() noexcept;

        void register_tag(ice::LogTagDefinition tag_def) noexcept;
        auto register_sink(ice::LogSinkFn fn_sink, void* userdata) noexcept -> ice::LogSinkID;
        void unregister_sink(ice::LogSinkID sinkid) noexcept;

        void enable_tag(ice::LogTag tag, bool enabled) noexcept;

        auto tag_name(ice::LogTag tag) const noexcept -> ice::String;
        bool tag_enabled(ice::LogTag tag) const noexcept;

        void flush(ice::LogSinkMessage const& message) noexcept;

    private:
        ice::Allocator& _allocator;

        ice::detail::LogTagInfo _empty_tag;
        ice::HashMap<ice::detail::LogTagInfo, ice::ContainerLogic::Complex> _tags;

        struct Sink
        {
            ice::LogSinkFn _callback;
            void* _userdata;
        };

        ice::Array<Sink> _sinks;
    };

    extern LogState* internal_log_state;

    auto default_register_sink_fn(ice::LogSinkFn fn_sink, void* userdata) noexcept -> ice::LogSinkID;
    void default_unregister_sink_fn(ice::LogSinkID sinkid) noexcept;

    void default_register_tag_fn(ice::LogTagDefinition tag_def) noexcept;
    void default_enable_tag_fn(ice::LogTag tag_def, bool enabled) noexcept;

    void default_log_fn(
        ice::LogSeverity severity,
        ice::LogTag tag,
        ice::String message,
        fmt::format_args args,
        ice::detail::LogLocation location
    ) noexcept;

    void default_assert_fn(
        ice::String condition,
        ice::String message,
        fmt::format_args args,
        ice::detail::LogLocation location
    ) noexcept;

    using RegisterLogSinkFn = auto(ice::LogSinkFn fn_sink, void* userdata) noexcept -> ice::LogSinkID;
    using UnregisterLogSinkFn = void(ice::LogSinkID sinkid) noexcept;

    extern RegisterLogSinkFn* fn_register_log_sink;
    extern UnregisterLogSinkFn* fn_unregister_log_sink;

    using RegisterLogTagFn = void(ice::LogTagDefinition tag_definition) noexcept;
    using EnableLogTagFn = void(ice::LogTag tag, bool enabled) noexcept;

    extern RegisterLogTagFn* fn_register_log_tag;
    extern EnableLogTagFn* fn_enable_log_tag;

    using LogFn = void (
        ice::LogSeverity severity,
        ice::LogTag tag,
        ice::String message,
        fmt::format_args args,
        ice::detail::LogLocation location
    ) noexcept;

    extern LogFn* log_fn;

    using AssertFn = void (
        ice::String condition,
        ice::String message,
        fmt::format_args args,
        ice::detail::LogLocation location
    ) noexcept;

    extern AssertFn* assert_fn;

    constexpr auto tag_hash(ice::LogTag tag) noexcept -> ice::u64
    {
        return ice::bit_cast<ice::u64>(tag) & 0x0000'0000'ffff'ffffllu;
    }

    constexpr auto fmt_string(char const* begin, char const* end) noexcept -> fmt::string_view
    {
        return fmt::string_view{ begin, static_cast<ice::usize::base_type>(end - begin) };
    }

    constexpr auto fmt_string(ice::String str) noexcept -> fmt::string_view
    {
        return fmt_string(str.begin(), str.end());
    }

    static constexpr ice::String severity_value[]{
        "",
        "CRIT",
        "INFO",

        "ERRR",
        "WARN",
        "INFO",
        "VERB",

        "DEBG",
    };

#if ISP_WEBAPP || ISP_ANDROID || ISP_LINUX
    inline auto local_time() noexcept -> std::tm
    {
        std::chrono::system_clock::time_point const now = std::chrono::system_clock::now();
        std::time_t const current_time = std::chrono::system_clock::to_time_t(now);
        std::tm const* localtime = std::localtime(&current_time);
        return *localtime;
    }
#else
    inline auto local_time() noexcept
    {
        static auto const current_timezone = std::chrono::current_zone();
        return current_timezone->to_local(std::chrono::system_clock::now());
    }
#endif

} // namespace ice::detail
