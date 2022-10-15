#pragma once
#include <ice/ecs/ecs_entity.hxx>
#include <ice/ecs/ecs_component.hxx>
#include <ice/container_types.hxx>

namespace ice::ecs::detail
{

    struct QueryTypeInfo
    {
        ice::StringID identifier;

        bool is_writable = false;
        bool is_optional = false;
    };


    template<typename T>
    concept QueryType = ice::ecs::IsEntityHandle<T>
        || (ice::ecs::Component<T> && !ice::ecs::ComponentTag<T> && (std::is_reference_v<T> || std::is_pointer_v<T>));

    template<QueryType T>
    constexpr bool Constant_QueryTypeIsWritable = ice::ecs::IsEntityHandle<T> == false
        && std::is_const_v<ice::clear_type_t<T>> == false;

    template<QueryType T>
    constexpr bool Constant_QueryTypeIsOptional = ice::ecs::IsEntityHandle<T> == false
        && std::is_pointer_v<T>;


    template<QueryType T>
    struct QueryComponentTypeInfo
    {
        ice::StringID const identifier = ice::ecs::Constant_ComponentIdentifier<ice::clear_type_t<T>>;

        bool const is_writable = ice::ecs::detail::Constant_QueryTypeIsWritable<T>;
        bool const is_optional = ice::ecs::detail::Constant_QueryTypeIsOptional<T>;

        constexpr operator QueryTypeInfo() const noexcept;
    };

    template<QueryType T>
    constexpr QueryComponentTypeInfo<T>::operator QueryTypeInfo() const noexcept
    {
        return QueryTypeInfo{ .identifier = identifier, .is_writable = is_writable, .is_optional = is_optional };
    }


    constexpr bool operator<(
        ice::ecs::detail::QueryTypeInfo const& left,
        ice::ecs::detail::QueryTypeInfo const& right
    ) noexcept
    {
        return ice::hash(left.identifier) < ice::hash(right.identifier);
    }

    template<QueryType... Components>
    struct UnsortedQueryRequirements
    {
        static constexpr ice::StaticArray<ice::ecs::detail::QueryTypeInfo, sizeof...(Components)> const Constant_Requirements{
            ice::ecs::detail::QueryComponentTypeInfo<Components>{}...
        };
    };

    template<QueryType First, QueryType... Components>
    struct QueryRequirements
    {
        static constexpr ice::StaticArray<ice::ecs::detail::QueryTypeInfo, 1 + sizeof...(Components)> const Constant_Requirements =
            constexpr_sort_array(UnsortedQueryRequirements<First, Components...>::Constant_Requirements, static_cast<ice::u32>(std::is_same_v<First, ice::ecs::EntityHandle>));
    };


    template<QueryType Arg>
    struct QueryIteratorArgument { };

    template<typename Arg> requires QueryType<Arg*>
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
    };

    template<typename Arg> requires QueryType<Arg&>
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
    };

    template<>
    struct QueryIteratorArgument<ice::ecs::EntityHandle>
    {
        using BlockIteratorArg = ice::ecs::EntityHandle const*;
        using EntityIteratorArg = ice::ecs::EntityHandle;

        static auto block_array(void* array_ptrs)
        {
            return reinterpret_cast<BlockIteratorArg>(array_ptrs);
        }

        static auto select_entity(BlockIteratorArg block_ptr, ice::u32 idx) noexcept -> ice::ecs::EntityHandle
        {
            return *(block_ptr + idx);
        }
    };

    template<QueryType... Args>
    using QueryBlockIteratorSignature = void (ice::u32, typename QueryIteratorArgument<Args>::BlockIteratorArg...);

    template<QueryType... Args>
    using QueryEntityIteratorSignature = void (typename QueryIteratorArgument<Args>::EntityIteratorArg...);


    template<QueryType... Components>
    inline auto argument_idx_map(
        ice::ecs::ArchetypeInstanceInfo const& archetype_info
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

    template<typename Fn, QueryType... T>
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

    template<typename Fn, QueryType... T>
    auto invoke_for_each_entity(Fn&& fn, ice::u32 count, void** component_pointer_array) noexcept
    {
        using QueryTypeTuple = std::tuple<T...>;

        auto const enumerate_entities = [&](ice::u32 count, typename ice::ecs::detail::QueryIteratorArgument<T>::BlockIteratorArg... args)
        {
            for (ice::u32 idx = 0; idx < count; ++idx)
            {
                ice::forward<Fn>(fn)(
                    ice::ecs::detail::QueryIteratorArgument<T>::select_entity(args, idx * static_cast<ice::u64>(args != nullptr))...
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

} // namespace ice::ecs::detail
