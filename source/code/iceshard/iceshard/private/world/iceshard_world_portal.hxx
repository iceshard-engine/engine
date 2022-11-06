/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task.hxx>
#include <ice/sync_manual_events.hxx>
#include <ice/data_storage.hxx>
#include <ice/world/world_portal.hxx>
#include <ice/mem_allocator_ring.hxx>

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
            ice::ecs::EntityStorage& entity_storage,
            bool is_owning
        ) noexcept;

        ~IceshardWorldPortal() noexcept override;

        bool is_owning() const noexcept { return _is_owning; }

        auto world() const noexcept -> ice::World const& override;
        auto trait() noexcept -> ice::WorldTrait*;

        auto allocator() noexcept -> ice::Allocator& override;

        auto storage() noexcept -> ice::DataStorage& override;
        auto entity_storage() noexcept -> ice::ecs::EntityStorage& override;

        void execute(ice::Task<void> task) noexcept override;

        void remove_finished_tasks() noexcept;

    private:
        ice::Allocator& _allocator;
        ice::World const& _world;
        ice::WorldTrait* _trait;
        bool const _is_owning;

        ice::HashedDataStorage _storage;
        ice::ecs::EntityStorage& _entity_storage;

        ice::RingAllocator _wait_event_allocator;

        struct TraitTask
        {
            ice::ManualResetEvent* event;
            std::coroutine_handle<> coroutine;
        };
        ice::Array<TraitTask> _trait_tasks;
    };

} // namespace ice
