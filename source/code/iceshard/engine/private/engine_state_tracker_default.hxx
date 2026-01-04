/// Copyright 2024 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/engine_state_tracker.hxx>
#include <ice/container/array.hxx>
#include <ice/container/queue.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/mem_allocator_stack.hxx>
#include <ice/log.hxx>

namespace ice
{

    struct EngineStateCurrent : ice::EngineState
    {
        ice::StringID subname;
    };

    struct EngineStatePending
    {
        ice::Shard trigger_shard;
        ice::EngineStateTrigger const& trigger;
        ice::EngineStateCommitter& committer;
        ice::EngineStateCurrent& current;
    };

    static EngineStateCurrent const Constant_InvalidState{ };

    class EngineStateTracker_Default : public ice::EngineStateTracker
    {
    public:
        EngineStateTracker_Default(ice::Allocator& alloc) noexcept;

        bool register_graph(
            ice::EngineStateRegisterParams params,
            ice::Span<ice::EngineStateTrigger const> triggers
        ) noexcept override;

        bool register_subname(
            ice::StringID_Arg subname
        ) noexcept override;

        auto current_state_internal(
            ice::EngineStateGraph state_graph,
            ice::StringID_Arg subname
        ) const noexcept -> ice::EngineStateCurrent;

        auto current_state(
            ice::EngineStateGraph state_graph
        ) const noexcept -> ice::EngineState override;

        auto current_state(
            ice::EngineStateGraph state_graph,
            ice::StringID_Arg subname
        ) const noexcept -> ice::EngineState override;

        auto update_states(
            ice::ShardContainer const& shards,
            ice::ShardContainer& out_shards
        ) noexcept -> ice::u32 override;

        void collect_pending_state(
            ice::Shard trigger_shard,
            ice::EngineStateTrigger const& trigger
        ) noexcept;

        bool collect_pending_states(
            ice::ShardContainer const& shards
        ) noexcept;

    private:
        ice::Array<ice::StringID> _subnames;
        ice::Array<ice::EngineStateTrigger> _available_triggers;
        ice::HashMap<ice::EngineState> _initial_states;
        ice::HashMap<ice::EngineStateCommitter*> _state_committers;
        ice::HashMap<ice::u32> _current_state_index;
        ice::Array<ice::EngineStateCurrent> _current_state;
        ice::Queue<ice::EngineStatePending> _pending_states;

    private:
        // New fields
    };

} // namespace ice
