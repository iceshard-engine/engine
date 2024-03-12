/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "iceshard_world.hxx"
#include <ice/engine_frame.hxx>
#include <ice/engine_shards.hxx>
#include <ice/profiler.hxx>
#include <ice/mem_allocator_stack.hxx>
#include <ice/task_scoped_container.hxx>
#include <ice/assert.hxx>
#include <ice/sort.hxx>

namespace ice
{

    namespace detail
    {

        static constexpr ice::EngineStateTrigger StateTrigger_Activate{
            .when = ice::ShardID_WorldActivate,
            .from = State_WorldCreated, .to = State_WorldActive,
            .results = ice::ShardID_WorldActivated
        };
        static constexpr ice::EngineStateTrigger StateTrigger_Deactivate{
            .when = ice::ShardID_WorldDeactivate,
            .from = State_WorldActive, .to = State_WorldInactive,
            .results = ice::ShardID_WorldDeactivated
        };

        static constexpr ice::EngineStateTrigger StateTriggers_WorldState[]{
            StateTrigger_Activate,
            StateTrigger_Deactivate,
        };

        auto execute_and_notify(
            ice::ShardContainer& out_shards,
            ice::Task<> task,
            ice::Shard shard
        ) noexcept -> ice::Task<>
        {
            co_await task;
            ice::shards::push_back(out_shards, shard);
        }

    } // namespace detail

    IceshardTraitTaskLauncher::IceshardTraitTaskLauncher(
        ice::Trait* trait,
        ice::HashMap<ice::IceshardEventHandler>& frame_handlers,
        ice::HashMap<ice::IceshardEventHandler>& runner_handlers
    ) noexcept
        : _trait{ trait }
        , _frame_handlers{ frame_handlers }
        , _runner_handlers{ runner_handlers }
    {
    }

    void IceshardTraitTaskLauncher::bind(ice::TraitTaskBinding const& binding) noexcept
    {
        ice::ShardID const trigger_event = ice::value_or_default(
            binding.trigger_event, ice::ShardID_FrameUpdate
        );

        ice::multi_hashmap::insert(
            binding.task_type == TraitTaskType::Frame ? _frame_handlers : _runner_handlers,
            ice::hash(trigger_event),
            ice::IceshardEventHandler{
                .event_id = trigger_event,
                .trait = _trait,
                .event_handler = binding.procedure,
                .userdata = binding.procedure_userdata
            }
        );
    }

    IceshardTasksLauncher::IceshardTasksLauncher(ice::Allocator& alloc) noexcept
        : _frame_handlers{ alloc }
        , _runner_handlers{ alloc }
    {
    }

    void IceshardTasksLauncher::gather(
        ice::TaskContainer& task_container,
        ice::Shard shard
    ) noexcept
    {
        ice::Span<ice::Task<>> tasks = task_container.create_tasks(
            ice::multi_hashmap::count(_frame_handlers, ice::hash(shard.id)),
            shard.id
        );

        auto out_it = ice::begin(tasks);
        auto it = ice::multi_hashmap::find_first(_frame_handlers, ice::hash(shard.id));
        while (it != nullptr)
        {
            ice::IceshardEventHandler const& handler = it.value();

            *out_it = handler.event_handler(handler.trait, shard, handler.userdata);

            out_it += 1;
            it = ice::multi_hashmap::find_next(_frame_handlers, it);
        }
    }

    void IceshardTasksLauncher::gather(
        ice::TaskContainer& out_tasks,
        ice::Span<ice::Shard const> shards
    ) noexcept
    {
        IPT_ZONE_SCOPED;

        for (ice::Shard shard : shards)
        {
            this->gather(out_tasks, shard);
        }
    }

