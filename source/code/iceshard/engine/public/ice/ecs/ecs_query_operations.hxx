/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/ecs/ecs_query_object.hxx>
#include <ice/ecs/ecs_query_provider.hxx>
#include <ice/ecs/ecs_query_awaitable.hxx>
#include <ice/task_generator.hxx>

namespace ice::ecs
{

    //! \brief A mixin class that extends Query-like classes with actual methods allowing to access entity components.
    //! \hideinheritancegraph
    struct TraitQueryOperations
    {
    public:
        template<typename Self>
        inline auto entity_count(this Self&& self) noexcept -> ice::ucount;

        template<typename Self>
        inline auto for_entity(this Self&& self, ice::ecs::Entity entity) noexcept -> typename ice::clear_type_t<Self>::ResultType;

        template<typename Self>
        inline auto try_entity(this Self&& self, ice::ecs::Entity entity) noexcept -> typename ice::clear_type_t<Self>::ResultType;

        template<typename Self>
        inline auto for_each_entity(this Self&& self) noexcept -> ice::Generator<typename ice::clear_type_t<Self>::ResultType>;

        template<typename Self, typename Fn>
        inline void for_each_entity(this Self&& self, Fn&& fn) noexcept;

    public:
        template<typename Self>
        inline auto block_count(this Self&& self) noexcept -> ice::ucount;

        template<typename Self>
        inline auto for_each_block(this Self&& self) noexcept -> ice::Generator<typename ice::clear_type_t<Self>::BlockResultType>;

        template<typename Self, typename Fn>
        inline void for_each_block(this Self&& self, Fn&& fn) noexcept;
    };

    namespace detail
    {

