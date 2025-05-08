/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/ecs/ecs_types.hxx>
#include <ice/ecs/ecs_archetype.hxx>
#include <ice/ecs/ecs_data_block.hxx>
#include <ice/ecs/ecs_query_definition.hxx>
#include <ice/ecs/ecs_query_provider.hxx>
#include <ice/mem_allocator.hxx>
#include <ice/container/array.hxx>
#include <ice/span.hxx>

namespace ice::ecs
{

    using ice::ecs::detail::QueryType;

    namespace query
    {

        template<ice::ecs::QueryType... QueryComponents>
        inline auto block_count(ice::ecs::Query<QueryComponents...> const& query) noexcept -> ice::u32;

        template<ice::ecs::QueryType... QueryComponents>
        inline auto entity_count(ice::ecs::Query<QueryComponents...> const& query) noexcept -> ice::u32;

        template<ice::ecs::QueryType... QueryComponents>
        inline auto entity_data(
            ice::ecs::Query<QueryComponents...> const& query,
            ice::ecs::Entity entity
        ) noexcept -> ice::ecs::detail::QueryEntityTupleResult<QueryComponents...>;

        template<typename Fn, ice::ecs::QueryType... QueryComponents>
        inline auto for_each_block(
            ice::ecs::Query<QueryComponents...> const& query,
            Fn&& fn
        ) noexcept;

        template<ice::ecs::QueryType... QueryComponents>
        inline auto for_each_block(
            ice::ecs::Query<QueryComponents...> const& query
        ) noexcept -> ice::Generator<ice::ecs::detail::QueryBlockTupleResult<QueryComponents...>>;

        template<typename Fn, ice::ecs::QueryType... QueryComponents>
        inline auto for_each_entity(
            ice::ecs::Query<QueryComponents...> const& query,
            Fn&& fn
        ) noexcept;

        template<ice::ecs::QueryType... QueryComponents>
        inline auto for_each_entity(
            ice::ecs::Query<QueryComponents...> const& query
        ) noexcept -> ice::Generator<ice::ecs::detail::QueryEntityTupleResult<QueryComponents...>>;

    } // namespace query

    //! \brief A query holds information about all archetypes that should be iterated but it's not yet executed.
    template<ice::ecs::QueryType... QueryComponents>
    struct Query
    {
        using Definition = ice::ecs::QueryDefinition<QueryComponents...>;
        static constexpr Definition Constant_Definition{ };

        Query(ice::Allocator& alloc) noexcept;

        Query(Query&& other) noexcept;
        auto operator=(Query&& other) noexcept -> Query&;
        Query(Query const& other) noexcept = delete;
        auto operator=(Query const& other) noexcept -> Query& = delete;

        ice::ecs::QueryProvider const* provider;
        ice::ecs::QueryAccessTracker* access_trackers[Constant_Definition.component_count];
        ice::Array<ice::ecs::ArchetypeInstanceInfo const*> archetype_instances;
        ice::Array<ice::ecs::DataBlock const*> archetype_data_blocks;
        ice::Array<ice::u32> archetype_argument_idx_map;
    };

    template<ice::ecs::QueryType... QueryComponents>
    Query<QueryComponents...>::Query(ice::Allocator& alloc) noexcept
        : access_trackers{ }
        , archetype_instances{ alloc }
        , archetype_data_blocks{ alloc }
        , archetype_argument_idx_map{ alloc }
    {
    }

    template<ice::ecs::QueryType... QueryComponents>
    Query<QueryComponents...>::Query(Query&& other) noexcept
        : access_trackers{ }
        , archetype_instances{ ice::move(other.archetype_instances) }
        , archetype_data_blocks{ ice::move(other.archetype_data_blocks) }
        , archetype_argument_idx_map{ ice::move(other.archetype_argument_idx_map) }
    {
        ice::memcpy(ice::addressof(access_trackers), ice::addressof(other.access_trackers), sizeof(access_trackers));
    }

