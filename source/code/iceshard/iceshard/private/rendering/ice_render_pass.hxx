#pragma once
#include <iceshard/renderer/render_system.hxx>
#include <iceshard/render/render_pass.hxx>

namespace iceshard
{

    class Frame;

    class Engine;

    class RenderStage;

    class IceRenderStage;

    class IceRenderPass : public RenderPass
    {
    public:
        IceRenderPass(
            core::allocator& alloc,
            iceshard::Engine& engine,
            core::pod::Array<iceshard::IceRenderStage*> render_stages
        ) noexcept;

        IceRenderPass(
            core::allocator& alloc,
            iceshard::Engine& engine,
            core::pod::Array<iceshard::IceRenderStage*> render_stages,
            core::pod::Array<IceRenderPass*> const& dependencies
        ) noexcept;

        ~IceRenderPass() noexcept;

        auto engine() noexcept -> iceshard::Engine&;

        auto render_system() noexcept -> iceshard::renderer::RenderSystem& override;

        auto handle() noexcept -> iceshard::renderer::api::RenderPass override;

        auto command_buffer() noexcept -> iceshard::renderer::api::CommandBuffer override;

        auto render_stage(core::stringid_arg_type name) noexcept -> RenderStage*;

        void execute(
            Frame& current
        ) noexcept;

    protected:
        void wait_finished() noexcept;

    private:
        core::allocator& _allocator;
        core::pod::Array<IceRenderPass*> _dependencies;

        iceshard::Engine& _engine;
        iceshard::renderer::RenderSystem& _native_render_system;

        core::pod::Hash<iceshard::IceRenderStage*> _render_stages;
        core::pod::Array<iceshard::IceRenderStage*> _render_stage_order;

        iceshard::renderer::RenderPass _render_pass;
        iceshard::renderer::CommandBuffer _render_pass_command_buffer;
    };

    auto create_default_renderpass(
        core::allocator& alloc,
        iceshard::Engine& engine,
        core::pod::Array<iceshard::IceRenderStage*> render_stages
    ) noexcept -> IceRenderPass*;

} // namespace iceshard
