/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/ecs/ecs_entity.hxx>
#include <ice/ecs/ecs_component.hxx>
#include <ice/ecs/ecs_query_type.hxx>
#include <ice/ecs/ecs_archetype.hxx>
#include <ice/container_types.hxx>

namespace ice::ecs::detail
{

    template<QueryArg... Components>
    struct UnsortedQueryRequirements
    {
        static constexpr ice::StaticArray<ice::ecs::detail::QueryTypeInfo, sizeof...(Components)> const Constant_Requirements{
            ice::ecs::detail::QueryComponentTypeInfo<Components>{}...
        };
    };

    template<QueryArg First, QueryArg... Components>
    struct QueryRequirements
    {
        static constexpr ice::StaticArray<ice::ecs::detail::QueryTypeInfo, 1 + sizeof...(Components)> const Constant_Requirements =
            ice::constexpr_sort_stdarray(
                UnsortedQueryRequirements<First, Components...>::Constant_Requirements,
                static_cast<ice::u32>(std::is_same_v<First, ice::ecs::Entity>)
            );
    };


    template<QueryTagType... Tags>
    struct UnsortedQueryTags
    {
        static constexpr ice::StaticArray<ice::StringID, sizeof...(Tags)> const Constant_Tags{
            ice::ecs::Constant_ComponentIdentifier<Tags>...
        };
    };

    template<QueryTagType... Tags>
    struct QueryTags
    {
        static constexpr ice::StaticArray<ice::StringID, sizeof...(Tags)> const Constant_Tags =
            ice::constexpr_sort_stdarray(UnsortedQueryTags<Tags...>::Constant_Tags, 0);
    };

    template<QueryArg Arg>
    struct QueryIteratorArgument { };

    template<typename Arg> requires QueryArg<Arg*>
    struct QueryIteratorArgument<Arg*>
    {
        using BlockIteratorArg = Arg*;
        using EntityIteratorArg = Arg*;

        static auto block_array(void* array_ptrs)
        {
            return reinterpret_cast<BlockIteratorArg>(array_ptrs);
        }

        static auto select_entity(BlockIteratorArg block_ptr, ice::u32 idx) noexcept -> Arg*
        {
            return block_ptr + idx;
        }

        static auto select_entity_ptr(BlockIteratorArg block_ptr, ice::u32 idx) noexcept -> Arg*
        {
            return block_ptr + idx;
        }
    };

    template<typename Arg> requires QueryArg<Arg&>
    struct QueryIteratorArgument<Arg&>
    {
        using BlockIteratorArg = Arg*;
        using EntityIteratorArg = Arg&;

        static auto block_array(void* array_ptrs)
        {
            return reinterpret_cast<BlockIteratorArg>(array_ptrs);
        }

        static auto select_entity(BlockIteratorArg block_ptr, ice::u32 idx) noexcept -> Arg&
        {
            return *(block_ptr + idx);
        }

        static auto select_entity_ptr(BlockIteratorArg block_ptr, ice::u32 idx) noexcept -> Arg*
        {
            return block_ptr + idx;
        }
    };

    template<>
    struct QueryIteratorArgument<ice::ecs::Entity>
    {
        using BlockIteratorArg = ice::ecs::Entity const*;
        using EntityIteratorArg = ice::ecs::Entity;

        static auto block_array(void* array_ptrs)
        {
            return reinterpret_cast<BlockIteratorArg>(array_ptrs);
        }

        static auto select_entity(BlockIteratorArg block_ptr, ice::u32 idx) noexcept -> ice::ecs::Entity
        {
            return *(block_ptr + idx);
        }

        static auto select_entity_ptr(BlockIteratorArg block_ptr, ice::u32 idx) noexcept -> ice::ecs::Entity const*
        {
            return block_ptr + idx;
        }
    };

    template<QueryArg... Args>
    using QueryBlockIteratorSignature = void (ice::u32, typename QueryIteratorArgument<Args>::BlockIteratorArg...);

