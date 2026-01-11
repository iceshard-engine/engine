/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/ecs/ecs_archetype_detail.hxx>
#include <ice/span.hxx>

namespace ice::ecs
{

    //! \brief Opaque handle identyfing a single archetype. This value is calculated from a sorted list of components defining that archetype.
    //!
    //! \remark If two archetypes are defined with the same components but in different order, the hash will still be the same.
    //! \remark The `Archetype::Invalid` value is referencing a special 'Null' archetype that cannot have entities and does not contain any data.
    //!
    //! \see `ice::ecs::static_validation` namespace for explicit compiletime checks and assertions.
    enum class Archetype : ice::u64
    {
        Invalid = 0x0
    };

    //! \brief Archetype compile-time definition using a set of components.
    //! \tparam ...Components A set of types that each entity will have access to.
    //!
    //! \see ice::ecs::concepts::Component
    template<ice::ecs::Component... Components>
    struct ArchetypeDefinition
    {
        //! \brief Quick access to the count of components for this archetype definition.
        //! \details To be used when no direct access to the typelist is available. (ex.: ArchType::ComponentCount)
        static constexpr ice::u32 ComponentCount = sizeof...(Components) + 1;

        //! \brief A optional name attached to this archetype. Can be used to access archetypes from the `ArchetypeIndex` without
        //!   knowing all or any of the components.
        ice::String name;

        //! \brief Unique identifier created from the sorted component list.
        ice::ecs::Archetype identifier;

        //! \brief List of identifiers for each component in the template list.
        //!
        //! \remark This array contains the sorted version, which means the value at index '0' might not correspond
        //!   to the type at index '0'.
        std::array<ice::StringID, ComponentCount> component_identifiers;

        //! \brief List of sizes for each component in the template list.
        //!
        //! \remark This array contains the sorted version, which means the value at index '0' might not correspond
        //!   to the type's size at index '0'.
        std::array<ice::u32, ComponentCount> component_sizes;

        //! \brief List of alignments for each component in the template list.
        //!
        //! \remark This array contains the sorted version, which means the value at index '0' might not correspond
        //!   to the type's alignment at index '0'.
        std::array<ice::u32, ComponentCount> component_alignments;

        constexpr inline ArchetypeDefinition(ice::String name = {}) noexcept;

        constexpr inline operator ice::ecs::Archetype() const noexcept;
    };

    //! \brief Quick access to an instance of `ArchetypeDefinition` with the given components.
    template<ice::ecs::Component... Components>
    static constexpr ArchetypeDefinition<Components...> Constant_ArchetypeDefinition{ };

    //! \brief Quick access to the identifier of an `ArchetypeDefinition` with the given components.
    template<ice::ecs::Component... Components>
    static constexpr ice::ecs::Archetype Constant_Archetype = Constant_ArchetypeDefinition<Components...>.identifier;

    //! \brief Provides the same information as `ArchetypeDefinition`, however it's not templated, allowing for type-erased access.
    struct ArchetypeInfo
    {
        //! \copydoc ArchetypeDefinition::name
        ice::String name;

        //! \copydoc ArchetypeDefinition::identifier
        ice::ecs::Archetype identifier;

        //! \copydoc ArchetypeDefinition::component_identifiers
        ice::Span<ice::StringID const> component_identifiers;

        //! \copydoc ArchetypeDefinition::component_sizes
        ice::Span<ice::u32 const> component_sizes;

        //! \copydoc ArchetypeDefinition::component_alignments
        ice::Span<ice::u32 const> component_alignments;

        constexpr inline ArchetypeInfo() noexcept = default;

        template<ice::ecs::Component... Components>
        constexpr inline ArchetypeInfo(
            ice::ecs::ArchetypeDefinition<Components...> const& archetype_info
        ) noexcept;
    };

