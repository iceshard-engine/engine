#pragma once
#include <ice/mem_allocator.hxx>
#include <ice/world/world_trait_context.hxx>

namespace ice
{

    struct IceshardEventHandler
    {
        ice::ShardID event_id;
        ice::u32 trait_idx;
        ice::TraitIndirectTaskFn event_handler;
        void* userdata;
    };

    struct IceshardWorldContext
    {
        IceshardWorldContext(ice::Allocator& alloc) noexcept;

        void close_checkpoints() noexcept;

        ice::TaskCheckpoint _always_reached_checkpoint;
        ice::HashMap<ice::TaskCheckpoint*> _checkpoints;
        ice::HashMap<ice::IceshardEventHandler> _frame_handlers;
        ice::HashMap<ice::IceshardEventHandler> _runner_handlers;
    };

    class IceshardTraitContext : public ice::TraitContext
    {
    public:
        ice::UniquePtr<ice::Trait> trait;

    public:
        IceshardTraitContext(ice::IceshardWorldContext& world_context, ice::u32 index) noexcept;
        ~IceshardTraitContext() noexcept;

        auto checkpoint(ice::StringID id) noexcept -> ice::TaskCheckpointGate override;
        bool register_checkpoint(ice::StringID id, ice::TaskCheckpoint& checkpoint) noexcept override;
        void unregister_checkpoint(ice::StringID id, ice::TaskCheckpoint& checkpoint) noexcept override;

        auto bind(ice::TraitTaskBinding const& binding) noexcept -> ice::Result override;

    private:
        ice::u32 const _trait_index;
        ice::IceshardWorldContext& _world_context;
    };

} // namespace ice
