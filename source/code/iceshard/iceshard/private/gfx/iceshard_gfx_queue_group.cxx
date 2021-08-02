#pragma once
#include "iceshard_gfx_queue_group.hxx"
#include "iceshard_gfx_queue.hxx"
#include <ice/pod/hash.hxx>
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
        ice::pod::hash::reserve(_gfx_queues, queue_count * 2);
    }

    IceGfxQueueGroup::~IceGfxQueueGroup() noexcept
    {
        for (auto const& entry : _gfx_queues)
        {
            _allocator.destroy(entry.value);
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
            ice::pod::hash::has(_gfx_queues, name_hash) == false,
            "Duplicate graphics queue encountered! [{}]",
            ice::stringid_hint(name)
        );

        ice::gfx::IceGfxQueue* queue = _allocator.make<ice::gfx::IceGfxQueue>(
            _allocator,
            name,
            commands,
            render_queue,
            flags,
            pool_index
        );

        ice::pod::hash::set(
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
        if (flags == ice::render::QueueFlags::Invalid)
        {
            return false;
        }

        for (auto const& entry : _gfx_queues)
        {
            if ((entry.value->queue_flags() & flags) == flags)
            {
                out_queue = entry.value;
                return true;
            }
        }
        return false;
    }

    auto IceGfxQueueGroup::get_queue(ice::StringID_Arg name) noexcept -> ice::gfx::IceGfxQueue*
    {
        return ice::pod::hash::get(
            _gfx_queues,
            ice::hash(name),
            nullptr
        );
    }

    void IceGfxQueueGroup::reset_all() noexcept
    {
        for (auto const& entry : _gfx_queues)
        {
            entry.value->reset();
        }
    }

    void IceGfxQueueGroup::query_queues(ice::pod::Array<ice::StringID_Hash>& out_names) noexcept
    {
        for (auto const& entry : _gfx_queues)
        {
            ice::pod::array::push_back(out_names, StringID_Hash{ entry.key });
        }
    }

    void IceGfxQueueGroup::get_render_queues(
        ice::pod::Array<ice::render::RenderQueue*>& queues_out
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

        for (auto const& entry : _gfx_queues)
        {
            ice::render::RenderQueue* queue = entry.value->render_queue();
            if (has_queue(queues_out, queue) == false)
            {
                ice::pod::array::push_back(
                    queues_out,
                    queue
                );
            }
        }
    }

    bool IceGfxQueueGroup::get_presenting_queue(
        ice::render::RenderQueue*& queue_out
    ) noexcept
    {
        queue_out = nullptr;
        for (auto const& entry : _gfx_queues)
        {
            if (entry.value->presenting())
            {
                queue_out = entry.value->render_queue();
                break;
            }
        }
        return queue_out != nullptr;
    }

} // namespace ice
