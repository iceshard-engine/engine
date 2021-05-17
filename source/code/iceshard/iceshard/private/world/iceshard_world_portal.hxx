#pragma once
#include <ice/task.hxx>
#include <ice/sync_manual_events.hxx>
#include <ice/data_storage.hxx>
#include <ice/world/world_portal.hxx>
#include <ice/memory/scratch_allocator.hxx>

namespace ice
{

    class WorldTrait;

    class IceshardWorldPortal : public ice::WorldPortal
    {
    public:
        IceshardWorldPortal(
            ice::Allocator& alloc,
            ice::World const& world,
            ice::WorldTrait* trait,
            ice::EntityStorage& entity_storage
        ) noexcept;

        ~IceshardWorldPortal() noexcept override;

        auto world() const noexcept -> ice::World const& override;
        auto trait() noexcept -> ice::WorldTrait*;

        auto allocator() noexcept -> ice::Allocator& override;

        auto storage() noexcept -> ice::DataStorage& override;
        auto entity_storage() noexcept -> ice::EntityStorage& override;

        void execute(ice::Task<void> task) noexcept override;

        void remove_finished_tasks() noexcept;

    private:
        ice::Allocator& _allocator;
        ice::World const& _world;
        ice::WorldTrait* _trait;

        ice::HashedDataStorage _storage;
        ice::EntityStorage& _entity_storage;

        ice::memory::ScratchAllocator _wait_event_allocator;

        struct TraitTask
        {
            ice::ManualResetEvent* event;
            std::coroutine_handle<> coroutine;
        };
        ice::pod::Array<TraitTask> _trait_tasks;
    };

} // namespace ice
