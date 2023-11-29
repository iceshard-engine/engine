/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
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
    { }

    LogState::~LogState() noexcept
    { }

    void LogState::register_tag(ice::LogTagDefinition tag_def) noexcept
    {
        ice::hashmap::set(
            _tags,
            tag_hash(tag_def.tag),
            { ice::HeapString<char>{ _allocator, tag_def.name }, true }
        );
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

} // namespace ice::detail
