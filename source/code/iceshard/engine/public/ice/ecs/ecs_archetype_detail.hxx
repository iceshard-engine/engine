/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/ecs/ecs_types.hxx>
#include <ice/ecs/ecs_data_block.hxx>
#include <ice/stringid.hxx>
#include <ice/sort.hxx>
#include <ice/algorithm.hxx>

namespace ice::ecs::detail
{

    //! \brief Opaque handle identyfying the direct instances of an archertype. This value should only be used by internal systems.
    enum class ArchetypeInstance : ice::u32 {};

    //! \brief Internal runtime information on how an `Archetype` and it's components are stored in each `DataBlock`.
    //!
    //! \details This information can be used to access entity data by the user in any custom fashion, however due to the complexity
    //!   of doing so, it's better to use queries or execute entity operations instead.
    struct ArchetypeInstanceInfo
    {
        ice::ecs::detail::ArchetypeInstance archetype_instance;
        ice::ecs::detail::DataBlockFilter data_block_filter;
        ice::Span<ice::StringID const> component_identifiers;
        ice::Span<ice::u32 const> component_sizes;
        ice::Span<ice::u32 const> component_alignments;
        ice::Span<ice::u32 const> component_offsets;
    };

    //! \brief Helper type to sort components during compiletime when instancing `ArchetypeDefinition`s.
    struct SortableArchetypeComponent
    {
        ice::StringID identifier;
        ice::u32 size;
        ice::u32 alignment;
    };

    constexpr bool operator<(SortableArchetypeComponent const& left, SortableArchetypeComponent const& right) noexcept
    {
        return ice::hash(left.identifier) < ice::hash(right.identifier);
    }

    //! \brief Helper type storing all archetype components before sorting.
    template<ice::ecs::Component... Components>
    struct UnsortedArchetypeInfo
    {
        static constexpr std::array<SortableArchetypeComponent, sizeof...(Components)> const Const_Components{
            SortableArchetypeComponent{
                .identifier = ice::ecs::Constant_ComponentIdentifier<Components>,
                .size = ice::ecs::Constant_ComponentSize<Components>,
                .alignment = ice::ecs::Constant_ComponentAlignment<Components>,
            }...
        };
    };

    //! \brief Helper type storing all archetype components sorted by their identifier.
    //!
    //! \important The sorting is important, because it allows us to check for components in O(N) instead of O(N^2)
    template<ice::ecs::Component... Components>
    struct SortedArchetypeInfo
    {
        static constexpr std::array<SortableArchetypeComponent, sizeof...(Components)> const Const_Components =
            ice::constexpr_sort_stdarray(UnsortedArchetypeInfo<Components...>::Const_Components);
    };

    //! \brief Create an archetype identifier / hash out of a list of component identifiers
    //! \pre The input list of components requires identifiers to be unique and sorted in ascending order.
    //!
    //! \param component_identifiers List of component identifiers to be mixed into the final hash.
    //! \return A 32bit hash representing a specific set of components.
    constexpr auto make_archetype_identifier(ice::Span<ice::StringID const> component_identifiers) noexcept -> ice::ecs::Archetype
    {
        ice::u64 handle_hash = ice::hash32("ice.__ecs_archetype__");
        for (ice::StringID_Arg component : component_identifiers)
        {
            handle_hash <<= 5;
            handle_hash ^= ice::hash(component);
        }
        return static_cast<Archetype>(handle_hash);
    }

    //! \brief Creates an static array that maps the index of each template parameter to the index in the input list.
    //! \pre The input list contains at least ALL component identifiers that are passed as type arguments.
    //!   This means that if components A, B and C are passed as template arguments the input argument requires these components to exist in that list.
    //!   Otherwise the resulting array is considered incomplete and accessing data using is undefined. The input list may contain additional entries.
    //!
    //! \tparam ...Components The component types which indicies we want to map to the locations in the input list.
    //! \param in_list_mapped_into The list we want the components to be mapped to.
    //! \return A static size array of `sizeof...(Components)` containing the index where each `Component::Identifier == in_list_mapped_into[indexof(Component)]`
    //!
    //! \example When called with types `<A, C, B>` with a list containing `(B, D, C, A)` the resulting array holds values `(3, 2, 0)`
    template<ice::ecs::Component... Components>
    constexpr auto make_argument_idx_map(
        ice::Span<ice::StringID const> in_list_mapped_into
    ) noexcept -> ice::StaticArray<ice::u32, sizeof...(Components)>
    {
        constexpr ice::u32 component_count = sizeof...(Components);
        constexpr ice::StringID components_identifiers_original_order[]{ ice::ecs::Constant_ComponentIdentifier<Components>... };

        ice::StaticArray<ice::u32, component_count> result{ ice::u32_max };
        for (ice::u32 idx = 0; idx < component_count; ++idx)
        {
            result[idx] = std::numeric_limits<ice::u32>::max();

            ice::u32 arg_idx = 0;
            for (ice::StringID_Arg component_arg : in_list_mapped_into)
            {
                if (component_arg == components_identifiers_original_order[idx])
                {
                    result[idx] = arg_idx;
                    break;
                }
                arg_idx += 1;
            }
        }
        return result;
    }

    constexpr auto calculate_entity_count_for_space(ice::ecs::detail::ArchetypeInstanceInfo const& arch, ice::usize space) noexcept -> ice::ucount
    {
        ice::u32 const component_size_sum = ice::accumulate(arch.component_sizes, 0);
        ice::u32 const component_alignment_sum = ice::accumulate(arch.component_alignments, 0);

        ice::usize const available_block_size = { ice::usize::subtract(space, arch.data_block_filter.data_size).value - component_alignment_sum };
        return ice::ucount(available_block_size.value) / component_size_sum;
    }

} // namespace ice::ecs