    void IceshardTasksLauncher::execute(
        ice::Array<ice::Task<>, ice::ContainerLogic::Complex>& out_tasks,
        ice::Shard shard
    ) noexcept
    {
        IPT_ZONE_SCOPED;

        auto it = ice::multi_hashmap::find_first(_frame_handlers, ice::hash(shard.id));
        while (it != nullptr)
        {
            ice::IceshardEventHandler const& handler = it.value();

            //ICE_ASSERT(ice::array::count(out_tasks) < ice::array::capacity(out_tasks), "Maximum number of tasks suppored by default launcher reached!");
            ice::array::push_back(
                out_tasks,
                handler.event_handler(handler.trait, shard, handler.userdata)
            );

            it = ice::multi_hashmap::find_next(_frame_handlers, it);
        }

    }

    void IceshardTasksLauncher::execute(
        ice::Array<ice::Task<>, ice::ContainerLogic::Complex>& out_tasks,
        ice::ShardContainer const& shards) noexcept
    {
        IPT_ZONE_SCOPED;

        // Not optimal, but for now sufficient
        for (ice::Shard shard : shards)
        {
            execute(out_tasks, shard);
        }
    }

    auto IceshardTasksLauncher::trait_launcher(ice::Trait* trait) noexcept -> ice::IceshardTraitTaskLauncher
    {
        return { trait, _frame_handlers, _runner_handlers };
    }

    IceshardWorld::IceshardWorld(
        ice::Allocator& alloc,
        ice::StringID_Arg worldid,
        ice::Array<ice::UniquePtr<ice::Trait>, ice::ContainerLogic::Complex> traits
    ) noexcept
        : worldID{ worldid }
        , _tasks_launcher{ alloc }
        , _traits{ ice::move(traits) }
    {
        for (auto& trait : _traits)
        {
            auto launcher = _tasks_launcher.trait_launcher(trait.get());
            trait->gather_tasks(launcher);
        }
    }

    auto IceshardWorld::activate(ice::WorldStateParams const& update) noexcept -> ice::Task<>
    {
        for (auto& trait : _traits)
        {
            co_await trait->activate(update);
        }
        co_return;
    }

    auto IceshardWorld::deactivate(ice::WorldStateParams const& update) noexcept -> ice::Task<>
    {
        for (auto& trait : _traits)
        {
            co_await trait->deactivate(update);
        }
        co_return;
    }

    IceshardWorldManager::IceshardWorldManager(
        ice::Allocator& alloc,
        ice::UniquePtr<ice::TraitArchive> trait_archive,
        ice::EngineStateTracker& tracker
    ) noexcept
        : _allocator{ alloc, "World Manager" }
        , _trait_archive{ ice::move(trait_archive) }
        , _state_tracker{ tracker }
        , _worlds{ _allocator }
        , _pending_events{ _allocator }
        , _state_flows{ _allocator }
        , _state_stages{ _allocator }
        , _pending_changes{ _allocator }
        , _flow_shards{ _allocator }
    {
        ice::array::push_back(_state_flows, FlowState{ .name = StringID_Invalid, .stage = ice::u8_max });

        ice::EngineStateRegisterParams state_params{
            .initial = ice::State_WorldCreated,
            .commiter = this,
            .enable_subname_states = true
        };
        _state_tracker.register_graph(state_params, detail::StateTriggers_WorldState);
    }

    IceshardWorldManager::~IceshardWorldManager() noexcept
    {
        ice::ucount active_worlds = 0;
        for (Entry const& entry : _worlds)
        {
            active_worlds += ice::ucount(entry.is_active);
        }

        ICE_ASSERT(
            active_worlds == 0,
            "WorlManager destroyed with {} worlds not deactivated!",
            active_worlds
        );
    }

