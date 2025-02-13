/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "iceshard_world.hxx"
#include "iceshard_world_devui.hxx"

namespace ice
{

    IceshardWorld::IceshardWorld(
        ice::Allocator& alloc,
        ice::StringID_Arg worldid,
        ice::ecs::EntityIndex& entities,
        ice::ecs::EntityStorage& entity_storage,
        ice::IceshardWorldContext& context,
        ice::Array<ice::UniquePtr<ice::IceshardTraitContext>> traits,
        ice::detail::TraitTaskTracker* task_tracker
    ) noexcept
        : worldID{ worldid }
        , _allocator{ alloc, "world-data" }
        , _entities{ entities }
        , _entity_storage{ entity_storage }
        , _entity_operations{ _allocator, 16 }
        , _traits{ ice::move(traits) }
        , _tasks_launcher{ context, ice::array::slice(_traits), task_tracker }
        , _devui{ create_devui(_allocator, context) }
    {
    }

    void IceshardWorld::pre_update(ice::ShardContainer& out_shards) noexcept
    {
        _entity_storage.execute_operations(_entity_operations, out_shards);
        _entity_operations.clear();

        // Sync trait events
        for (ice::UniquePtr<ice::IceshardTraitContext>& trait : _traits)
        {
            trait->sync(out_shards);
        }

        if (_devui != nullptr && _devui->world_operation != ice::Shard_Invalid)
        {
            ice::shards::push_back(out_shards, ice::exchange(_devui->world_operation, ice::Shard_Invalid));
        }
    }

    auto IceshardWorld::activate(ice::WorldStateParams const& update) noexcept -> ice::Task<>
    {
        for (auto& trait_ctx : _traits)
        {
            co_await trait_ctx->trait->activate(update);
        }
        co_return;
    }

    auto IceshardWorld::deactivate(ice::WorldStateParams const& update) noexcept -> ice::Task<>
    {
        for (auto& trait_ctx : _traits)
        {
            co_await trait_ctx->trait->deactivate(update);
        }
        co_return;
    }

    auto IceshardWorld::devui() noexcept -> DevUI&
    {
        ICE_ASSERT_CORE(_devui != nullptr);
        return *_devui;
    }

} // namespace ice
