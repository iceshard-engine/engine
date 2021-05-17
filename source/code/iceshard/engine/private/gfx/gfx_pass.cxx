#include <ice/gfx/gfx_pass.hxx>

#include <ice/render/render_pass.hxx>

namespace ice::gfx
{

    struct IceGfxSubPass
    {

    };

    class IceGfxStaticPass : public GfxPass
    {
    public:
        IceGfxStaticPass() noexcept;
        ~IceGfxStaticPass() noexcept override;
    };

    auto create_static_pass(
        ice::Allocator& alloc,
        ice::render::RenderDevice& render_device,
        ice::gfx::GfxPassInfo const& pass_description
    ) noexcept -> ice::UniquePtr<ice::gfx::GfxPass>
    {
        using namespace ice::render;

        RenderAttachment attachments[]{
            RenderAttachment{
                .format = ImageFormat::Invalid,
                .final_layout = ImageLayout::ShaderReadOnly,
                .type = AttachmentType::TextureImage,
                .operations = {
                    AttachmentOperation::Load_Clear,
                    AttachmentOperation::Store_DontCare,
                },
            },
            RenderAttachment{
                .format = ImageFormat::Invalid,
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
        return ice::make_unique_null<ice::gfx::GfxPass>();
    }

} // namespace ice::gfx
