/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "iceshard_world.hxx"
#include "iceshard_trait_context.hxx"

#include <ice/world/world_trait.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/engine_runner.hxx>

namespace ice
{

    IceshardTraitContext::IceshardTraitContext(
        ice::IceshardWorldContext& world_context,
        ice::u32 trait_idx
    ) noexcept
        : _trait_index{ trait_idx }
        , _world_context{ world_context }
        , _interface_selector{ nullptr }
        , _events{ world_context.allocator() }
        , _events_expired{ world_context.allocator() }
    {
    }

    IceshardTraitContext::~IceshardTraitContext() noexcept
    {
    }

    auto IceshardTraitContext::world() noexcept -> ice::World&
    {
        return _world_context.world();
    }

    void IceshardTraitContext::send(ice::detail::TraitEvent event) noexcept
    {
        ice::array::push_back(_events, event);
    }

    void IceshardTraitContext::sync(ice::ShardContainer& out_shards) noexcept
    {
        // Cleanup all expired events first
        for (ice::detail::TraitEvent const& ev : _events_expired)
        {
            if (ev.fn_on_expire != nullptr)
            {
                ev.fn_on_expire(ev.ud_on_expire, ev);
            }
        }

        // Copy all current events into the _expired events list.
        //   We use copy+clean so we don't allocate one of the arrays every time.
        _events_expired = _events;
        ice::array::clear(_events);

        // Push shards into the out container. (ice::detail::TraitEvent decays into ice::Shard)
        for (ice::Shard shard : _events_expired)
        {
            ice::shards::push_back(out_shards, shard);
        }
    }

    void IceshardTraitContext::register_interface_selector(ice::InterfaceSelector* selector) noexcept
    {
        _interface_selector = selector;
    }

    auto IceshardTraitContext::checkpoint(ice::StringID id) noexcept -> ice::TaskCheckpointGate
    {
        ice::TaskCheckpoint* const checkpoint = ice::hashmap::get(_world_context._checkpoints, ice::hash(id), nullptr);
        if (checkpoint != nullptr)
        {
            return checkpoint->checkpoint_gate();
        }
        return _world_context._always_reached_checkpoint.checkpoint_gate();
    }

    bool IceshardTraitContext::register_checkpoint(ice::StringID id, ice::TaskCheckpoint& checkpoint) noexcept
    {
        if (ice::hashmap::has(_world_context._checkpoints, ice::hash(id)))
        {
            return false;
        }

        ice::hashmap::set(_world_context._checkpoints, ice::hash(id), ice::addressof(checkpoint));
        return true;
    }

    void IceshardTraitContext::unregister_checkpoint(ice::StringID id, ice::TaskCheckpoint& checkpoint) noexcept
    {
        ice::TaskCheckpoint* const checkpoint_ptr = ice::hashmap::get(_world_context._checkpoints, ice::hash(id), nullptr);
        if (checkpoint_ptr == ice::addressof(checkpoint))
        {
            ice::hashmap::remove(_world_context._checkpoints, ice::hash(id));
        }
    }

    auto IceshardTraitContext::bind(ice::TraitTaskBinding const& binding) noexcept -> ice::Result
    {
        ice::ShardID const trigger_event = ice::value_or_default(
            binding.trigger_event, ice::ShardID_FrameUpdate
        );

        ice::multi_hashmap::insert(
            binding.task_type == TraitTaskType::Frame ? _world_context._frame_handlers : _world_context._runner_handlers,
            ice::hash(trigger_event),
            ice::IceshardEventHandler{
                .event_id = trigger_event,
                .trait_idx = _trait_index,
                .procedure = binding.procedure,
                .procedure_userdata = binding.procedure_userdata
            }
        );

        return ice::E_Fail;
    }

    auto IceshardTraitContext::query_interface(ice::StringID_Arg id) noexcept -> ice::Expected<void*>
    {
        if (_interface_selector != nullptr)
        {
            return _interface_selector->query_interface(id);
        }
        return ice::E_Fail;
    }

} // namespace ice
