#pragma once
#include <ice/stringid.hxx>
#include <ice/unique_ptr.hxx>
#include <ice/render/render_declarations.hxx>
#include <ice/render/render_command_buffer.hxx>
#include <ice/pod/array.hxx>

#include <ice/gfx/gfx_stage.hxx>

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


    class GfxPass
    {
    public:
        virtual ~GfxPass() noexcept = default;

        virtual void query_stage_order(
            ice::pod::Array<ice::StringID_Hash>& stage_order_out
        ) const noexcept = 0;
    };

    class GfxDynamicPass : public GfxPass
    {
    public:
        virtual void clear() noexcept = 0;

        virtual void add_stages(
            ice::Span<ice::gfx::GfxStageInfo const> stage_info
        ) noexcept = 0;

        inline void add_stage(
            ice::gfx::GfxStageInfo stage_info
        ) noexcept;

        inline void add_stage(
            ice::StringID_Arg name,
            std::initializer_list<ice::StringID const> dependencies
        ) noexcept;
    };

    inline void GfxDynamicPass::add_stage(
        ice::StringID_Arg name,
        std::initializer_list<ice::StringID const> dependencies
    ) noexcept
    {
        add_stage(
            GfxStageInfo{
                .name = name,
                .dependencies = ice::Span<ice::StringID const>{ dependencies.begin(), dependencies.size() }
            }
        );
    }

    inline void GfxDynamicPass::add_stage(
        ice::gfx::GfxStageInfo stage_info
    ) noexcept
    {
        add_stages({ &stage_info, 1 });
    }

    auto create_static_pass(
        ice::Allocator& alloc,
        ice::render::RenderDevice& render_device,
        ice::gfx::GfxPassInfo const& pass_description
    ) noexcept -> ice::UniquePtr<ice::gfx::GfxPass>;

    auto create_dynamic_pass(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::gfx::GfxDynamicPass>;

} // namespace ice::gfx
