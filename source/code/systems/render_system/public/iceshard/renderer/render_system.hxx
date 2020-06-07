#pragma once
#include <asset_system/asset.hxx>
#include <iceshard/renderer/render_types.hxx>

namespace iceshard::renderer
{

    class RenderSystem
    {
    public:
        virtual ~RenderSystem() noexcept = default;

        virtual void begin_frame() noexcept { }

        virtual void end_frame() noexcept { }


        virtual auto get_resource_set(
            core::stringid_arg_type name
        ) noexcept -> ResourceSet = 0;

        virtual auto create_resource_set(
            core::stringid_arg_type name,
            iceshard::renderer::RenderPipelineLayout layout,
            core::pod::Array<RenderResource> const& resources
        ) noexcept -> ResourceSet = 0;

        virtual void update_resource_set(
            core::stringid_arg_type name,
            core::pod::Array<RenderResource> const& resources
        ) noexcept = 0;

        virtual void destroy_resource_set(
            core::stringid_arg_type name
        ) noexcept = 0;


        virtual auto create_pipeline(
            core::stringid_arg_type name,
            RenderPipelineLayout layout,
            core::pod::Array<asset::AssetData> const& shader_assets
        ) noexcept -> Pipeline = 0;

        virtual void destroy_pipeline(
            core::stringid_arg_type name
        ) noexcept = 0;

        virtual void submit_command_buffer_v2(
            CommandBuffer buffer
        ) noexcept = 0;

        virtual auto renderpass(
            RenderPassFeatures features
        ) noexcept -> RenderPass = 0;

        virtual auto renderpass_command_buffer(
            RenderPassFeatures features
        ) noexcept -> CommandBuffer = 0;

        virtual auto current_framebuffer() noexcept -> iceshard::renderer::api::Framebuffer = 0;
    };

} // namespace iceshard::renderer

