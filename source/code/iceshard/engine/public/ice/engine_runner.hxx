/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/clock.hxx>
#include <ice/ecs/ecs_entity_index.hxx>
#include <ice/gfx/gfx_types.hxx>
#include <ice/input/input_types.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/platform_event.hxx>
#include <ice/render/render_declarations.hxx>
#include <ice/shard.hxx>
#include <ice/stringid.hxx>
#include <ice/task_scheduler.hxx>
#include <ice/task_types.hxx>
#include <ice/task_utils.hxx>
#include <ice/asset_types.hxx>

namespace ice
{

    using EngineFrameFactoryUserdata = void*;
    using EngineFrameFactory = auto(*)(
        ice::Allocator&, ice::EngineFrameData&, EngineFrameFactoryUserdata
    ) noexcept -> ice::UniquePtr<ice::EngineFrame>;

    struct WorldStateParams;

    //struct EngineTaskResult
    //{
    //    // TODO: if awaited resumes on last finished tasks thread
    //    ice::Span<ice::Task<>> tasks;
    //    ice::ManualResetBarrier barrier{ };

    //    ~EngineTaskResult() noexcept
    //    {
    //        barrier.wait();
    //    }

    //    auto operator co_await() & noexcept
    //    {
    //        struct Result
    //        {
    //            ice::Span<ice::Task<>> tasks;
    //            ice::ManualResetBarrier& barrier;

    //            auto await_ready() const noexcept { return ice::count(tasks) == 0; }
    //            void await_suspend(ice::coroutine_handle<>) noexcept
    //            {
    //                ice::manual_wait_for_all(tasks, barrier);
    //            }
    //            void await_resume() noexcept
    //            {
    //                // TODO: Change to is-set?
    //                ICE_ASSERT_CORE(barrier.is_set());
    //            }

    //        } result{ tasks, barrier };
    //        return result;
    //    }

    //    auto operator co_await() && noexcept
    //    {
    //        struct Result
    //        {
    //            ice::Span<ice::Task<>> tasks;
    //            ice::ManualResetBarrier& barrier;

    //            auto await_ready() const noexcept { return ice::count(tasks) == 0; }
    //            void await_suspend(ice::coroutine_handle<>) const noexcept
    //            {
    //                // todo: Pass the current coro to a method, which will call it after everything is finished.
    //                ice::manual_wait_for_all(tasks, barrier);
    //            }
    //            void await_resume() const noexcept
    //            {
    //                // TODO: Change to is-set?
    //                ICE_ASSERT_CORE(barrier.is_set());
    //            }

    //        } result{ ice::move(tasks), barrier };
    //        return result;
    //    }
    //};

    //struct EngineTaskRequest
    //{
    //    virtual ~EngineTaskRequest() noexcept = default;
    //    virtual auto gather_tasks(ice::Shard shard) noexcept -> ice::EngineTaskResult = 0;
    //};

    struct EngineFrameUpdate
    {
        ice::Clock const& clock;
        ice::AssetStorage& assets;
        ice::EngineFrame& frame;
        ice::EngineFrame const& last_frame;
        ice::EngineSchedulers thread;
    };

    struct EngineRunnerCreateInfo
    {
        ice::Engine& engine;
        ice::Clock const& clock;
        ice::u32 concurrent_frame_count = 2;
        ice::EngineFrameFactory frame_factory;
        ice::EngineFrameFactoryUserdata frame_factory_userdata;
        ice::EngineSchedulers schedulers;
    };

    struct EngineRunner
    {
        virtual ~EngineRunner() noexcept = default;

        virtual void update_states(
            ice::WorldStateTracker& state_tracker,
            ice::WorldStateParams const& update_params
        ) noexcept = 0;

        virtual auto aquire_frame() noexcept -> ice::Task<ice::UniquePtr<ice::EngineFrame>> = 0;
        virtual auto update_frame(ice::EngineFrame& current_frame, ice::EngineFrame const& last_frame) noexcept -> ice::Task<> = 0;
        virtual void release_frame(ice::UniquePtr<ice::EngineFrame> frame) noexcept = 0;
    };

    static constexpr ice::ShardID ShardID_FrameUpdate = "event/engine/frame-update`ice::EngineFrameUpdate const*"_shardid;

} // namespace ice

template<>
inline constexpr ice::ShardPayloadID ice::Constant_ShardPayloadID<ice::EngineFrameUpdate const*> = ice::shard_payloadid("ice::EngineFrameUpdate const*");
