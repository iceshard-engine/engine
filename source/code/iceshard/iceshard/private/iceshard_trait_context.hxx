#pragma once
#include <ice/mem_allocator.hxx>
#include <ice/world/world_trait_context.hxx>

namespace ice
{

    struct IceshardEventHandler
    {
        ice::ShardID event_id;
        ice::Trait* trait;
        ice::TraitIndirectTaskFn event_handler;
        void* userdata;
    };

    class IceshardTraitContext : public ice::detail::TraitContextImpl
    {
    public:
        IceshardTraitContext(ice::Allocator& alloc) noexcept;
        ~IceshardTraitContext() noexcept;

        auto checkpoint(ice::StringID id) noexcept -> ice::TaskCheckpointGate override;
        bool register_checkpoint(ice::StringID id, ice::TaskCheckpoint& checkpoint) noexcept override;
        void close_checkpoints() noexcept;

        auto bind(ice::Trait* trait, ice::TraitTaskBinding const& binding) noexcept -> ice::Result override;

        ice::TaskCheckpoint _always_reached_checkpoint;
        ice::HashMap<ice::TaskCheckpoint*> _checkpoints;

        ice::HashMap<ice::IceshardEventHandler> _frame_handlers;
        ice::HashMap<ice::IceshardEventHandler> _runner_handlers;
    };

} // namespace ice
