#include "iceshard_gfx_device.hxx"
#include "iceshard_gfx_frame.hxx"
#include <ice/render/render_surface.hxx>
#include <ice/render/render_swapchain.hxx>
#include <ice/memory/stack_allocator.hxx>

namespace ice::gfx
{

    using ice::render::QueueID;
    using ice::render::QueueInfo;
    using ice::render::QueueFlags;
    using ice::render::QueueFamilyInfo;

    static QueueFlags constexpr DefaultQueueFlags = QueueFlags::Graphics | QueueFlags::Compute | QueueFlags::Present;

    namespace detail
    {

        auto find_default_queue_id(ice::render::RenderDriver* driver) noexcept -> QueueID;

    } // namespace detail

    IceGfxDevice::IceGfxDevice(
        ice::Allocator& alloc,
        ice::render::RenderDriver* driver,
        ice::render::RenderSurface* render_surface
    ) noexcept
        : _render_driver{ driver }
        , _render_surface{ render_surface }
        , _default_queue_id{ detail::find_default_queue_id(_render_driver) }
        , _render_device{ nullptr }
        , _render_swapchain{ nullptr }
        , _render_queues{ alloc }
    {
        QueueInfo queue_infos[]{
            QueueInfo{
                .id = _default_queue_id,
                .count = 1,
            }
        };

        _render_device = _render_driver->create_device(queue_infos);
        _render_swapchain = _render_device->create_swapchain(_render_surface);

        ice::pod::array::push_back(
            _render_queues,
            _render_device->create_queue(_default_queue_id, 0, 3)
        );

        create_temporary_resources();
    }

    IceGfxDevice::~IceGfxDevice() noexcept
    {
        _render_device->destroy_framebuffer(_render_framebuffers[1]);
        _render_device->destroy_framebuffer(_render_framebuffers[0]);
        _render_device->destroy_image(_depth_stencil_image);
        _render_device->destroy_renderpass(_render_pass);

        for (ice::render::RenderQueue* queue : _render_queues)
        {
            _render_device->destroy_queue(queue);
        }

        _render_device->destroy_swapchain(_render_swapchain);
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
        return *ice::pod::array::front(_render_queues);
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
            _render_queues[0],
            framebuffer_index
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

    auto detail::find_default_queue_id(ice::render::RenderDriver* driver) noexcept -> QueueID
    {
        ice::memory::StackAllocator_512 temp_alloc;
        ice::pod::Array<ice::render::QueueFamilyInfo> queue_familiy_infos{ temp_alloc };
        ice::pod::array::reserve(queue_familiy_infos, 10);

        driver->query_queue_infos(queue_familiy_infos);

        QueueID queue_id = QueueID::Invalid;
        for (ice::render::QueueFamilyInfo const& queue_family : queue_familiy_infos)
        {
            if ((queue_family.flags & DefaultQueueFlags) == DefaultQueueFlags)
            {
                queue_id = queue_family.id;
                break;
            }
        }

        return queue_id;
    }

} // namespace ice::gfx
