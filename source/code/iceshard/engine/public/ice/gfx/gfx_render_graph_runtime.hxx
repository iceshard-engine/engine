#pragma once
#include <ice/engine_types.hxx>
#include <ice/mem_allocator.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/render/render_declarations.hxx>
#include <ice/stringid.hxx>

namespace ice::gfx::v2
{

    struct GfxRenderParams
    {
        ice::EngineFrame const& frame;
    };

    struct GfxRenderGraphRuntime
    {
        virtual ~GfxRenderGraphRuntime() noexcept = default;

        virtual auto storage() noexcept -> ice::gfx::v2::GfxObjectStorage& = 0;

        virtual void bind_stage(ice::StringID_Arg identifier, ice::gfx::v2::GfxStage* stage) noexcept = 0;

        virtual void import_buffer(ice::StringID_Arg name, ice::render::Buffer buffer) noexcept = 0;
        virtual void import_image(ice::StringID_Arg name, ice::render::Image buffer) noexcept = 0;

        virtual void setup() noexcept = 0;
        virtual bool execute(
            ice::gfx::v2::GfxRenderParams const& params,
            ice::render::RenderFence& fence
        ) noexcept = 0;
    };

    auto create_default_rendergraph_runtime(
        ice::Allocator& alloc,
        ice::gfx::v2::GfxRenderGraph const& render_graph,
        ice::gfx::GfxDevice& gfx_device
    ) noexcept -> ice::UniquePtr<ice::gfx::v2::GfxRenderGraphRuntime>;

} // namespace ice::gfx::v2
