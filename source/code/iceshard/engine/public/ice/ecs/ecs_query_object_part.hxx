/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/ecs/ecs_query_definition.hxx>

namespace ice::ecs::detail
{

    template<ice::u32 ReferencedIdx, ice::ecs::QueryArg... Components>
    struct QueryObjectPart
    {
        using Definition = ice::ecs::QueryDefinition<Components...>;
        using ResultType = ice::ecs::detail::QueryEntityTupleResult<Components...>;
        using BlockResultType = ice::ecs::detail::QueryBlockTupleResult<Components...>;
        using ComponentsTypeList = std::tuple<Components...>;

        static constexpr Definition Constant_QueryDefinition;

        static constexpr ice::u32 RefIndex = ReferencedIdx;
        static constexpr ice::u32 ComponentCount = sizeof...(Components);
    };

} // namespace ice::ecs
