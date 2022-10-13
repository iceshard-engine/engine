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

    auto fmt_string(char const* begin, char const* end) noexcept -> fmt::string_view
    {
#if _MSC_VER == 1927
        return fmt::string_view{ begin, static_cast<ice::u64>(end - begin) };
#else
        return fmt::string_view{ begin, static_cast<ice::u64>(end - begin) };
#endif
    }

    auto fmt_string(ice::String str) noexcept -> fmt::string_view
    {
        return fmt_string(str._data, str._data + str._size);
    }

} // namespace ice::detail
