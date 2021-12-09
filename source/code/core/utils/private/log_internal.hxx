#pragma once
#include <ice/map.hxx>
#include <ice/heap_string.hxx>
#include <ice/log_tag.hxx>

namespace ice::detail
{

    class LogState final
    {
    public:
        LogState(ice::Allocator& alloc) noexcept;
        ~LogState() noexcept;

        auto register_tag(ice::LogTagDefinition tag_def) noexcept;

        auto tag_name(ice::LogTag tag) const noexcept -> ice::String;

    private:
        ice::Allocator& _allocator;

        ice::HeapString<> _empty_tag;
        ice::Map<ice::LogTag, ice::HeapString<>> _tags;
    };

    static LogState* internal_log_state = nullptr;


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
        ice::LogTag tag,
        ice::String message,
        fmt::format_args args,
        ice::detail::LogLocation location
    ) noexcept;


    static uint32_t log_header_size = 0;


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

    auto make_string(char const* begin, char const* end) noexcept -> ice::String;

} // namespace ice::detail
