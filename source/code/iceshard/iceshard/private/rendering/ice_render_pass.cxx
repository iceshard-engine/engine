#include "ice_render_pass.hxx"
#include "render_stages/ice_render_stage.hxx"

#include <core/pod/array.hxx>
#include <iceshard/engine.hxx>
#include <iceshard/renderer/render_pass.hxx>
#include <iceshard/render/render_stage.hxx>


namespace iceshard
{

    IceRenderPass::IceRenderPass(
        core::allocator& alloc,
        iceshard::Engine& engine,
        core::pod::Array<iceshard::IceRenderStage*> render_stages
    ) noexcept
        : _allocator{ alloc }
        , _dependencies{ alloc }
        , _engine{ engine }
        , _native_render_system{ _engine.render_system() }
        , _render_stages{ alloc }
        , _render_stage_order{ std::move(render_stages) }
    {
        _render_pass = _native_render_system.renderpass(
            renderer::RenderPassFeatures::PostProcess
        );

        _render_pass_command_buffer = _native_render_system.renderpass_command_buffer(
            renderer::RenderPassFeatures::PostProcess
        );

        for (auto* stage : _render_stage_order)
        {
            stage->prepare(*this);

            core::pod::hash::set(
                _render_stages,
                core::hash(stage->name()),
                stage
            );
        }
    }

    IceRenderPass::IceRenderPass(
        core::allocator& alloc,
        iceshard::Engine& engine,
        core::pod::Array<iceshard::IceRenderStage*> render_stages,
        core::pod::Array<IceRenderPass*> const& dependencies
    ) noexcept
        : IceRenderPass{ alloc, engine, std::move(render_stages) }
    {
        core::pod::array::push_back(
            _dependencies,
            dependencies
        );
    }

    IceRenderPass::~IceRenderPass() noexcept
    {
        for (auto* stage : _render_stage_order)
        {
            stage->cleanup(*this);
        }
        core::pod::array::clear(_render_stage_order);

        for (auto const& entry: _render_stages)
        {
            _allocator.destroy(entry.value);
        }
        core::pod::hash::clear(_render_stages);
    }

    auto IceRenderPass::engine() noexcept -> iceshard::Engine&
    {
        return _engine;
    }

    auto IceRenderPass::render_system() noexcept -> iceshard::renderer::RenderSystem&
    {
        return _native_render_system;
    }

    auto IceRenderPass::handle() noexcept -> renderer::api::RenderPass
    {
        return _render_pass;
    }

    auto IceRenderPass::command_buffer() noexcept -> iceshard::renderer::api::CommandBuffer
    {
        return _render_pass_command_buffer;
    }

    auto IceRenderPass::render_stage(core::stringid_arg_type name) noexcept -> RenderStage*
    {
        return core::pod::hash::get(_render_stages, core::hash(name), nullptr);
    }

    void IceRenderPass::execute(Frame& current) noexcept
    {
        for (auto* dependency : _dependencies)
        {
            dependency->wait_finished();
        }

        for (auto* stage : _render_stage_order)
        {
            stage->execute(current, *this);
        }
    }

    void IceRenderPass::wait_finished() noexcept
    {
    }

    auto create_default_renderpass(
        core::allocator& alloc,
        iceshard::Engine& engine,
        core::pod::Array<iceshard::IceRenderStage*> render_stages
    ) noexcept -> IceRenderPass*
    {
        return alloc.make<IceRenderPass>(alloc, engine, std::move(render_stages));;
    }

} // namespace iceshard
