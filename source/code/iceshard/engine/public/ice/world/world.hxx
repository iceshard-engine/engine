/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/data_storage.hxx>
#include <ice/ecs/ecs_entity_storage.hxx>
#include <ice/ecs/ecs_query_storage.hxx>
#include <ice/world/world_trait_types.hxx>
#include <ice/engine_state.hxx>
#include <ice/span.hxx>

namespace ice
{

    static constexpr ice::EngineStateGraph StateGraph_WorldState = "world-state"_state_graph;
    static constexpr ice::EngineState State_WorldCreated = StateGraph_WorldState | "created";
    static constexpr ice::EngineState State_WorldActive = StateGraph_WorldState | "active";
    static constexpr ice::EngineState State_WorldInactive = StateGraph_WorldState | "inactive";

    struct World
    {
        virtual ~World() noexcept = default;

        virtual auto activate(ice::WorldStateParams const& params) noexcept -> ice::Task<> = 0;
        virtual auto deactivate(ice::WorldStateParams const& params) noexcept -> ice::Task<> = 0;

        virtual auto trait(ice::StringID_Arg trait_identifier) noexcept -> ice::Trait* = 0;
        virtual auto trait(ice::StringID_Arg trait_identifier) const noexcept -> ice::Trait const* = 0;

        virtual auto trait_storage(ice::Trait* trait) noexcept -> ice::DataStorage* = 0;
        virtual auto trait_storage(ice::Trait const* trait) const noexcept -> ice::DataStorage const* = 0;

        virtual auto entities() noexcept -> ice::ecs::EntityIndex& = 0;
        virtual auto entity_queries() noexcept -> ice::ecs::QueryProvider const& = 0;
        virtual auto entity_queries_storage() noexcept -> ice::ecs::QueryStorage & = 0;
        virtual auto entity_operations() noexcept -> ice::ecs::EntityOperations& = 0;
    };

} // namespace ice
