#include "iceshard_gfx_device.hxx"
#include "iceshard_gfx_frame.hxx"
#include <ice/render/render_surface.hxx>
#include <ice/render/render_swapchain.hxx>
#include <ice/memory/stack_allocator.hxx>
#include <ice/pod/hash.hxx>
#include <ice/assert.hxx>

#include "iceshard_gfx_pass_group.hxx"
#include "iceshard_gfx_pass.hxx"

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
        ice::render::RenderDriver* driver,
        ice::render::RenderSurface* render_surface,
        ice::render::RenderDevice* render_device,
        ice::pod::Array<ice::gfx::IceGfxPassGroup*> graphics_passes
    ) noexcept
        : _allocator{ alloc }
        , _render_driver{ driver }
        , _render_surface{ render_surface }
        , _render_device{ render_device }
        , _render_swapchain{ nullptr }
        , _graphics_passes{ ice::move(graphics_passes) }
    {
        _render_swapchain = _render_device->create_swapchain(_render_surface);

        create_temporary_resources();
    }

    IceGfxDevice::~IceGfxDevice() noexcept
    {
        _render_device->destroy_framebuffer(_render_framebuffers[1]);
        _render_device->destroy_framebuffer(_render_framebuffers[0]);
        _render_device->destroy_image(_depth_stencil_image);
        _render_device->destroy_renderpass(_render_pass);
        _render_device->destroy_swapchain(_render_swapchain);

        bool first = true;
        ice::pod::Array<ice::render::RenderQueue*> queues{ _allocator };
        for (ice::gfx::IceGfxPassGroup* group : _graphics_passes)
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

        _render_driver->destroy_device(_render_device);
    }

    auto IceGfxDevice::device() noexcept -> ice::render::RenderDevice&
    {
        return *_render_device;
    }

    auto IceGfxDevice::swapchain() noexcept -> ice::render::RenderSwapchain&
    {
        return *_render_swapchain;
    }

    auto IceGfxDevice::default_queue() noexcept -> ice::render::RenderQueue&
    {
        return *((ice::render::RenderQueue*)0);
    }

    auto IceGfxDevice::next_frame(ice::Allocator& alloc) noexcept -> ice::UniquePtr<ice::gfx::IceGfxFrame>
    {
        ice::u32 const framebuffer_index = _render_swapchain->aquire_image();

        return ice::make_unique<gfx::IceGfxFrame>(
            alloc,
            _render_device,
            _render_swapchain,
            _render_pass,
            _render_framebuffers[framebuffer_index],
            _graphics_passes[framebuffer_index]
        );
    }

    void IceGfxDevice::create_temporary_resources() noexcept
    {
        using namespace ice::render;

        ice::vec2u const swapchain_extent = _render_swapchain->extent();

        RenderAttachment attachments[]{
            RenderAttachment{
                .format = _render_swapchain->image_format(),
                .layout = ImageLayout::Present,
                .type = AttachmentType::SwapchainImage,
                .operations = {
                    AttachmentOperation::Load_Clear,
                    AttachmentOperation::Store_Store
                },
            },
            RenderAttachment{
                .format = ImageFormat::UNORM_D24_UINT_S8,
                .layout = ImageLayout::DepthStencil,
                .type = AttachmentType::DepthStencil,
            },
        };

        AttachmentReference references[]{
            AttachmentReference{
                .attachment_index = 0,
                .layout = ImageLayout::Color
            },
            AttachmentReference{
                .attachment_index = 1,
                .layout = ImageLayout::DepthStencil
            },
        };

        RenderSubPass subpasses[]{
            RenderSubPass{
                .color_attachments = { references + 0, 1 },
            },
            RenderSubPass{
                .color_attachments = { references + 0, 1 },
                .depth_stencil_attachment = references[1],
            }
        };

        SubpassDependency dependencies[]{
            SubpassDependency{
                .source_subpass = 0,
                .source_stage = PipelineStage::ColorAttachmentOutput,
                .source_access = AccessFlags::ColorAttachmentWrite,
                .destination_subpass = 1,
                .destination_stage = PipelineStage::ColorAttachmentOutput,
                .destination_access = AccessFlags::ColorAttachmentWrite,
            }
        };

        RenderPassInfo renderpass_info{
            .attachments = attachments,
            .subpasses = subpasses,
            .dependencies = dependencies,
        };

        _render_pass = _render_device->create_renderpass(renderpass_info);

        ImageInfo image_info;
        image_info.type = ImageType::Image2D;
        image_info.format = ImageFormat::UNORM_D24_UINT_S8;
        image_info.usage = ImageUsageFlags::DepthStencilAttachment;
        image_info.width = swapchain_extent.x;
        image_info.height = swapchain_extent.y;
        _depth_stencil_image = _render_device->create_image(image_info, { });

        Image framebuffer_images[]{
            Image::Invalid,
            _depth_stencil_image
        };

        framebuffer_images[0] = _render_swapchain->image(0);
        _render_framebuffers[0] = _render_device->create_framebuffer(
            swapchain_extent,
            _render_pass,
            framebuffer_images
        );

        framebuffer_images[0] = _render_swapchain->image(1);
        _render_framebuffers[1] = _render_device->create_framebuffer(
            swapchain_extent,
            _render_pass,
            framebuffer_images
        );
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
        ice::gfx::GfxDeviceCreateInfo const& create_info
    ) noexcept -> ice::UniquePtr<ice::gfx::IceGfxDevice>
    {
        ice::render::RenderDriver* render_driver = create_info.render_driver;

        //ice::memory::StackAllocator<sizeof(ice::render::QueueFamilyInfo) * 21> temp_alloc;
        ice::pod::Array<ice::render::QueueFamilyInfo> queue_families{ alloc };
        ice::pod::array::reserve(queue_families, 20);
        render_driver->query_queue_infos(queue_families);

        using ice::render::QueueFlags;
        using ice::render::QueueInfo;
        using ice::render::QueueID;

        ice::pod::Array<ice::render::QueueInfo> queues{ alloc };
        ice::pod::array::reserve(queues, ice::size(create_info.pass_list));

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
        for (ice::gfx::GfxPassCreateInfo const& pass_info : create_info.pass_list)
        {
            QueueID const pass_queue_id = detail::find_queue_id(queue_families, pass_info.queue_flags);
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

        ice::render::RenderDevice* const render_device = render_driver->create_device(queues);
        if (render_device != nullptr)
        {
            ice::pod::Array<ice::gfx::IceGfxPassGroup*> pass_groups{ alloc };
            ice::pod::array::reserve(pass_groups, pass_group_count);

            for (ice::u32 group_pool_index = 0; group_pool_index < pass_group_count; ++group_pool_index)
            {
                ice::pod::array::push_back(
                    pass_groups,
                    alloc.make<IceGfxPassGroup>(
                        alloc,
                        ice::size(create_info.pass_list)
                    )
                );
            }

            for (ice::gfx::GfxPassCreateInfo const& pass_info : create_info.pass_list)
            {
                QueueID const pass_queue_id = detail::find_queue_id(queue_families, pass_info.queue_flags);
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
                    pass_groups[group_index]->add_pass(
                        pass_info.name,
                        render_queue,
                        group_index
                    )->set_presenting((pass_info.queue_flags & QueueFlags::Present) == QueueFlags::Present);
                }
            }

            return ice::make_unique<ice::gfx::IceGfxDevice>(
                alloc, alloc,
                create_info.render_driver,
                create_info.render_surface,
                render_device,
                ice::move(pass_groups)
            );
        }
        return ice::make_unique_null<IceGfxDevice>();
    }

} // namespace ice::gfx
