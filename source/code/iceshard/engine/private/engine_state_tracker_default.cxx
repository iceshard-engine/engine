/// Copyright 2024 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "engine_state_tracker_default.hxx"

namespace ice
{

    EngineStateTracker_Default::EngineStateTracker_Default(ice::Allocator& alloc) noexcept
        : _subnames{ alloc }
        , _available_triggers{ alloc }
        , _initial_states{ alloc }
        , _state_committers{ alloc }
        , _current_state_index{ alloc }
        , _current_state{ alloc }
        , _pending_states{ alloc }
    {
        ice::queue::reserve(_pending_states, 16);
    }

    // auto EngineStateTracker_Default::current_states() const noexcept -> ice::Span<ice::EngineStateCurrent const>
    // {
    //     return _current_state;
    // }

    bool EngineStateTracker_Default::register_graph(
        ice::EngineStateRegisterParams params,
        ice::Span<ice::EngineStateTrigger const> triggers
    ) noexcept
    {
        if (ice::hashmap::has(_initial_states, ice::hash(params.initial.graph.value)))
        {
            return false;
        }

        if (params.enable_subname_states == false)
        {
            ice::EngineState const current = current_state(params.initial.graph);
            if (current != Constant_InvalidState)
            {
                // State already exists
                return false;
            }

            EngineStateCurrent initial_state{ { params.initial.graph } };
            initial_state.value = params.initial.value;
            initial_state.subname = ice::StringID_Invalid;

            ice::multi_hashmap::insert(
                _current_state_index,
                ice::hash(params.initial.graph.value),
                _current_state.size().u32()
            );
            _current_state.push_back(initial_state);
        }
        else
        {
            for (ice::StringID_Arg subname : _subnames)
            {
                // Already has a state
                if (current_state(params.initial.graph, subname) != Constant_InvalidState)
                {
                    ICE_LOG(LogSeverity::Warning, LogTag::Engine, "Unexpected graph state {} for subname: {}", params.initial.graph.value, subname);
                    continue;
                }

                EngineStateCurrent initial_state{ { params.initial.graph } };
                initial_state.value = params.initial.value;
                initial_state.subname = subname;

                ice::multi_hashmap::insert(
                    _current_state_index,
                    ice::hash(params.initial.graph.value),
                    _current_state.size().u32()
                );
                _current_state.push_back(initial_state);
            }

            ice::hashmap::set(
                _initial_states,
                ice::hash(params.initial.graph.value),
                params.initial
            );
        }

        ice::hashmap::get_or_set(
            _state_committers,
            ice::hash(params.initial.graph.value),
            params.committer
        );

        ice::array::push_back(
            _available_triggers,
            triggers
        );

        return true;
    }

    bool EngineStateTracker_Default::register_subname(ice::StringID_Arg subname) noexcept
    {
        for (ice::EngineState initial_state : _initial_states)
        {
            // Already has a state
            if (current_state(initial_state.graph, subname) != Constant_InvalidState)
            {
                ICE_LOG(LogSeverity::Warning, LogTag::Engine, "Unexpected graph state {} for subname: {}", initial_state.graph.value, subname);
                continue;
            }

            EngineStateCurrent engine_state{ { initial_state.graph } };
            engine_state.value = initial_state.value;
            engine_state.subname = subname;

            ice::multi_hashmap::insert(
                _current_state_index,
                ice::hash(engine_state.graph.value),
                _current_state.size().u32()
            );
            _current_state.push_back(engine_state);
        }
        return true;
    }

    auto EngineStateTracker_Default::current_state_internal(
        ice::EngineStateGraph state_graph,
        ice::StringID_Arg subname
    ) const noexcept -> ice::EngineStateCurrent
    {
        auto it = ice::multi_hashmap::find_first(_current_state_index, ice::hash(state_graph.value));
        while (it != nullptr)
        {
            ice::EngineStateCurrent const& current = _current_state[it.value()];

            if (current.subname == subname)
            {
                break;
            }
            it = ice::multi_hashmap::find_next(_current_state_index, it);
        }

        if (it == nullptr)
        {
            return Constant_InvalidState;
        }

        return _current_state[it.value()];
    }

