#pragma once
#include <ice/gfx/gfx_runner.hxx>
#include <ice/task_thread.hxx>
#include <ice/world/world.hxx>

#include <ice/memory/proxy_allocator.hxx>
#include <ice/memory/scratch_allocator.hxx>

namespace ice::gfx
{

    class IceGfxWorld;
    class IceGfxDevice;
    class IceGfxFrame;

    class IceGfxRunner : public ice::gfx::GfxRunner
    {
    public:
        IceGfxRunner(
            ice::Allocator& alloc,
            ice::UniquePtr<ice::gfx::IceGfxDevice> device,
            ice::UniquePtr<ice::gfx::IceGfxWorld> world
        ) noexcept;
        ~IceGfxRunner() noexcept override;

        void add_trait(ice::gfx::GfxTrait* trait) noexcept;

        void set_event(ice::ManualResetEvent* event) noexcept;

        void draw_frame(
            ice::EngineFrame const& engine_frame
        ) noexcept override;

        auto device() noexcept -> ice::gfx::GfxDevice& override;
        auto frame() noexcept -> ice::gfx::GfxFrame& override;

    protected:
        auto task_frame(
            ice::EngineFrame const& engine_frame,
            ice::UniquePtr<ice::gfx::IceGfxFrame> frame
        ) noexcept -> ice::Task<>;

    private:
        ice::memory::ProxyAllocator _allocator;
        ice::UniquePtr<ice::TaskThread> _thread;
        ice::UniquePtr<ice::gfx::IceGfxDevice> _device;
        //ice::UniquePtr<ice::gfx::IceGfxWorld> _world;

        ice::memory::ScratchAllocator _frame_allocator[2];
        ice::u32 _next_free_allocator;

        ice::UniquePtr<ice::gfx::IceGfxFrame> _current_frame;

        ice::ManualResetEvent _mre_internal;
        ice::ManualResetEvent* _mre_selected;
    };

} // namespace ice::gfx
