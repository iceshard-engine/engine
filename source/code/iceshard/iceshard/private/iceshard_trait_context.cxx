#include "iceshard_trait_context.hxx"
#include <ice/container/hashmap.hxx>
#include <ice/engine_runner.hxx>

namespace ice
{

    IceshardTraitContext::IceshardTraitContext(ice::Allocator& alloc) noexcept
        : _always_reached_checkpoint{ }
        , _checkpoints{ alloc }
        , _frame_handlers{ alloc }
        , _runner_handlers{ alloc }
    {
    }

    IceshardTraitContext::~IceshardTraitContext() noexcept
    {
    }

    auto IceshardTraitContext::checkpoint(ice::StringID id) noexcept -> ice::TaskCheckpointGate
    {
        if (ice::TaskCheckpoint* const checkpoint = ice::hashmap::get(_checkpoints, ice::hash(id), nullptr); checkpoint != nullptr)
        {
            return checkpoint->opened();
        }
        return _always_reached_checkpoint.opened();
    }

    bool IceshardTraitContext::register_checkpoint(ice::StringID id, ice::TaskCheckpoint& checkpoint) noexcept
    {
        if (ice::hashmap::has(_checkpoints, ice::hash(id)))
        {
            return false;
        }

        ice::hashmap::set(_checkpoints, ice::hash(id), ice::addressof(checkpoint));
        return true;
    }

    void IceshardTraitContext::close_checkpoints() noexcept
    {
        for (ice::TaskCheckpoint* checkpoint : _checkpoints)
        {
            checkpoint->close();
        }
    }

    auto IceshardTraitContext::bind(ice::Trait* trait, ice::TraitTaskBinding const& binding) noexcept -> ice::Result
    {
        ice::ShardID const trigger_event = ice::value_or_default(
            binding.trigger_event, ice::ShardID_FrameUpdate
        );

        ice::multi_hashmap::insert(
            binding.task_type == TraitTaskType::Frame ? _frame_handlers : _runner_handlers,
            ice::hash(trigger_event),
            ice::IceshardEventHandler{
                .event_id = trigger_event,
                .trait = trait,
                .event_handler = binding.procedure,
                .userdata = binding.procedure_userdata
            }
        );

        return ice::E_Fail;
    }

} // namespace ice
