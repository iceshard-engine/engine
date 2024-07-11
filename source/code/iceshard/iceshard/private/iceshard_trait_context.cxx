#include "iceshard_trait_context.hxx"
#include <ice/container/hashmap.hxx>
#include <ice/engine_runner.hxx>

namespace ice
{

    IceshardWorldContext::IceshardWorldContext(ice::Allocator& alloc) noexcept
        : _always_reached_checkpoint{ true }
        , _checkpoints{ alloc }
        , _frame_handlers{ alloc }
        , _runner_handlers{ alloc }
    { }

    void IceshardWorldContext::close_checkpoints() noexcept
    {
        for (ice::TaskCheckpoint* checkpoint : _checkpoints)
        {
            checkpoint->close();
        }
    }

    IceshardTraitContext::IceshardTraitContext(
        ice::IceshardWorldContext& world_context,
        ice::u32 trait_idx
    ) noexcept
        : _trait_index{ trait_idx }
        , _world_context{ world_context }
    {
    }

    IceshardTraitContext::~IceshardTraitContext() noexcept
    {
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
                .event_handler = binding.procedure,
                .userdata = binding.procedure_userdata
            }
        );

        return ice::E_Fail;
    }

} // namespace ice