    auto IceshardWorldManager::create_world(
        ice::WorldTemplate const& world_template
    ) noexcept -> World*
    {

        ICE_ASSERT(
            ice::hashmap::has(_worlds, ice::hash(world_template.name)) == false,
            "A world with this name {} was already created!",
            world_template.name
        );

        ice::Array<ice::UniquePtr<ice::Trait>, ice::ContainerLogic::Complex> world_traits{ _allocator };
        for (ice::StringID_Arg traitid : world_template.traits)
        {
            ice::TraitDescriptor const* desc = _trait_archive->trait(traitid);
            ICE_LOG_IF(
                desc == nullptr, LogSeverity::Warning, LogTag::Engine,
                "Unknonw trait {} when creating world template {}",
                traitid, world_template.name
            );
            if (desc != nullptr)
            {
                ice::UniquePtr<ice::Trait> trait = desc->fn_factory(_allocator, desc->fn_factory_userdata);
                if (trait != nullptr)
                {
                    ice::array::push_back(world_traits, ice::move(trait));
                }
            }
        }

        _state_tracker.register_subname(world_template.name);
        //_state_tracker.initialize_subname_state(ice::StateGraph_WorldState, world_template.name);

        Entry world_entry{
            .world = ice::make_unique<ice::IceshardWorld>(
                _allocator,
                _allocator,
                world_template.name,
                ice::move(world_traits)
            ),
            .states = Array<FlowState>{ _allocator }
        };
        ice::array::resize(world_entry.states, ice::array::count(_state_flows));

        // Add a new pending event
        ice::shards::push_back(
            _pending_events,
            ice::ShardID_WorldCreated | ice::stringid_hash(world_template.name)
        );

        return ice::hashmap::get_or_set(
            _worlds,
            ice::hash(world_template.name),
            ice::move(world_entry)
        ).world.get();
    }

    auto IceshardWorldManager::find_world(
        ice::StringID_Arg name
    ) noexcept -> World*
    {
        static StackAllocator<1_B> emptyAlloc;
        static Entry invalid_entry{ .states = Array<FlowState>{ emptyAlloc } };
        return ice::hashmap::get(_worlds, ice::hash(name), invalid_entry).world.get();
    }

    void IceshardWorldManager::destroy_world(
        ice::StringID_Arg name
    ) noexcept
    {
        static StackAllocator<1_B> emptyAlloc;
        static Entry invalid_entry{ .states = Array<FlowState>{ emptyAlloc } };
        ICE_ASSERT(
            ice::hashmap::get(_worlds, ice::hash(name), invalid_entry).is_active == false,
            "Trying to destroy active world: {}!",
            name
        );

        // Add a new pending event
        ice::shards::push_back(
            _pending_events,
            ice::ShardID_WorldDestroyed | ice::stringid_hash(name)
        );

        ice::hashmap::remove(_worlds, ice::hash(name));
    }

    void IceshardWorldManager::query_worlds(ice::Array<ice::StringID>& out_worlds) const noexcept
    {
        for (Entry const& entry : _worlds)
        {
            if (entry.is_active)
            {
                ice::array::push_back(out_worlds, entry.world->worldID);
            }
        }
    }

    void IceshardWorldManager::query_pending_events(ice::ShardContainer& out_events) noexcept
    {
        ice::shards::push_back(out_events, ice::array::slice(_pending_events._data));
        ice::shards::clear(_pending_events);
    }

    void IceshardWorldManager::update(
        ice::TaskContainer& out_tasks,
        ice::WorldUpdateParams const& params
    ) noexcept
    {
        for (Entry& world_entry : ice::hashmap::values(_worlds))
        {
            bool world_allowed = world_entry.is_active;
            if (world_allowed && params.required_state.flow_id != 0)
            {
                FlowState const& state = world_entry.states[params.required_state.flow_id];
                world_allowed = state.stage == params.required_state.flow_value;
            }

            if (world_allowed)
            {
                update_world(out_tasks, world_entry, params);
            }
        }
    }

    void IceshardWorldManager::update_world(
        ice::TaskContainer& out_tasks,
        Entry& entry,
        ice::WorldUpdateParams const& params
    ) noexcept
    {
        for (ice::Shard shard : params.request_shards)
        {
            entry.world->task_launcher().gather(out_tasks, shard);
        }
    }

    auto IceshardWorldManager::flowid(ice::StringID_Arg flow_name) const noexcept -> ice::u8
    {
        ice::ucount out_index = 0;
        bool const found = ice::search(
            ice::array::slice(_state_flows),
            flow_name,
            [](auto const& a, auto const& b) noexcept { return a.name == b; },
            out_index
        );
        return found ? static_cast<ice::u8>(found) : 0;
    }