    template<ice::ecs::QueryType... QueryComponents>
    auto Query<QueryComponents...>::operator=(Query&& other) noexcept -> Query&
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

        template<ice::ecs::QueryType... QueryComponents>
        inline auto block_count(ice::ecs::Query<QueryComponents...> const& query) noexcept -> ice::u32
        {
            ice::u32 result = 0;
            for (ice::ecs::DataBlock const* const head_block : query.archetype_data_blocks)
            {
                // We don't want to count the head-block since it never contains actual entity data.
                ice::ecs::DataBlock const* it = head_block->next;
                while (it != nullptr)
                {
                    result += 1;
                    it = it->next;
                }
            }
            return result;
        }

        template<ice::ecs::QueryType... QueryComponents>
        inline auto entity_count(ice::ecs::Query<QueryComponents...> const& query) noexcept -> ice::u32
        {
            ice::u32 result = 0;
            for (ice::ecs::DataBlock const* const head_block : query.archetype_data_blocks)
            {
                // We don't want to count the head-block since it never contains actual entity data.
                ice::ecs::DataBlock const* it = head_block->next;
                while (it != nullptr)
                {
                    result += it->block_entity_count;
                    it = it->next;
                }
            }
            return result;
        }

        template<ice::ecs::QueryType... QueryComponents>
        inline auto entity_data(
            ice::ecs::Query<QueryComponents...> const& query,
            ice::ecs::Entity entity
        ) noexcept -> ice::ecs::detail::QueryEntityTupleResult<QueryComponents...>
        {
            static constexpr ice::ucount component_count = sizeof...(QueryComponents);
            ice::ecs::EntityDataSlot const slotinfo = query.provider->query_data_slot(entity);

            ice::u32 arch_idx = 0;
            ice::ecs::ArchetypeInstanceInfo const* arch = nullptr;
            ice::ecs::DataBlock const* block = nullptr;

            ice::u32 const arch_count = ice::count(query.archetype_instances);
            for (; arch_idx < arch_count; ++arch_idx)
            {
                arch = query.archetype_instances[arch_idx];
                if (arch->archetype_instance == ice::ecs::ArchetypeInstance{ slotinfo.archetype })
                {
                    // Find the specific block
                    ice::u32 slot_block = slotinfo.block;
                    block = query.archetype_data_blocks[arch_idx];
                    while (slot_block > 0 && block != nullptr)
                    {
                        block = block->next;
                        slot_block -= 1;
                    }
                    break;
                }
            }

            ICE_ASSERT_CORE(arch != nullptr && block != nullptr);

            void* helper_pointer_array[component_count]{ nullptr };
            ice::Span<ice::u32 const> argument_idx_map = ice::array::slice(query.archetype_argument_idx_map, arch_idx * component_count, component_count);

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

            return ice::ecs::detail::create_entity_tuple<QueryComponents...>(slotinfo.index, helper_pointer_array);
        }

        template<typename Fn, ice::ecs::QueryType... QueryComponents>
        inline auto for_each_block(ice::ecs::Query<QueryComponents...> const& query, Fn&& fn) noexcept
        {
            using Definition = typename ice::ecs::Query<QueryComponents...>::Definition;
            static constexpr Definition const& query_definition = ice::ecs::Query<QueryComponents...>::Constant_Definition;

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


        template<typename Fn, ice::ecs::QueryType... QueryComponents>
        inline auto for_each_entity(ice::ecs::Query<QueryComponents...> const& query, Fn&& fn) noexcept
        {
            using Definition = typename ice::ecs::Query<QueryComponents...>::Definition;
            static constexpr Definition const& query_definition = ice::ecs::Query<QueryComponents...>::Constant_Definition;

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
            ice::ecs::Query<QueryComponents...> const& query
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
            ice::ecs::Query<QueryComponents...> const& query
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
                    while (entity_idx < block->block_entity_count)
                    {
                        co_yield ice::ecs::detail::create_entity_tuple<QueryComponents...>(entity_idx, helper_pointer_array);
                        entity_idx += 1;
                    }

                    block = block->next;
                }
            }
        }

