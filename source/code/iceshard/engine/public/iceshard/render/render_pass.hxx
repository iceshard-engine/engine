#pragma once
#include <iceshard/renderer/render_api.hxx>
#include <iceshard/renderer/render_system.hxx>

namespace iceshard
{

    class RenderStage;

    class RenderPass
    {
    public:
        virtual ~RenderPass() noexcept = default;

        virtual auto handle() noexcept -> iceshard::renderer::api::RenderPass = 0;

        virtual auto command_buffer() noexcept -> iceshard::renderer::api::CommandBuffer = 0;

        virtual auto render_system() noexcept -> iceshard::renderer::RenderSystem& = 0;

        virtual auto render_stage(core::stringid_arg_type name) noexcept -> RenderStage* = 0;
    };

} // namespace iceshard
