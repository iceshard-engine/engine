#pragma once
#include <ice/allocator.hxx>
#include <ice/render/render_queue.hxx>
#include <ice/gfx/gfx_pass.hxx>
#include <ice/pod/array.hxx>

namespace ice::gfx
{

    class IceGfxPass;

    class IceGfxPassGroup
    {
    public:
        IceGfxPassGroup(
            ice::Allocator& alloc,
            ice::u32 pass_count
        ) noexcept;

        ~IceGfxPassGroup() noexcept;

        auto add_pass(
            ice::StringID_Arg name,
            ice::render::RenderCommands& commands,
            ice::render::RenderQueue* queue,
            ice::u32 pool_index
        ) noexcept -> ice::gfx::IceGfxPass*;

        auto get_pass(
            ice::StringID_Arg name
        ) noexcept -> ice::gfx::IceGfxPass*;

        void prepare_all() noexcept;
        void execute_all(ice::render::Semaphore) noexcept;

        void get_render_queues(
            ice::pod::Array<ice::render::RenderQueue*>& queues_out
        ) noexcept;

        bool get_presenting_queue(
            ice::render::RenderQueue*& queue_out
        ) noexcept;

    private:
        ice::Allocator& _allocator;
        ice::pod::Hash<ice::gfx::IceGfxPass*> _gfx_passes;
    };

} // namespace ice
