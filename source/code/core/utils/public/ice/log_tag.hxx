#pragma once
#include <ice/hash.hxx>
#include <ice/string.hxx>

namespace ice
{

    enum class LogTag : ice::u64
    {
        None = 0x0,

        Core = 0x1ull << 32,
        System = Core << 1,
        Module = Core << 2,
        Engine = Core << 3,
    };

    struct LogTagDefinition
    {
        ice::LogTag const tag;
        ice::String const name;
    };

    constexpr auto create_log_tag(LogTag base_tag, ice::String name) noexcept -> LogTagDefinition
    {
        ice::u64 const name_hash = ice::hash32(name);
        ice::u64 const tag_hash = ice::hash(base_tag);
        return {
            .tag = static_cast<LogTag>(tag_hash | name_hash),
            .name = name
        };
    }

    void register_log_tag(ice::LogTagDefinition tag_def) noexcept;

    namespace detail
    {

        constexpr auto get_tag(ice::LogTag tag) noexcept
        {
            return tag;
        }

        constexpr auto get_tag(ice::LogTagDefinition const& tag_def) noexcept
        {
            return tag_def.tag;
        }

    } // namespace detail

} // namespace ice
