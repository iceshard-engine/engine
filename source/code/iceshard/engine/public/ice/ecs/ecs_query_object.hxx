/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/ecs/ecs_query_object_part.hxx>

namespace ice::ecs
{

    using ice::ecs::detail::QueryObjectPart;

    template<typename... Parts>
    struct QueryObject
    {
    public:
        using ResultType = typename ice::tuples_merged_t<typename Parts::ResultType...>;
        using BlockResultType = typename ice::tuples_merged_t<typename Parts::BlockResultType...>;
        using ComponentsTypeList = typename ice::tuples_merged_t<typename Parts::ComponentsTypeList...>;
        using Definition = typename ice::ecs::QueryDefinitionFromTuple<ComponentsTypeList>;

        QueryObject(ice::Allocator& alloc) noexcept
            : provider{ nullptr }
            , access_trackers{ }
            , archetype_instances{ alloc }
            , archetype_data_blocks{ alloc }
            , archetype_argument_idx_map{ alloc }
            , archetype_count_for_part{ }
        { }

    public:
        static constexpr ice::u32 PartCount = sizeof...(Parts);
        static constexpr ice::u32 ComponentCount = (0 + ... + Parts::ComponentCount);

        static constexpr ice::u32 QueryPartRefs[PartCount]{};
        static constexpr ice::u32 QueryPartStarts[PartCount]{};

        ice::ecs::QueryProvider const* provider;
        ice::ecs::QueryAccessTracker* access_trackers[std::tuple_size_v<ComponentsTypeList>];
        ice::Array<ice::ecs::ArchetypeInstanceInfo const*> archetype_instances;
        ice::Array<ice::ecs::DataBlock const*> archetype_data_blocks;
        ice::Array<ice::u32> archetype_argument_idx_map;
        ice::u32 archetype_count_for_part[PartCount];
    };

} // namespace ice::ecs
