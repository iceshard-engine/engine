/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "trait_render_gfx.hxx"

#include <ice/gfx/gfx_device.hxx>
#include <ice/gfx/gfx_frame.hxx>
#include <ice/gfx/gfx_resource_tracker.hxx>
#include <ice/gfx/gfx_stage.hxx>

#include <ice/platform_event.hxx>

#include <ice/engine_runner.hxx>
#include <ice/world/world_trait_archive.hxx>

#include <ice/render/render_device.hxx>
#include <ice/render/render_framebuffer.hxx>
#include <ice/render/render_pass.hxx>
#include <ice/render/render_swapchain.hxx>
#include <ice/render/render_image.hxx>
#include <ice/render/render_resource.hxx>

namespace ice
{

    void IceWorldTrait_RenderGfx::on_update(
        ice::EngineFrame& frame,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        frame.storage().create_named_object<ice::render::Renderpass>("ice.gfx.renderpass"_sid, _default_renderpass);
    }

    void IceWorldTrait_RenderGfx::gfx_setup(
        ice::gfx::GfxFrame& gfx_frame,
        ice::gfx::GfxDevice& gfx_device
    ) noexcept
    {
        using namespace gfx;
        using namespace ice::render;

        GfxResourceTracker& res_tracker = gfx_device.resource_tracker();

        RenderDevice& render_device = gfx_device.device();
        RenderSwapchain const& swapchain = gfx_device.swapchain();


        RenderAttachment attachments[]{
            RenderAttachment{
                .format = swapchain.image_format(),
                .final_layout = ImageLayout::ShaderReadOnly,
                .type = AttachmentType::TextureImage,
                .operations = {
                    AttachmentOperation::Load_Clear,
                    AttachmentOperation::Store_DontCare,
                },
            },
            RenderAttachment{
                .format = swapchain.image_format(),
                .final_layout = ImageLayout::Present,
                .type = AttachmentType::SwapchainImage,
                .operations = {
                    AttachmentOperation::Load_Clear,
                    AttachmentOperation::Store_Store
                },
            },
            RenderAttachment{
                .format = ImageFormat::SFLOAT_D32_UINT_S8,
                .final_layout = ImageLayout::DepthStencil,
                .type = AttachmentType::DepthStencil,
                .operations = {
                    AttachmentOperation::Load_Clear
                }
            },
        };

        AttachmentReference references[]{
            AttachmentReference{
                .attachment_index = 0,
                .layout = ImageLayout::Color
            },
            AttachmentReference{
                .attachment_index = 0,
                .layout = ImageLayout::ShaderReadOnly
            },
            AttachmentReference{
                .attachment_index = 1,
                .layout = ImageLayout::Color
            },
            AttachmentReference{
                .attachment_index = 2,
                .layout = ImageLayout::DepthStencil
            },
        };

        RenderSubPass subpasses[]{
            RenderSubPass{
                .color_attachments = { references + 0, 1 },
            },
            RenderSubPass{
                .color_attachments = { references + 0, 1 },
                .depth_stencil_attachment = references[3],
            },
            RenderSubPass{
                .input_attachments = { references + 1, 1 },
                .color_attachments = { references + 2, 1 },
            },
        };

        SubpassDependency dependencies[]{
            SubpassDependency{
                .source_subpass = 0,
                .source_stage = PipelineStage::ColorAttachmentOutput,
                .source_access = AccessFlags::ColorAttachmentWrite,
                .destination_subpass = 1,
                .destination_stage = PipelineStage::ColorAttachmentOutput,
                .destination_access = AccessFlags::ColorAttachmentWrite,
            },
            SubpassDependency{
                .source_subpass = 1,
                .source_stage = PipelineStage::ColorAttachmentOutput,
                .source_access = AccessFlags::ColorAttachmentWrite,
                .destination_subpass = 2,
                .destination_stage = PipelineStage::FramentShader,
                .destination_access = AccessFlags::InputAttachmentRead,
            }
        };

        RenderpassInfo renderpass_info{
            .attachments = attachments,
            .subpasses = subpasses,
            .dependencies = dependencies,
        };

        _default_renderpass = render_device.create_renderpass(renderpass_info);

        ice::vec2u extent = swapchain.extent();

        ImageInfo image_info;
        image_info.type = ImageType::Image2D;
        image_info.format = ImageFormat::SFLOAT_D32_UINT_S8;
        image_info.usage = ImageUsageFlags::DepthStencilAttachment;
        image_info.width = extent.x;
        image_info.height = extent.y;

        _default_attachment_depth_stencil = render_device.create_image(image_info, { });

        image_info.type = ImageType::Image2D;
        image_info.format = swapchain.image_format();
        image_info.usage = ImageUsageFlags::ColorAttachment | ImageUsageFlags::InputAttachment | ImageUsageFlags::Sampled;
        image_info.width = extent.x;
        image_info.height = extent.y;

        _default_attachment_color = render_device.create_image(image_info, { });

        Image framebuffer_images[]{
            _default_attachment_color,
            Image::Invalid,
            _default_attachment_depth_stencil
        };

        //SamplerInfo sampler_info{
        //    .min_filter = SamplerFilter::Linear,
        //    .mag_filter = SamplerFilter::Linear,
        //    .address_mode = {
        //        .u = SamplerAddressMode::Repeat,
        //        .v = SamplerAddressMode::Repeat,
        //        .w = SamplerAddressMode::Repeat,
        //    },
        //    .mip_map_mode = SamplerMipMapMode::Linear,
        //};

        //Sampler basic_sampler = render_device.create_sampler(sampler_info);

        framebuffer_images[1] = swapchain.image(0);
        _default_framebuffers[0] = render_device.create_framebuffer(
            extent,
            _default_renderpass,
            framebuffer_images
        );
        framebuffer_images[1] = swapchain.image(1);
        _default_framebuffers[1] = render_device.create_framebuffer(
            extent,
            _default_renderpass,
            framebuffer_images
        );

        ice::gfx::track_resource(res_tracker, "ice.gfx.renderpass.default"_sid, _default_renderpass);
        ice::gfx::track_resource(res_tracker, "ice.gfx.attachment.image.depth_stencil"_sid, _default_attachment_depth_stencil);
        ice::gfx::track_resource(res_tracker, "ice.gfx.attachment.image.color"_sid, _default_attachment_color);
        ice::gfx::track_resource(res_tracker, "ice.gfx.framebuffer.0"_sid, _default_framebuffers[0]);
        ice::gfx::track_resource(res_tracker, "ice.gfx.framebuffer.1"_sid, _default_framebuffers[1]);
    }

