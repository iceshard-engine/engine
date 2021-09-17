#pragma once
#include <ice/stringid.hxx>
#include <ice/ecs/ecs_component.hxx>
#include <ice/span.hxx>
#include <array>

namespace ice::ecs
{

    enum class Archetype : ice::u64
    {
        Invalid = 0x0
    };

    template<ice::ecs::Component... Components>
    struct ArchetypeInfo
    {
        static constexpr ice::u32 Const_ComponentCount = sizeof...(Components) + 1;

        std::array<ice::StringID const, Const_ComponentCount> component_identifiers;
        std::array<ice::u32 const, Const_ComponentCount> component_sizes;
        std::array<ice::u32 const, Const_ComponentCount> component_alignments;

        constexpr ArchetypeInfo() noexcept;
    };

    struct ArchetypeComponentsInfo
    {
        ice::Span<ice::StringID const> component_names;
        ice::Span<ice::u32 const> component_sizes;
        ice::Span<ice::u32 const> component_alignments;
    };

} // namespace ice::ecs
