/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/clock.hxx>
#include <ice/stringid.hxx>
#include <ice/task_types.hxx>
#include <ice/task_scheduler.hxx>

#include <ice/input/input_types.hxx>
#include <ice/gfx/gfx_types.hxx>
#include <ice/ecs/ecs_entity_index.hxx>
#include <ice/platform_event.hxx>


namespace ice
{

    struct ShardContainer;

    class AssetStorage;
    class TaskThreadPool;

    class World;

    class EngineFrame;
    class EngineRunner;

    enum class EngineContext : ice::u32
    {
        EngineRunner,
        LogicFrame,
        GraphicsFrame [[deprecated]],
    };

    class EngineRunner
    {
    public:
        virtual ~EngineRunner() noexcept = default;

        virtual auto clock() const noexcept -> ice::Clock const& = 0;

        virtual auto entity_index() const noexcept -> ice::ecs::EntityIndex& = 0;

        virtual auto input_tracker() noexcept -> ice::input::InputTracker& = 0;
        virtual void process_device_queue(
            ice::input::DeviceEventQueue const& device_queue
        ) noexcept = 0;

        virtual auto task_scheduler() noexcept -> ice::TaskScheduler & = 0;

        virtual auto asset_storage() noexcept -> ice::AssetStorage& = 0;

        [[deprecated]]
        virtual auto graphics_device() noexcept -> ice::gfx::GfxDevice& = 0;

        [[deprecated]]
        virtual auto graphics_frame() noexcept -> ice::gfx::GfxFrame& = 0;

        virtual auto previous_frame() const noexcept -> ice::EngineFrame const& = 0;
        virtual auto current_frame() const noexcept -> ice::EngineFrame const& = 0;
        virtual auto current_frame() noexcept -> ice::EngineFrame& = 0;
        virtual void next_frame(ice::ShardContainer const& shards) noexcept = 0;

        virtual void execute_task(ice::Task<> task, ice::EngineContext context) noexcept = 0;

        virtual auto stage_current_frame() noexcept -> ice::TaskStage<ice::EngineFrame> = 0;
        virtual auto stage_next_frame() noexcept -> ice::TaskStage<ice::EngineFrame> = 0;
    };

} // namespace ice
