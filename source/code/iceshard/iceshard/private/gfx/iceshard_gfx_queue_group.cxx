/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "iceshard_gfx_queue_group.hxx"
#include "iceshard_gfx_queue.hxx"
#include <ice/container/hashmap.hxx>
#include <ice/assert.hxx>

namespace ice::gfx
{

    IceGfxQueueGroup::IceGfxQueueGroup(
        ice::Allocator& alloc,
        ice::u32 queue_count
    ) noexcept
        : _allocator{ alloc }
        , _gfx_queues{ _allocator }
    {
        ice::hashmap::reserve(_gfx_queues, queue_count * 2);
    }

    IceGfxQueueGroup::~IceGfxQueueGroup() noexcept
    {
        for (IceGfxQueue* queue : _gfx_queues)
        {
            _allocator.destroy(queue);
        }
    }

    auto IceGfxQueueGroup::add_queue(
        ice::StringID_Arg name,
        ice::render::RenderCommands& commands,
        ice::render::RenderQueue* render_queue,
        ice::render::QueueFlags flags,
        ice::u32 pool_index
    ) noexcept -> ice::gfx::IceGfxQueue*
    {
        ice::u64 const name_hash = ice::hash(name);
        ICE_ASSERT(
            ice::hashmap::has(_gfx_queues, name_hash) == false,
            "Duplicate graphics queue encountered! [{}]",
            ice::stringid_hint(name)
        );

        ice::gfx::IceGfxQueue* queue = _allocator.create<ice::gfx::IceGfxQueue>(
            _allocator,
            name,
            commands,
            render_queue,
            flags,
            pool_index
        );

        ice::hashmap::set(
            _gfx_queues,
            name_hash,
            queue
        );

        return queue;
    }

    bool IceGfxQueueGroup::get_queue(
        ice::render::QueueFlags flags,
        ice::gfx::IceGfxQueue*& out_queue
    ) noexcept
    {
        if (flags == ice::render::QueueFlags::None)
        {
            return false;
        }

        for (IceGfxQueue* queue : _gfx_queues)
        {
            if ((queue->queue_flags() & flags) == flags)
            {
                out_queue = queue;
                return true;
            }
        }
        return false;
    }

    auto IceGfxQueueGroup::get_queue(ice::StringID_Arg name) noexcept -> ice::gfx::IceGfxQueue*
    {
        return ice::hashmap::get(
            _gfx_queues,
            ice::hash(name),
            nullptr
        );
    }

    void IceGfxQueueGroup::reset_all() noexcept
    {
        for (IceGfxQueue* queue : _gfx_queues)
        {
            queue->reset();
        }
    }

    void IceGfxQueueGroup::query_queues(ice::Array<ice::StringID_Hash>& out_names) noexcept
    {
        for (IceGfxQueue* queue : _gfx_queues)
        {
            ice::array::push_back(out_names, ice::stringid_hash(queue->name()));
        }
    }

    void IceGfxQueueGroup::get_render_queues(
        ice::Array<ice::render::RenderQueue*>& queues_out
    ) noexcept
    {
        auto has_queue = [](auto const& array_, ice::render::RenderQueue const* const expected_queue) noexcept -> bool
        {
            bool has_queue = false;
            for (ice::render::RenderQueue const* const queue : array_)
            {
                has_queue |= (queue == expected_queue);
            }
            return has_queue;
        };

        for (IceGfxQueue* queue : _gfx_queues)
        {
            ice::render::RenderQueue* render_queue = queue->render_queue();
            if (has_queue(queues_out, render_queue) == false)
            {
                ice::array::push_back(
                    queues_out,
                    render_queue
                );
            }
        }
    }

    bool IceGfxQueueGroup::get_presenting_queue(
        ice::render::RenderQueue*& queue_out
    ) noexcept
    {
        queue_out = nullptr;
        for (IceGfxQueue* queue : _gfx_queues)
        {
            if (queue->presenting())
            {
                queue_out = queue->render_queue();
                break;
            }
        }
        return queue_out != nullptr;
    }

} // namespace ice
