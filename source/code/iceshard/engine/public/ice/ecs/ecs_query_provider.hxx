/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/container/array.hxx>
#include <ice/ecs/ecs_data_block.hxx>
#include <ice/ecs/ecs_query_definition.hxx>
#include <ice/ecs/ecs_query_object.hxx>

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

        template<typename... QueryParts>
        void initialize_query_object(
            ice::ecs::QueryObject<QueryParts...>& out_query,
            ice::Span<ice::StringID const> tags = {}
        ) const noexcept;

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
            ice::Span<ice::StringID const> query_tags,
            ice::Span<ice::ecs::QueryAccessTracker*> out_access_trackers,
            ice::Array<ice::ecs::ArchetypeInstanceInfo const*>& out_instance_infos,
            ice::Array<ice::ecs::DataBlock const*>& out_data_blocks
        ) const noexcept = 0;

        template<ice::u32 RefIdx, ice::ecs::QueryType... QueryComponents>
        bool initialize_query_object_part(
            ice::ecs::detail::QueryObjectPart<RefIdx, QueryComponents...> const& query_part,
            ice::Span<ice::StringID const> query_tags,
            ice::Span<ice::ecs::QueryAccessTracker*> out_access_trackers,
            ice::Array<ice::ecs::ArchetypeInstanceInfo const*>& out_instance_infos,
            ice::Array<ice::ecs::DataBlock const*>& out_data_blocks,
            ice::Array<ice::u32>& out_argument_idx_map,
            ice::u32& out_archetype_count
        ) const noexcept;
    };

    template<ice::u32 RefIdx, ice::ecs::QueryType... QueryComponents>
    bool QueryProvider::initialize_query_object_part(
        ice::ecs::detail::QueryObjectPart<RefIdx, QueryComponents...> const& query_part,
        ice::Span<ice::StringID const> query_tags,
        ice::Span<ice::ecs::QueryAccessTracker*> out_access_trackers,
        ice::Array<ice::ecs::ArchetypeInstanceInfo const*>& out_instance_infos,
        ice::Array<ice::ecs::DataBlock const*>& out_data_blocks,
        ice::Array<ice::u32>& out_argument_idx_map,
        ice::u32& out_archetype_count
    ) const noexcept
    {
        using Part = ice::ecs::detail::QueryObjectPart<RefIdx, QueryComponents...>;

        ice::u32 const prev_arch_count = ice::count(out_instance_infos);

        this->query_internal(
            ice::span::from_std_const(Part::Definition::Constant_Requirements),
            (RefIdx == 0 ? query_tags : ice::Span<ice::StringID const>{}), // We only want to apply tags on the main part
            out_access_trackers,
            out_instance_infos,
            out_data_blocks
        );

        ice::u32 const new_arch_count = ice::count(out_instance_infos);
        out_archetype_count = new_arch_count - prev_arch_count;

        ice::u32 const prev_arim_count = ice::count(out_argument_idx_map);
        ice::array::resize(out_argument_idx_map, prev_arim_count + out_archetype_count * Part::ComponentCount);

        // Copy values to the array
        ice::u32* it = ice::array::begin(out_argument_idx_map) + prev_arim_count;
        for (ice::ecs::ArchetypeInstanceInfo const* instance : ice::array::slice(out_instance_infos, prev_arch_count))
        {
            auto const archetype_argument_idx_map = ice::ecs::detail::argument_idx_map<QueryComponents...>(*instance);
            ice::memcpy(it, archetype_argument_idx_map.data(), archetype_argument_idx_map.size() * sizeof(ice::u32));
            it += Part::ComponentCount;
        }
        return true;
    }

    template<typename... QueryParts>
    void QueryProvider::initialize_query_object(
        ice::ecs::QueryObject<QueryParts...>& out_query,
        ice::Span<ice::StringID const> query_required_tags
    ) const noexcept
    {
        // Store the query provider.
        out_query.provider = this;

        auto const helper = [this, &out_query, &query_required_tags]<std::size_t... Idx>(std::index_sequence<Idx...> seq) noexcept
        {
            [[maybe_unused]]
            bool const query_results[]{
                this->initialize_query_object_part(
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

} // namespace ice::ecs
