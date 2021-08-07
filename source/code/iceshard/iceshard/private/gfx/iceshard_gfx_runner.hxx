#pragma once
#include <ice/gfx/gfx_runner.hxx>
#include <ice/gfx/gfx_trait.hxx>
#include <ice/gfx/gfx_context.hxx>

#include <ice/task_thread.hxx>
#include <ice/world/world.hxx>

#include <ice/pod/hash.hxx>
#include <ice/memory/proxy_allocator.hxx>
#include <ice/memory/scratch_allocator.hxx>

#include "iceshard_gfx_runner_trait.hxx"
#include "iceshard_gfx_context.hxx"

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

        void add_trait(
            ice::StringID_Arg name,
            ice::gfx::GfxTrait* trait
        ) noexcept override;

        auto graphics_world_name() noexcept -> ice::StringID override;
        void set_graphics_world(
            ice::StringID_Arg world_name
        ) noexcept override;

        void prepare_world(ice::World* world) noexcept override;
        void cleanup_world(ice::World* world) noexcept override;

        void set_event(ice::ManualResetEvent* event) noexcept override;

        void draw_frame(
            ice::EngineFrame const& engine_frame
        ) noexcept override;

        auto device() noexcept -> ice::gfx::GfxDevice& override;
        auto frame() noexcept -> ice::gfx::GfxFrame& override;

        void setup_traits() noexcept;
        void cleanup_traits() noexcept;

    protected:
        auto task_cleanup_gfx_contexts() noexcept  -> ice::Task<>;

        auto task_setup_gfx_traits() noexcept -> ice::Task<>;
        auto task_cleanup_gfx_traits() noexcept  -> ice::Task<>;

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
        ice::memory::ProxyAllocator _allocator;
        ice::UniquePtr<ice::TaskThread> _thread;
        ice::UniquePtr<ice::gfx::IceGfxDevice> _device;

        ice::render::RenderFence* _fences[4];

        ice::memory::ScratchAllocator _frame_allocator[2];
        ice::u32 _next_free_allocator;

        ice::UniquePtr<ice::gfx::IceGfxFrame> _current_frame;

        ice::StringID _gfx_world_name;
        ice::pod::Array<ice::gfx::IceGfxTraitEntry> _traits;

        ice::gfx::IceGfxRunnerTrait _runner_trait;
        ice::pod::Hash<ice::gfx::IceGfxContext*> _contexts;

        ice::ManualResetEvent _mre_internal;
        ice::ManualResetEvent* _mre_selected;
    };

} // namespace ice::gfx
