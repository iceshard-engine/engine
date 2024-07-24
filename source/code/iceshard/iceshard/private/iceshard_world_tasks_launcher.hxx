/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include "iceshard_trait_context.hxx"
#include <ice/world/world_trait.hxx>

namespace ice
{

    static_assert(sizeof(IceshardEventHandler) <= 32);

    class IceshardTasksLauncher
    {
    public:
        IceshardTasksLauncher(
            ice::IceshardWorldContext& world_context,
            ice::Span<ice::UniquePtr<IceshardTraitContext>> traits
        ) noexcept;

        void gather(ice::TaskContainer& out_tasks, ice::Shard shard) noexcept;
        void gather(ice::TaskContainer& out_tasks, ice::Span<ice::Shard const> shards) noexcept;

        void execute(ice::Array<ice::Task<>, ice::ContainerLogic::Complex>& out_tasks, ice::Shard shard) noexcept;
        void execute(ice::Array<ice::Task<>, ice::ContainerLogic::Complex>& out_tasks, ice::ShardContainer const& shards) noexcept;

    private:
        ice::IceshardWorldContext& _world_context;
        ice::Span<ice::UniquePtr<IceshardTraitContext>> _traits;
    };

} // namespace ice