    auto IceshardWorldManager::flow_stage(ice::u8 flowid) const noexcept -> ice::u8
    {
        return _state_flows[flowid].stage;
    }

    bool IceshardWorldManager::has_pending_changes() const noexcept
    {
        return ice::queue::any(_pending_changes);
    }

    auto IceshardWorldManager::register_flow(ice::WorldStateFlow flow) noexcept -> ice::u8
    {
        ICE_ASSERT(
            ice::array::count(_state_flows) < ice::u8_max,
            "Can't register more than {} state flows!",
            ice::u8_max
        );

        auto const comp = [](
            FlowState const& lhs,
            ice::WorldStateFlow const& rhs
        ) noexcept
        {
            return lhs.name == rhs.name;
        };

        ice::ucount index;
        if (ice::search(ice::array::slice(_state_flows), flow, comp, index) == false)
        {
            index = ice::array::count(_state_flows);
            ice::array::push_back(_state_flows, { .name = flow.name, .stage = flow.initial_state });

            ice::u8 const flow_id = static_cast<ice::u8>(index);
            for (ice::WorldStateStage stage : flow.stages)
            {
                // We don't allow changing states of other flows.

                ICE_ASSERT_CORE(stage.dependent_state.flow_id < ice::array::count(_state_flows));
                ICE_ASSERT_CORE(stage.blocked_state.flow_id < ice::array::count(_state_flows));
                //ICE_ASSERT_CORE(stage.resulting_state.flow_id < ice::array::count(_state_flows));

                // Save the final stage.
                ice::array::push_back(_state_stages, FlowStage{ .stage = stage, .flowid = flow_id });
            }

            ice::array::push_back(
                _flow_shards,
                FlowStateShards{
                    .userdata = flow.userdata,
                    .fn = flow.fn_shards,
                    .flowid = flow_id,
                }
            );
        }
        else
        {
            ICE_LOG(
                LogSeverity::Warning, LogTag::Engine,
                "A state flow with name '{}' already exists, new flow was not registered!",
                flow.name
            );
        }

        return static_cast<ice::u8>(index);
    }

