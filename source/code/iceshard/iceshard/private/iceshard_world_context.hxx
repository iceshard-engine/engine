#pragma once
#include <ice/container/hashmap.hxx>
#include <ice/task_checkpoint.hxx>
#include <ice/world/world_assembly.hxx>
#include <ice/world/world_trait_types.hxx>

#include "iceshard_trait_context.hxx"

namespace ice
{

    class IceshardWorld;

    struct IceshardEventHandler
    {
        ice::ShardID event_id;
        ice::u32 trait_idx;
        ice::TraitIndirectTaskFn procedure;
        void* procedure_userdata;
    };

    class IceshardWorldContext
    {
    public:
        IceshardWorldContext(ice::Allocator& alloc, ice::StringID_Arg worldid) noexcept;

        auto allocator() noexcept -> ice::Allocator& { return _allocator; }
        auto world() noexcept -> ice::IceshardWorld& { return *_world; }

        auto create_world(
            ice::WorldTemplate const& world_template,
            ice::ecs::EntityIndex& entitites,
            ice::Array<ice::UniquePtr<ice::IceshardTraitContext>> traits,
            ice::detail::TraitTaskTracker* task_tracker
        ) noexcept -> ice::IceshardWorld*;

        void close_checkpoints() noexcept;

    private:
        ice::ProxyAllocator _allocator;
        ice::UniquePtr<ice::IceshardWorld> _world;

    public:
        ice::TaskCheckpoint _always_reached_checkpoint;
        ice::HashMap<ice::TaskCheckpoint*> _checkpoints;
        ice::HashMap<ice::IceshardEventHandler> _frame_handlers;
        ice::HashMap<ice::IceshardEventHandler> _runner_handlers;
    };

} // namespace ice
