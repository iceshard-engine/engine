/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "iceshard_world_context.hxx"
#include "iceshard_world.hxx"

namespace ice
{

    IceshardWorldContext::IceshardWorldContext(ice::Allocator& alloc, ice::StringID_Arg worldid) noexcept
        : _allocator{ alloc, ice::stringid_hint(worldid) }
        , _always_reached_checkpoint{ true }
        , _checkpoints{ alloc }
        , _frame_handlers{
            ice::HashMap<ice::IceshardEventHandler>{ alloc },
            ice::HashMap<ice::IceshardEventHandler>{ alloc },
            ice::HashMap<ice::IceshardEventHandler>{ alloc }
        }
        , _runner_handlers{ alloc }
    {
    }

    auto IceshardWorldContext::create_world(
        ice::WorldTemplate const& world_template,
        ice::ecs::EntityStorage& entity_storage,
        ice::Array<ice::UniquePtr<ice::IceshardTraitContext>> traits,
        ice::detail::TraitTaskTracker* task_tracker
    ) noexcept -> ice::IceshardWorld*
    {
        _world = ice::make_unique<ice::IceshardWorld>(
            _allocator,
            _allocator,
            world_template.name,
            entity_storage.entities(),
            entity_storage,
            *this,
            ice::move(traits),
            task_tracker
        );
        return _world.get();
    }

    void IceshardWorldContext::close_checkpoints() noexcept
    {
        for (ice::TaskCheckpoint* checkpoint : _checkpoints)
        {
            checkpoint->close();
        }
    }

} // namespace ice

