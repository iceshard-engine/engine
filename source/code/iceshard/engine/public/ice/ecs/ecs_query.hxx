/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/ecs/ecs_archetype.hxx>
#include <ice/ecs/ecs_data_block.hxx>
#include <ice/ecs/ecs_query_details.hxx>
#include <ice/container/array.hxx>
#include <ice/span.hxx>

namespace ice::ecs
{

    using ice::ecs::detail::QueryType;

    template<typename Definition>
    struct Query;

    namespace query
    {

        template<typename Definition>
        auto block_count(ice::ecs::Query<Definition> const& query) noexcept -> ice::u32;

        template<typename Definition>
        auto entity_count(ice::ecs::Query<Definition> const& query) noexcept -> ice::u32;

        template<typename Definition>
        auto for_each_block(ice::ecs::Query<Definition> const& query, typename Definition::ForEachBlockFn&& fn) noexcept;

        template<typename Definition>
        auto for_each_entity(ice::ecs::Query<Definition> const& query, typename Definition::ForEachEntityFn&& fn) noexcept;

    } // namespace query


    //! \brief A static compile-time definition of a Query that can be executed during frame.
    //! \tparam ...QueryComponents Component types, with decorators, we want to access in the qyery.
    //!
    //! \example QueryDefinition<ComponentB&, const ComponentA*>
    template<ice::ecs::QueryType... QueryComponents>
    struct QueryDefinition
    {
        using Query = ice::ecs::Query<ice::ecs::QueryDefinition<QueryComponents...>>;

        template<typename Fn>
        static void invoke_for_each_block(Fn&& fn, ice::u32 count, void** component_pointer_array) noexcept
        {
            return ice::ecs::detail::invoke_for_each_block<Fn, QueryComponents...>(
                ice::forward<Fn>(fn),
                count,
                component_pointer_array
            );
        }

        template<typename Fn>
        static void invoke_for_each_entity(Fn&& fn, ice::u32 count, void** component_pointer_array) noexcept
        {
            return ice::ecs::detail::invoke_for_each_entity<Fn, QueryComponents...>(
                ice::forward<Fn>(fn),
                count,
                component_pointer_array
            );
        }

        ice::u32 const component_count = sizeof...(QueryComponents);

        ice::StaticArray<ice::ecs::detail::QueryTypeInfo, sizeof...(QueryComponents)> const requirements = ice::ecs::detail::QueryRequirements<QueryComponents...>::Constant_Requirements;
    };

    //! \brief A query holds information about all archetypes that should be iterated but it's not yet executed.
    template<typename Definition>
    struct Query
    {
        static constexpr Definition Constant_Definition{ };

        ice::Array<ice::ecs::ArchetypeInstanceInfo const*> const archetype_instances;
        ice::Array<ice::ecs::DataBlock const*> const archetype_data_blocks;

        ice::Array<ice::StaticArray<ice::u32, Constant_Definition.component_count>> const archetype_argument_idx_map;
    };


    namespace query
    {

        template<typename Definition>
        auto block_count(ice::ecs::Query<Definition> const& query) noexcept -> ice::u32
        {
            ice::u32 result = 0;
            for (ice::ecs::DataBlock const* const head_block : query.archetype_data_blocks)
            {
                ice::ecs::DataBlock const* it = head_block;
                while (it != nullptr)
                {
                    result += 1;
                    it = it->next;
                }
            }
            return result;
        }

        template<typename Definition>
        auto entity_count(ice::ecs::Query<Definition> const& query) noexcept -> ice::u32
        {
            ice::u32 result = 0;
            for (ice::ecs::DataBlock const* const head_block : query.archetype_data_blocks)
            {
                ice::ecs::DataBlock const* it = head_block;
                while (it != nullptr)
                {
                    result += it->block_entity_count;
                    it = it->next;
                }
            }
            return result;
        }


        template<typename Definition, typename Fn>
        auto for_each_block(ice::ecs::Query<Definition> const& query, Fn&& fn) noexcept
        {
            static constexpr Definition const& query_definition = ice::ecs::Query<Definition>::Constant_Definition;

            void* helper_pointer_array[query_definition.component_count]{ nullptr };

            ice::u32 const arch_count = ice::count(query.archetype_instances);
            for (ice::u32 arch_idx = 0; arch_idx < arch_count; ++arch_idx)
            {
                ice::ecs::ArchetypeInstanceInfo const* arch = query.archetype_instances[arch_idx];
                ice::ecs::DataBlock const* block = query.archetype_data_blocks[arch_idx];
                ice::StaticArray<ice::u32, query_definition.component_count> const& argument_idx_map = query.archetype_argument_idx_map[arch_idx];

                while (block != nullptr)
                {
                    for (ice::u32 arg_idx = 0; arg_idx < query_definition.component_count; ++arg_idx)
                    {
                        if (argument_idx_map[arg_idx] == ice::u32_max)
                        {
                            helper_pointer_array[arg_idx] = nullptr;
                        }
                        else
                        {
                            ice::u32 const cmp_idx = argument_idx_map[arg_idx];

                            helper_pointer_array[arg_idx] = ice::ptr_add(
                                block->block_data,
                                { arch->component_offsets[cmp_idx] }
                            );
                        }
                    }

                    Definition::invoke_for_each_block(
                        ice::forward<Fn>(fn),
                        block->block_entity_count,
                        helper_pointer_array
                    );

                    block = block->next;
                }
            }
        }


        template<typename Definition, typename Fn>
        auto for_each_entity(ice::ecs::Query<Definition> const& query, Fn&& fn) noexcept
        {
            static constexpr Definition const& query_definition = ice::ecs::Query<Definition>::Constant_Definition;

            void* helper_pointer_array[query_definition.component_count]{ nullptr };

            ice::u32 const arch_count = ice::count(query.archetype_instances);
            for (ice::u32 arch_idx = 0; arch_idx < arch_count; ++arch_idx)
            {
                ice::ecs::ArchetypeInstanceInfo const* arch = query.archetype_instances[arch_idx];
                ice::ecs::DataBlock const* block = query.archetype_data_blocks[arch_idx];
                ice::StaticArray<ice::u32, query_definition.component_count> const& argument_idx_map = query.archetype_argument_idx_map[arch_idx];

                while (block != nullptr)
                {
                    for (ice::u32 arg_idx = 0; arg_idx < query_definition.component_count; ++arg_idx)
                    {
                        if (argument_idx_map[arg_idx] == ice::u32_max)
                        {
                            helper_pointer_array[arg_idx] = nullptr;
                        }
                        else
                        {
                            ice::u32 const cmp_idx = argument_idx_map[arg_idx];

                            helper_pointer_array[arg_idx] = ice::ptr_add(
                                block->block_data,
                                { arch->component_offsets[cmp_idx] }
                            );
                        }
                    }

                    Definition::invoke_for_each_entity(
                        ice::forward<Fn>(fn),
                        block->block_entity_count,
                        helper_pointer_array
                    );

                    block = block->next;
                }
            }
        }

    } // namespace query


} // namespace ice::ecs
