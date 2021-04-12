#pragma once
#include <ice/engine_runner.hxx>

#include <ice/render/render_driver.hxx>
#include <ice/render/render_device.hxx>
#include <ice/render/render_swapchain.hxx>
#include <ice/render/render_framebuffer.hxx>
#include <ice/render/render_pass.hxx>

#include <ice/memory/proxy_allocator.hxx>
#include <ice/memory/scratch_allocator.hxx>
#include <ice/unique_ptr.hxx>

#include "world/iceshard_world_tracker.hxx"

#include "gfx/iceshard_gfx_device.hxx"
#include "gfx/iceshard_gfx_frame.hxx"

namespace ice
{

    class IceshardWorldManager;
    class IceshardWorldTracker;

    class IceshardMemoryFrame;

    class IceshardEngineRunner final : public ice::EngineRunner
    {
    public:
        IceshardEngineRunner(
            ice::Allocator& alloc,
            ice::IceshardWorldManager& world_manager,
            ice::UniquePtr<ice::gfx::IceGfxDevice> gfx_device
        ) noexcept;
        ~IceshardEngineRunner() noexcept override;

        auto clock() const noexcept -> ice::Clock const& override;

        auto graphics_device() noexcept -> ice::gfx::GfxDevice& override;
        auto graphics_frame() noexcept -> ice::gfx::GfxFrame& override;

        auto previous_frame() const noexcept -> EngineFrame const& override;
        auto current_frame() const noexcept -> EngineFrame const& override;
        auto current_frame() noexcept -> EngineFrame& override;
        void next_frame() noexcept override;

    protected:
        void activate_worlds() noexcept;
        void deactivate_worlds() noexcept;

    private:
        ice::Allocator& _allocator;
        ice::SystemClock _clock;

        ice::memory::ProxyAllocator _frame_allocator;
        ice::memory::ScratchAllocator _frame_data_allocator[2];
        ice::u32 _next_free_allocator = 0;

        ice::UniquePtr<ice::IceshardMemoryFrame> _previous_frame;
        ice::UniquePtr<ice::IceshardMemoryFrame> _current_frame;

        ice::IceshardWorldManager& _world_manager;
        ice::IceshardWorldTracker _world_tracker;

        ice::UniquePtr<ice::gfx::IceGfxDevice> _gfx_device;
        ice::UniquePtr<ice::gfx::IceGfxBaseFrame> _gfx_current_frame;
    };

} // namespace ice
