/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/ecs/ecs_types.hxx>
#include <ice/ecs/ecs_archetype.hxx>
#include <ice/ecs/ecs_data_block.hxx>
#include <ice/ecs/ecs_query_definition.hxx>
#include <ice/container/array.hxx>
#include <ice/span.hxx>

namespace ice::ecs
{

    using ice::ecs::detail::QueryType;

    namespace query
    {

        template<typename Definition>
        inline auto block_count(ice::ecs::Query<Definition> const& query) noexcept -> ice::u32;

        template<typename Definition>
        inline auto entity_count(ice::ecs::Query<Definition> const& query) noexcept -> ice::u32;

        template<typename Definition>
        inline auto for_each_block(
            ice::ecs::Query<Definition> const& query,
            typename Definition::ForEachBlockFn&& fn
        ) noexcept;

        template<ice::ecs::QueryType... QueryComponents>
        inline auto for_each_block(
            ice::ecs::Query<ice::ecs::QueryDefinition<QueryComponents...>> const& query
        ) noexcept -> ice::Generator<ice::ecs::detail::QueryBlockTupleResult<QueryComponents...>>;

        template<typename Definition>
        inline auto for_each_entity(
            ice::ecs::Query<Definition> const& query,
            typename Definition::ForEachEntityFn&& fn
        ) noexcept;

        template<ice::ecs::QueryType... QueryComponents>
        inline auto for_each_entity(
            ice::ecs::Query<ice::ecs::QueryDefinition<QueryComponents...>> const& query
        ) noexcept -> ice::Generator<ice::ecs::detail::QueryEntityTupleResult<QueryComponents...>>;

    } // namespace query


    //! \brief A query holds information about all archetypes that should be iterated but it's not yet executed.
    template<typename Definition>
    struct Query
    {
        static constexpr Definition Constant_Definition{ };

        Query(ice::Allocator& alloc) noexcept;

        Query(Query&& other) noexcept;
        auto operator=(Query&& other) noexcept -> Query&;
        Query(Query const& other) noexcept = delete;
        auto operator=(Query const& other) noexcept -> Query& = delete;

        using ArrayArchetypeInstances = ice::Array<ice::ecs::ArchetypeInstanceInfo const*>;
        using ArrayDataBlock = ice::Array<ice::ecs::DataBlock const*>;
        using ArrayArgumentIndexMap = ice::Array<ice::StaticArray<ice::u32, Constant_Definition.component_count>>;

        ice::ecs::QueryAccessTracker* access_trackers[Constant_Definition.component_count];
        ice::Array<ice::ecs::ArchetypeInstanceInfo const*> archetype_instances;
        ice::Array<ice::ecs::DataBlock const*> archetype_data_blocks;
        ice::Array<ice::u32> archetype_argument_idx_map;
    };

    template<typename Definition>
    Query<Definition>::Query(ice::Allocator& alloc) noexcept
        : access_trackers{ }
        , archetype_instances{ alloc }
        , archetype_data_blocks{ alloc }
        , archetype_argument_idx_map{ alloc }
    {
    }

    template<typename Definition>
    Query<Definition>::Query(Query&& other) noexcept
        : access_trackers{ }
        , archetype_instances{ ice::move(other.archetype_instances) }
        , archetype_data_blocks{ ice::move(other.archetype_data_blocks) }
        , archetype_argument_idx_map{ ice::move(other.archetype_argument_idx_map) }
    {
        ice::memcpy(ice::addressof(access_trackers), ice::addressof(other.access_trackers), sizeof(access_trackers));
    }

    template<typename Definition>
    auto Query<Definition>::operator=(Query&& other) noexcept -> Query&
    {
        if (this == ice::addressof(other))
        {
            ice::memcpy(ice::addressof(access_trackers), ice::addressof(other.access_trackers), sizeof(access_trackers));
            archetype_instances = ice::move(other.archetype_instances);
            archetype_data_blocks = ice::move(other.archetype_data_blocks);
            archetype_argument_idx_map = ice::move(other.archetype_argument_idx_map);
        }
        return *this;
    }

    namespace query
    {

