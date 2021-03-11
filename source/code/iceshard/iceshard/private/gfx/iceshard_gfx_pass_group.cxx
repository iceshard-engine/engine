#pragma once
#include "iceshard_gfx_pass_group.hxx"
#include "iceshard_gfx_pass.hxx"
#include <ice/pod/hash.hxx>
#include <ice/assert.hxx>

namespace ice::gfx
{

    IceGfxPassGroup::IceGfxPassGroup(
        ice::Allocator& alloc,
        ice::u32 pass_count
    ) noexcept
        : _allocator{ alloc }
        , _gfx_passes{ _allocator }
    {
        ice::pod::hash::reserve(_gfx_passes, pass_count * 2);
    }

    IceGfxPassGroup::~IceGfxPassGroup() noexcept
    {
        for (auto const& entry : _gfx_passes)
        {
            _allocator.destroy(entry.value);
        }
    }

    auto IceGfxPassGroup::add_pass(
        ice::StringID_Arg name,
        ice::render::RenderQueue* queue,
        ice::u32 pool_index
    ) noexcept -> ice::gfx::IceGfxPass*
    {
        ice::u64 const name_hash = ice::hash(name);
        ICE_ASSERT(
            ice::pod::hash::has(_gfx_passes, name_hash) == false,
            "Duplicate graphics pass encountered! [{}]",
            ice::stringid_hint(name)
        );

        ice::gfx::IceGfxPass* pass = _allocator.make<ice::gfx::IceGfxPass>(
            queue,
            pool_index
        );

        ice::pod::hash::set(
            _gfx_passes,
            name_hash,
            pass
        );

        return pass;
    }

    auto IceGfxPassGroup::get_pass(ice::StringID_Arg name) noexcept -> ice::gfx::IceGfxPass*
    {
        return ice::pod::hash::get(
            _gfx_passes,
            ice::hash(name),
            nullptr
        );
    }

    void IceGfxPassGroup::get_render_queues(
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

        for (auto const& entry : _gfx_passes)
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

    bool IceGfxPassGroup::get_presenting_queue(
        ice::render::RenderQueue*& queue_out
    ) noexcept
    {
        queue_out = nullptr;
        for (auto const& entry : _gfx_passes)
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
