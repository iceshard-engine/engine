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
            commands,
            render_queue,
            pool_index
        );

        ice::pod::hash::set(
            _gfx_queues,
            name_hash,
            queue
        );

        return queue;
    }

    auto IceGfxQueueGroup::get_queue(ice::StringID_Arg name) noexcept -> ice::gfx::IceGfxQueue*
    {
        return ice::pod::hash::get(
            _gfx_queues,
            ice::hash(name),
            nullptr
        );
    }

    void IceGfxQueueGroup::prepare_all() noexcept
    {
        for (auto const& entry : _gfx_queues)
        {
            entry.value->prepare();
        }
    }

    void IceGfxQueueGroup::execute_all() noexcept
    {
        for (auto const& entry : _gfx_queues)
        {
            entry.value->execute();
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
