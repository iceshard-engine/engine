/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_allocator_stack.hxx>
#include <ice/task_scoped_container.hxx>
#include <ice/ecs/ecs_entity_storage.hxx>
#include <ice/world/world_assembly.hxx>
#include <ice/engine_types.hxx>
#include <ice/asset_types.hxx>
#include <ice/stringid.hxx>
#include <ice/clock.hxx>

namespace ice
{

    struct WorldStateParams
    {
        ice::Clock const& clock;
        ice::AssetStorage& assets;
        ice::Engine& engine;
        ice::EngineSchedulers thread;
        //ice::EngineDevUI* devui;
    };

    struct WorldState2
    {
        ice::u8 flow_value;
        ice::u8 flow_id; // If '0' will be assigned to the owning state/
    };

    struct WorldUpdateParams
    {
        //! \brief Shards used to call update callbacks.
        ice::Span<ice::Shard const> request_shards;

        ice::WorldState2 required_state;
    };

    struct WorldUpdater
    {
        virtual ~WorldUpdater() noexcept = default;

        virtual void update(
            ice::TaskContainer& out_tasks,
            ice::WorldUpdateParams const& params
        ) noexcept = 0;
    };

    struct WorldStateStage
    {
        ice::ShardID trigger;
        ice::u8 required_state;
        ice::u8 resulting_state;
        ice::WorldState2 dependent_state;
        ice::WorldState2 blocked_state;
        ice::ShardID event_shard;
        ice::ShardID notification;

        //using StageFunction = void(*)(void* userdata) noexcept;
        //StageFunction stage_begin;
        //StageFunction stage_frame;
        //StageFunction stage_end;
    };

    struct WorldStateFlow
    {
        ice::StringID name;
        ice::Span<ice::WorldStateStage const> stages;
        ice::u8 initial_state = 0;

        void* userdata;
        using FnStateShards = void(*)(void* userdata, ice::ShardContainer& out_shards);
        FnStateShards fn_shards;
    };

    struct WorldStateTracker
    {
        virtual ~WorldStateTracker() noexcept = default;

        //! \returns A non-zero value if a flow with the given name was found.
        virtual auto flowid(ice::StringID_Arg flow_name) const noexcept -> ice::u8 = 0;

        virtual bool has_pending_changes() const noexcept = 0;

        virtual auto flow_stage(ice::u8 flowid) const noexcept -> ice::u8 = 0;

        //! \brief Registers a new state flow for all worlds.
        //! \details The flow ID returned from this function can be used to define state dependencies
        //!   between different state flows.
        //! \returns Assigned flow ID.
        virtual auto register_flow(ice::WorldStateFlow flow) noexcept -> ice::u8 = 0;

        //! \brief Checks for state changes and keeps a list of pending state changes.
        virtual void process_state_events(ice::ShardContainer const& shards) noexcept = 0;
    };


} // namespace ice

template<>
inline constexpr ice::ShardPayloadID ice::Constant_ShardPayloadID<ice::WorldStateParams const*> = ice::shard_payloadid("ice::WorldStateParams const*");
