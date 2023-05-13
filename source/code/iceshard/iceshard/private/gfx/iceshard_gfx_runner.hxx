/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/gfx/gfx_runner.hxx>
#include <ice/gfx/gfx_trait.hxx>
#include <ice/gfx/gfx_context.hxx>

#include <ice/world/world.hxx>
#include <ice/world/world_assembly.hxx>
#include <ice/task_scheduler.hxx>
#include <ice/task_queue.hxx>
#include <ice/task.hxx>

#include <ice/container/hashmap.hxx>
#include <ice/mem_allocator_proxy.hxx>
#include <ice/mem_allocator_ring.hxx>

#include "iceshard_gfx_runner_trait.hxx"
#include "iceshard_gfx_context.hxx"

#include "../world/iceshard_world.hxx"

namespace ice::gfx
{

    class IceGfxWorld;
    class IceGfxDevice;
    class IceGfxFrame;

    struct IceGfxTraitEntry
    {
        ice::StringID name;
        ice::gfx::GfxTrait* trait;
    };

    class IceGfxRunner : public ice::gfx::GfxRunner
    {
    public:
        IceGfxRunner(
            ice::Allocator& alloc,
            ice::UniquePtr<ice::gfx::IceGfxDevice> device,
            ice::IceshardWorld* graphics_world
        ) noexcept;
        ~IceGfxRunner() noexcept override;

        auto aquire_world() noexcept -> ice::World* override;
        void release_world(ice::World* world) noexcept override;

        void set_event(ice::ManualResetEvent* event) noexcept override;

        void draw_frame(
            ice::EngineFrame const& engine_frame
        ) noexcept override;

        auto device() noexcept -> ice::gfx::GfxDevice& override;
        auto frame() noexcept -> ice::gfx::GfxFrame& override;

        void setup_traits() noexcept;
        void cleanup_traits() noexcept;

    protected:
        auto task_cleanup_gfx_contexts() noexcept -> ice::Task<>;

        auto task_setup_gfx_traits() noexcept -> ice::Task<>;
        auto task_cleanup_gfx_traits() noexcept -> ice::Task<>;

        // GitHub Issue: #108
        void cleanup_gfx_contexts() noexcept;
        void setup_gfx_traits() noexcept;
        void cleanup_gfx_traits() noexcept;

    private:
        auto task_frame(
            ice::EngineFrame const& engine_frame,
            ice::UniquePtr<ice::gfx::IceGfxFrame> frame
        ) noexcept -> ice::Task<>;

        auto get_or_create_context(
            ice::StringID_Arg context_name,
            ice::gfx::GfxPass const& gfx_pass
        ) noexcept -> ice::gfx::IceGfxContext*;

    private:
        ice::ProxyAllocator _allocator;

        ice::TaskQueue _task_queue;
        ice::TaskScheduler _task_scheduler;
        ice::UniquePtr<ice::TaskThread> _thread;
        ice::UniquePtr<ice::gfx::IceGfxDevice> _device;

        ice::render::RenderFence* _fences[4];

        ice::RingAllocator _frame_allocator[2];
        ice::u32 _next_free_allocator;

        ice::UniquePtr<ice::gfx::IceGfxFrame> _current_frame;

        ice::IceshardWorld* _graphics_world[2];
        ice::gfx::IceGfxRunnerTrait _runner_trait;
        ice::HashMap<ice::gfx::IceGfxContext*> _contexts;

        ice::ManualResetEvent _mre_internal;
        ice::ManualResetEvent* _mre_selected;
    };

} // namespace ice::gfx
