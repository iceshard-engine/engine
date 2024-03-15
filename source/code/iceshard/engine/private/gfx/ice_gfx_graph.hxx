/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/gfx/gfx_graph.hxx>
#include <ice/gfx/gfx_graph_runtime.hxx>
#include <ice/gfx/gfx_stage.hxx>
#include <ice/gfx/gfx_stage_registry.hxx>
#include <ice/gfx/gfx_context.hxx>
#include <ice/render/render_swapchain.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/task_scheduler.hxx>
#include <ice/mem_allocator_proxy.hxx>
#include "ice_gfx_graph_snapshot.hxx"

namespace ice::gfx
{

    static constexpr GfxResource Const_ResourceFrameBuffer{ 0x8000'0000 };

    class IceshardGfxGraph : public GfxGraph
    {
    public:
        IceshardGfxGraph(ice::Allocator& alloc) noexcept;
        ~IceshardGfxGraph() noexcept override = default;

        auto get_framebuffer() const noexcept -> GfxResource override;
        auto get_resource(ice::StringID_Arg name, GfxResourceType type) noexcept -> GfxResource override;
        bool add_pass(GfxGraphPass const& ) noexcept override;

        auto passes() const noexcept -> ice::Span<GfxGraphPass const> override;

    private:
        ice::HashMap<GfxGraphPass> _passes;
        ice::HashMap<GfxResource> _resources;
        ice::u32 _resources_ids[ice::u8(ice::gfx::GfxResourceType::DepthStencil) + 1];
    };

    enum class GraphStageType : ice::u8
    {
        Allocate,
        Release,
        Barrier,
        Draw,
    };

    struct GraphStage
    {
        GraphStageType type;
        union
        {
            ice::u32 barrier_idx;
            ice::StringID stage_name;
            GfxResource resource;
        };
    };

} // namespace ice::gfx
