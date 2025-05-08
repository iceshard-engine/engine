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
            ice::ecs::Query<Types...>& out_query
        ) const noexcept;

        template<typename... QueryParts>
        void initialize_query_v2(
            ice::ecs::Query_v2<QueryParts...>& out_query,
            ice::Span<ice::StringID const> tags = {}
        ) const noexcept;

#if 0
        template<ice::ecs::QueryType... Types, ice::ecs::QueryType... SubTypes>
        void expand_query(
            ice::ecs::Query<Types...>& out_query
        ) const noexcept;
#endif

        template<ice::ecs::QueryType... Types>
        auto create_query(
            ice::Allocator& alloc,
            ice::ecs::QueryDefinition<Types...> const& = { }
        ) const noexcept -> ice::ecs::Query<Types...>;

        virtual auto find_archetype(
            ice::String name
        ) const noexcept -> ice::ecs::Archetype = 0;

        virtual auto query_data_slot(
            ice::ecs::Entity entity
        ) const noexcept -> ice::ecs::EntityDataSlot = 0;

        virtual auto query_data_slots(
            ice::Span<ice::ecs::Entity const> requested,
            ice::Span<ice::ecs::EntityDataSlot> out_data_slots
        ) const noexcept -> ice::ucount = 0;

    protected:
        virtual void query_internal(
            ice::Span<ice::ecs::detail::QueryTypeInfo const> query_info,
            ice::Span<ice::ecs::QueryAccessTracker*> out_access_trackers,
            ice::Array<ice::ecs::ArchetypeInstanceInfo const*>& out_instance_infos,
            ice::Array<ice::ecs::DataBlock const*>& out_data_blocks
        ) const noexcept = 0;

        virtual void query_internal_v2(
            ice::Span<ice::ecs::detail::QueryTypeInfo const> query_info,
            ice::Span<ice::StringID const> query_tags,
            ice::Span<ice::ecs::QueryAccessTracker*> out_access_trackers,
            ice::Array<ice::ecs::ArchetypeInstanceInfo const*>& out_instance_infos,
            ice::Array<ice::ecs::DataBlock const*>& out_data_blocks
        ) const noexcept = 0;

        template<ice::u32 RefIdx, ice::ecs::QueryType... QueryComponents>
        bool initialize_query_part(
            ice::ecs::QueryPart<RefIdx, QueryComponents...> const& query_part,
            ice::Span<ice::StringID const> query_tags,
            ice::Span<ice::ecs::QueryAccessTracker*> out_access_trackers,
            ice::Array<ice::ecs::ArchetypeInstanceInfo const*>& out_instance_infos,
            ice::Array<ice::ecs::DataBlock const*>& out_data_blocks,
            ice::Array<ice::u32>& out_argument_idx_map,
            ice::u32& out_archetype_count
        ) const noexcept;

#if 0
        virtual void query_expand_internal(
            ice::Span<ice::ecs::detail::QueryTypeInfo const> query_info,
            ice::Span<ice::ecs::QueryAccessTracker*> out_access_trackers,
            ice::Array<ice::ecs::ArchetypeInstanceInfo const*>& out_instance_infos,
            ice::Array<ice::ecs::DataBlock const*>& out_data_blocks
        ) const noexcept = 0;
