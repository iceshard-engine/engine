/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "webgpu_commands.hxx"
#include "webgpu_command_buffer.hxx"
#include "webgpu_framebuffer.hxx"
#include "webgpu_buffer.hxx"
#include "webgpu_image.hxx"
#include "webgpu_pipeline.hxx"
#include "webgpu_renderpass.hxx"
#include "webgpu_resources.hxx"
#include "webgpu_utils.hxx"
#include <ice/mem_allocator_stack.hxx>

namespace ice::render::webgpu
{

    void WebGPUCommands::begin(
        ice::render::CommandBuffer cmds
    ) noexcept
    {
        WebGPUCommandBuffer* webgpu_cmds = WebGPUCommandBuffer::native(cmds);
        ICE_ASSERT_CORE(webgpu_cmds->command_buffer == nullptr);

        // We releast the previous buffer
        // if (webgpu_cmds->command_buffer != nullptr)
        // {
        //     wgpuCommandBufferRelease(ice::exchange(webgpu_cmds->command_buffer, nullptr));
        // }
        //wgpuCommandEncoderInsertDebugMarker(webgpu_cmds->command_encoder, "Start");
    }

    void WebGPUCommands::begin_renderpass(
        ice::render::CommandBuffer cmds,
        ice::render::Renderpass renderpass,
        ice::render::Framebuffer framebuffer,
        ice::Span<ice::vec4f const> clear_values,
        ice::vec2u extent
    ) noexcept
    {
        WebGPUCommandBuffer* webgpu_cmds = WebGPUCommandBuffer::native(cmds);
        WebGPUFrameBuffer const* webgpu_fb = WebGPUFrameBuffer::native(framebuffer);
        WebGPURenderPass const* webgpu_rp = WebGPURenderPass::native(renderpass);

        // Max 4 attachments for now
        ice::ucount attachment_count = 0;
        WGPURenderPassColorAttachment attachments[4];

        RenderSubPass const& subpass = webgpu_rp->subpasses[0];
        for (AttachmentReference const& ref : subpass.color_attachments)
        {
            RenderAttachment const& att = webgpu_rp->attachments[ref.attachment_index];

            WGPURenderPassColorAttachment& attachment = attachments[attachment_count];
            ice::vec4f clear_value =  clear_values[attachment_count];
            attachment.clearValue = { (double)clear_value.x, (double)clear_value.y, (double)clear_value.z, (double)clear_value.w };
            attachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
            attachment.loadOp = att.operations[0] == AttachmentOperation::Load_Clear ? WGPULoadOp_Clear : WGPULoadOp_Undefined;
            attachment.storeOp = att.operations[1] == AttachmentOperation::Store_Store ? WGPUStoreOp_Store : WGPUStoreOp_Discard;
            attachment.view = webgpu_fb->_images[ref.attachment_index]->wgpu_texture_view;
            attachment.resolveTarget = nullptr;
            attachment.nextInChain = nullptr;
            attachment_count += 1;
        }

        ICE_ASSERT_CORE(subpass.depth_stencil_attachment.layout != ImageLayout::DepthStencil);
        // WGPURenderPassDepthStencilAttachment depthstencil;
        // if (subpass.depth_stencil_attachment.layout != ImageLayout::DepthStencil)
        // {
        //     RenderAttachment const& att = webgpu_rp->attachments[subpass.depth_stencil_attachment.attachment_index];

        //     ice::vec4f clear_value =  clear_values[attachment_count];
        //     attachment.clearValue = { (double)clear_value.x, (double)clear_value.y, (double)clear_value.z, (double)clear_value.w };
        //     attachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
        //     attachment.loadOp = att.operations[0] == AttachmentOperation::Load_Clear ? WGPULoadOp_Clear : WGPULoadOp_Undefined;
        //     attachment.storeOp = att.operations[1] == AttachmentOperation::Store_Store ? WGPUStoreOp_Store : WGPUStoreOp_Discard;
        //     attachment.view = webgpu_fb->_images[subpass.depth_stencil_attachment.attachment_index]->wgpu_texture_view;
        //     attachment.resolveTarget = nullptr;
        //     attachment.nextInChain = nullptr;
        //     attachment_count += 1;
        // }

        WGPURenderPassDescriptor descriptor{};
        descriptor.label = "Default Renderpass";
        descriptor.colorAttachmentCount = attachment_count;
        descriptor.colorAttachments = attachments;
        descriptor.depthStencilAttachment = nullptr;
        descriptor.timestampWrites = nullptr;
        webgpu_cmds->renderpass_encoder = wgpuCommandEncoderBeginRenderPass(webgpu_cmds->command_encoder, &descriptor);

        // for (WebGPUImage const* image : webgpu_fb->_images)
        // {
        //     WGPURenderPassColorAttachment& attachment = attachments[attachment_count];
        //     ice::vec4f clear_value =  clear_values[attachment_count];
        //     attachment.clearValue = { (double)clear_value.x, (double)clear_value.y, (double)clear_value.z, (double)clear_value.w };
        //     attachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
        //     attachment.loadOp = WGPULoadOp_Clear;
        //     attachment.storeOp = WGPUStoreOp_Store;
        //     attachment.view = image->wgpu_texture_view;
        //     attachment.resolveTarget = nullptr;
        //     attachment.nextInChain = nullptr;
        //     attachment_count += 1;
        // }

        webgpu_cmds->framebuffer = webgpu_fb;
        webgpu_cmds->renderpass = webgpu_rp;
        webgpu_cmds->renderpass_subpass = 0;
    }

