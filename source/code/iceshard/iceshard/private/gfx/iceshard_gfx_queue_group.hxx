#pragma once
#include <ice/allocator.hxx>
#include <ice/render/render_queue.hxx>
#include <ice/gfx/gfx_queue.hxx>
#include <ice/pod/array.hxx>

namespace ice::gfx
{

    class IceGfxQueue;

    class IceGfxQueueGroup
    {
    public:
        IceGfxQueueGroup(
            ice::Allocator& alloc,
            ice::u32 queue_count
        ) noexcept;

        ~IceGfxQueueGroup() noexcept;

        auto add_queue(
            ice::StringID_Arg name,
            ice::render::RenderCommands& commands,
            ice::render::RenderQueue* queue,
            ice::u32 pool_index
        ) noexcept -> ice::gfx::IceGfxQueue*;

        auto get_queue(
            ice::StringID_Arg name
        ) noexcept -> ice::gfx::IceGfxQueue*;

        void prepare_all() noexcept;

        void get_render_queues(
            ice::pod::Array<ice::render::RenderQueue*>& queues_out
        ) noexcept;

        bool get_presenting_queue(
            ice::render::RenderQueue*& queue_out
        ) noexcept;

    private:
        ice::Allocator& _allocator;
        ice::pod::Hash<ice::gfx::IceGfxQueue*> _gfx_queues;
    };

} // namespace ice
