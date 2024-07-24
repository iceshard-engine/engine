#pragma once
#include <ice/ecs/ecs_query_details.hxx>

namespace ice::ecs
{

    struct QueryView
    {
        template<typename Definition>
        QueryView(ice::ecs::Query<Definition> const& query) noexcept
            : access_trackers{ query.access_trackers }
            , archetype_instances{ query.archetype_instances }
            , archetype_data_blocks{ query.archetype_data_blocks }
            , archetype_argument_idx_map{ query.archetype_argument_idx_map }
        {
        }

        ice::Span<ice::ecs::QueryAccessTracker* const> access_trackers;
        ice::Span<ice::ecs::ArchetypeInstanceInfo const* const> archetype_instances;
        ice::Span<ice::ecs::DataBlock const* const> archetype_data_blocks;
        ice::Span<ice::u32 const> archetype_argument_idx_map;
    };

} // namespace ice::ecs
