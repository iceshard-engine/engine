/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/container/array.hxx>
#include <ice/ecs/ecs_data_block.hxx>
#include <ice/ecs/ecs_query_definition.hxx>

namespace ice::ecs
{

    struct QueryAccessTracker
    {
        std::atomic<ice::u32> access_stage_executed;
        std::atomic<ice::u32> access_stage_next;
    };

    struct QueryProvider
    {
        virtual ~QueryProvider() noexcept = default;

        template<ice::ecs::QueryType... Types>
        void initialize_query(
            ice::ecs::Query<ice::ecs::QueryDefinition<Types...>>& out_query
        ) const noexcept;

        template<ice::ecs::QueryType... Types>
        auto create_query(
            ice::Allocator& alloc,
            ice::ecs::QueryDefinition<Types...> const& = { }
        ) const noexcept -> ice::ecs::Query<ice::ecs::QueryDefinition<Types...>>;

        virtual auto resolve_entities(
            ice::Span<ice::ecs::Entity const> requested,
            ice::Span<ice::ecs::EntityHandle> resolved
        ) const noexcept -> ice::ucount = 0;

    protected:
        virtual void query_internal(
            ice::Span<ice::ecs::detail::QueryTypeInfo const> query_info,
            ice::Span<ice::ecs::QueryAccessTracker*> out_access_trackers,
            ice::Array<ice::ecs::ArchetypeInstanceInfo const*>& out_instance_infos,
            ice::Array<ice::ecs::DataBlock const*>& out_data_blocks
        ) const noexcept = 0;
    };

    template<ice::ecs::QueryType... Types>
    void QueryProvider::initialize_query(
        ice::ecs::Query<ice::ecs::QueryDefinition<Types...>>& out_query
    ) const noexcept
    {
        using Definition = ice::ecs::QueryDefinition<Types...>;
        static constexpr Definition definition{ };

        // Run the internal query to access all data that is not available here.
        this->query_internal(
            ice::span::from_std_const(definition.requirements),
            out_query.access_trackers,
            out_query.archetype_instances,
            out_query.archetype_data_blocks
        );

        ice::array::resize(out_query.archetype_argument_idx_map, ice::count(out_query.archetype_instances) * definition.component_count);

        // Copy values to the array
        ice::u32* it = ice::array::begin(out_query.archetype_argument_idx_map);
        for (ice::ecs::ArchetypeInstanceInfo const* instance : out_query.archetype_instances)
        {
            auto const archetype_argument_idx_map = ice::ecs::detail::argument_idx_map<Types...>(*instance);
            ice::memcpy(it, archetype_argument_idx_map.data(), archetype_argument_idx_map.size() * sizeof(ice::u32));
            it += definition.component_count;
        }
    }

    template<ice::ecs::QueryType... Types>
    [[nodiscard]]
    auto QueryProvider::create_query(
        ice::Allocator& alloc,
        ice::ecs::QueryDefinition<Types...> const&
    ) const noexcept -> ice::ecs::Query<ice::ecs::QueryDefinition<Types...>>
    {
        using Definition = ice::ecs::QueryDefinition<Types...>;
        using Query = typename Definition::Query;

        Query result{ alloc };
        this->initialize_query(result);
        return result;
    }


    // template<ice::ecs::QueryType First, ice::ecs::QueryType... Args>
    // struct QueryWork<First, Args...>
    // {
    //     using QueryDefinition = ice::ecs::QueryDefinition<First, Args...>;
    //     using QueryWorkFn = typename QueryDefinition::ForEachEntityFn;
    //     using Query = typename QueryDefinition::Query;

    //     static void execute(void const* self_ptr) noexcept;

    //     Query const query;
    //     QueryWorkFn* const work_function;
    // };

    // template<ice::ecs::QueryType First, ice::ecs::QueryType... Args>
    // void QueryWork<First, Args...>::execute(void const* self_ptr) noexcept
    // {
    //     QueryWork<ice::u32, Args...> const* const self = reinterpret_cast<QueryWork<ice::u32, Args...> const*>(self_ptr);

    //     ice::ecs::query::for_each_entity<QueryDefinition>(
    //         self->query,
    //         self->work_function
    //     );
    // }

} // namespace ice::ecs
