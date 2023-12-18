/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/gfx/gfx_types.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/render/render_fence.hxx>
#include <ice/render/render_declarations.hxx>
#include <ice/engine_types.hxx>
#include <ice/task_scheduler.hxx>
#include <ice/stringid.hxx>
#include <ice/shard.hxx>
#include <ice/span.hxx>

namespace ice::gfx::v3
{

    enum class GfxResourceType : uint8_t
    {
        Invalid = 0,
        RenderTarget,
        DepthStencil,
    };

    struct GfxResource
    {
        using TypeTag = ice::StrongValue;

        ice::uptr value;
    };

    struct GfxStage
    {
        virtual void draw(
            ice::EngineFrame const& frame,
            ice::render::CommandBuffer cmds,
            ice::render::RenderCommands& render_api
        ) const noexcept = 0;
    };

    struct GfxGraphStage
    {
        ice::StringID name;
        ice::Span<GfxResource const> inputs;
        ice::Span<GfxResource const> outputs;
        ice::Span<GfxResource const> depth_stencil;
    };

    struct GfxGraphPass
    {
        ice::StringID name;
        ice::Span<GfxGraphStage const> stages;
    };

    struct GfxGraph
    {
    public:
        virtual ~GfxGraph() noexcept = default;
        virtual auto get_framebuffer() const noexcept -> GfxResource = 0;
        virtual auto get_resource(ice::StringID_Arg name, GfxResourceType type) noexcept -> GfxResource = 0;
        virtual bool add_pass(GfxGraphPass const&) noexcept = 0;

        virtual auto passes() const noexcept -> ice::Span<GfxGraphPass const> = 0;
    };

    class GfxStageRegistry
    {
    public:
        virtual ~GfxStageRegistry() noexcept = default;

        //virtual auto transfer() noexcept -> ice::TaskStage<ice::render::CommandBuffer> = 0;

        virtual void add_stage(ice::StringID_Arg name, GfxStage const* stage) noexcept = 0;

        virtual void execute_stages(
            ice::EngineFrame const& frame,
            ice::StringID_Arg name,
            ice::render::CommandBuffer cmds,
            ice::render::RenderCommands& render_api
        ) const noexcept = 0;
    };

    struct GfxGraphRuntime
    {
    public:
        virtual ~GfxGraphRuntime() noexcept = default;

        virtual auto renderpass() const noexcept -> ice::render::Renderpass = 0;

        virtual bool execute(
            ice::EngineFrame const& frame,
            GfxStageRegistry const& stage_registry,
            ice::render::RenderFence& fence
        ) noexcept = 0;
    };

    auto create_graph(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<GfxGraph>;

    auto create_graph_runtime(
        ice::Allocator& alloc,
        GfxDevice& device,
        GfxGraph const& base_definition,
        GfxGraph const* dynamic_definition = nullptr
    ) noexcept -> ice::UniquePtr<GfxGraphRuntime>;

} // namespace ice::gfx::v3
