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

    void LogState::register_tag(ice::LogTagDefinition tag_def) noexcept
    {
        ice::map::set(_tags, tag_def.tag, ice::HeapString<char>{ _allocator, tag_def.name });
    }

    auto LogState::tag_name(ice::LogTag tag) const noexcept -> ice::String
    {
        return ice::map::get(_tags, tag, _empty_tag);
    }


    auto make_string(char const* begin, char const* end) noexcept -> ice::String
    {
#if _MSC_VER == 1927
        return ice::String{ begin, static_cast<ice::u64>(end - begin) };
#else
        return ice::String{ begin, end };
#endif
    }

} // namespace ice::detail