        template<typename Definition>
        inline auto block_count(ice::ecs::Query<Definition> const& query) noexcept -> ice::u32
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
        inline auto entity_count(ice::ecs::Query<Definition> const& query) noexcept -> ice::u32
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
        inline auto for_each_block(ice::ecs::Query<Definition> const& query, Fn&& fn) noexcept
        {
            static constexpr Definition const& query_definition = ice::ecs::Query<Definition>::Constant_Definition;

            void* helper_pointer_array[query_definition.component_count]{ nullptr };

            ice::u32 const arch_count = ice::count(query.archetype_instances);
            for (ice::u32 arch_idx = 0; arch_idx < arch_count; ++arch_idx)
            {
                ice::ecs::ArchetypeInstanceInfo const* arch = query.archetype_instances[arch_idx];
                ice::ecs::DataBlock const* block = query.archetype_data_blocks[arch_idx];
                ice::Span<ice::u32 const> argument_idx_map = ice::array::slice(query.archetype_argument_idx_map, arch_idx * query_definition.component_count, query_definition.component_count);

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
        inline auto for_each_entity(ice::ecs::Query<Definition> const& query, Fn&& fn) noexcept
        {
            static constexpr Definition const& query_definition = ice::ecs::Query<Definition>::Constant_Definition;

            void* helper_pointer_array[query_definition.component_count]{ nullptr };

            ice::u32 const arch_count = ice::count(query.archetype_instances);
            for (ice::u32 arch_idx = 0; arch_idx < arch_count; ++arch_idx)
            {
                ice::ecs::ArchetypeInstanceInfo const* arch = query.archetype_instances[arch_idx];
                ice::ecs::DataBlock const* block = query.archetype_data_blocks[arch_idx];
                ice::Span<ice::u32 const> argument_idx_map = ice::array::slice(query.archetype_argument_idx_map, arch_idx * query_definition.component_count, query_definition.component_count);

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

                    // if constexpr (std::is_object_v<Fn>)
                    // {
                    //     Definition::invoke_for_each_entity_op<Fn>(
                    //         (void*)&fn,
                    //         block->block_entity_count,
                    //         helper_pointer_array
                    //     );
                    // }
                    // else
                    // {
                    //     Definition::invoke_for_each_entity_fn<Fn>(
                    //         (void*)fn,
                    //         block->block_entity_count,
                    //         helper_pointer_array
                    //     );
                    // }

                    Definition::invoke_for_each_entity(
                        ice::forward<Fn>(fn),
                        block->block_entity_count,
                        helper_pointer_array
                    );

                    block = block->next;
                }
            }
        }

        template<ice::ecs::QueryType... QueryComponents>
        inline auto for_each_block(
            ice::ecs::Query<ice::ecs::QueryDefinition<QueryComponents...>> const& query
        ) noexcept -> ice::Generator<ice::ecs::detail::QueryBlockTupleResult<QueryComponents...>>
        {
            static constexpr ice::ucount component_count = sizeof...(QueryComponents);

            void* helper_pointer_array[component_count]{ nullptr };

            ice::u32 const arch_count = ice::count(query.archetype_instances);
            for (ice::u32 arch_idx = 0; arch_idx < arch_count; ++arch_idx)
            {
                ice::ecs::ArchetypeInstanceInfo const* arch = query.archetype_instances[arch_idx];
                ice::ecs::DataBlock const* block = query.archetype_data_blocks[arch_idx];
                ice::Span<ice::u32 const> argument_idx_map = ice::array::slice(query.archetype_argument_idx_map, arch_idx * component_count, component_count);

                while (block != nullptr)
                {
                    for (ice::u32 arg_idx = 0; arg_idx < component_count; ++arg_idx)
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

                    co_yield ice::ecs::detail::create_block_tuple<QueryComponents...>(block->block_entity_count, helper_pointer_array);

                    block = block->next;
                }
            }
        }

        template<ice::ecs::QueryType... QueryComponents>
        inline auto for_each_entity(
            ice::ecs::Query<ice::ecs::QueryDefinition<QueryComponents...>> const& query
        ) noexcept -> ice::Generator<ice::ecs::detail::QueryEntityTupleResult<QueryComponents...>>
        {
            static constexpr ice::ucount component_count = sizeof...(QueryComponents);

            void* helper_pointer_array[component_count]{ nullptr };

            ice::u32 const arch_count = ice::count(query.archetype_instances);
            for (ice::u32 arch_idx = 0; arch_idx < arch_count; ++arch_idx)
            {
                ice::ecs::ArchetypeInstanceInfo const* arch = query.archetype_instances[arch_idx];
                ice::ecs::DataBlock const* block = query.archetype_data_blocks[arch_idx];
                ice::Span<ice::u32 const> argument_idx_map = ice::array::slice(query.archetype_argument_idx_map, arch_idx * component_count, component_count);

                while (block != nullptr)
                {
                    for (ice::u32 arg_idx = 0; arg_idx < component_count; ++arg_idx)
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

                    ice::ucount entity_idx = 0;
                    while(entity_idx < block->block_entity_count)
                    {
                        co_yield ice::ecs::detail::create_entity_tuple<QueryComponents...>(entity_idx, helper_pointer_array);
                        entity_idx += 1;
                    }

                    block = block->next;
                }
            }
        }

    } // namespace query


} // namespace ice::ecs
