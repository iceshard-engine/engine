#pragma once
#include <ice/container/array.hxx>
#include <ice/ecs/ecs_query.hxx>

namespace ice::ecs
{

    template<typename T, typename... Args>
    struct QueryWork { };

    template<ice::ecs::QueryType First, ice::ecs::QueryType... Args>
    struct QueryWork<First, Args...>;

    class QueryProvider
    {
    public:
        virtual ~QueryProvider() noexcept = default;

        template<ice::ecs::QueryType... Args>
        constexpr auto create_query_work(
            ice::Allocator& alloc,
            void (*function_ptr)(Args...)
        ) const noexcept -> ice::ecs::QueryWork<Args...>;

        template<ice::ecs::QueryType... Types>
        auto create_query(
            ice::Allocator& alloc,
            ice::ecs::QueryDefinition<Types...> const& = { }
        ) const noexcept -> ice::ecs::Query<ice::ecs::QueryDefinition<Types...>>;

    protected:
        virtual void query_internal(
            ice::Span<ice::ecs::detail::QueryTypeInfo const> query_info,
            ice::Array<ice::ecs::ArchetypeInstanceInfo const*>& out_instance_infos,
            ice::Array<ice::ecs::DataBlock const*>& out_data_blocks
        ) const noexcept = 0;
    };


    template<ice::ecs::QueryType... Args>
    constexpr auto QueryProvider::create_query_work(ice::Allocator& alloc, void (*function_ptr)(Args...)) const noexcept -> ice::ecs::QueryWork<Args...>
    {
        return { .query = create_query(alloc, ice::ecs::QueryDefinition<Args...>{}), .work_function = function_ptr };
    }

    template<ice::ecs::QueryType... Types>
    auto QueryProvider::create_query(
        ice::Allocator& alloc,
        ice::ecs::QueryDefinition<Types...> const&
    ) const noexcept -> ice::ecs::Query<ice::ecs::QueryDefinition<Types...>>
    {
        using Definition = ice::ecs::QueryDefinition<Types...>;
        using Query = typename Definition::Query;

        static constexpr Definition definition{ };

        ice::Array<ice::ecs::ArchetypeInstanceInfo const*> instance_infos{ alloc };
        ice::Array<ice::ecs::DataBlock const*> data_blocks{ alloc };

        // Run the internal query to access all data that is not available here.
        this->query_internal(
            ice::span::from_std_const(definition.requirements),
            instance_infos,
            data_blocks
        );

        ice::Array<ice::StaticArray<ice::u32, definition.component_count>> argument_idx_map{ alloc };

        ice::u32 const archetype_count = ice::count(instance_infos);
        ice::array::reserve(argument_idx_map, archetype_count);

        for (ice::ecs::ArchetypeInstanceInfo const* instance : instance_infos)
        {
            ice::array::push_back(argument_idx_map, ice::ecs::detail::argument_idx_map<Types...>(*instance));
        }

        return Query{
            .archetype_instances = ice::move(instance_infos),
            .archetype_data_blocks = ice::move(data_blocks),
            .archetype_argument_idx_map = ice::move(argument_idx_map),
        };
    }


    template<ice::ecs::QueryType First, ice::ecs::QueryType... Args>
    struct QueryWork<First, Args...>
    {
        using QueryDefinition = ice::ecs::QueryDefinition<First, Args...>;
        using QueryWorkFn = typename QueryDefinition::ForEachEntityFn;
        using Query = typename QueryDefinition::Query;

        static void execute(void const* self_ptr) noexcept;

        Query const query;
        QueryWorkFn* const work_function;
    };

    template<ice::ecs::QueryType First, ice::ecs::QueryType... Args>
    void QueryWork<First, Args...>::execute(void const* self_ptr) noexcept
    {
        QueryWork<ice::u32, Args...> const* const self = reinterpret_cast<QueryWork<ice::u32, Args...> const*>(self_ptr);

        ice::ecs::query::for_each_entity<QueryDefinition>(
            self->query,
            self->work_function
        );
    }

} // namespace ice::ecs
