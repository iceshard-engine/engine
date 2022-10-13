#include "log_internal.hxx"
#include <ice/container/hashmap.hxx>

namespace ice::detail
{

    ice::u32 LogState::minimal_header_length = 0;

    LogState::LogState(ice::Allocator& alloc) noexcept
        : _allocator{ alloc }
        , _empty_tag{ _allocator }
        , _tags{ _allocator }
    { }

    LogState::~LogState() noexcept
    { }

    void LogState::register_tag(ice::LogTagDefinition tag_def) noexcept
    {
        ice::hashmap::set(_tags, ice::hash(tag_def.tag), ice::HeapString<char>{ _allocator, tag_def.name });
    }

    auto LogState::tag_name(ice::LogTag tag) const noexcept -> ice::String
    {
        return ice::hashmap::get(_tags, ice::hash(tag), _empty_tag);
    }

} // namespace ice::detail
