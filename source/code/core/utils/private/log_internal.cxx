/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "log_internal.hxx"
#include <ice/container/hashmap.hxx>
#include <ice/string/heap_string.hxx>

namespace ice::detail
{

    ice::u32 LogState::minimal_header_length = 0;

    LogState::LogState(ice::Allocator& alloc) noexcept
        : _allocator{ alloc }
        , _empty_tag{ ice::HeapString<>{ _allocator }, true }
        , _tags{ _allocator }
        , _sinks{ _allocator }
    {
        ice::array::resize(_sinks, 5);
    }

    LogState::~LogState() noexcept = default;

    void LogState::register_tag(ice::LogTagDefinition tag_def) noexcept
    {
        ice::hashmap::set(
            _tags,
            tag_hash(tag_def.tag),
            { ice::HeapString<char>{ _allocator, tag_def.name }, true }
        );
    }

    auto LogState::register_sink(ice::LogSinkFn fn_sink, void* userdata) noexcept -> ice::LogSinkID
    {
        ice::u32 const sinkidx = ice::count(_sinks);
        // Pottentially an error when sinks are added and remove all the time!
        // NOTE: Once added sinks should only be reset when a module was reloaded!
        ICE_ASSERT_CORE(sinkidx < 50);
        ice::array::push_back(_sinks, Sink{ fn_sink, userdata });
        return static_cast<ice::LogSinkID>(sinkidx);
    }

    void LogState::unregister_sink(ice::LogSinkID sinkid) noexcept
    {
        ice::u32 const sinkidx = static_cast<ice::u32>(sinkid);
        if (ice::count(_sinks) > sinkidx)
        {
            // Just clear the values
            _sinks[sinkidx] = Sink{ nullptr, nullptr };
        }
    }

    void LogState::enable_tag(ice::LogTag tag, bool enabled) noexcept
    {
        if (LogTagInfo* tagv = ice::hashmap::try_get(_tags, tag_hash(tag)))
        {
            tagv->enabled = enabled;
        }
    }

    auto LogState::tag_name(ice::LogTag tag) const noexcept -> ice::String
    {
        return ice::hashmap::get(
            _tags,
            tag_hash(tag),
            _empty_tag
        ).name;
    }

    bool LogState::tag_enabled(ice::LogTag tag) const noexcept
    {
        return ice::hashmap::get(
            _tags,
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
