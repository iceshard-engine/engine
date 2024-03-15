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
        ) noexcept override
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

                EngineStateCurrent initial_state{ params.initial.graph };
                initial_state.value = params.initial.value;
                initial_state.subname = ice::StringID_Invalid;

                ice::multi_hashmap::insert(
                    _current_state_index,
                    ice::hash(params.initial.graph.value),
                    ice::count(_current_state)
                );
                ice::array::push_back(_current_state, initial_state);
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

                    EngineStateCurrent initial_state{ params.initial.graph };
                    initial_state.value = params.initial.value;
                    initial_state.subname = subname;

                    ice::multi_hashmap::insert(
                        _current_state_index,
                        ice::hash(params.initial.graph.value),
                        ice::count(_current_state)
                    );
                    ice::array::push_back(_current_state, initial_state);
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
                params.commiter
            );

            ice::array::push_back(
                _available_triggers,
                triggers
            );

            return true;
        }

        bool register_subname(
            ice::StringID_Arg subname
        ) noexcept override
        {
            for (ice::EngineState initial_state : _initial_states)
            {
                // Already has a state
                if (current_state(initial_state.graph, subname) != Constant_InvalidState)
                {
                    ICE_LOG(LogSeverity::Warning, LogTag::Engine, "Unexpected graph state {} for subname: {}", initial_state.graph.value, subname);
                    continue;
                }

                EngineStateCurrent engine_state{ initial_state.graph };
                engine_state.value = initial_state.value;
                engine_state.subname = subname;

                ice::multi_hashmap::insert(
                    _current_state_index,
                    ice::hash(engine_state.graph.value),
                    ice::count(_current_state)
                );
                ice::array::push_back(_current_state, engine_state);
            }
            return true;
        }

#if 0
        bool initialize_subname_state(
            ice::EngineStateGraph graph,
            ice::StringID subname
        ) noexcept override
        {
            ice::u64 const hash = ice::hash(graph.value);
            if (ice::hashmap::has(_state_committers, hash) == false)
            {
                return false;
            }

            // State already initialized
            if (current_state(graph, subname) != Constant_InvalidState)
            {
                return false;
            }

            ice::EngineState const graph_initial_state = ice::hashmap::get(_initial_states, hash, Constant_InvalidState);
            EngineStateCurrent initial_state{ graph };
            initial_state.value = graph_initial_state.value;
            initial_state.subname = subname;

            ice::multi_hashmap::insert(_current_state_index, hash, ice::count(_current_state));
            ice::array::push_back(_current_state, initial_state);
            return true;
        }
#endif

        auto current_state_internal(
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

        auto current_state(
            ice::EngineStateGraph state_graph
        ) const noexcept -> ice::EngineState override
        {
            return current_state(state_graph, ice::StringID_Invalid);
        }

        auto current_state(
            ice::EngineStateGraph state_graph,
            ice::StringID_Arg subname
        ) const noexcept -> ice::EngineState override
        {
            return current_state_internal(state_graph, ice::StringID_Invalid);
        }

        auto update_states(
            ice::ShardContainer const& shards,
            ice::ShardContainer& out_shards
        ) noexcept -> ice::ucount override
        {
            ice::StackAllocator<512_B> temp_alloc;
            ice::ShardContainer temp_shards{ temp_alloc };
            ice::array::reserve(
                temp_shards._data,
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
                if (collect_pending_states(_pending_states, *input_shards))
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
                    // Clear temporary shards so we can escape the loop normally.
                    ice::shards::clear(temp_shards);
                }

                // Set input shards to temp_shards
                input_shards = &temp_shards;

                // If we have any shards added, check if we can collect another set of states.
            } while (ice::shards::empty(temp_shards) == false);

            return true;
        }

        void collect_pending_state(
            ice::Shard trigger_shard,
            ice::EngineStateTrigger const& trigger,
            ice::Queue<ice::EngineStatePending>& out_pending
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
                ice::queue::for_each(out_pending, [&](ice::EngineStatePending const& pending) noexcept
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
                    out_pending,
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

        bool collect_pending_states(
            ice::Queue<ice::EngineStatePending>& out_pending,
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
                        collect_pending_state(shard, trigger, out_pending);
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
                    ice::queue::for_each(_pending_states, [&, this](ice::EngineStatePending const& pending) noexcept
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
                        out_pending,
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

            return ice::queue::any(out_pending);
        }

    private:
        ice::Array<ice::EngineStateTrigger> _available_triggers;
        ice::HashMap<ice::EngineState> _initial_states;
        ice::HashMap<ice::EngineStateCommitter*> _state_committers;
        ice::HashMap<ice::u32> _current_state_index;
        ice::Array<ice::EngineStateCurrent> _current_state;
        ice::Queue<ice::EngineStatePending> _pending_states;

    private:
        // New fields
        ice::Array<ice::StringID> _subnames;
    };

} // namespace ice
