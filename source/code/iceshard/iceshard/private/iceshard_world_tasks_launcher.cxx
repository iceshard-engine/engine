/// Copyright 2023 - 2026, Dandielo <dandielo@iceshard.net>
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
        ice::EngineParamsBase const& params,
        ice::Shard shard
    ) noexcept
    {
        IPT_ZONE_SCOPED;
        ICE_ASSERT_CORE(params.task_type > TraitTaskType::Invalid);
        ICE_ASSERT_CORE(params.task_type <= TraitTaskType::Render);

        ice::u32 const tasktype_idx = ice::u32(params.task_type) - 1;
        ice::MultiHashMap<ice::IceshardEventHandler>& handlers = _world_context._frame_handlers[tasktype_idx];

        ice::Span<ice::Task<>> tasks = task_container.create_tasks(
            handlers.count_values(shard.id).u32(),
            shard.id
        );

        auto out_it = tasks.begin();
        auto it = handlers.find_values(shard.id);
        while (it.valid())
        {
            ice::IceshardEventHandler const& handler = it.value();
            void* const userdata = handler.procedure_userdata != nullptr
                ? handler.procedure_userdata
                : _traits[handler.trait_idx]->trait.get();

            *out_it = handler.procedure(userdata, params, shard);

            out_it += 1;
            it.next();
        }
    }

    void IceshardTasksLauncher::gather(
        ice::TaskContainer& out_tasks,
        ice::EngineParamsBase const& params,
        ice::Span<ice::Shard const> shards
    ) noexcept
    {
        IPT_ZONE_SCOPED;

        for (ice::Shard shard : shards)
        {
            this->gather(out_tasks, params, shard);
        }
    }

    void IceshardTasksLauncher::execute(
        ice::Array<ice::Task<>, ice::ContainerLogic::Complex>& out_tasks,
        ice::EngineParamsBase const& params,
        ice::Shard shard
    ) noexcept
    {
        IPT_ZONE_SCOPED;
        ICE_ASSERT_CORE(params.task_type > TraitTaskType::Invalid);
        ICE_ASSERT_CORE(params.task_type <= TraitTaskType::Render);

        ice::u32 const tasktype_idx = ice::u32(params.task_type) - 1;
        ice::MultiHashMap<ice::IceshardEventHandler>& handlers = _world_context._frame_handlers[tasktype_idx];

        auto it = handlers.find_values(shard.id);
        while (it.valid())
        {
            ice::IceshardEventHandler const& handler = it.value();

            void* const userdata = handler.procedure_userdata != nullptr
                ? handler.procedure_userdata
                : _traits[handler.trait_idx]->trait.get();

            //ICE_ASSERT(ice::array::count(out_tasks) < ice::array::capacity(out_tasks), "Maximum number of tasks suppored by default launcher reached!");
            out_tasks.push_back(
                handler.procedure(userdata, params, shard)
            );

            it.next();
        }

    }

    void IceshardTasksLauncher::execute(
        ice::Array<ice::Task<>,
        ice::ContainerLogic::Complex>& out_tasks,
        ice::EngineParamsBase const& params,
        ice::ShardContainer const& shards
    ) noexcept
    {
        IPT_ZONE_SCOPED;

        // Not optimal, but for now sufficient
        for (ice::Shard shard : shards)
        {
            execute(out_tasks, params, shard);
        }
    }

} // namespace ice
