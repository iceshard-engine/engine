#pragma once
#include <ice/stringid.hxx>
#include <ice/unique_ptr.hxx>
#include <ice/render/render_declarations.hxx>
#include <ice/render/render_command_buffer.hxx>

namespace ice::gfx
{

    struct GfxSubPassInfo
    {
        ice::Span<ice::StringID const> stage_names;
        ice::Span<ice::StringID const> color_attachments;
        ice::Span<ice::StringID const> input_attachments;
        ice::Span<ice::StringID const> depth_stencil_attachments;
    };

    struct GfxPassAttachmentInfo
    {
        ice::StringID name;
        ice::render::AttachmentType type;
        ice::render::ImageFormat format;
    };

    struct GfxPassInfo
    {
        ice::StringID name;
        ice::Span<ice::gfx::GfxPassAttachmentInfo const> attachments;
        ice::Span<ice::gfx::GfxSubPassInfo const> subpass_list;
    };


    class GfxStage;

    class GfxPass
    {
    public:
        virtual ~GfxPass() noexcept = default;

        virtual void add_stage(
            ice::StringID_Arg name,
            ice::gfx::GfxStage* stage
        ) noexcept = 0;

        [[deprecated("A gfx pass will soon no longer accept dynamic dependencies out of the box.")]]
        virtual void add_stage(
            ice::StringID_Arg name,
            ice::Span<ice::StringID const> dependencies,
            ice::gfx::GfxStage* stage
        ) noexcept = 0;
    };

    auto create_static_pass(
        ice::Allocator& alloc,
        ice::render::RenderDevice& render_device,
        ice::gfx::GfxPassInfo const& pass_description
    ) noexcept -> ice::UniquePtr<ice::gfx::GfxPass>;

} // namespace ice::gfx
