/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_allocator.hxx>
#include <ice/render/render_queue.hxx>
#include <ice/gfx/gfx_queue.hxx>
#include <ice/container/hashmap.hxx>

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
            ice::render::QueueFlags flags,
            ice::u32 pool_index
        ) noexcept -> ice::gfx::IceGfxQueue*;

        bool get_queue(ice::render::QueueFlags flags, ice::gfx::IceGfxQueue*& out_queue) noexcept;
        auto get_queue(ice::StringID_Arg name) noexcept -> ice::gfx::IceGfxQueue*;

        void reset_all() noexcept;

        void query_queues(ice::Array<ice::StringID_Hash>& out_names) noexcept;
        void get_render_queues(ice::Array<ice::render::RenderQueue*>& queues_out) noexcept;
        bool get_presenting_queue(ice::render::RenderQueue*& queue_out) noexcept;

    private:
        ice::Allocator& _allocator;
        ice::HashMap<ice::gfx::IceGfxQueue*> _gfx_queues;
    };

} // namespace ice
