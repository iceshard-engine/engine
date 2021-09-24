#pragma once
#include <ice/stringid.hxx>
#include <ice/ecs/ecs_component.hxx>
#include <ice/ecs/ecs_detail.hxx>
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

        std::array<ice::StringID, Const_ComponentCount> component_identifiers;
        std::array<ice::u32, Const_ComponentCount> component_sizes;
        std::array<ice::u32, Const_ComponentCount> component_alignments;

        inline constexpr ArchetypeInfo() noexcept;
    };

    template<ice::ecs::Component... Components>
    static constexpr ArchetypeInfo<Components...> Constant_ArchetypeInfo{ };


    struct ArchetypeComponentsInfo
    {
        ice::Span<ice::StringID const> component_identifiers;
        ice::Span<ice::u32 const> component_sizes;
        ice::Span<ice::u32 const> component_alignments;

        template<ice::ecs::Component... Components>
        inline constexpr ArchetypeComponentsInfo(
            ice::ecs::ArchetypeInfo<Components...> const& archetype_info
        ) noexcept;
    };


    namespace detail
    {

        struct SortableArchetypeComponent
        {
            ice::StringID identifier;
            ice::u32 size;
            ice::u32 alignment;
        };

        constexpr bool operator<(
            SortableArchetypeComponent const& left,
            SortableArchetypeComponent const& right
        ) noexcept
        {
            return ice::hash(left.identifier) < ice::hash(right.identifier);
        }

        template<ice::ecs::Component... Components>
        struct UnsortedArchetypeInfo
        {
            static constexpr std::array<SortableArchetypeComponent, sizeof...(Components)> const Const_UnsortedCcomponents{
                SortableArchetypeComponent{
                    .identifier = ice::ecs::ComponentIdentifier<Components>,
                    .size = sizeof(Components),
                    .alignment = alignof(Components),
                }...
            };
        };

        template<ice::ecs::Component... Components>
        struct SortedArchetypeInfo
        {
            static constexpr std::array<SortableArchetypeComponent, sizeof...(Components)> const Const_SortedComponents =
                constexpr_sort_array(UnsortedArchetypeInfo<Components...>::Const_UnsortedCcomponents);
        };

    } // namespace detail


    template<ice::ecs::Component... Components>
    inline constexpr ArchetypeInfo<Components...>::ArchetypeInfo() noexcept
        : component_identifiers{ ice::ecs::ComponentIdentifier<ice::ecs::EntityHandle> }
        , component_sizes{ sizeof(ice::ecs::EntityHandle) }
        , component_alignments{ alignof(ice::ecs::EntityHandle) }
    {
        auto const& sorted_components = ice::ecs::detail::SortedArchetypeInfo<Components...>::Const_SortedComponents;

        for (ice::u32 idx = 1; idx < Const_ComponentCount; ++idx)
        {
            component_identifiers[idx] = sorted_components[idx-1].identifier;
            component_sizes[idx] = sorted_components[idx-1].size;
            component_alignments[idx] = sorted_components[idx-1].alignment;
        }
    }

    template<ice::ecs::Component ...Components>
    inline constexpr ArchetypeComponentsInfo::ArchetypeComponentsInfo(
        ice::ecs::ArchetypeInfo<Components...> const& archetype_info
    ) noexcept
        : component_identifiers{ archetype_info.component_identifiers }
        , component_sizes{ archetype_info.component_sizes }
        , component_alignments{ archetype_info.component_alignments }
    {
    }


    namespace static_validation
    {

        struct ValidationComponent_0x10
        {
            static constexpr ice::StringID Identifier = ice::StringID{ ice::StringID_Hash{ 0x10 } };

            ice::vec2f pos;
        };

        struct ValidationComponent_0x20
        {
            static constexpr ice::StringID Identifier = ice::StringID{ ice::StringID_Hash{ 0x20 } };

            ice::vec2f vel;
            ice::vec2f angVel;
        };

        static constexpr auto Validation_Archetype_1 = ice::ecs::Constant_ArchetypeInfo<ValidationComponent_0x10, ValidationComponent_0x20>;
        static constexpr auto Validation_Archetype_2 = ice::ecs::Constant_ArchetypeInfo<ValidationComponent_0x20, ValidationComponent_0x10>;

        static_assert(Validation_Archetype_1.component_identifiers.size() == 3);
        static_assert(Validation_Archetype_2.component_identifiers.size() == 3);

        static_assert(Validation_Archetype_1.component_identifiers[0] == ice::ecs::ComponentIdentifier<ice::ecs::EntityHandle>);
        static_assert(Validation_Archetype_1.component_identifiers[1] == ice::ecs::ComponentIdentifier<ice::ecs::static_validation::ValidationComponent_0x10>);
        static_assert(Validation_Archetype_1.component_identifiers[2] == ice::ecs::ComponentIdentifier<ice::ecs::static_validation::ValidationComponent_0x20>);

        static_assert(Validation_Archetype_1.component_identifiers[0] == Validation_Archetype_2.component_identifiers[0]);
        static_assert(Validation_Archetype_1.component_identifiers[1] == Validation_Archetype_2.component_identifiers[1]);
        static_assert(Validation_Archetype_1.component_identifiers[2] == Validation_Archetype_2.component_identifiers[2]);

        static_assert(Validation_Archetype_1.component_sizes[0] == Validation_Archetype_2.component_sizes[0]);
        static_assert(Validation_Archetype_1.component_sizes[1] == Validation_Archetype_2.component_sizes[1]);
        static_assert(Validation_Archetype_1.component_sizes[2] == Validation_Archetype_2.component_sizes[2]);

    } // namespace static_validation

} // namespace ice::ecs
