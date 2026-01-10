/// Copyright 2023 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "iceshard_world_manager.hxx"
#include "iceshard_world_manager_devui.hxx"
#include "iceshard_world_context.hxx"
#include "iceshard_trait_context.hxx"

#include <ice/devui_context.hxx>
#include <ice/engine_shards.hxx>
#include <ice/engine_state_tracker.hxx>

namespace ice
{

    namespace detail
    {

        static constexpr ice::EngineStateTrigger StateTrigger_ActivateAfterCreate{
            .when = ice::ShardID_WorldActivate,
            .from = State_WorldCreated, .to = State_WorldActive,
            .results = ice::ShardID_WorldActivated
        };
        static constexpr ice::EngineStateTrigger StateTrigger_Activate{
            .when = ice::ShardID_WorldActivate,
            .from = State_WorldInactive, .to = State_WorldActive,
            .results = ice::ShardID_WorldActivated
        };
        static constexpr ice::EngineStateTrigger StateTrigger_Deactivate{
            .when = ice::ShardID_WorldDeactivate,
            .from = State_WorldActive, .to = State_WorldInactive,
            .results = ice::ShardID_WorldDeactivated
        };

        static constexpr ice::EngineStateTrigger StateTriggers_WorldState[]{
            StateTrigger_ActivateAfterCreate,
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

    IceshardWorldManager::IceshardWorldManager(
        ice::Allocator& alloc,
        ice::ecs::EntityStorage& entity_storage,
        ice::UniquePtr<ice::TraitArchive> trait_archive,
        ice::EngineStateTracker& tracker
    ) noexcept
        : _allocator{ alloc, "World Manager" }
        , _entity_storage{ entity_storage }
        , _trait_archive{ ice::move(trait_archive) }
        , _state_tracker{ tracker }
        , _worlds{ _allocator }
        , _pending_events{ _allocator }
        , _devui{  }
        , _devui_tasks{ }
    {
        ice::EngineStateRegisterParams state_params{
            .initial = ice::State_WorldCreated,
            .committer = this,
            .enable_subname_states = true
        };
        _state_tracker.register_graph(state_params, detail::StateTriggers_WorldState);

        if (ice::devui_available())
        {
            // _devui_tasks = ice::make_unique<ice::TraitTasksTrackerDevUI>(_allocator, _allocator);
            _devui = create_devui();
        }
   }

    IceshardWorldManager::~IceshardWorldManager() noexcept
    {
        ice::u32 active_worlds = 0;
        for (Entry const& entry : _worlds)
        {
            active_worlds += ice::u32(entry.is_active);
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

        ice::UniquePtr<ice::IceshardWorldContext> world_context = ice::make_unique<ice::IceshardWorldContext>(
            _allocator, _allocator, world_template.name
        );
        ice::Array<ice::UniquePtr<ice::IceshardTraitContext>, ice::ContainerLogic::Complex> world_traits{ _allocator };
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
                ice::UniquePtr<ice::IceshardTraitContext> trait_context = ice::make_unique<ice::IceshardTraitContext>(
                    world_context->allocator(), *world_context.get(), world_traits.size().u32()
                );
                ice::UniquePtr<ice::Trait> trait = desc->fn_factory(world_context->allocator(), *trait_context.get(), desc->fn_factory_userdata);
                if (trait != nullptr)
                {
                    trait_context->trait = ice::move(trait);
                    world_traits.push_back(ice::move(trait_context));
                }
            }
        }

        _state_tracker.register_subname(world_template.name);
        //_state_tracker.initialize_subname_state(ice::StateGraph_WorldState, world_template.name);

        IceshardWorld* world = world_context->create_world(
            world_template,
            _entity_storage,
            ice::move(world_traits),
            _devui_tasks.get()
        );

        Entry world_entry{ .context = ice::move(world_context), .world = world };

        // Add a new pending event
        ice::shards::push_back(
            _pending_events,
            ice::ShardID_WorldCreated | ice::stringid_hash(world_template.name)
        );

        return ice::hashmap::get_or_set(
            _worlds,
            ice::hash(world_template.name),
            ice::move(world_entry)
        ).world;
    }

    auto IceshardWorldManager::find_world(
        ice::StringID_Arg name
    ) noexcept -> World*
    {
        static Entry invalid_entry{ };
        return ice::hashmap::get(_worlds, ice::hash(name), invalid_entry).world;
    }

    void IceshardWorldManager::destroy_world(
        ice::StringID_Arg name
    ) noexcept
    {
        static Entry invalid_entry{ };
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
                out_worlds.push_back(entry.world->worldID);
            }
        }
    }

    void IceshardWorldManager::query_pending_events(ice::ShardContainer& out_events) noexcept
    {
        ice::shards::push_back(out_events, _pending_events._data);
        ice::shards::clear(_pending_events);
    }

    void IceshardWorldManager::pre_update(
        ice::ShardContainer& out_shards
    ) noexcept
    {
        for (Entry& world_entry : ice::hashmap::values(_worlds))
        {
            world_entry.context->close_checkpoints();
            world_entry.world->pre_update(out_shards);
        }
    }

    void IceshardWorldManager::update(
        ice::TaskContainer& out_tasks,
        ice::EngineParamsBase const& params,
        ice::Span<ice::Shard const> event_shards
    ) noexcept
    {
        for (Entry& world_entry : ice::hashmap::values(_worlds))
        {
            if (world_entry.is_active)
            {
                world_entry.world->task_launcher().gather(out_tasks, params, event_shards);
            }
        }
    }

    void IceshardWorldManager::update(
        ice::StringID_Arg world_name,
        ice::TaskContainer& out_tasks,
        ice::EngineParamsBase const& params,
        ice::Span<ice::Shard const> event_shards
    ) noexcept
    {
        Entry const* const entry = ice::hashmap::try_get(_worlds, ice::hash(world_name));
        if (entry != nullptr && entry->is_active)
        {
            entry->world->task_launcher().gather(out_tasks, params, event_shards);
        }
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
