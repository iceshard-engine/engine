#include "iceshard_gfx_device.hxx"
#include "iceshard_gfx_frame.hxx"
#include <ice/render/render_surface.hxx>
#include <ice/render/render_swapchain.hxx>
#include <ice/render/render_buffer.hxx>
#include <ice/memory/stack_allocator.hxx>
#include <ice/pod/hash.hxx>
#include <ice/assert.hxx>

#include "iceshard_gfx_queue_group.hxx"
#include "iceshard_gfx_queue.hxx"
#include "subpass/iceshard_gfx_subpass.hxx"

namespace ice::gfx
{

    using ice::render::QueueID;
    using ice::render::QueueInfo;
    using ice::render::QueueFlags;
    using ice::render::QueueFamilyInfo;

    static QueueFlags constexpr DefaultQueueFlags = QueueFlags::Graphics | QueueFlags::Compute | QueueFlags::Present;

    namespace detail
    {

        auto find_queue_id(
            ice::Span<ice::render::QueueFamilyInfo const> queue_family_infos,
            ice::render::QueueFlags flags
        ) noexcept -> QueueID;

    } // namespace detail

    IceGfxDevice::IceGfxDevice(
        ice::Allocator& alloc,
        ice::render::RenderDriver& driver,
        ice::render::RenderSurface& render_surface,
        ice::render::RenderDevice* render_device,
        ice::pod::Array<ice::gfx::IceGfxQueueGroup*> graphics_queues
    ) noexcept
        : _allocator{ alloc }
        , _render_driver{ driver }
        , _render_surface{ render_surface }
        , _render_device{ render_device }
        , _render_swapchain{ nullptr }
        , _graphics_queues{ ice::move(graphics_queues) }
        , _resource_tracker{ _allocator }
    {
        _render_swapchain = _render_device->create_swapchain(&_render_surface);

        track_resource(
            _resource_tracker,
            "temp.buffer.image_transfer"_sid,
            _render_device->create_buffer(ice::render::BufferType::Transfer, 2048 * 2048 * 4 * sizeof(ice::u8))
        );

        create_subpass_resources_primitives(*_render_device, _resource_tracker);
        create_subpass_resources_terrain(*_render_device, _resource_tracker);
        create_subpass_resources_imgui(*_render_device, _resource_tracker);
    }

    IceGfxDevice::~IceGfxDevice() noexcept
    {
        destroy_subpass_resources_imgui(*_render_device, _resource_tracker);
        destroy_subpass_resources_terrain(*_render_device, _resource_tracker);
        destroy_subpass_resources_primitives(*_render_device, _resource_tracker);

        _render_device->destroy_buffer(
            find_resource<ice::render::Buffer>(_resource_tracker, "temp.buffer.image_transfer"_sid)
        );

        _render_device->destroy_swapchain(_render_swapchain);

        bool first = true;
        ice::pod::Array<ice::render::RenderQueue*> queues{ _allocator };
        for (ice::gfx::IceGfxQueueGroup* group : _graphics_queues)
        {
            if (first)
            {
                group->get_render_queues(queues);
                first = false;
            }
            _allocator.destroy(group);
        }

        for (ice::render::RenderQueue* queue : queues)
        {
            _render_device->destroy_queue(queue);
        }

        _render_driver.destroy_device(_render_device);
    }

    auto IceGfxDevice::device() noexcept -> ice::render::RenderDevice&
    {
        return *_render_device;
    }

    auto IceGfxDevice::swapchain() noexcept -> ice::render::RenderSwapchain const&
    {
        return *_render_swapchain;
    }

    void IceGfxDevice::recreate_swapchain() noexcept
    {
        _render_device->wait_idle();
        _render_device->destroy_swapchain(_render_swapchain);
        _render_swapchain = _render_device->create_swapchain(&_render_surface);
    }

    auto IceGfxDevice::resource_tracker() noexcept -> ice::gfx::GfxResourceTracker&
    {
        return _resource_tracker;
    }

    auto IceGfxDevice::next_frame() noexcept -> ice::u32
    {
        return _render_swapchain->aquire_image();
    }

    void IceGfxDevice::present(ice::u32 image_index) noexcept
    {
        ice::render::RenderQueue* presenting_queue;
        if (_graphics_queues[image_index]->get_presenting_queue(presenting_queue))
        {
            presenting_queue->present(_render_swapchain);
        }
        else
        {
            ICE_ASSERT(
                false, "No graphics pass set as presenting!"
            );
        }
    }

    auto IceGfxDevice::queue_group(ice::u32 image_index) noexcept -> ice::gfx::IceGfxQueueGroup&
    {
        return *_graphics_queues[image_index];
    }

