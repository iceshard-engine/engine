/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/world/world_trait.hxx>

namespace ice
{

    struct IceshardEventHandler
    {
        ice::ShardID event_id;
        ice::Trait* trait;
        ice::TraitIndirectTaskFn event_handler;
        void* userdata;
    };

    static_assert(sizeof(IceshardEventHandler) == 32);

    class IceshardTraitTaskLauncher final : public ice::TraitTaskRegistry
    {
    public:
        IceshardTraitTaskLauncher(
            ice::Trait* trait,
            ice::HashMap<ice::IceshardEventHandler>& frame_handlers,
            ice::HashMap<ice::IceshardEventHandler>& runner_handlers
        ) noexcept;

        void bind(ice::TraitTaskBinding const& binding) noexcept override;

    private:
        ice::Trait* _trait;
        ice::HashMap<ice::IceshardEventHandler>& _frame_handlers;
        ice::HashMap<ice::IceshardEventHandler>& _runner_handlers;
    };

    class IceshardTasksLauncher
    {
    public:
        IceshardTasksLauncher(ice::Allocator& alloc) noexcept;

        void gather(ice::TaskContainer& out_tasks, ice::Shard shard) noexcept;
        void gather(ice::TaskContainer& out_tasks, ice::Span<ice::Shard const> shards) noexcept;

        void execute(ice::Array<ice::Task<>, ice::ContainerLogic::Complex>& out_tasks, ice::Shard shard) noexcept;
        void execute(ice::Array<ice::Task<>, ice::ContainerLogic::Complex>& out_tasks, ice::ShardContainer const& shards) noexcept;

        auto trait_launcher(ice::Trait* trait) noexcept -> ice::IceshardTraitTaskLauncher;

    private:
        ice::HashMap<IceshardEventHandler> _frame_handlers;
        ice::HashMap<IceshardEventHandler> _runner_handlers;
    };

} // namespace ice
