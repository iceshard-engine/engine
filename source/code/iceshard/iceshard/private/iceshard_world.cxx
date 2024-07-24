/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "iceshard_world.hxx"

namespace ice
{

    IceshardWorld::IceshardWorld(
        ice::Allocator& alloc,
        ice::StringID_Arg worldid,
        ice::ecs::EntityStorage& entity_storage,
        ice::UniquePtr<ice::IceshardWorldContext> context,
        ice::Array<ice::UniquePtr<ice::IceshardTraitContext>> traits
    ) noexcept
        : worldID{ worldid }
        , _entity_storage{ entity_storage }
        , _entity_operations{ alloc, 16 }
        , _context{ ice::move(context) }
        , _traits{ ice::move(traits) }
        , _tasks_launcher{ *_context, ice::array::slice(_traits) }
    {
    }

    void IceshardWorld::pre_update(ice::ShardContainer& out_shards) noexcept
    {
        _entity_storage.execute_operations(_entity_operations, out_shards);
        _entity_operations.clear();

        _context->close_checkpoints();
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

} // namespace ice