#endif
    };

    template<ice::ecs::QueryType... Types>
    void QueryProvider::initialize_query(
        ice::ecs::Query<Types...>& out_query
    ) const noexcept
    {
        using Definition = ice::ecs::QueryDefinition<Types...>;
        static constexpr Definition definition{ };

        // Store the query provider.
        out_query.provider = this;

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

    template<ice::u32 RefIdx, ice::ecs::QueryType... QueryComponents>
    bool QueryProvider::initialize_query_part(
        ice::ecs::QueryPart<RefIdx, QueryComponents...> const& query_part,
        ice::Span<ice::StringID const> query_tags,
        ice::Span<ice::ecs::QueryAccessTracker*> out_access_trackers,
        ice::Array<ice::ecs::ArchetypeInstanceInfo const*>& out_instance_infos,
        ice::Array<ice::ecs::DataBlock const*>& out_data_blocks,
        ice::Array<ice::u32>& out_argument_idx_map,
        ice::u32& out_archetype_count
    ) const noexcept
    {
        using Part = QueryPart<RefIdx, QueryComponents...>;

        ice::u32 const prev_arch_count = ice::count(out_instance_infos);

        this->query_internal_v2(
            ice::span::from_std_const(Part::Definition::Constant_Requirements),
            (RefIdx == 0 ? query_tags : ice::Span<ice::StringID const>{}), // We only want to apply tags on the main part
            out_access_trackers,
            out_instance_infos,
            out_data_blocks
        );

        ice::u32 const new_arch_count = ice::count(out_instance_infos);
        out_archetype_count = new_arch_count - prev_arch_count;

        ice::u32 const prev_arim_count = ice::count(out_argument_idx_map);
        ice::array::resize(out_argument_idx_map, prev_arim_count + out_archetype_count * Part::CompCount);

        // Copy values to the array
        ice::u32* it = ice::array::begin(out_argument_idx_map) + prev_arim_count;
        for (ice::ecs::ArchetypeInstanceInfo const* instance : ice::array::slice(out_instance_infos, prev_arch_count))
        {
            auto const archetype_argument_idx_map = ice::ecs::detail::argument_idx_map<QueryComponents...>(*instance);
            ice::memcpy(it, archetype_argument_idx_map.data(), archetype_argument_idx_map.size() * sizeof(ice::u32));
            it += Part::CompCount;
        }
        return true;
    }

    template<typename... QueryParts>
    void QueryProvider::initialize_query_v2(
        ice::ecs::Query_v2<QueryParts...>& out_query,
        ice::Span<ice::StringID const> query_required_tags
    ) const noexcept
    {
        // Store the query provider.
        out_query.provider = this;

        auto const helper = [this, &out_query, &query_required_tags]<std::size_t... Idx>(std::index_sequence<Idx...> seq) noexcept
        {
            [[maybe_unused]]
            bool const query_results[]{
                this->initialize_query_part(
                    QueryParts{},
                    query_required_tags,
                    out_query.access_trackers,
                    out_query.archetype_instances,
                    out_query.archetype_data_blocks,
                    out_query.archetype_argument_idx_map,
                    out_query.archetype_count_for_part[Idx]
                )...
            };
        };

        helper(std::index_sequence_for<QueryParts...>{});
    }

#if 0
    template<ice::ecs::QueryType... Types, ice::ecs::QueryType... SubTypes>
    void QueryProvider::expand_query(
        ice::ecs::Query<Types...>& out_query
    ) const noexcept
    {
        using Definition = ice::ecs::QueryDefinition<SubTypes...>;
        static constexpr Definition definition{ };

        ICE_ASSERT_CORE(out_query.provider == this);

        ice::ucount const arch_count = ice::array::count(out_query.archetype_instances);

        // Run the internal query to access all data that is not available here.
        this->query_expand_internal(
            ice::span::from_std_const(definition.requirements),
            out_query.access_trackers,
            out_query.archetype_instances,
            out_query.archetype_data_blocks
        );

        ice::ucount const idx_map_size = ice::array::count(out_query.archetype_argument_idx_map);
        ice::ucount const arch_diff_count = ice::count(out_query.archetype_instances) - arch_count;

        // Expand the map to the additional size of the sub-query
        ice::array::resize(out_query.archetype_argument_idx_map, idx_map_size + arch_diff_count * definition.component_count);

        // Append new values to the array
        ice::u32* it = ice::array::begin(out_query.archetype_argument_idx_map) + idx_map_size;
        for (ice::ecs::ArchetypeInstanceInfo const* instance : ice::span::subspan(out_query.archetype_instances, arch_count))
        {
            auto const archetype_argument_idx_map = ice::ecs::detail::argument_idx_map<SubTypes...>(*instance);
            ice::memcpy(it, archetype_argument_idx_map.data(), archetype_argument_idx_map.size() * sizeof(ice::u32));
            it += definition.component_count;
        }
    }
#endif

    template<ice::ecs::QueryType... Types>
    [[nodiscard]]
    auto QueryProvider::create_query(
        ice::Allocator& alloc,
        ice::ecs::QueryDefinition<Types...> const&
    ) const noexcept -> ice::ecs::Query<Types...>
    {
        using Definition = ice::ecs::QueryDefinition<Types...>;
        using Query = typename Definition::Query;

        Query result{ alloc };
        this->initialize_query(result);
        return result;
    }

} // namespace ice::ecs
