/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/clock.hxx>
#include <ice/stringid.hxx>
#include <ice/input/input_types.hxx>
#include <ice/gfx/gfx_types.hxx>
#include <ice/gfx/gfx_operations.hxx>
#include <ice/task_operations.hxx>
#include <ice/engine_task_operations.hxx>
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

    struct CurrentFrameOperationData : ice::EngineTaskOperationBaseData { };

    struct NextFrameOperationData
    {
        std::coroutine_handle<> coroutine = nullptr;
        ice::NextFrameOperationData* next = nullptr;
        ice::EngineFrame* frame;
    };

    using CurrentFrameOperation = ice::EngineTaskOperation<ice::EngineRunner, ice::CurrentFrameOperationData>;
    class NextFrameOperation : public ice::EngineTaskOperation<ice::EngineRunner, ice::NextFrameOperationData>
    {
    public:
        using EngineTaskOperation::EngineTaskOperation;

        inline auto await_resume() const noexcept -> ice::EngineFrame& { return *_data.frame; }
        inline auto await_suspend(std::coroutine_handle<> coro) noexcept;
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

        virtual auto thread_pool() noexcept -> ice::TaskThreadPool& = 0;

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

        virtual auto schedule_current_frame() noexcept -> ice::CurrentFrameOperation = 0;
        virtual auto schedule_next_frame() noexcept -> ice::NextFrameOperation = 0;

    protected:
        friend CurrentFrameOperation;
        friend NextFrameOperation;

        virtual void schedule_internal(
            ice::CurrentFrameOperationData& operation
        ) noexcept = 0;

        virtual void schedule_internal(
            ice::NextFrameOperationData& operation
        ) noexcept = 0;
    };

    inline auto NextFrameOperation::await_suspend(std::coroutine_handle<> coro) noexcept
    {
        _data.coroutine = coro;
        _target.schedule_internal(_data);
    }

} // namespace ice