    void IceshardWorldManager::process_state_events(ice::ShardContainer const& shards) noexcept
    {
        IPT_ZONE_SCOPED;

        // Remove all changes that where processed
        while (ice::queue::any(_pending_changes) && ice::queue::front(_pending_changes).trigger == Shard_Invalid)
        {
            ice::queue::pop_front(_pending_changes, 1);
        }

        for (FlowStage const& stage : _state_stages)
        {
            ice::shards::for_each(
                shards,
                stage.stage.trigger,
                [this, &stage](ice::Shard shard) noexcept
                {
                    ice::StringID_Hash worldid;
                    if (ice::shard_inspect(shard, worldid))
                    {
                        Entry* const entry = ice::hashmap::try_get(
                            _worlds, ice::hash(worldid)
                        );

                        if (entry == nullptr)
                        {
                            return;
                        }

                        // Check for the required state value
                        ice::WorldStateStage const& fs = stage.stage;
                        if (entry != nullptr)
                        {
                            bool const required_dependent_found = fs.dependent_state.flow_id == 0
                                || (entry->states[fs.dependent_state.flow_id].stage == fs.dependent_state.flow_value);

                            // Early exit
                            if (required_dependent_found == false)
                            {
                                return;
                            }

                            bool const required_state_found = entry->states[stage.flowid].stage == fs.required_state;
                            bool required_state_later = false;

                            auto const fn_handle_pending = [&](PendingStateChange const& state) noexcept
                            {
                                // Skip invalid pending changes
                                if (state.trigger == ice::Shard_Invalid)
                                {
                                    return;
                                }

                                if (state.entry->world.get() != entry->world.get())
                                {
                                    return;
                                }

                                // Don't apply same state twice
                                if (state.stage->stage.resulting_state == fs.resulting_state
                                    && state.stage->flowid == stage.flowid)
                                {
                                    required_state_later = false;
                                    return;
                                }

                                // We check if any of the pending states already added result in the state we need.
                                required_state_later =
                                    state.stage->stage.resulting_state == fs.required_state
                                    && state.stage->flowid == stage.flowid;
                            };

                            ice::queue::for_each_reverse(_pending_changes, fn_handle_pending);

                            if (required_state_found || required_state_later)
                            {
                                ice::ShardID const event_shard = ice::value_or_default(
                                    fs.event_shard, fs.trigger
                                );

                                //ICE_LOG(
                                //    LogSeverity::Info, LogTag::Engine,
                                //    "[{}] Pending state-change on {} ({}, {}) -> ({}, {}) => {}",
                                //    entry->world->worldID,
                                //    ice::hash(fs.trigger),
                                //    _state_flows[stage.flowid].name, fs.required_state,
                                //    _state_flows[stage.flowid].name, fs.resulting_state,
                                //    ice::hash(event_shard)
                                //);

                                ice::queue::push_back(
                                    _pending_changes,
                                    PendingStateChange{
                                        .trigger = event_shard,
                                        .entry = entry,
                                        .stage = &stage
                                    }
                                );
                            }
                        }
                    }
                }
            );
        }

        // TODO: Check if there is anything pending
        if constexpr (true)
        {
            static ice::StackAllocator_1024 event_shards_alloc{};
            event_shards_alloc.reset();

            ice::ShardContainer event_shards{ event_shards_alloc };
            for (FlowStateShards const& flow_shards : _flow_shards)
            {
                // TODO: Dont ask for non-pending shards
                flow_shards.fn(flow_shards.userdata, event_shards);
            }

            ice::ScopedTaskContainer tasks{ event_shards_alloc };
            this->finalize_state_changes(tasks, event_shards._data);
        }
    }

    namespace detail
    {

        void sort(ice::Queue<IceshardWorldManager::PendingStateChange>& states) noexcept
        {
            using PendingStateChange = IceshardWorldManager::PendingStateChange;

            // We know that the queue is partially sorted, so we want to only sort one time to get everything in line with
            //   blocking requirements.
            ice::ucount const queue_count = ice::queue::count(states);
            for (ice::u32 idx = 0; idx < queue_count;)
            {
                // ... find a state that is blocking our current state
                bool is_blocked = false;
                ice::u32 block_idx = idx + 1;
                for (; block_idx < queue_count && !is_blocked; ++block_idx)
                {
                    PendingStateChange& blocking = states[block_idx];
                    if (blocking.trigger != ice::Shard_Invalid
                        && blocking.entry->world.get() == states[idx].entry->world.get()
                        && blocking.stage->stage.blocked_state.flow_id != 0
                        && blocking.stage->stage.blocked_state.flow_id == states[idx].stage->flowid)
                    {
                        is_blocked = true;
                        break;
                    }
                }

                // If no blocks are found we finish.
                if (is_blocked == false)
                {
                    break;
                }

                // ... move the blocked state forward until we arrive after the blocking state
                ice::u32 exchange_idx = idx;
                ice::u32 exchange_count = 1;
                for (; is_blocked && exchange_idx < block_idx; ++exchange_idx)
                {
                    PendingStateChange& exchange = states[exchange_idx];
                    PendingStateChange& next = states[exchange_idx + exchange_count];

                    auto const swap_many = [&](ice::u32 count) noexcept
                    {
                        while(count > 0)
                        {
                            ice::swap(states[exchange_idx + (count - 1)], states[exchange_idx + count]);
                        }
                    };

                    // Swap if the next state does not affect our state
                    if (next.trigger == ice::Shard_Invalid
                        || next.entry->world.get() != exchange.entry->world.get()
                        || next.stage->stage.dependent_state.flow_id == 0
                        || next.stage->stage.dependent_state.flow_id != exchange.stage->flowid)
                    {
                        ice::swap(exchange, next);
                    }
                    else
                    {
                        exchange_count += 1;
                    }
                }
            }
        }
    }