    void next_subpass(
        ice::render::CommandBuffer cmds,
        ice::render::SubPassContents contents
    ) noexcept
    {
        WebGPUCommandBuffer* webgpu_cmds = WebGPUCommandBuffer::native(cmds);
        WebGPUFrameBuffer const* webgpu_fb = webgpu_cmds->framebuffer;
        WebGPURenderPass const* webgpu_rp = webgpu_cmds->renderpass;

        wgpuRenderPassEncoderEnd(webgpu_cmds->renderpass_encoder);

        // Max 4 attachments for now
        ice::ucount attachment_count = 0;
        WGPURenderPassColorAttachment attachments[4];

        RenderSubPass const& subpass = webgpu_rp->subpasses[webgpu_cmds->renderpass_subpass];
        for (AttachmentReference const& ref : subpass.color_attachments)
        {
            RenderAttachment const& att = webgpu_rp->attachments[ref.attachment_index];

            WGPURenderPassColorAttachment& attachment = attachments[attachment_count];
            attachment.clearValue = { (double)0.3, (double)0.3, (double)0.3, (double)1 };
            attachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
            attachment.loadOp = att.operations[0] == AttachmentOperation::Load_Clear ? WGPULoadOp_Clear : WGPULoadOp_Undefined;
            attachment.storeOp = att.operations[1] == AttachmentOperation::Store_Store ? WGPUStoreOp_Store : WGPUStoreOp_Discard;
            attachment.view = webgpu_fb->_images[ref.attachment_index]->wgpu_texture_view;
            attachment.resolveTarget = nullptr;
            attachment.nextInChain = nullptr;
            attachment_count += 1;
        }

        ICE_ASSERT_CORE(subpass.depth_stencil_attachment.layout != ImageLayout::DepthStencil);

        WGPURenderPassDescriptor descriptor{};
        descriptor.label = "Default Renderpass";
        descriptor.colorAttachmentCount = attachment_count;
        descriptor.colorAttachments = attachments;
        descriptor.depthStencilAttachment = nullptr;
        descriptor.timestampWrites = nullptr;
        webgpu_cmds->renderpass_encoder = wgpuCommandEncoderBeginRenderPass(webgpu_cmds->command_encoder, &descriptor);
        webgpu_cmds->renderpass_subpass += 1;
    }

    void WebGPUCommands::set_viewport(
        ice::render::CommandBuffer cmds,
        ice::vec4u viewport_rect
    ) noexcept
    {
        WebGPUCommandBuffer* webgpu_cmds = WebGPUCommandBuffer::native(cmds);
        wgpuRenderPassEncoderSetViewport(
            webgpu_cmds->renderpass_encoder,
            viewport_rect.x, viewport_rect.y,
            viewport_rect.z, viewport_rect.w,
            0.0, 1.f
        );
    }

    void WebGPUCommands::set_scissor(
        ice::render::CommandBuffer cmds,
        ice::vec4u scissor_rect
    ) noexcept
    {
        WebGPUCommandBuffer* webgpu_cmds = WebGPUCommandBuffer::native(cmds);
        wgpuRenderPassEncoderSetScissorRect(
            webgpu_cmds->renderpass_encoder,
            scissor_rect.x, scissor_rect.y,
            scissor_rect.z, scissor_rect.w
        );
    }

    void WebGPUCommands::bind_pipeline(
        ice::render::CommandBuffer cmds,
        ice::render::Pipeline pipeline
    ) noexcept
    {
        WebGPUCommandBuffer* webgpu_cmds = WebGPUCommandBuffer::native(cmds);
        wgpuRenderPassEncoderSetPipeline(webgpu_cmds->renderpass_encoder, WebGPUPipeline::native(pipeline));
    }

    void WebGPUCommands::bind_resource_set(
        ice::render::CommandBuffer cmds,
        ice::render::PipelineLayout pipeline_layout,
        ice::render::ResourceSet resource_set,
        ice::u32 first_set
    ) noexcept
    {
        WebGPUCommandBuffer* webgpu_cmds = WebGPUCommandBuffer::native(cmds);
        wgpuRenderPassEncoderSetBindGroup(
            webgpu_cmds->renderpass_encoder,
            first_set,
            WebGPUResourceSet::native(resource_set)->_wgpu_group,
            0, nullptr
        );
    }