#if 0
        template<ice::ecs::QueryType... QueryComponents, ice::ecs::QueryType... QuerySubComponents>
        inline auto for_each_entity(
            ice::ecs::Query<QueryComponents...> const& query,
            ice::ecs::Query<QuerySubComponents...> const& sub_query
        ) noexcept -> ice::Generator<ice::ecs::detail::QueryEntityTupleResult<QueryComponents..., QuerySubComponents...>>
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
                    while (entity_idx < block->block_entity_count)
                    {
                        auto first_tuple = ice::ecs::detail::create_entity_tuple<QueryComponents...>(entity_idx, helper_pointer_array);
                        auto entity = std::get<sizeof...(QueryComponents) - 1, QueryComponents>(first_tuple);

                        co_yield std::tuple_cat(
                            ice::move(first_tuple),
                            ice::ecs::query::entity_data(sub_query, entity)
                        );
                        entity_idx += 1;
                    }

                    block = block->next;
                }
            }
        }
#endif

    } // namespace query


    template<typename First, typename... Tail>
    struct QueryTupleConcat;

    template<typename... Types>
    struct QueryTupleConcat<std::tuple<Types...>>
    {
        using Tuple = std::tuple<Types...>;
    };

    //template<typename... FirstTypes, typename... SecondTypes>
    //struct QueryTupleConcat<std::tuple<FirstTypes...>, std::tuple<SecondTypes...>>
    //{
    //    using Tuple = std::tuple<FirstTypes..., SecondTypes...>;
    //};

    template<typename... FirstTypes, typename... SecondTypes, typename... Tail>
    struct QueryTupleConcat<std::tuple<FirstTypes...>, std::tuple<SecondTypes...>, Tail...>
    {
        using Tuple = typename QueryTupleConcat<std::tuple<FirstTypes..., SecondTypes...>, Tail...>::Tuple;
    };

    template<ice::u32 CompIdx, ice::ecs::QueryType... QueryComponents>
    struct QueryPart
    {
        using Definition = ice::ecs::QueryDefinition<QueryComponents...>;
        using QueryPartTupleResult = ice::ecs::detail::QueryEntityTupleResult<QueryComponents...>;
        using QueryPartBlockTupleResult = ice::ecs::detail::QueryBlockTupleResult<QueryComponents...>;

        static constexpr Definition Constant_QueryDefinition;

        static constexpr ice::u32 RefIndex = CompIdx;
        static constexpr ice::u32 CompCount = sizeof...(QueryComponents);
    };

    template<typename... QueryParts>
    struct Query_v2
    {
    public:
        using QueryTupleResult = typename ice::ecs::QueryTupleConcat<typename QueryParts::QueryPartTupleResult...>::Tuple;
        using QueryBlockTupleResult = typename ice::ecs::QueryTupleConcat<typename QueryParts::QueryPartBlockTupleResult...>::Tuple;

        Query_v2(ice::Allocator& alloc) noexcept
            : provider{ nullptr }
            , access_trackers{ }
            , archetype_instances{ alloc }
            , archetype_data_blocks{ alloc }
            , archetype_argument_idx_map{ alloc }
            , archetype_count_for_part{ }
        { }

    public:
        static ice::u32 constexpr ComponentCount = (0 + ... + QueryParts::CompCount);

        static ice::u32 constexpr QueryPartRefs[sizeof...(QueryParts)]{};
        static ice::u32 constexpr QueryPartStarts[sizeof...(QueryParts)]{};

        ice::ecs::QueryProvider const* provider;
        ice::ecs::QueryAccessTracker* access_trackers[ComponentCount];
        ice::Array<ice::ecs::ArchetypeInstanceInfo const*> archetype_instances;
        ice::Array<ice::ecs::DataBlock const*> archetype_data_blocks;
        ice::Array<ice::u32> archetype_argument_idx_map;
        ice::u32 archetype_count_for_part[sizeof...(QueryParts)];
    };

    namespace detail_v2
    {

        template<ice::u32 RefIdx, QueryType... T>
        inline auto create_entity_tuple(
            ice::u32 index,
            void** component_pointer_array,
            ice::ecs::QueryPart<RefIdx, T...>
        ) noexcept -> ice::ecs::detail::QueryEntityTupleResult<T...>
        {
            using QueryTypeTuple = std::tuple<T...>;
            using QueryTypeTupeResult = ice::ecs::detail::QueryEntityTupleResult<T...>;

            auto const select_entity = [&]<std::size_t... Idx>(std::index_sequence<Idx...>) noexcept
            {
                return QueryTypeTupeResult{
                    ice::ecs::detail::QueryIteratorArgument<std::tuple_element_t<Idx, QueryTypeTuple>>::select_entity_ptr(
                        ice::ecs::detail::QueryIteratorArgument<std::tuple_element_t<Idx, QueryTypeTuple>>::block_array(component_pointer_array[Idx]),
                        index * static_cast<ice::u32>(component_pointer_array[Idx] != nullptr)
                    )...
                };
            };

            return select_entity(std::make_index_sequence<sizeof...(T)>{});
        }

        template<QueryType... TupleTypes, ice::u32 RefIdx, QueryType... SubQueryTypes>
        inline auto create_entity_tuple_concat(
            std::tuple<TupleTypes...>&& in_tuple,
            ice::ecs::QueryProvider const& provider,
            ice::Span<ice::ecs::ArchetypeInstanceInfo const* const> archetype_instances,
            ice::Span<ice::ecs::DataBlock const* const> archetype_data_blocks,
            ice::Span<ice::u32 const> archetype_argument_idx_map,
            ice::Span<ice::u32 const> archetype_parts_count,
            ice::ecs::QueryPart<RefIdx, SubQueryTypes...>
        ) noexcept -> ice::ecs::detail::QueryEntityTupleResult<TupleTypes..., SubQueryTypes...>
        {
            using InTuple = ice::ecs::detail::QueryEntityTupleResult<TupleTypes...>;
            using RefType = std::tuple_element_t<RefIdx, InTuple>;
            using SubTuple = ice::ecs::detail::QueryEntityTupleResult<SubQueryTypes...>;

            RefType const& ref = std::get<RefIdx>(in_tuple);


            static constexpr ice::ucount component_count = sizeof...(SubQueryTypes);
            ice::ecs::EntityDataSlot const slotinfo = provider.query_data_slot(ref->entity);

            ice::u32 arch_idx = 0;
            ice::ecs::ArchetypeInstanceInfo const* arch = nullptr;
            ice::ecs::DataBlock const* block = nullptr;

            for (; arch_idx < archetype_parts_count[0]; ++arch_idx)
            {
                arch = archetype_instances[arch_idx];
                if (arch->archetype_instance == ice::ecs::ArchetypeInstance{ slotinfo.archetype })
                {
                    // Find the specific block
                    ice::u32 slot_block = slotinfo.block;
                    block = archetype_data_blocks[arch_idx];
                    while (slot_block > 0 && block != nullptr)
                    {
                        block = block->next;
                        slot_block -= 1;
                    }
                    break;
                }
            }

            ICE_ASSERT_CORE(arch != nullptr && block != nullptr);

            void* helper_pointer_array[component_count]{ nullptr };
            ice::Span<ice::u32 const> argument_idx_map = ice::span::subspan(archetype_argument_idx_map, arch_idx * component_count, component_count);

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

            return std::tuple_cat(
                in_tuple,
                ice::ecs::detail::create_entity_tuple<SubQueryTypes...>(slotinfo.index, helper_pointer_array)
            );

            //return std::tuple_cat(std::move(in_tuple), std::make_tuple<SubQueryTypes...>(SubQueryTypes{}...));
        }

        template<ice::u32 RefIdx, QueryType... T>
        inline auto create_block_tuple(
            ice::u32 count,
            void** component_pointer_array,
            ice::ecs::QueryPart<RefIdx, T...>
        ) noexcept -> ice::ecs::detail::QueryBlockTupleResult<T...>
        {
            using QueryTypeTuple = std::tuple<T...>;
            using QueryTypeTupleResult = ice::ecs::detail::QueryBlockTupleResult<T...>;

            auto const enumerate_types = [&]<std::size_t... Idx>(std::index_sequence<Idx...> seq) noexcept -> QueryTypeTupleResult
            {
                return QueryTypeTupleResult{ count, ice::ecs::detail::QueryIteratorArgument<std::tuple_element_t<Idx, QueryTypeTuple>>::block_array(component_pointer_array[Idx])... };
            };

            return enumerate_types(std::make_index_sequence<sizeof...(T)>{});
        }

    } // namespace detail

    namespace query_v2
    {

        template<typename MainPart, typename... RefParts>
        inline auto entity_count(
            ice::ecs::Query_v2<MainPart, RefParts...> const& query
        ) noexcept -> ice::ucount
        {
            // On a query with multiple parts we only want to check the blocks of the main part.
            ice::Span const blocks_to_check = ice::array::slice(query.archetype_data_blocks, 0, query.archetype_count_for_part[0]);

            ice::ucount result = 0;
            for (ice::ecs::DataBlock const* const head_block : blocks_to_check)
            {
                // We don't want to count the head-block since it never contains actual entity data.
                ice::ecs::DataBlock const* it = head_block->next;
                while (it != nullptr)
                {
                    result += it->block_entity_count;
                    it = it->next;
                }
            }
            return result;
        }

        template<typename MainPart, typename... RefParts>
        inline auto block_count(
            ice::ecs::Query_v2<MainPart, RefParts...> const& query
        ) noexcept -> ice::ucount
        {
            // On a query with multiple parts we only want to check the blocks of the main part.
            ice::Span const blocks_to_check = ice::array::slice(query.archetype_data_blocks, 0, query.archetype_count_for_part[0]);

            ice::ucount result = 0;
            for (ice::ecs::DataBlock const* const head_block : blocks_to_check)
            {
                // We don't want to count the head-block since it never contains actual entity data.
                ice::ecs::DataBlock const* it = head_block->next;
                while (it != nullptr)
                {
                    result += 1;
                    it = it->next;
                }
            }
            return result;
        }

        template<typename MainPart, typename... RefParts>
        inline auto for_entity(
            ice::ecs::Query_v2<MainPart, RefParts...> const& query,
            ice::ecs::Entity entity
        ) noexcept -> typename ice::ecs::Query_v2<MainPart, RefParts...>::QueryTupleResult
        {
            static constexpr ice::ucount component_count = MainPart::CompCount;
            ice::ecs::EntityDataSlot const slotinfo = query.provider->query_data_slot(entity);

            ice::u32 arch_idx = 0;
            ice::ecs::ArchetypeInstanceInfo const* arch = nullptr;
            ice::ecs::DataBlock const* block = nullptr;

            ice::u32 const arch_count = ice::count(query.archetype_instances);
            for (; arch_idx < arch_count; ++arch_idx)
            {
                arch = query.archetype_instances[arch_idx];
                if (arch->archetype_instance == ice::ecs::ArchetypeInstance{ slotinfo.archetype })
                {
                    // Find the specific block
                    ice::u32 slot_block = slotinfo.block;
                    block = query.archetype_data_blocks[arch_idx];
                    while (slot_block > 0 && block != nullptr)
                    {
                        block = block->next;
                        slot_block -= 1;
                    }
                    break;
                }
            }

            ICE_ASSERT_CORE(arch != nullptr && block != nullptr);

            void* helper_pointer_array[component_count]{ nullptr };
            ice::Span<ice::u32 const> argument_idx_map = ice::array::slice(
                query.archetype_argument_idx_map, arch_idx * component_count, component_count
            );

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

            if constexpr (sizeof...(RefParts) == 0)
            {
                return ice::ecs::detail_v2::create_entity_tuple(slotinfo.index, helper_pointer_array, MainPart{});
            }
            else
            {
                return ice::ecs::detail_v2::create_entity_tuple_concat(
                    ice::ecs::detail_v2::create_entity_tuple(slotinfo.index, helper_pointer_array, MainPart{}),
                    *query.provider,
                    ice::array::slice(query.archetype_instances, arch_count),
                    ice::array::slice(query.archetype_data_blocks, arch_count),
                    ice::array::slice(query.archetype_argument_idx_map, arch_count * component_count),
                    ice::span::subspan(ice::Span{ query.archetype_count_for_part }, 1),
                    RefParts{}...
                );
            }
        }

        template<typename MainPart, typename... RefParts>
        inline auto for_each_entity(
            ice::ecs::Query_v2<MainPart, RefParts...> const& query
        ) noexcept -> ice::Generator<typename ice::ecs::Query_v2<MainPart, RefParts...>::QueryTupleResult>
        {
            static constexpr ice::ucount component_count = MainPart::CompCount;

            void* helper_pointer_array[component_count]{ nullptr };

            // Only go over the archetypes of the main part
            ice::u32 const arch_count = query.archetype_count_for_part[0];
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
                    while (entity_idx < block->block_entity_count)
                    {
                        if constexpr (sizeof...(RefParts) == 0)
                        {
                            co_yield ice::ecs::detail_v2::create_entity_tuple(entity_idx, helper_pointer_array, MainPart{});
                        }
                        else
                        {
                            co_yield ice::ecs::detail_v2::create_entity_tuple_concat(
                                ice::ecs::detail_v2::create_entity_tuple(entity_idx, helper_pointer_array, MainPart{}),
                                *query.provider,
                                ice::array::slice(query.archetype_instances, arch_count),
                                ice::array::slice(query.archetype_data_blocks, arch_count),
                                ice::array::slice(query.archetype_argument_idx_map, arch_count * component_count),
                                ice::span::subspan(ice::Span{ query.archetype_count_for_part }, 1),
                                RefParts{}...
                            );
                        }
                        entity_idx += 1;
                    }

                    block = block->next;
                }
            }
        }

        template<typename MainPart, typename... RefParts>
        inline auto for_each_block(
            ice::ecs::Query_v2<MainPart, RefParts...> const& query
        ) noexcept -> ice::Generator<typename ice::ecs::Query_v2<MainPart, RefParts...>::QueryBlockTupleResult>
        {
            static_assert(sizeof...(RefParts) == 0, "'for_each_block' only supports basic queries with no entity references!");

            static constexpr ice::ucount component_count = MainPart::CompCount;

            void* helper_pointer_array[component_count]{ nullptr };

            ice::u32 const arch_count = ice::count(query.archetype_instances);
            for (ice::u32 arch_idx = 0; arch_idx < arch_count; ++arch_idx)
            {
                ice::ecs::ArchetypeInstanceInfo const* arch = query.archetype_instances[arch_idx];
                ice::ecs::DataBlock const* block = query.archetype_data_blocks[arch_idx];
                ice::Span<ice::u32 const> argument_idx_map = ice::array::slice(query.archetype_argument_idx_map, arch_idx * component_count, component_count);

                // We skip the first block because it will be always empty.
                ICE_ASSERT_CORE(block->block_entity_count == 0);
                block = block->next;

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

                    co_yield ice::ecs::detail_v2::create_block_tuple(block->block_entity_count, helper_pointer_array, MainPart{});

                    block = block->next;
                }
            }
        }

    } // namespace query

} // namespace ice::ecs