    void IceshardWorldManager::finalize_state_changes(
        ice::TaskContainer& out_tasks,
        ice::Span<ice::Shard const> shards
    ) noexcept
    {
        IPT_ZONE_SCOPED;

        detail::sort(_pending_changes);

        ice::queue::for_each(
            _pending_changes,
            [&, this](PendingStateChange& pending) noexcept
            {
                for (ice::Shard shard : shards)
                {
                    if (pending.trigger == shard.id)
                    {
                        pending.trigger = ice::Shard_Invalid;
                        if (shard.id == ice::ShardID_WorldActivate) // Special case for now
                        {
                            ice::WorldStateParams const* update = ice::shard_shatter<ice::WorldStateParams const*>(shard, nullptr);
                            ICE_ASSERT_CORE(update != nullptr);
                            out_tasks.create_tasks(1, shard.id)[0] = pending.entry->world->activate(*update);
                            pending.entry->is_active = true;
                        }
                        else if (shard.id == ice::ShardID_WorldDeactivate) // Special case for now
                        {
                            ice::WorldStateParams const* update = ice::shard_shatter<ice::WorldStateParams const*>(shard, nullptr);
                            ICE_ASSERT_CORE(update != nullptr);
                            out_tasks.create_tasks(1, shard.id)[0] = pending.entry->world->deactivate(*update);
                            pending.entry->is_active = false;
                        }
                        else if (pending.entry->is_active)
                        {
                            pending.entry->world->task_launcher().gather(out_tasks, shard);
                        }

                        //ICE_LOG(
                        //    LogSeverity::Info, LogTag::Engine,
                        //    "[{}] Finalized state-change on {} ({}, {}) -> ({}, {})",
                        //    pending.entry->world->worldID,
                        //    ice::hash(pending.trigger),
                        //    _state_flows[pending.stage->flowid].name, pending.stage->stage.required_state,
                        //    _state_flows[pending.stage->flowid].name, pending.stage->stage.resulting_state
                        //);

                        // Apply the new state value
                        FlowState& state = pending.entry->states[pending.stage->flowid];

                        // Call stage on end
                        //if (state.on_end) state.on_end(state.userdata);
                        //state.on_begin = pending.stage->stage.stage_begin;
                        //state.on_frame = pending.stage->stage.stage_frame;
                        //state.on_end = pending.stage->stage.stage_end;

                        state.stage = pending.stage->stage.resulting_state;
                        //if (state.on_begin) state.on_begin(state.userdata);

                        // If notification is valid, add it to pending events
                        if (pending.stage->stage.notification != ice::Shard_Invalid)
                        {
                            ice::shards::push_back(
                                _pending_events,
                                pending.stage->stage.notification | ice::stringid_hash(pending.entry->world->worldID)
                            );
                        }
                    }
                }
            }
        );

        // Additional checks in non-release builds
        if constexpr ((ice::build::is_release || ice::build::is_profile) == false)
        {
            ice::queue::for_each(_pending_changes, [](auto const& pending) noexcept
                {
                    ICE_ASSERT_CORE(pending.trigger == ice::Shard_Invalid);
                }
            );
        }

        ice::queue::clear(_pending_changes);
    }

    bool IceshardWorldManager::commit(
        ice::EngineStateTrigger const& trigger,
        ice::Shard trigger_shard,
        ice::ShardContainer& out_shards
    ) noexcept
    {
        ice::StringID_Hash world_name;
        if (ice::shard_inspect(trigger_shard, world_name))
        {
            Entry* const entry = ice::hashmap::try_get(_worlds, ice::hash(world_name));
            ICE_ASSERT_CORE(entry != nullptr);

            // Activated
            entry->is_active = trigger.to == ice::State_WorldActive;

            if (trigger.results != ice::Shard_Invalid)
            {
                ice::shards::push_back(out_shards, trigger.results | world_name);
            }
        }

        return true;
    }

} // namespace ice
