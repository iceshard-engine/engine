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
            ice::UniquePtr<ice::gfx::IceGfxWorld> world
        ) noexcept;
        ~IceGfxRunner() noexcept override;

        auto trait_count() const noexcept -> ice::u32 override;

        void query_traits(
            ice::Span<ice::StringID> out_trait_names,
            ice::Span<ice::gfx::GfxTrait*> out_traits
        ) const noexcept override;

        void add_trait(
            ice::StringID_Arg name,
            ice::gfx::GfxTrait* trait
        ) noexcept override;

        auto get_graphics_world() noexcept -> ice::StringID override;
        void set_graphics_world(
            ice::StringID_Arg world_name
        ) noexcept override;

        void setup_graphics_world(ice::World* gfx_world) noexcept;
        void teardown_graphics_world(ice::World* gfx_world) noexcept;

        void set_event(ice::ManualResetEvent* event) noexcept override;

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

        ice::memory::ScratchAllocator _frame_allocator[2];
        ice::u32 _next_free_allocator;

        ice::UniquePtr<ice::gfx::IceGfxFrame> _current_frame;

        ice::StringID _gfx_world_name;
        ice::pod::Array<ice::gfx::IceGfxTraitEntry> _traits;

        ice::ManualResetEvent _mre_internal;
        ice::ManualResetEvent* _mre_selected;
    };

} // namespace ice::gfx
