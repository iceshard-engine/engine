/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "iceshard_world.hxx"
#include "iceshard_world_tasks_devui.hxx"

#include <ice/engine_frame.hxx>
#include <ice/engine_shards.hxx>
#include <ice/engine_runner.hxx>
#include <ice/profiler.hxx>
#include <ice/mem_allocator_stack.hxx>
#include <ice/task_scoped_container.hxx>
#include <ice/assert.hxx>
#include <ice/sort.hxx>

namespace ice
{

    IceshardTasksLauncher::IceshardTasksLauncher(
        ice::IceshardWorldContext& world_context,
        ice::Span<ice::UniquePtr<IceshardTraitContext>> traits,
        ice::detail::TraitTaskTracker* task_tracker
    ) noexcept
        : _world_context{ world_context }
        , _traits{ traits }
        , _task_tracker{ task_tracker }
    {
    }

    void IceshardTasksLauncher::gather(
        ice::TaskContainer& task_container,
        ice::Shard shard
    ) noexcept
    {
        ice::Span<ice::Task<>> tasks = task_container.create_tasks(
            ice::multi_hashmap::count(_world_context._frame_handlers, ice::hash(shard.id)),
            shard.id
        );

        auto out_it = ice::begin(tasks);
        auto it = ice::multi_hashmap::find_first(_world_context._frame_handlers, ice::hash(shard.id));
        while (it != nullptr)
        {
            ice::IceshardEventHandler const& handler = it.value();

            *out_it = handler.event_handler(_traits[handler.trait_idx]->trait.get(), shard, _task_tracker);

            out_it += 1;
            it = ice::multi_hashmap::find_next(_world_context._frame_handlers, it);
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

        auto it = ice::multi_hashmap::find_first(_world_context._frame_handlers, ice::hash(shard.id));
        while (it != nullptr)
        {
            ice::IceshardEventHandler const& handler = it.value();

            //ICE_ASSERT(ice::array::count(out_tasks) < ice::array::capacity(out_tasks), "Maximum number of tasks suppored by default launcher reached!");
            ice::array::push_back(
                out_tasks,
                handler.event_handler(_traits[handler.trait_idx]->trait.get(), shard, _task_tracker)
            );

            it = ice::multi_hashmap::find_next(_world_context._frame_handlers, it);
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

} // namespace ice