    void WebGPUCommands::bind_index_buffer(
        ice::render::CommandBuffer cmds,
        ice::render::Buffer buffer
    ) noexcept
    {
        WebGPUCommandBuffer* webgpu_cmds = WebGPUCommandBuffer::native(cmds);
        WebGPUBuffer* native_buffer = WebGPUBuffer::native(buffer);
        wgpuRenderPassEncoderSetIndexBuffer(
            webgpu_cmds->renderpass_encoder,
            native_buffer->wgpu_buffer,
            WGPUIndexFormat_Uint16,
            0, native_buffer->size
        );
    }

    void WebGPUCommands::bind_vertex_buffer(
        ice::render::CommandBuffer cmds,
        ice::render::Buffer buffer,
        ice::u32 binding
    ) noexcept
    {
        WebGPUCommandBuffer* webgpu_cmds = WebGPUCommandBuffer::native(cmds);
        WebGPUBuffer* native_buffer = WebGPUBuffer::native(buffer);
        wgpuRenderPassEncoderSetVertexBuffer(
            webgpu_cmds->renderpass_encoder,
            binding,
            native_buffer->wgpu_buffer,
            0, native_buffer->size
        );
    }

    void WebGPUCommands::draw(
        ice::render::CommandBuffer cmds,
        ice::u32 vertex_count,
        ice::u32 instance_count,
        ice::u32 vertex_offset,
        ice::u32 instance_offset
    ) noexcept
    {
        WebGPUCommandBuffer* webgpu_cmds = WebGPUCommandBuffer::native(cmds);
        wgpuRenderPassEncoderDraw(
            webgpu_cmds->renderpass_encoder,
            vertex_count,
            instance_count,
            vertex_offset,
            instance_offset
        );
    }

    void WebGPUCommands::draw_indexed(
        ice::render::CommandBuffer cmds,
        ice::u32 vertex_count,
        ice::u32 instance_count
    ) noexcept
    {
    }

    void WebGPUCommands::draw_indexed(
        ice::render::CommandBuffer cmds,
        ice::u32 vertex_count,
        ice::u32 instance_count,
        ice::u32 index_offset,
        ice::u32 vertex_offset,
        ice::u32 instance_offset
    ) noexcept
    {
        WebGPUCommandBuffer* webgpu_cmds = WebGPUCommandBuffer::native(cmds);
        wgpuRenderPassEncoderDrawIndexed(
            webgpu_cmds->renderpass_encoder,
            vertex_count,
            instance_count,
            index_offset,
            vertex_offset,
            instance_offset
        );
    }

    void WebGPUCommands::end_renderpass(
        ice::render::CommandBuffer cmds
    ) noexcept
    {
        WebGPUCommandBuffer* webgpu_cmds = WebGPUCommandBuffer::native(cmds);
        wgpuRenderPassEncoderEnd(webgpu_cmds->renderpass_encoder);
    }

    void WebGPUCommands::end(
        ice::render::CommandBuffer cmds
    ) noexcept
    {
        WebGPUCommandBuffer* webgpu_cmds = WebGPUCommandBuffer::native(cmds);
        ICE_ASSERT_CORE(webgpu_cmds->command_buffer == nullptr);

        //wgpuCommandEncoderInsertDebugMarker(webgpu_cmds->command_encoder, "End");

        WGPUCommandBufferDescriptor descriptor{};
        descriptor.label = "Command Buffer";
        webgpu_cmds->command_buffer = wgpuCommandEncoderFinish(webgpu_cmds->command_encoder, &descriptor);
    }

    void WebGPUCommands::update_texture(
        ice::render::CommandBuffer cmds,
        ice::render::Image image,
        ice::render::Buffer image_contents,
        ice::vec2u extents
    ) noexcept
    {
        WebGPUCommandBuffer* webgpu_cmds = WebGPUCommandBuffer::native(cmds);

        [[maybe_unused]]
        ice::AlignResult result = ice::align_to(extents.x * 4, ice::ualign::b_256);
        ice::u32 bytes_per_row = extents.x * 4;

        ICE_LOG_IF(
            is_aligned(bytes_per_row, ice::ualign::b_256) == false,
            LogSeverity::Warning, LogTag::Engine,
            "Copy buffer to texture alignment issue! Row '{}' is not aligned to '256'",
            bytes_per_row
        );

        WGPUImageCopyBuffer source{};
        source.buffer = WebGPUBuffer::native(image_contents)->wgpu_buffer;

        source.layout.offset = 0;
        source.layout.bytesPerRow = bytes_per_row;
        source.layout.rowsPerImage = extents.y;

        WGPUImageCopyTexture destination{};
        destination.aspect = WGPUTextureAspect_Undefined;
        destination.mipLevel = 0;
        destination.origin = { 0, 0, 0 };
        destination.texture = WebGPUImage::native(image)->wgpu_texture;

        WGPUExtent3D extent{ extents.x, extents.y, 1 };
        wgpuCommandEncoderCopyBufferToTexture(
            webgpu_cmds->command_encoder,
            &source,
            &destination,
            &extent
        );
    }

} // namespace ice::render::webgpu