    auto detail::find_queue_id(
        ice::Span<ice::render::QueueFamilyInfo const> queue_family_infos,
        ice::render::QueueFlags flags
    ) noexcept -> QueueID
    {
        QueueID queue_id = QueueID::Invalid;
        for (ice::render::QueueFamilyInfo const& queue_family : queue_family_infos)
        {
            if ((queue_family.flags & DefaultQueueFlags) == DefaultQueueFlags)
            {
                queue_id = queue_family.id;
                break;
            }
        }

        return queue_id;
    }

    auto create_graphics_device(
        ice::Allocator& alloc,
        ice::render::RenderDriver& render_driver,
        ice::render::RenderSurface& render_surface,
        ice::Span<ice::RenderQueueDefinition const> render_queues
    ) noexcept -> ice::UniquePtr<ice::gfx::IceGfxDevice>
    {
        ice::pod::Array<ice::render::QueueFamilyInfo> queue_families{ alloc };
        ice::pod::array::reserve(queue_families, 20);
        render_driver.query_queue_infos(queue_families);

        using ice::render::QueueFlags;
        using ice::render::QueueInfo;
        using ice::render::QueueID;

        ice::pod::Array<ice::render::QueueInfo> queues{ alloc };
        ice::pod::array::reserve(queues, ice::size(render_queues));

        auto find_queue_index = [](auto const& array_, QueueID id_, ice::u32& idx_out) noexcept -> bool
        {
            ice::u32 const size = ice::pod::array::size(array_);

            idx_out = 0;
            while (idx_out < size && array_[idx_out].id != id_)
            {
                idx_out += 1;
            }

            return ice::pod::array::size(array_) > idx_out;
        };

        ice::pod::Hash<ice::u32> queue_index_tracker{ alloc };
        for (ice::RenderQueueDefinition const& pass_info : render_queues)
        {
            QueueID const pass_queue_id = detail::find_queue_id(queue_families, pass_info.flags);
            ICE_ASSERT(
                pass_queue_id != QueueID::Invalid,
                "Graphics pass queue flags cannot be satisfied!"
            );

            ice::u32 queue_info_idx;
            if (find_queue_index(queues, pass_queue_id, queue_info_idx))
            {
                queues[queue_info_idx].count += 1;
            }
            else
            {
                ice::pod::array::push_back(
                    queues,
                    QueueInfo{
                        .id = pass_queue_id,
                        .count = 1
                    }
                );
            }

            ice::pod::hash::set(
                queue_index_tracker,
                ice::hash(reinterpret_cast<ice::uptr>(&pass_info)),
                queues[queue_info_idx].count - 1
            );
        }

        for (ice::render::QueueFamilyInfo const& family_info : queue_families)
        {
            ice::u32 queue_info_idx;
            if (find_queue_index(queues, family_info.id, queue_info_idx))
            {
                ICE_ASSERT(
                    queues[queue_info_idx].count <= family_info.count,
                    "Requested Queue count to be created is larger than family can support! [{} > {}]",
                    queues[queue_info_idx].count,
                    family_info.count
                );
            }
        }

        ice::u32 constexpr pass_group_count = 2;

        ice::render::RenderDevice* const render_device = render_driver.create_device(queues);
        if (render_device != nullptr)
        {
            ice::pod::Array<ice::gfx::IceGfxQueueGroup*> pass_groups{ alloc };
            ice::pod::array::reserve(pass_groups, pass_group_count);

            for (ice::u32 group_pool_index = 0; group_pool_index < pass_group_count; ++group_pool_index)
            {
                ice::pod::array::push_back(
                    pass_groups,
                    alloc.make<IceGfxQueueGroup>(
                        alloc,
                        ice::size(render_queues)
                    )
                );
            }

            for (ice::RenderQueueDefinition const& pass_info : render_queues)
            {
                QueueID const pass_queue_id = detail::find_queue_id(queue_families, pass_info.flags);
                ice::u32 const pass_queue_index = ice::pod::hash::get(
                    queue_index_tracker,
                    ice::hash(reinterpret_cast<ice::uptr>(&pass_info)),
                    ~0u
                );

                ice::render::RenderQueue* render_queue = render_device->create_queue(
                    pass_queue_id,
                    pass_queue_index,
                    pass_group_count
                );

                for (ice::u32 group_index = 0; group_index < pass_group_count; ++group_index)
                {
                    pass_groups[group_index]->add_queue(
                        pass_info.name,
                        render_device->get_commands(),
                        render_queue,
                        group_index
                    )->set_presenting((pass_info.flags & QueueFlags::Present) == QueueFlags::Present);
                }
            }

            return ice::make_unique<ice::gfx::IceGfxDevice>(
                alloc, alloc,
                render_driver,
                render_surface,
                render_device,
                ice::move(pass_groups)
            );
        }
        return ice::make_unique_null<IceGfxDevice>();
    }

} // namespace ice::gfx
