#pragma once
#include <iceshard/renderer/render_types.hxx>

namespace iceshard::renderer
{

    class RenderSystem
    {
    public:
        virtual ~RenderSystem() noexcept = default;

        virtual auto renderpass(RenderPassStage type) noexcept -> RenderPass = 0;

        virtual auto acquire_command_buffer(RenderPassStage stage) noexcept -> CommandBuffer = 0;
        virtual void submit_command_buffer(CommandBuffer) noexcept = 0;
    };

} // namespace iceshard::renderer

