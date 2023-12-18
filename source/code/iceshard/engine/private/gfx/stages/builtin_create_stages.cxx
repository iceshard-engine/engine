/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "builtin_create_stages.hxx"
#include <ice/mem_allocator_host.hxx>
#include <ice/mem_allocator_stack.hxx>
#include <ice/render/render_pass.hxx>
#include <ice/render/render_swapchain.hxx>
#include <ice/gfx/gfx_object.hxx>

namespace ice::gfx::v2::builtin
{

    void Stage_CreateImage::execute(
        ice::gfx::v2::GfxStageParams const& params,
        ice::gfx::v2::GfxObjectStorage& storage
    ) noexcept
    {
        ImageInfo image_info;
        image_info.type = _params->type;
        image_info.format = _params->format;
        image_info.usage = _params->flags;
        image_info.width = ice::u32(_params->size.x);
        image_info.height = ice::u32(_params->size.y);

        if (image_info.format == render::ImageFormat::Invalid)
        {
            image_info.format = params.device.swapchain().image_format();
        }

        if ((image_info.width * image_info.height) == 0)
        {
            ice::vec2u const extent = params.device.swapchain().extent();
            image_info.width = extent.x;
            image_info.height = extent.y;
        }

        *_runtime->_image = params.device.device().create_image(image_info);
    }

    void Stage_CreateFramebuffer::execute(
        ice::gfx::v2::GfxStageParams const& params,
        ice::gfx::v2::GfxObjectStorage& storage
    ) noexcept
    {
        ice::StackAllocator<256_B> salloc;
        ice::Array<ice::render::Image> images{ salloc };
        ice::array::reserve(images, ice::ucount((ice::StackAllocator<256_B>::Constant_InternalCapacity / ice::size_of<render::Image>).value));

        ICE_ASSERT_CORE(false);
        //for (ice::ucount idx = 0; idx < _params->input_count; ++idx)
        //{
        //    ice::array::push_back(images, _runtime->_inputs[idx]->get<render::Image>());
        //}

        //_runtime->_out_framebuffer->set(
        //    params.device.device().create_framebuffer(
        //        _runtime->_size,
        //        _runtime->_renderpass->get<render::Renderpass>(),
        //        images
        //    )
        //);
        *_runtime->_framebuffer = params.device.device().create_framebuffer(
            _runtime->_size,
            *_runtime->_renderpass,
            images
        );
    }

    void Stage_CreateRenderpass::execute(
        ice::gfx::v2::GfxStageParams const& params,
        ice::gfx::v2::GfxObjectStorage& storage
    ) noexcept
    {
        using namespace ice::render;

        // TODO: Remove this reference here, handle it differently.
        ice::HostAllocator alloc;

        ice::Array<RenderAttachment> attachments{ alloc };
        ice::Array<AttachmentReference> references{ alloc };
        ice::Array<RenderSubPass> subpasses{ alloc };
        ice::Array<SubpassDependency> dependencies{ alloc };

        RenderpassInfo renderpass_info{
            .attachments = attachments,
            .subpasses = subpasses,
            .dependencies = dependencies,
        };

        *_runtime->_renderpass = params.device.device().create_renderpass(renderpass_info);
    }

} // namespace ice::gfx::v2::builtin
