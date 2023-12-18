/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "iceshard_world.hxx"
#include <ice/engine_frame.hxx>
#include <ice/engine_shards.hxx>
#include <ice/profiler.hxx>
#include <ice/assert.hxx>
#include <ice/mem_allocator_stack.hxx>

namespace ice
{

    namespace detail
    {

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

    IceshardTraitTaskLauncher::IceshardTraitTaskLauncher(ice::Trait* trait, ice::HashMap<ice::IceshardEventHandler>& handlers) noexcept
        : _trait{ trait }
        , _handlers{ handlers }
    {
    }

    void IceshardTraitTaskLauncher::bind(ice::TraitIndirectTaskFn func, ice::ShardID event) noexcept
    {
        ice::multi_hashmap::insert(
            _handlers,
            ice::hash(event),
            ice::IceshardEventHandler{
                .event_id = event,
                .trait = _trait,
                .event_handler = func
            }
        );
    }

    IceshardTasksLauncher::IceshardTasksLauncher(ice::Allocator& alloc) noexcept
        : _handlers{ alloc }
        //, _barrier{ }
    {
    }

    void IceshardTasksLauncher::execute(
        ice::Array<ice::Task<>, ice::ContainerLogic::Complex>& out_tasks,
        ice::Shard shard
    ) noexcept
    {
        IPT_ZONE_SCOPED;

        auto it = ice::multi_hashmap::find_first(_handlers, ice::hash(shard.id));
        while (it != nullptr)
        {
            ice::IceshardEventHandler const& handler = it.value();

            //ICE_ASSERT(ice::array::count(out_tasks) < ice::array::capacity(out_tasks), "Maximum number of tasks suppored by default launcher reached!");
            ice::array::push_back(
                out_tasks,
                handler.event_handler(handler.trait, shard, handler.userdata)
            );

            it = ice::multi_hashmap::find_next(_handlers, it);
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

    //bool IceshardTasksLauncher::finished() noexcept
    //{
    //    return _barrier.is_set();
    //}

    auto IceshardTasksLauncher::trait_launcher(ice::Trait* trait) noexcept -> ice::IceshardTraitTaskLauncher
    {
        return { trait, _handlers };
    }

    IceshardWorld::IceshardWorld(
        ice::Allocator& alloc,
        ice::Array<ice::UniquePtr<ice::Trait>, ice::ContainerLogic::Complex> traits
    ) noexcept
        : _tasks_launcher{ alloc }
        , _traits{ ice::move(traits) }
    {
        for (auto& trait : _traits)
        {
            auto launcher = _tasks_launcher.trait_launcher(trait.get());
            trait->gather_tasks(launcher);
        }
    }

    auto IceshardWorld::activate(ice::EngineWorldUpdate const& update) noexcept -> ice::Task<>
    {
        for (auto& trait : _traits)
        {
            co_await trait->activate(update);
        }
        co_return;
    }

    auto IceshardWorld::deactivate(ice::EngineWorldUpdate const& update) noexcept -> ice::Task<>
    {
        for (auto& trait : _traits)
        {
            co_await trait->deactivate(update);
        }
        co_return;
    }

    IceshardWorldManager::IceshardWorldManager(
        ice::Allocator& alloc,
        ice::UniquePtr<ice::TraitArchive> trait_archive
    ) noexcept
        : _allocator{ alloc }
        , _trait_archive{ ice::move(trait_archive) }
        , _worlds{ alloc }
    {
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

        Entry world_entry{
            .world = ice::make_unique<ice::IceshardWorld>(_allocator, _allocator, ice::move(world_traits)),
            .is_active = world_template.is_initially_active
        };

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
        static Entry invalid_entry{ };
        return ice::hashmap::get(_worlds, ice::hash(name), invalid_entry).world.get();
    }

    void IceshardWorldManager::destroy_world(
        ice::StringID_Arg name
    ) noexcept
    {
        ice::hashmap::remove(_worlds, ice::hash(name));
    }

    void IceshardWorldManager::activate(
        ice::StringID_Arg world_name,
        ice::EngineFrame& frame,
        ice::EngineWorldUpdate const& world_update,
        ice::Array<ice::Task<>, ContainerLogic::Complex>& out_tasks
    ) noexcept
    {
        Entry* const entry = ice::hashmap::try_get(_worlds, ice::hash(world_name));
        ICE_LOG_IF(
            entry == nullptr, LogSeverity::Warning, LogTag::Engine,
            "Trying to activate unknown world {}",
            world_name
        );
        if (entry != nullptr)
        {
            if (std::exchange(entry->is_active, true) == false)
            {
                ice::array::push_back(out_tasks,
                    detail::execute_and_notify(
                        frame.shards(),
                        entry->world->activate(world_update),
                        ice::ShardID_WorldActivated | ice::stringid_hash(world_name)
                    )
                );
            }
        }
    }

    void IceshardWorldManager::deactivate(
        ice::StringID_Arg world_name,
        ice::EngineFrame& frame,
        ice::EngineWorldUpdate const& world_update,
        ice::Array<ice::Task<>, ContainerLogic::Complex>& out_tasks
    ) noexcept
    {
        Entry* const entry = ice::hashmap::try_get(_worlds, ice::hash(world_name));
        ICE_LOG_IF(
            entry == nullptr, LogSeverity::Warning, LogTag::Engine,
            "Trying to deactivate unknown world {}",
            world_name
        );
        if (entry != nullptr)
        {
            if (std::exchange(entry->is_active, false))
            {
                ice::array::push_back(out_tasks,
                    detail::execute_and_notify(
                        frame.shards(),
                        entry->world->deactivate(world_update),
                        ice::ShardID_WorldDeactivated | ice::stringid_hash(world_name)
                    )
                );
            }
        }
    }

    void IceshardWorldManager::update(
        ice::EngineFrame& frame,
        ice::EngineWorldUpdate const& world_update,
        ice::Array<ice::Task<>, ContainerLogic::Complex>& out_tasks
    ) noexcept
    {
        static ice::StackAllocator<256_B> temp_alloc;
        temp_alloc.reset();

        ice::ShardContainer new_shards{ temp_alloc };
        ice::array::reserve(new_shards._data, ice::mem_max_capacity<ice::Shard>(temp_alloc.Constant_InternalCapacity));

        for (ice::Shard const shard : frame.shards())
        {
            ice::StringID const worldid = { ice::shard_shatter(shard, ice::StringID_Invalid.value) };

            switch (shard.id.name.value)
            {
            case ice::ShardID_WorldActivate.name.value:
                activate(worldid, frame, world_update, out_tasks);
                break;
            case ice::ShardID_WorldDeactivate.name.value:
                deactivate(worldid, frame, world_update, out_tasks);
                break;
            default:
                break;
            }
        }

        ice::shards::push_back(frame.shards(), new_shards._data);
    }

    void IceshardWorldManager::force_update(
        ice::StringID_Arg world_name,
        ice::Shard shard,
        ice::Array<ice::Task<>, ContainerLogic::Complex>& out_tasks
    ) noexcept
    {
        Entry* const entry = ice::hashmap::try_get(_worlds, ice::hash(world_name));
        ICE_LOG_IF(
            entry == nullptr, LogSeverity::Warning, LogTag::Engine,
            "Trying to update unknown world {}",
            world_name
        );

        if (entry != nullptr)// && entry->is_active)
        {
            entry->world->task_launcher().execute(out_tasks, shard);
        }
    }

    void IceshardWorldManager::update(
        ice::Shard shard,
        ice::Array<ice::Task<>, ContainerLogic::Complex>& out_tasks
    ) noexcept
    {
        IPT_ZONE_SCOPED;
        for (auto const& entry : _worlds)
        {
            if (entry.is_active)
            {
                entry.world->task_launcher().execute(out_tasks, shard);
            }
        }
    }

    void IceshardWorldManager::update(
        ice::ShardContainer const& shards,
        ice::Array<ice::Task<>, ContainerLogic::Complex>& out_tasks
    ) noexcept
    {
        IPT_ZONE_SCOPED;
        for (auto const& entry : _worlds)
        {
            if (entry.is_active)
            {
                entry.world->task_launcher().execute(out_tasks, shards);
            }
        }
    }

} // namespace ice::v2
