#pragma once
#include <ice/span.hxx>
#include <ice/render/render_image.hxx>
#include <ice/render/render_pipeline.hxx>

namespace ice::render
{

    enum class Renderpass : ice::uptr
    {
        Invalid = 0x0
    };

    enum class AttachmentType : ice::u32
    {
        Undefined = 0x0,
        SwapchainImage,
        TextureImage,
        DepthStencil,
    };

    enum class AttachmentOperation : ice::u32
    {
        Load_DontCare = 0x40,
        Load_Clear = 0x41,
        Store_DontCare = 0x80,
        Store_Store = 0x81,
    };

    enum class AccessFlags : ice::u32
    {
        ColorAttachmentWrite,
        InputAttachmentRead,
    };

    struct RenderAttachment
    {
        ice::render::ImageFormat format = ImageFormat::Invalid;
        ice::render::ImageLayout layout = ImageLayout::Undefined;
        ice::render::AttachmentType type = AttachmentType::Undefined;
        ice::render::AttachmentOperation operations[2]{
            AttachmentOperation::Load_DontCare,
            AttachmentOperation::Store_DontCare,
        };
        ice::render::AttachmentOperation stencil_operations[2]{
            AttachmentOperation::Load_DontCare,
            AttachmentOperation::Store_DontCare,
        };
    };

    struct AttachmentReference
    {
        ice::u32 attachment_index;
        ice::render::ImageLayout layout = ImageLayout::Undefined;
    };

    struct SubpassDependency
    {
        ice::u32 source_subpass;
        PipelineStage source_stage;
        AccessFlags source_access;

        ice::u32 destination_subpass;
        PipelineStage destination_stage;
        AccessFlags destination_access;
    };

    struct RenderSubPass
    {
        ice::Span<ice::render::AttachmentReference> input_attachments;
        ice::Span<ice::render::AttachmentReference> color_attachments;
        ice::render::AttachmentReference depth_stencil_attachment;
    };

    struct RenderpassInfo
    {
        ice::Span<ice::render::RenderAttachment> attachments;
        ice::Span<ice::render::RenderSubPass> subpasses;
        ice::Span<ice::render::SubpassDependency> dependencies;
    };

} // namespace ice::render
