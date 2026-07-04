/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "log_internal.hxx"
#include <ice/hashmap.hxx>
#include <ice/heap_string.hxx>
#include <ice/i18n_core_module.hxx>

namespace ice::detail
{

    ice::u32 LogState::minimal_header_length = 0;

    LogState::LogState(ice::Allocator& alloc) noexcept
        : _allocator{ alloc }
        , _empty_tag{ ice::HeapString<>{ _allocator }, true }
        , _tags{ _allocator }
        , _sinks{ _allocator }
    {
        _sinks.resize(5);
    }

    LogState::~LogState() noexcept = default;

    auto LogState::resolve(ice::I18NReference const& i18n_string, fmt::format_args const& args) const noexcept -> ice::String
    {
        return I18NCoreModule::resolve(i18n_string, args);
    }

    void LogState::register_tag(ice::LogTagDefinition tag_def) noexcept
    {
        _tags.set(
            tag_hash(tag_def.tag),
            { ice::HeapString<char>{ _allocator, tag_def.name }, true }
        );
    }

    auto LogState::register_sink(ice::LogSinkFn fn_sink, void* userdata) noexcept -> ice::LogSinkID
    {
        ice::u32 const sinkidx = _sinks.size().u32();
        // Potentially an error when sinks are added and remove all the time!
        // NOTE: Once added sinks should only be reset when a module was reloaded!
        ICE_ASSERT_CORE(sinkidx < 50);
        _sinks.push_back(Sink{ fn_sink, userdata });
        return static_cast<ice::LogSinkID>(sinkidx);
    }

    void LogState::unregister_sink(ice::LogSinkID sinkid) noexcept
    {
        ice::u32 const sinkidx = static_cast<ice::u32>(sinkid);
        if (_sinks.size() > sinkidx)
        {
            // Just clear the values
            _sinks[sinkidx] = Sink{ nullptr, nullptr };
        }
    }

    void LogState::enable_tag(ice::LogTag tag, bool enabled) noexcept
    {
        if (LogTagInfo* tagv = _tags.try_get(tag_hash(tag)))
        {
            tagv->enabled = enabled;
        }
    }

    auto LogState::tag_name(ice::LogTag tag) const noexcept -> ice::String
    {
        return _tags.get(
            tag_hash(tag),
            _empty_tag
        ).name;
    }

    bool LogState::tag_enabled(ice::LogTag tag) const noexcept
    {
        return _tags.get(
            tag_hash(tag),
            _empty_tag
        ).enabled;
    }

    void LogState::flush(ice::LogSinkMessage const& message) noexcept
    {
        for (Sink const& sink : _sinks)
        {
            if (sink._callback != nullptr)
            {
                sink._callback(sink._userdata, message);
            }
        }
    }

} // namespace ice::detail
