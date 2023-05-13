/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/engine.hxx>
#include <ice/asset_storage.hxx>
#include <ice/input/input_types.hxx>
#include <ice/mem_allocator_proxy.hxx>
#include <ice/ecs/ecs_entity_index.hxx>
#include <ice/task_scheduler.hxx>

#include "world/iceshard_world_manager.hxx"

namespace ice
{

    class EngineDevUI;
    class WorldTraitArchive;

    struct EngineCreateInfo;

    class IceshardEngine final : public ice::Engine
    {
    public:
        IceshardEngine(
            ice::Allocator& alloc,
            ice::EngineCreateInfo const& create_info
        ) noexcept;
        ~IceshardEngine() noexcept override;

        auto create_runner(
            ice::UniquePtr<ice::input::InputTracker> input_tracker,
            ice::UniquePtr<ice::gfx::GfxRunner> graphics_runner
        ) noexcept -> ice::UniquePtr<ice::EngineRunner> override;

        auto create_graphics_runner(
            ice::render::RenderDriver& render_driver,
            ice::render::RenderSurface& render_surface,
            ice::WorldTemplate const& render_world_template,
            ice::Span<ice::RenderQueueDefinition const> render_queues
        ) noexcept -> ice::UniquePtr<ice::gfx::GfxRunner> override;

        void update_runner_graphics(
            ice::EngineRunner& runner,
            ice::UniquePtr<ice::gfx::GfxRunner> graphics_runner
        ) noexcept override;

        auto entity_index() noexcept -> ice::ecs::EntityIndex& override;
        auto asset_storage() noexcept -> ice::AssetStorage& override;
        auto world_manager() noexcept -> ice::WorldManager& override;
        auto world_trait_archive() const noexcept -> ice::WorldTraitArchive const& override;
        auto developer_ui() noexcept -> ice::EngineDevUI& override;

        auto backing_allocator() noexcept -> ice::Allocator& { return _allocator.backing_allocator(); }

    private:
        ice::ProxyAllocator _allocator;
        ice::TaskScheduler _task_scheduler;
        ice::AssetStorage& _asset_storage;
        ice::WorldTraitArchive const& _trait_archive;

        ice::ecs::EntityIndex _entity_index;
        ice::IceshardWorldManager _world_manager;
        ice::EngineDevUI* const _devui;
    };

} // namespace ice
