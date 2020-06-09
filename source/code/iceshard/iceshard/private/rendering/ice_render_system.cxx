#include "ice_render_system.hxx"
#include "render_stages/ice_rs_clear.hxx"
#include "render_stages/ice_rs_opaque.hxx"
#include "render_stages/ice_rs_postprocess.hxx"
#include "render_stages/ice_rs_debugui.hxx"

#include <core/pod/array.hxx>
#include <iceshard/engine.hxx>

#include <iceshard/renderer/render_commands.hxx>
#include <iceshard/renderer/render_pipeline.hxx>

namespace iceshard
{

    IceRenderSystem::IceRenderSystem(
        core::allocator& alloc,
        iceshard::Engine& engine
    ) noexcept
        : _engine{ engine }
        , _native_render_system{ engine.render_system() }
        , _allocator{ alloc }
        , _render_passes{ _allocator }
        , _render_pass_order{ _allocator }
    {
        core::pod::Array<IceRenderStage*> render_stages{ alloc };
        core::pod::array::push_back(render_stages, alloc.make<IceRS_Clear>(alloc));
        {
            auto* stage = alloc.make<IceRS_Opaque>(alloc);
            stage->add_system("isc.system.illumination"_sid);
            stage->add_system("isc.system.static-mesh-renderer"_sid);
            core::pod::array::push_back(render_stages, stage);
        }
        core::pod::array::push_back(render_stages, alloc.make<IceRS_PostProcess>(alloc));
        {
            auto* stage = alloc.make<IceRS_DebugUI>(alloc);
            stage->add_system("isc.system.debug-imgui"_sid);
            core::pod::array::push_back(render_stages, stage);
        }

        {
            auto* rp_default = create_default_renderpass(alloc, engine, std::move(render_stages));
            core::pod::array::push_back(
                _render_pass_order,
                rp_default
            );
            core::pod::hash::set(
                _render_passes,
                "isc.render-pass.default"_sid_hash,
                rp_default
            );
        }

        _native_render_system.create_resource_set(
            "static-mesh.3d"_sid,
            iceshard::renderer::RenderPipelineLayout::Default,
            { _allocator } // Empty array
        );
    }

    IceRenderSystem::~IceRenderSystem() noexcept
    {
        for (auto const& entry : _render_passes)
        {
            _allocator.destroy(entry.value);
        }
        _native_render_system.destroy_resource_set("static-mesh.3d"_sid);
    }

    void IceRenderSystem::begin_frame() const noexcept
    {
        _native_render_system.begin_frame();
    }

    void IceRenderSystem::end_frame() const noexcept
    {
        _native_render_system.end_frame();
    }

    void IceRenderSystem::execute_passes(Frame& current, Frame const&) noexcept
    {
        for (auto* render_pass : _render_pass_order)
        {
            render_pass->execute(current);
        }
    }

    auto IceRenderSystem::render_pass(core::stringid_arg_type name) noexcept -> RenderPass*
    {
        return core::pod::hash::get(_render_passes, core::hash(name), nullptr);
    }

} // namespace iceshard
