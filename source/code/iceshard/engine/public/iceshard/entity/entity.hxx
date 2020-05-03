#pragma once
#include <core/base.hxx>

namespace iceshard
{

    //! \brief A handle for a single entity.
    enum class Entity : uint64_t { };

    namespace component
    {

        struct Entity
        {
            static constexpr auto identifier = "isc.entity"_sid;

            iceshard::Entity e;
        };

    } // namespace component

} // namespace iceshard

template<>
constexpr auto core::hash<iceshard::Entity>(iceshard::Entity value) noexcept -> uint64_t
{
    return static_cast<std::underlying_type_t<iceshard::Entity>>(value);
}

template<>
struct fmt::formatter<iceshard::Entity>
{
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx) noexcept
    {
        return ctx.begin();
    }

    template<typename FormatContext>
    constexpr auto format(iceshard::Entity value, FormatContext& ctx) noexcept
    {
        return fmt::format_to(ctx.out(), "[entity:{}]", static_cast<uint64_t>(value));
    }
};