    auto EngineStateTracker_Default::current_state(ice::EngineStateGraph state_graph) const noexcept -> ice::EngineState
    {
        return current_state(state_graph, ice::StringID_Invalid);
    }

    auto EngineStateTracker_Default::current_state(
        ice::EngineStateGraph state_graph,
        ice::StringID_Arg subname
    ) const noexcept -> ice::EngineState
    {
        return current_state_internal(state_graph, subname);
    }

    auto EngineStateTracker_Default::update_states(
        ice::ShardContainer const& shards,
        ice::ShardContainer& out_shards
    ) noexcept -> ice::u32
    {
        ice::StackAllocator<512_B> temp_alloc;
        ice::ShardContainer temp_shards{ temp_alloc };
        temp_shards._data.reserve(
            ice::mem_max_capacity(
                ice::size_of<ice::Shard>,
                decltype(temp_alloc)::Constant_InternalCapacity
            )
        );

        // Start with the input shards we got.
        ice::ShardContainer const* input_shards = &shards;
        do
        {
            // Only check for temporary shards.
            if (collect_pending_states(*input_shards))
            {
                // Push back temporary shards into output shards
                ice::shards::push_back(out_shards, temp_shards._data);
                ice::shards::clear(temp_shards);

                // Commit the new states and gather the new shards
                ice::queue::for_each(_pending_states, [&temp_shards](EngineStatePending const& pending) noexcept
                    {
                        bool const success = pending.committer.commit(pending.trigger, pending.trigger_shard, temp_shards);
                        ICE_LOG_IF(success == false, LogSeverity::Error, LogTag::Engine,
                            "Failed state change: {} ({})",
                            pending.trigger.from.graph.value,
                            pending.trigger.from.value
                        );
                        ICE_LOG_IF(success, LogSeverity::Info, LogTag::Engine,
                            "State change: {} ({})",
                            pending.trigger.to.graph.value,
                            pending.trigger.to.value
                        );

                        if (success) // Update state
                        {
                            pending.current.value = pending.trigger.to.value;
                        }
                    }
                );

                ice::queue::clear(_pending_states);
            }
            else
            {
                // Output temporary shards since they might trigger other events.
                ice::shards::push_back(out_shards, temp_shards._data);

                // Clear temporary shards so we can escape the loop normally.
                ice::shards::clear(temp_shards);
            }

            // Set input shards to temp_shards
            input_shards = &temp_shards;

            // If we have any shards added, check if we can collect another set of states.
        } while (ice::shards::empty(temp_shards) == false);

        return true;
    }

    void EngineStateTracker_Default::collect_pending_state(
        ice::Shard trigger_shard,
        ice::EngineStateTrigger const& trigger
    ) noexcept
    {
        ice::StringID trigger_subname;
        bool const has_trigger_subname = ice::shard_inspect(trigger_shard, trigger_subname.value);

        auto it = ice::multi_hashmap::find_first(_current_state_index, ice::hash(trigger.from.graph.value));
        while (it != nullptr)
        {
            ice::EngineStateCurrent& from_state = _current_state[it.value()];
            ICE_ASSERT_CORE(from_state.graph == trigger.from.graph);

            // Check that the graph state we checking is what we expect
            if (from_state.value != trigger.from.value)
            {
                if (has_trigger_subname)
                {
                    it = ice::multi_hashmap::find_next(_current_state_index, it);
                    continue;
                }
                return;
            }

            // Check the subname for the trigger if necessary
            if (has_trigger_subname)
            {
                if (trigger_subname != from_state.subname)
                {
                    it = ice::multi_hashmap::find_next(_current_state_index, it);
                    continue;
                }
            }

            // Check that this pending state was not added already.
            bool already_added = false;
            ice::queue::for_each(_pending_states, [&](ice::EngineStatePending const& pending) noexcept
                {
                    already_added |= pending.trigger.to == trigger.to && pending.current.subname == from_state.subname;
                }
            );

            // Don't add the same state twice
            if (already_added)
            {
                return;
            }

            ICE_LOG(LogSeverity::Verbose, LogTag::Engine,
                "Pending state change {} ({}):\n- from: {}\n- to: {}",
                from_state.graph.value, from_state.subname,
                trigger.from.value,
                trigger.to.value
            );

            ice::queue::push_back(
                _pending_states,
                EngineStatePending
                {
                    .trigger_shard = trigger_shard,
                    .trigger = trigger,
                    // TODO: Provide a default committer
                    .committer = *ice::hashmap::get(_state_committers, ice::hash(trigger.to.graph.value), nullptr),
                    .current = from_state
                }
            );

            it = ice::multi_hashmap::find_next(_current_state_index, it);
        }
    }