    template<QueryArg... Args>
    using QueryEntityIteratorSignature = void (typename QueryIteratorArgument<Args>::EntityIteratorArg...);

    template<QueryArg... Args>
    using QueryBlockTupleResult = std::tuple<ice::u32, typename QueryIteratorArgument<Args>::BlockIteratorArg...>;

    template<QueryArg... Args>
    using QueryEntityTupleResult = std::tuple<typename QueryIteratorArgument<Args>::BlockIteratorArg...>;


    template<QueryArg... Components>
    inline auto make_argument_idx_map(
        ice::ecs::detail::ArchetypeInstanceInfo const& archetype_info
    ) noexcept -> std::array<ice::u32, sizeof...(Components)>
    {
        constexpr ice::u32 component_count = sizeof...(Components);
        constexpr ice::StringID components[]{ ice::ecs::Constant_ComponentIdentifier<ice::clear_type_t<Components>>... };

        std::array<ice::u32, component_count> result{ ice::u32_max };
        for (ice::u32 idx = 0; idx < component_count; ++idx)
        {
            result[idx] = std::numeric_limits<ice::u32>::max();

            ice::u32 arg_idx = 0;
            for (ice::StringID_Arg component_arg : archetype_info.component_identifiers)
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

    template<typename Fn, QueryArg... T>
    inline auto invoke_for_each_block(Fn&& fn, ice::u32 count, void** component_pointer_array) noexcept
    {
        using QueryTypeTuple = std::tuple<T...>;

        auto const enumerate_types = [&]<std::size_t... Idx>(std::index_sequence<Idx...> seq) noexcept
        {
            ice::forward<Fn>(fn)(
                count,
                ice::ecs::detail::QueryIteratorArgument<std::tuple_element_t<Idx, QueryTypeTuple>>::block_array(component_pointer_array[Idx])...
            );
        };

        enumerate_types(std::make_index_sequence<sizeof...(T)>{});
    }

    template<typename Fn, QueryArg... T>
    inline auto invoke_for_each_entity(Fn&& fn, ice::u32 count, void** component_pointer_array) noexcept
    {
        using QueryTypeTuple = std::tuple<T...>;

        auto const enumerate_entities = [&](ice::u32 count, typename ice::ecs::detail::QueryIteratorArgument<T>::BlockIteratorArg... args)
        {
            for (ice::u32 idx = 0; idx < count; ++idx)
            {
                ice::forward<Fn>(fn)(
                    ice::ecs::detail::QueryIteratorArgument<T>::select_entity(args, idx * static_cast<ice::u32>(args != nullptr))...
                );
            }
        };

        auto const enumerate_types = [&]<std::size_t... Idx>(std::index_sequence<Idx...> seq) noexcept
        {
            enumerate_entities(
                count,
                ice::ecs::detail::QueryIteratorArgument<std::tuple_element_t<Idx, QueryTypeTuple>>::block_array(component_pointer_array[Idx])...
            );
        };

        enumerate_types(std::make_index_sequence<sizeof...(T)>{});
    }

    template<QueryArg... T>
    inline auto create_block_tuple(ice::u32 count, void** component_pointer_array) noexcept -> ice::ecs::detail::QueryBlockTupleResult<T...>
    {
        using QueryTypeTuple = std::tuple<T...>;
        using QueryTypeTupeResult = ice::ecs::detail::QueryBlockTupleResult<T...>;

        auto const enumerate_types = [&]<std::size_t... Idx>(std::index_sequence<Idx...> seq) noexcept -> QueryTypeTupeResult
        {
            return QueryTypeTupeResult{ count, ice::ecs::detail::QueryIteratorArgument<std::tuple_element_t<Idx, QueryTypeTuple>>::block_array(component_pointer_array[Idx])... };
        };

        return enumerate_types(std::make_index_sequence<sizeof...(T)>{});
    }

    template<QueryArg... T>
    inline auto create_entity_tuple(ice::u32 index, void** component_pointer_array) noexcept -> ice::ecs::detail::QueryEntityTupleResult<T...>
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

} // namespace ice::ecs::detail
