#include "log_internal.hxx"

namespace ice::detail
{

    LogState::LogState(ice::Allocator& alloc) noexcept
        : _allocator{ alloc }
        , _empty_tag{ _allocator }
        , _tags{ _allocator }
    { }

    LogState::~LogState() noexcept
    { }

    auto LogState::register_tag(ice::LogTagDefinition tag_def) noexcept
    {
        ice::map::set(_tags, tag_def.tag, ice::HeapString<char>{ _allocator, tag_def.name });
    }

    auto LogState::tag_name(ice::LogTag tag) const noexcept -> ice::String
    {
        return ice::map::get(_tags, tag, _empty_tag);
    }

} // namespace ice::detail