    bool EngineStateTracker_Default::collect_pending_states(
        ice::ShardContainer const& shards
    ) noexcept
    {
        for (ice::EngineStateTrigger const& trigger : _available_triggers)
        {
            ice::shards::for_each(
                shards,
                trigger.when,
                [&](ice::Shard shard) noexcept
                {
                    collect_pending_state(shard, trigger);
                }
            );
        }

        // Append additional pending states using 'before' triggers
        for (ice::EngineStateTrigger const& trigger : _available_triggers)
        {
            // Skip if not a before trigger
            if (trigger.before == Constant_InvalidState)
            {
                continue;
            }

            // Find the states for the possibly affected graphs
            auto it = ice::multi_hashmap::find_first(_current_state_index, ice::hash(trigger.from.graph.value));
            while (it != nullptr)
            {
                ice::EngineStateCurrent& from_state = _current_state[it.value()];
                ICE_ASSERT_CORE(from_state.graph == trigger.from.graph);

                ice::Shard trigger_shard = ice::Shard_Invalid;
                ice::queue::for_each(_pending_states, [&](ice::EngineStatePending const& pending) noexcept
                    {
                        // Not a valid 'before' trigger...
                        if (pending.trigger.to != trigger.before)
                        {
                            return;
                        }

                        // Only valid if the pending state is 'generic' or has the same 'subname'
                        if (pending.current.subname == ice::StringID_Invalid
                            || from_state.subname == pending.current.subname)
                        {
                            trigger_shard = pending.trigger_shard;
                        }
                    }
                );

                it = ice::multi_hashmap::find_next(_current_state_index, it);

                // If we have a trigger shard, then we can almost submit this trigger as pending
                if (trigger_shard == ice::Shard_Invalid)
                {
                    continue;
                }

                // Check that the graph state we checking is what we expect
                if (from_state.value != trigger.from.value)
                {
                    continue;
                }

                ICE_LOG(LogSeverity::Verbose, LogTag::Engine,
                    "Pending state change {} ({}):\n- from: {}\n- to: {}",
                    from_state.graph.value, from_state.subname,
                    trigger.from.value,
                    trigger.to.value
                );

                // Push to the front so it's before all the current pending states
                ice::queue::push_front(
                    _pending_states,
                    EngineStatePending
                    {
                        .trigger_shard = trigger_shard,
                        .trigger = trigger,
                        // TODO: Provide a default committer
                        .committer = *ice::hashmap::get(_state_committers, ice::hash(trigger.to.graph.value), nullptr),
                        .current = from_state
                    }
                );
            }
        }

        return ice::queue::any(_pending_states);
    }

    auto create_state_tracker(ice::Allocator& alloc) noexcept -> ice::UniquePtr<ice::EngineStateTracker>
    {
        return ice::make_unique<ice::EngineStateTracker_Default>(alloc, alloc);
    }

} // namespace ice
