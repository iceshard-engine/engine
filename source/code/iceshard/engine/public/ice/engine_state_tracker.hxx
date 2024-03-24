/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/engine_state_definition.hxx>
#include <ice/mem_unique_ptr.hxx>

namespace ice
{

    struct EngineStateRegisterParams
    {
        ice::EngineState initial;
        ice::EngineStateCommitter* committer = nullptr;
        bool enable_subname_states = false;
    };

    struct EngineStateTracker
    {
        virtual ~EngineStateTracker() noexcept = default;

        virtual bool register_graph(
            ice::EngineStateRegisterParams params,
            ice::Span<ice::EngineStateTrigger const> triggers
        ) noexcept = 0;

        virtual bool register_subname(
            ice::StringID_Arg subname
        ) noexcept = 0;

        virtual auto current_state(
            ice::EngineStateGraph state_graph
        ) const noexcept -> ice::EngineState = 0;

        virtual auto current_state(
            ice::EngineStateGraph state_graph,
            ice::StringID_Arg subname
        ) const noexcept -> ice::EngineState = 0;

        virtual auto update_states(
            ice::ShardContainer const& shards,
            ice::ShardContainer& out_shards
        ) noexcept -> ice::ucount = 0;
    };

    auto create_state_tracker(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::EngineStateTracker>;

} // namespace ice