    void IceWorldTrait_RenderGfx::gfx_cleanup(
        ice::gfx::GfxFrame& gfx_frame,
        ice::gfx::GfxDevice& gfx_device
    ) noexcept
    {
        using namespace ice::gfx;
        using namespace ice::render;

        RenderDevice& render_device = gfx_device.device();

        render_device.destroy_framebuffer(_default_framebuffers[0]);
        render_device.destroy_framebuffer(_default_framebuffers[1]);
        render_device.destroy_image(_default_attachment_depth_stencil);
        render_device.destroy_image(_default_attachment_color);
        render_device.destroy_renderpass(_default_renderpass);
    }

    void IceWorldTrait_RenderGfx::gfx_update(
        ice::EngineFrame const& engine_frame,
        ice::gfx::GfxFrame& gfx_frame,
        ice::gfx::GfxDevice& gfx_device
    ) noexcept
    {
        if (ice::shards::contains(engine_frame.shards(), ice::platform::Shard_WindowResized))
        {
            gfx_device.recreate_swapchain();

            gfx_cleanup(gfx_frame, gfx_device);
            gfx_setup(gfx_frame, gfx_device);
        }
    }

    void register_trait_render_gfx(
        ice::WorldTraitArchive& archive
    ) noexcept
    {
        ice::register_trait_default<IceWorldTrait_RenderGfx>(
            archive,
            ice::Constant_TraitName_RenderBase
        );
    }

} // namespace ice
