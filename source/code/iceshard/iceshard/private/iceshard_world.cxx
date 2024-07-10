/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "iceshard_world.hxx"

namespace ice
{

    IceshardWorld::IceshardWorld(
        ice::Allocator& alloc,
        ice::StringID_Arg worldid,
        ice::ecs::EntityStorage& entity_storage,
        ice::UniquePtr<ice::IceshardTraitContext> context,
        ice::Array<ice::UniquePtr<ice::Trait>, ice::ContainerLogic::Complex> traits
    ) noexcept
        : worldID{ worldid }
        , _tasks_launcher{ context->_frame_handlers, context->_runner_handlers }
        , _entity_storage{ entity_storage }
        , _entity_operations{ alloc, 16 }
        , _context{ ice::move(context) }
        , _traits{ ice::move(traits) }
    {
        for (auto& trait : _traits)
        {
            auto launcher = _tasks_launcher.trait_launcher(trait.get());
            trait->gather_tasks(launcher);
        }
    }

    void IceshardWorld::pre_update(ice::ShardContainer& out_shards) noexcept
    {
        _entity_storage.execute_operations(_entity_operations, out_shards);
        _entity_operations.clear();

        _context->close_checkpoints();
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

} // namespace ice
