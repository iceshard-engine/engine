#pragma once
#include <ice/stringid.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/render/render_declarations.hxx>
#include <ice/render/render_command_buffer.hxx>
#include <ice/container/array.hxx>

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

        virtual auto stage_count() const noexcept -> ice::u32 = 0;

        virtual void query_stage_order(
            ice::Array<ice::StringID_Hash>& stage_order_out
        ) const noexcept = 0;
    };

    class GfxDynamicPass : public GfxPass
    {
    public:
        virtual void clear() noexcept = 0;

        virtual void add_stage(
            ice::StringID_Arg stage_name,
            ice::Span<ice::StringID const> dependencies
        ) noexcept = 0;

        template<typename... Deps>
        void add_stage(ice::StringID_Arg name, Deps... deps) noexcept
        {
            ice::StringID const dependency_names[]{ deps... };
            add_stage(name, { dependency_names + 0, sizeof...(Deps) });
        }

        inline void add_stage(
            ice::StringID_Arg name
        ) noexcept;
    };

    inline void GfxDynamicPass::add_stage(
        ice::StringID_Arg name
    ) noexcept
    {
        add_stage(name, { });
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
