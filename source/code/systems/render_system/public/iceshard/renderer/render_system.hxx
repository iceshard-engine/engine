#pragma once
#include <iceshard/renderer/render_types.hxx>

namespace iceshard::renderer
{

    class RenderSystem
    {
    public:
        virtual ~RenderSystem() noexcept = default;

        virtual auto renderpass(RenderPassStage type) noexcept -> RenderPass = 0;

        virtual auto create_resource_set(
            core::stringid_arg_type name,
            core::pod::Array<RenderResource> const& resources
        ) noexcept -> ResourceSet = 0;

        virtual void update_resource_set(
            core::stringid_arg_type name,
            core::pod::Array<RenderResource> const& resources
        ) noexcept = 0;

        virtual void destroy_resource_set(
            core::stringid_arg_type name
        ) noexcept = 0;

        virtual auto acquire_command_buffer(RenderPassStage stage) noexcept -> CommandBuffer = 0;
        virtual void submit_command_buffer(CommandBuffer) noexcept = 0;
    };

} // namespace iceshard::renderer