    template<ice::ecs::Component... Components>
    constexpr inline ArchetypeDefinition<Components...>::ArchetypeDefinition(ice::String name) noexcept
        : name{ name }
        , component_identifiers{ ice::ecs::Constant_ComponentIdentifier<ice::ecs::Entity> }
        , component_sizes{ ice::ecs::Constant_ComponentSize<ice::ecs::Entity> }
        , component_alignments{ ice::ecs::Constant_ComponentAlignment<ice::ecs::Entity> }
    {
        auto const& sorted_components = ice::ecs::detail::SortedArchetypeInfo<Components...>::Const_Components;

        for (ice::u32 idx = 1; idx < ComponentCount; ++idx)
        {
            component_identifiers[idx] = sorted_components[idx - 1].identifier;
            component_sizes[idx] = sorted_components[idx - 1].size;
            component_alignments[idx] = sorted_components[idx - 1].alignment;
        }

        identifier = ice::ecs::detail::make_archetype_identifier(ice::make_span(component_identifiers));
    }

    template<ice::ecs::Component... Components>
    constexpr inline ArchetypeDefinition<Components...>::operator ice::ecs::Archetype() const noexcept
    {
        return identifier;
    }

    template<ice::ecs::Component... Components>
    constexpr inline ArchetypeInfo::ArchetypeInfo(
        ice::ecs::ArchetypeDefinition<Components...> const& archetype_info
    ) noexcept
        : name{ archetype_info.name }
        , identifier{ archetype_info.identifier }
        , component_identifiers{ ice::make_span(archetype_info.component_identifiers) }
        , component_sizes{ ice::make_span(archetype_info.component_sizes) }
        , component_alignments{ ice::make_span(archetype_info.component_alignments) }
    {
    }

    //! \brief A namespace containing various compile-time checks and validations to ensure all compile-time invariants are holding
    //!   after refactors or invasive changes into implementation details
    namespace static_validation
    {

        struct ValidationComponent_0x10
        {
            static constexpr ice::StringID Identifier = ice::StringID{ ice::StringID_Hash{ 0x10 } };

            ice::f32 pos[2];
        };

        struct ValidationComponent_0x20
        {
            static constexpr ice::StringID Identifier = ice::StringID{ ice::StringID_Hash{ 0x20 } };

            ice::f32 vel[2];
            ice::f32 angVel;
        };

        static constexpr auto Validation_Archetype_1 = ice::ecs::Constant_ArchetypeDefinition<ValidationComponent_0x10, ValidationComponent_0x20>;
        static constexpr auto Validation_Archetype_2 = ice::ecs::Constant_ArchetypeDefinition<ValidationComponent_0x20, ValidationComponent_0x10>;

        static_assert(Validation_Archetype_1.identifier == Validation_Archetype_2.identifier);

        static_assert(Validation_Archetype_1.component_identifiers.size() == 3);
        static_assert(Validation_Archetype_2.component_identifiers.size() == 3);

        static_assert(Validation_Archetype_1.component_identifiers[0] == ice::ecs::Constant_ComponentIdentifier<ice::ecs::Entity>);
        static_assert(Validation_Archetype_1.component_identifiers[1] == ice::ecs::Constant_ComponentIdentifier<ice::ecs::static_validation::ValidationComponent_0x10>);
        static_assert(Validation_Archetype_1.component_identifiers[2] == ice::ecs::Constant_ComponentIdentifier<ice::ecs::static_validation::ValidationComponent_0x20>);

        static_assert(Validation_Archetype_1.component_identifiers[0] == Validation_Archetype_2.component_identifiers[0]);
        static_assert(Validation_Archetype_1.component_identifiers[1] == Validation_Archetype_2.component_identifiers[1]);
        static_assert(Validation_Archetype_1.component_identifiers[2] == Validation_Archetype_2.component_identifiers[2]);

        static_assert(Validation_Archetype_1.component_sizes[0] == Validation_Archetype_2.component_sizes[0]);
        static_assert(Validation_Archetype_1.component_sizes[1] == Validation_Archetype_2.component_sizes[1]);
        static_assert(Validation_Archetype_1.component_sizes[2] == Validation_Archetype_2.component_sizes[2]);

    } // namespace static_validation

} // namespace ice::ecs
