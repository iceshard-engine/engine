/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/ecs/ecs_query.hxx>

namespace ice::ecs
{

    //! \brief An entity instance is a specific query result for a single entity.
    //!
    //! \detail This query result cannot be cached, but allows to quickly access a single entity and selected components.
    //! \detail Because this operation is very similar to a regular query, the same Query object is used to associate data with specific component types.
    template<typename... QueryComponents>
    struct EntityInstance
    {

    };

} // namespace ice::ecs
