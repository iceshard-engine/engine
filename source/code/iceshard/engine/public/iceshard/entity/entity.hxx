#pragma once
#include <core/base.hxx>

namespace iceshard
{

    //! \brief A handle for a single entity.
    enum class Entity : uint64_t
    {
        Invalid = 0x0
    };

    //! \brief Checks if the entity handle is valid.
    constexpr bool valid(Entity handle) noexcept
    {
        return handle != Entity::Invalid;
    }

} // namespace iceshard::entity

template<>
constexpr auto core::hash<iceshard::Entity>(iceshard::Entity value) noexcept
{
    return static_cast<std::underlying_type_t<iceshard::Entity>>(value);
}
