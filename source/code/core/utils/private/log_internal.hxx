/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/string_types.hxx>
#include <ice/container_types.hxx>
#include <ice/log_tag.hxx>
#include <ice/log_severity.hxx>
#include <ice/log.hxx>

#include <fmt/format.h>

namespace ice::detail
{

    class LogState final
    {
    public:
        static ice::u32 minimal_header_length;

        LogState(ice::Allocator& alloc) noexcept;
        ~LogState() noexcept;

        void register_tag(ice::LogTagDefinition tag_def) noexcept;

        auto tag_name(ice::LogTag tag) const noexcept -> ice::String;

    private:
        ice::Allocator& _allocator;

        ice::HeapString<> _empty_tag;
        ice::HashMap<ice::HeapString<>, ice::ContainerLogic::Complex> _tags;
    };

    extern LogState* internal_log_state;

    void default_register_tag_fn(
        ice::LogTagDefinition tag_def
    ) noexcept;

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

    using RegisterLogTagFn = void (
        ice::LogTagDefinition tag_definition
    ) noexcept;

    extern RegisterLogTagFn* register_log_tag_fn;

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

    constexpr auto fmt_string(char const* begin, char const* end) noexcept -> fmt::string_view
    {
        return fmt::string_view{ begin, static_cast<ice::u64>(end - begin) };
    }

    constexpr auto fmt_string(ice::String str) noexcept -> fmt::string_view
    {
        return fmt_string(str._data, str._data + str._size);
    }

    static constexpr ice::String severity_value[]{
        "",
        "CRIT",
        "HIGH",

        "ERRR",
        "WARN",
        "INFO",
        "VERB",

        "DEBG",
    };

} // namespace ice::detail
