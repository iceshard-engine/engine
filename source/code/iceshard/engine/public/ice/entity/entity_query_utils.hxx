#pragma once
#include <ice/entity/entity_archetype.hxx>
#include <ice/archetype/archetype_info.hxx>

namespace ice
{

    namespace detail
    {

        template<EntityComponent... Components>
        constexpr auto map_arguments_to_archetype_index() noexcept -> std::array<ice::u32, sizeof...(Components)>
        {
            constexpr ice::Archetype<Components...> archetype;

            constexpr ice::u32 component_count = sizeof...(Components);
            constexpr ice::StringID_Hash components[]{ ice::stringid_hash(ice::ComponentIdentifier<Components>)... };

            std::array<ice::u32, sizeof...(Components)> result{ };
            for (ice::u32 idx = 0; idx < component_count; ++idx)
            {
                ice::u32 arg_idx = 0;
                for (ice::ArchetypeComponent const& component_arg : archetype.components)
                {
                    if (component_arg.name == components[idx])
                    {
                        break;
                    }
                    arg_idx += 1;
                }
                result[idx] = arg_idx;
            }

            return result;
        }

        template<typename... Components>
        inline auto map_arguments_to_archetype_index(
            ice::ArchetypeInfo const& archetype
        ) noexcept -> std::array<ice::u32, sizeof...(Components)>
        {
            constexpr ice::u32 component_count = sizeof...(Components);
            constexpr ice::StringID components[]{ ice::ComponentIdentifier<Components>... };

            std::array<ice::u32, sizeof...(Components)> result{ };
            for (ice::u32 idx = 0; idx < component_count; ++idx)
            {
                result[idx] = std::numeric_limits<ice::u32>::max();

                ice::u32 arg_idx = 0;
                for (ice::StringID_Arg component_arg : archetype.components)
                {
                    if (component_arg == components[idx])
                    {
                        result[idx] = arg_idx;
                        break;
                    }
                    arg_idx += 1;
                }
            }

            return result;
        }

        struct ArchetypeComponentIndexPair
        {
            ice::u32 idx_first;
            ice::u32 idx_second;
        };

        inline auto map_components_between_related_archetypes_in_order(
            ice::ArchetypeInfo const& first,
            ice::ArchetypeInfo const& second,
            ice::Span<ArchetypeComponentIndexPair> out_pairs,
            bool const swap_pairs
        ) noexcept
        {
            ice::u32 const count_first = ice::size(first.components);
            ice::u32 const count_second = ice::size(second.components);

            ice::u32 idx_first = 0;
            ice::u32 idx_second = 0;

            for (; idx_first < count_first && idx_second < count_second; ++idx_first)
            {
                if (first.components[idx_first] != second.components[idx_second])
                {
                    continue;
                }

                if (swap_pairs)
                {
                    out_pairs[idx_second] = { idx_second, idx_first };
                }
                else
                {
                    out_pairs[idx_second] = { idx_first, idx_second };
                }

                idx_second += 1;
            }

            return idx_second;
        }

        inline auto map_components_between_related_archetypes(
            ice::ArchetypeInfo const& first,
            ice::ArchetypeInfo const& second,
            ice::Span<ArchetypeComponentIndexPair> out_pairs
        ) noexcept
        {
            if (ice::size(first.components) > ice::size(second.components))
            {
                return map_components_between_related_archetypes_in_order(first, second, out_pairs, false);
            }
            else
            {
                return map_components_between_related_archetypes_in_order(second, first, out_pairs, true);
            }
        }

        constexpr ice::Archetype<ValidationComponent_Position2D, ValidationComponent_Velocity2D> validation_argument_archetype{};

        constexpr ice::StringID validation_argument_identifiers_1[]{ ValidationComponent_Velocity2D::Identifier, ValidationComponent_Position2D::Identifier };
        constexpr auto validation_mapped_arguments_1 = map_arguments_to_archetype_index<ValidationComponent_Velocity2D, ValidationComponent_Position2D>();

        constexpr ice::StringID validation_argument_identifiers_2[]{ ValidationComponent_Position2D::Identifier, ValidationComponent_Velocity2D::Identifier };
        constexpr auto validation_mapped_arguments_2 = map_arguments_to_archetype_index<ValidationComponent_Position2D, ValidationComponent_Velocity2D>();

        static_assert(validation_mapped_arguments_1.size() == 2);
        static_assert(validation_mapped_arguments_2.size() == 2);

        // NOTE: In the following tests:
        //  We are subtracting '1' from 'validation_mapped_arguments_1[...]', as the function calculates the absolute position of the component in an archetype.
        //  Which means that it takes into account that the first component is always the Entity itself.
        static_assert(
            ice::StringID{ validation_argument_archetype.components[1].name }
            ==
            validation_argument_identifiers_1[validation_mapped_arguments_1[0] - 1]
        );
        static_assert(
            ice::StringID{ validation_argument_archetype.components[2].name }
            ==
            validation_argument_identifiers_1[validation_mapped_arguments_1[1] - 1]
        );

        static_assert(
            ice::StringID{ validation_argument_archetype.components[1].name }
            ==
            validation_argument_identifiers_2[validation_mapped_arguments_2[0] - 1]
        );
        static_assert(
            ice::StringID{ validation_argument_archetype.components[2].name }
            ==
            validation_argument_identifiers_2[validation_mapped_arguments_2[1] - 1]
        );

    } // namespace detail

} // namespace ice
