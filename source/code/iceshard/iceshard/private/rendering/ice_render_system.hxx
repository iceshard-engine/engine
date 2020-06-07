#pragma once
#include <iceshard/renderer/render_system.hxx>
#include <iceshard/render/render_system.hxx>

#include <core/pod/collections.hxx>

#include "ice_render_pass.hxx"
#include "render_stages/ice_render_stage.hxx"

namespace iceshard
{

    class Frame;

    class IceRenderSystem final : public RenderSystem
    {
    public:
        IceRenderSystem(
            core::allocator& alloc,
            iceshard::Engine& engine
        ) noexcept;

        ~IceRenderSystem() noexcept;

        void begin_frame() const noexcept;
        void end_frame() const noexcept;

        void execute_passes(Frame& current, Frame const& previous) noexcept;

        auto render_pass(core::stringid_arg_type name) noexcept -> iceshard::RenderPass* override;

    private:
        iceshard::Engine& _engine;
        iceshard::renderer::RenderSystem& _native_render_system;

        core::allocator& _allocator;

        core::pod::Hash<iceshard::IceRenderPass*> _render_passes;
        core::pod::Array<iceshard::IceRenderPass*> _render_pass_order;
    };

} // namespace iceshard::render