        template<ice::u32 RefIdx, QueryArg... T>
        inline auto create_entity_tuple(
            ice::u32 index,
            void** component_pointer_array,
            ice::ecs::detail::QueryObjectPart<RefIdx, T...>
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

        template<QueryArg... TupleTypes, ice::u32 RefIdx, QueryArg... SubQueryTypes>
        inline auto create_entity_tuple_concat(
            std::tuple<TupleTypes...>&& in_tuple,
            ice::ecs::QueryProvider const& provider,
            ice::Span<ice::ecs::detail::ArchetypeInstanceInfo const* const> archetype_instances,
            ice::Span<ice::ecs::detail::DataBlock const* const> archetype_data_blocks,
            ice::Span<ice::u32 const> archetype_argument_idx_map,
            ice::Span<ice::u32 const> archetype_parts_count,
            ice::ecs::detail::QueryObjectPart<RefIdx, SubQueryTypes...>
        ) noexcept -> ice::ecs::detail::QueryEntityTupleResult<TupleTypes..., SubQueryTypes...>
        {
            using InTuple = ice::ecs::detail::QueryEntityTupleResult<TupleTypes...>;
            using RefType = std::tuple_element_t<RefIdx, InTuple>;
            using SubTuple = ice::ecs::detail::QueryEntityTupleResult<SubQueryTypes...>;

            RefType const& ref = std::get<RefIdx>(in_tuple);


            static constexpr ice::ucount component_count = sizeof...(SubQueryTypes);
            ice::ecs::EntityDataSlot const slotinfo = provider.query_data_slot(ref->entity);

            ice::u32 arch_idx = 0;
            ice::ecs::detail::ArchetypeInstanceInfo const* arch = nullptr;
            ice::ecs::detail::DataBlock const* block = nullptr;

            for (; arch_idx < archetype_parts_count[0]; ++arch_idx)
            {
                arch = archetype_instances[arch_idx];
                if (arch->archetype_instance == ice::ecs::detail::ArchetypeInstance{ slotinfo.archetype })
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
            ice::Span<ice::u32 const> make_argument_idx_map = ice::span::subspan(archetype_argument_idx_map, arch_idx * component_count, component_count);

            for (ice::u32 arg_idx = 0; arg_idx < component_count; ++arg_idx)
            {
                if (make_argument_idx_map[arg_idx] == ice::u32_max)
                {
                    helper_pointer_array[arg_idx] = nullptr;
                }
                else
                {
                    ice::u32 const cmp_idx = make_argument_idx_map[arg_idx];

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
        }

        template<ice::u32 RefIdx, QueryArg... T>
        inline auto create_block_tuple(
            ice::u32 count,
            void** component_pointer_array,
            ice::ecs::detail::QueryObjectPart<RefIdx, T...>
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

        template<typename Fn, ice::ecs::QueryArg... QueryTypes, ice::u32 RefIdx, ice::ecs::QueryArg... SubQueryTypes>
        static void invoke_for_each_entity(
            Fn&& fn,
            ice::u32 count,
            void** component_pointer_array,
            ice::ecs::QueryProvider const& provider,
            ice::Span<ice::ecs::detail::ArchetypeInstanceInfo const* const> archetype_instances,
            ice::Span<ice::ecs::detail::DataBlock const* const> archetype_data_blocks,
            ice::Span<ice::u32 const> archetype_argument_idx_map,
            ice::Span<ice::u32 const> archetype_parts_count,
            ice::ecs::detail::QueryObjectPart<0, QueryTypes...>,
            ice::ecs::detail::QueryObjectPart<RefIdx, SubQueryTypes...>
        ) noexcept
        {
            using QueryTypeTuple = std::tuple<QueryTypes...>;
            using SubQueryTypeTuple = std::tuple<SubQueryTypes...>;

            static constexpr ice::ucount component_count = sizeof...(SubQueryTypes);
            void* helper_pointer_array[component_count]{ nullptr };

            auto const invoke_for_entity = [&](
                typename ice::ecs::detail::QueryIteratorArgument<QueryTypes>::BlockIteratorArg... args,
                typename ice::ecs::detail::QueryIteratorArgument<SubQueryTypes>::BlockIteratorArg... subargs
            ) noexcept
            {
                ice::forward<Fn>(fn)(
                    ice::ecs::detail::QueryIteratorArgument<QueryTypes>::select_entity(args, 0)...,
                    ice::ecs::detail::QueryIteratorArgument<SubQueryTypes>::select_entity(subargs, 0)...
                );
            };

            auto const enumerate_entities = [&](
                ice::u32 count,
                typename ice::ecs::detail::QueryIteratorArgument<std::tuple_element_t<RefIdx, QueryTypeTuple>>::BlockIteratorArg refcmp,
                typename ice::ecs::detail::QueryIteratorArgument<QueryTypes>::BlockIteratorArg... args
            ) noexcept
            {
                for (ice::u32 idx = 0; idx < count; ++idx)
                {
                    ice::ecs::EntityDataSlot const slotinfo = provider.query_data_slot(refcmp[idx].entity);

                    ice::u32 arch_idx = 0;
                    ice::ecs::detail::ArchetypeInstanceInfo const* arch = nullptr;
                    ice::ecs::detail::DataBlock const* block = nullptr;

                    for (; arch_idx < archetype_parts_count[0]; ++arch_idx)
                    {
                        arch = archetype_instances[arch_idx];
                        if (arch->archetype_instance == ice::ecs::detail::ArchetypeInstance{ slotinfo.archetype })
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

                    ice::Span<ice::u32 const> make_argument_idx_map = ice::span::subspan(archetype_argument_idx_map, arch_idx * component_count, component_count);

                    for (ice::u32 arg_idx = 0; arg_idx < component_count; ++arg_idx)
                    {
                        if (make_argument_idx_map[arg_idx] == ice::u32_max)
                        {
                            helper_pointer_array[arg_idx] = nullptr;
                        }
                        else
                        {
                            ice::u32 const cmp_idx = make_argument_idx_map[arg_idx];

                            helper_pointer_array[arg_idx] = ice::ptr_add(
                                block->block_data,
                                { arch->component_offsets[cmp_idx] }
                            );
                        }
                    }

                    auto const enumerate_sub_types = [&]<std::size_t... SubIdx>(std::index_sequence<SubIdx...> seq) noexcept
                    {
                        invoke_for_entity(
                            args...,
                            ice::ecs::detail::QueryIteratorArgument<std::tuple_element_t<SubIdx, SubQueryTypeTuple>>::block_array(helper_pointer_array[SubIdx])...
                        );
                    };

                    enumerate_sub_types(std::make_index_sequence<sizeof...(SubQueryTypes)>{});
                }
            };

            auto const enumerate_types = [&]<std::size_t... Idx>(std::index_sequence<Idx...> seq) noexcept
            {
                enumerate_entities(
                    count,
                    ice::ecs::detail::QueryIteratorArgument<std::tuple_element_t<RefIdx, QueryTypeTuple>>::block_array(component_pointer_array[RefIdx]),
                    ice::ecs::detail::QueryIteratorArgument<std::tuple_element_t<Idx, QueryTypeTuple>>::block_array(component_pointer_array[Idx])...
                );
            };

            enumerate_types(std::make_index_sequence<sizeof...(QueryTypes)>{});
        }

    } // namespace detail

    namespace query
    {

        template<typename MainPart, typename... RefParts>
        inline auto entity_count(
            ice::ecs::QueryObject<MainPart, RefParts...> const& query,
            ice::ecs::detail::DataBlockFilter::QueryFilter filter
        ) noexcept -> ice::ucount
        {
            // On a query with multiple parts we only want to check the blocks of the main part.
            ice::Span const blocks_to_check = ice::array::slice(query.archetype_data_blocks, 0, query.archetype_count_for_part[0]);

            ice::ucount result = 0;
            for (ice::ecs::detail::DataBlock const* const head_block : blocks_to_check)
            {
                // We don't want to count the head-block since it never contains actual entity data.
                ice::ecs::detail::DataBlock const* it = head_block->next;
                while (it != nullptr)
                {
                    if (filter.check(it))
                    {
                        result += it->block_entity_count;
                    }
                    it = it->next;
                }
            }
            return result;
        }

        template<typename MainPart, typename... RefParts>
        inline auto block_count(
            ice::ecs::QueryObject<MainPart, RefParts...> const& query
        ) noexcept -> ice::ucount
        {
            // On a query with multiple parts we only want to check the blocks of the main part.
            ice::Span const blocks_to_check = ice::array::slice(query.archetype_data_blocks, 0, query.archetype_count_for_part[0]);

            ice::ucount result = 0;
            for (ice::ecs::detail::DataBlock const* const head_block : blocks_to_check)
            {
                // We don't want to count the head-block since it never contains actual entity data.
                ice::ecs::detail::DataBlock const* it = head_block->next;
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
            ice::ecs::QueryObject<MainPart, RefParts...> const& query,
            ice::ecs::Entity entity,
            bool allow_failure
        ) noexcept -> typename ice::ecs::QueryObject<MainPart, RefParts...>::ResultType
        {
            static constexpr ice::ucount component_count = MainPart::ComponentCount;
            ice::ecs::EntityDataSlot const slotinfo = query.provider->query_data_slot(entity);

            ice::u32 arch_idx = 0;
            ice::ecs::detail::ArchetypeInstanceInfo const* arch = nullptr;
            ice::ecs::detail::DataBlock const* block = nullptr;

            ice::u32 const arch_count = ice::count(query.archetype_instances);
            for (; arch_idx < arch_count; ++arch_idx)
            {
                arch = query.archetype_instances[arch_idx];
                if (arch->archetype_instance == ice::ecs::detail::ArchetypeInstance{ slotinfo.archetype })
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

            // Return early with nullptrs (if allowed)
            if ((arch == nullptr || block == nullptr) && allow_failure)
            {
                return {};
            }

            ICE_ASSERT_CORE(arch != nullptr && block != nullptr);

            void* helper_pointer_array[component_count]{ nullptr };
            ice::Span<ice::u32 const> make_argument_idx_map = ice::array::slice(
                query.archetype_argument_idx_map, arch_idx * component_count, component_count
            );

            for (ice::u32 arg_idx = 0; arg_idx < component_count; ++arg_idx)
            {
                if (make_argument_idx_map[arg_idx] == ice::u32_max)
                {
                    helper_pointer_array[arg_idx] = nullptr;
                }
                else
                {
                    ice::u32 const cmp_idx = make_argument_idx_map[arg_idx];

                    helper_pointer_array[arg_idx] = ice::ptr_add(
                        block->block_data,
                        { arch->component_offsets[cmp_idx] }
                    );
                }
            }

            if constexpr (sizeof...(RefParts) == 0)
            {
                return ice::ecs::detail::create_entity_tuple(slotinfo.index, helper_pointer_array, MainPart{});
            }
            else
            {
                return ice::ecs::detail::create_entity_tuple_concat(
                    ice::ecs::detail::create_entity_tuple(slotinfo.index, helper_pointer_array, MainPart{}),
                    *query.provider,
                    ice::array::slice(query.archetype_instances, arch_count),
                    ice::array::slice(query.archetype_data_blocks, arch_count),
                    ice::array::slice(query.archetype_argument_idx_map, arch_count * component_count),
                    ice::span::subspan(ice::Span{ query.archetype_count_for_part }, 1),
                    RefParts{}...
                );
            }
        }

        template<typename MainPart, typename... RefParts, typename QueryObjectOwner>
        inline auto for_each_entity_gen(
            ice::ecs::QueryObject<MainPart, RefParts...> const& query_object,
            ice::ecs::detail::DataBlockFilter::QueryFilter filter,
            QueryObjectOwner
        ) noexcept -> ice::Generator<typename ice::ecs::QueryObject<MainPart, RefParts...>::ResultType>
        {
            static constexpr ice::ucount component_count = MainPart::ComponentCount;

            void* helper_pointer_array[component_count]{ nullptr };

            // Only go over the archetypes of the main part
            ice::u32 const arch_count = query_object.archetype_count_for_part[0];
            for (ice::u32 arch_idx = 0; arch_idx < arch_count; ++arch_idx)
            {
                ice::ecs::detail::ArchetypeInstanceInfo const* arch = query_object.archetype_instances[arch_idx];
                ice::ecs::detail::DataBlock const* block = query_object.archetype_data_blocks[arch_idx];
                ice::Span<ice::u32 const> make_argument_idx_map = ice::array::slice(query_object.archetype_argument_idx_map, arch_idx * component_count, component_count);

                // We skip the first block because it will be always empty.
                ICE_ASSERT_CORE(block->block_entity_count == 0);
                block = filter.next(block);

                while (block != nullptr && filter.check(block))
                {
                    for (ice::u32 arg_idx = 0; arg_idx < component_count; ++arg_idx)
                    {
                        if (make_argument_idx_map[arg_idx] == ice::u32_max)
                        {
                            helper_pointer_array[arg_idx] = nullptr;
                        }
                        else
                        {
                            ice::u32 const cmp_idx = make_argument_idx_map[arg_idx];

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
                            co_yield ice::ecs::detail::create_entity_tuple(entity_idx, helper_pointer_array, MainPart{});
                        }
                        else
                        {
                            co_yield ice::ecs::detail::create_entity_tuple_concat(
                                ice::ecs::detail::create_entity_tuple(entity_idx, helper_pointer_array, MainPart{}),
                                *query_object.provider,
                                ice::array::slice(query_object.archetype_instances, arch_count),
                                ice::array::slice(query_object.archetype_data_blocks, arch_count),
                                ice::array::slice(query_object.archetype_argument_idx_map, arch_count * component_count),
                                ice::span::subspan(ice::Span{ query_object.archetype_count_for_part }, 1),
                                RefParts{}...
                            );
                        }
                        entity_idx += 1;
                    }

                    block = filter.next(block);
                }
            }
        }

        template<typename MainPart, typename... RefParts, typename Fn>
        inline auto for_each_entity(
            ice::ecs::QueryObject<MainPart, RefParts...> const& query_object,
            ice::ecs::detail::DataBlockFilter::QueryFilter filter,
            Fn&& fn
        ) noexcept
        {
            using Definition = typename ice::ecs::QueryObject<MainPart, RefParts...>::Definition;
            static constexpr ice::ucount component_count = MainPart::ComponentCount;

            void* helper_pointer_array[component_count]{ nullptr };

            // Only go over the archetypes of the main part
            ice::u32 const arch_count = query_object.archetype_count_for_part[0];
            for (ice::u32 arch_idx = 0; arch_idx < arch_count; ++arch_idx)
            {
                ice::ecs::detail::ArchetypeInstanceInfo const* arch = query_object.archetype_instances[arch_idx];
                ice::ecs::detail::DataBlock const* block = query_object.archetype_data_blocks[arch_idx];
                ice::Span<ice::u32 const> make_argument_idx_map = ice::array::slice(query_object.archetype_argument_idx_map, arch_idx * component_count, component_count);

                // We skip the first block because it will be always empty.
                ICE_ASSERT_CORE(block->block_entity_count == 0);
                block = filter.next(block);

                while (block != nullptr && filter.check(block))
                {
                    for (ice::u32 arg_idx = 0; arg_idx < component_count; ++arg_idx)
                    {
                        if (make_argument_idx_map[arg_idx] == ice::u32_max)
                        {
                            helper_pointer_array[arg_idx] = nullptr;
                        }
                        else
                        {
                            ice::u32 const cmp_idx = make_argument_idx_map[arg_idx];

                            helper_pointer_array[arg_idx] = ice::ptr_add(
                                block->block_data,
                                { arch->component_offsets[cmp_idx] }
                            );
                        }
                    }

                    if constexpr (sizeof...(RefParts) == 0)
                    {
                        MainPart::Definition::invoke_for_each_entity(
                            ice::forward<Fn>(fn),
                            block->block_entity_count,
                            helper_pointer_array
                        );
                    }
                    else
                    {
                        ice::ecs::detail::invoke_for_each_entity(
                            ice::forward<Fn>(fn),
                            block->block_entity_count,
                            helper_pointer_array,
                            // Ref parts
                            *query_object.provider,
                            ice::array::slice(query_object.archetype_instances, arch_count),
                            ice::array::slice(query_object.archetype_data_blocks, arch_count),
                            ice::array::slice(query_object.archetype_argument_idx_map, arch_count * component_count),
                            ice::span::subspan(ice::Span{ query_object.archetype_count_for_part }, 1),
                            MainPart{},
                            RefParts{}...
                        );
                    }

                    block = filter.next(block);
                }
            }
        }

        template<typename MainPart, typename... RefParts, typename QueryObjectOwner>
        inline auto for_each_block_gen(
            ice::ecs::QueryObject<MainPart, RefParts...> const& query,
            ice::ecs::detail::DataBlockFilter::QueryFilter filter,
            QueryObjectOwner
        ) noexcept -> ice::Generator<typename ice::ecs::QueryObject<MainPart, RefParts...>::BlockResultType>
        {
            static_assert(sizeof...(RefParts) == 0, "'for_each_block' only supports basic queries with no entity references!");

            static constexpr ice::ucount component_count = MainPart::ComponentCount;

            void* helper_pointer_array[component_count]{ nullptr };

            ice::u32 const arch_count = ice::count(query.archetype_instances);
            for (ice::u32 arch_idx = 0; arch_idx < arch_count; ++arch_idx)
            {
                ice::ecs::detail::ArchetypeInstanceInfo const* arch = query.archetype_instances[arch_idx];
                ice::ecs::detail::DataBlock const* block = query.archetype_data_blocks[arch_idx];
                ice::Span<ice::u32 const> make_argument_idx_map = ice::array::slice(query.archetype_argument_idx_map, arch_idx * component_count, component_count);

                // We skip the first block because it will be always empty.
                ICE_ASSERT_CORE(block->block_entity_count == 0);
                block = filter.next(block);

                while (block != nullptr && filter.check(block))
                {
                    for (ice::u32 arg_idx = 0; arg_idx < component_count; ++arg_idx)
                    {
                        if (make_argument_idx_map[arg_idx] == ice::u32_max)
                        {
                            helper_pointer_array[arg_idx] = nullptr;
                        }
                        else
                        {
                            ice::u32 const cmp_idx = make_argument_idx_map[arg_idx];

                            helper_pointer_array[arg_idx] = ice::ptr_add(
                                block->block_data,
                                { arch->component_offsets[cmp_idx] }
                            );
                        }
                    }

                    co_yield ice::ecs::detail::create_block_tuple(block->block_entity_count, helper_pointer_array, MainPart{});

                    block = filter.next(block);
                }
            }
        }

        template<typename MainPart, typename... RefParts, typename Fn>
        inline auto for_each_block(
            ice::ecs::QueryObject<MainPart, RefParts...> const& query,
            ice::ecs::detail::DataBlockFilter::QueryFilter filter,
            Fn&& fn
        ) noexcept
        {
            static_assert(sizeof...(RefParts) == 0, "'for_each_block' only supports basic queries with no entity references!");
            static constexpr ice::ucount component_count = MainPart::ComponentCount;

            void* helper_pointer_array[component_count]{ nullptr };

            ice::u32 const arch_count = ice::count(query.archetype_instances);
            for (ice::u32 arch_idx = 0; arch_idx < arch_count; ++arch_idx)
            {
                ice::ecs::detail::ArchetypeInstanceInfo const* arch = query.archetype_instances[arch_idx];
                ice::ecs::detail::DataBlock const* block = query.archetype_data_blocks[arch_idx];
                ice::Span<ice::u32 const> make_argument_idx_map = ice::array::slice(query.archetype_argument_idx_map, arch_idx * component_count, component_count);

                // We skip the first block because it will be always empty.
                ICE_ASSERT_CORE(block->block_entity_count == 0);
                block = filter.next(block);

                while (block != nullptr)
                {
                    for (ice::u32 arg_idx = 0; arg_idx < component_count; ++arg_idx)
                    {
                        if (make_argument_idx_map[arg_idx] == ice::u32_max)
                        {
                            helper_pointer_array[arg_idx] = nullptr;
                        }
                        else
                        {
                            ice::u32 const cmp_idx = make_argument_idx_map[arg_idx];

                            helper_pointer_array[arg_idx] = ice::ptr_add(
                                block->block_data,
                                { arch->component_offsets[cmp_idx] }
                            );
                        }
                    }

                    MainPart::Definition::invoke_for_each_block(
                        ice::forward<Fn>(fn),
                        block->block_entity_count,
                        helper_pointer_array
                    );

                    block = filter.next(block);
                }
            }
        }

    } // namespace query

    template<typename Self>
    inline auto TraitQueryOperations::entity_count(this Self&& self) noexcept -> ice::ucount
    {
        return ice::ecs::query::entity_count(self.query_object(), self.filter_object());
    }

    template<typename Self>
    inline auto TraitQueryOperations::block_count(this Self&& self) noexcept -> ice::ucount
    {
        return ice::ecs::query::block_count(self.query_object());
    }

    template<typename Self>
    inline auto TraitQueryOperations::for_entity(
        this Self&& self, ice::ecs::Entity entity
    ) noexcept -> typename ice::clear_type_t<Self>::ResultType
    {
        return ice::ecs::query::for_entity(self.query_object(), entity, false);
    }

    template<typename Self>
    inline auto TraitQueryOperations::try_entity(
        this Self&& self, ice::ecs::Entity entity
    ) noexcept -> typename ice::clear_type_t<Self>::ResultType
    {
        return ice::ecs::query::for_entity(self.query_object(), entity, true);
    }

    template<typename Self>
    inline auto TraitQueryOperations::for_each_entity(this Self&& self) noexcept -> ice::Generator<typename ice::clear_type_t<Self>::ResultType>
    {
        if constexpr (ice::clear_type_t<Self>::Type == QueryType::Synchronized && std::is_rvalue_reference_v<decltype(self)>)
        {
            return ice::ecs::query::for_each_entity_gen(self.query_object(), self.filter_object(), ice::move(self));
        }
        else
        {
            return ice::ecs::query::for_each_entity_gen(self.query_object(), self.filter_object(), int{});
        }
    }

    template<typename Self, typename Fn>
    inline void TraitQueryOperations::for_each_entity(this Self&& self, Fn&& fn) noexcept
    {
        return ice::ecs::query::for_each_entity(self.query_object(), self.filter_object(), ice::forward<Fn>(fn));
    }

    template<typename Self>
    inline auto TraitQueryOperations::for_each_block(this Self&& self) noexcept -> ice::Generator<typename ice::clear_type_t<Self>::BlockResultType>
    {
        if constexpr (ice::clear_type_t<Self>::Type == QueryType::Synchronized && std::is_rvalue_reference_v<decltype(self)>)
        {
            return ice::ecs::query::for_each_block_gen(self.query_object(), self.filter_object(), ice::move(self));
        }
        else
        {
            return ice::ecs::query::for_each_block_gen(self.query_object(), self.filter_object(), int{});
        }
    }

    template<typename Self, typename Fn>
    inline void TraitQueryOperations::for_each_block(this Self&& self, Fn&& fn) noexcept
    {
        return ice::ecs::query::for_each_block(self.query_object(), self.filter_object(), ice::forward<Fn>(fn));
    }

} // namespace ice::ecs
