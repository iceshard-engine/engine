/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/gfx/gfx_types.hxx>
#include <ice/gfx/gfx_graph_resource.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/span.hxx>

namespace ice::gfx
{

    //! \brief Represents a single stage in a render graph.
    //! \note On the RenderAPI represents one or multiple draw commands.
    struct GfxGraphStage
    {
        ice::StringID name;
        ice::Span<ice::gfx::GfxResource const> inputs;
        ice::Span<ice::gfx::GfxResource const> outputs;
        ice::Span<ice::gfx::GfxResource const> depth_stencil;
    };

    //! \brief Represents a single pass of multiple stages in full render graph.
    //! \note On the RenderAPI represents a sub-pass.
    struct GfxGraphPass
    {
        ice::StringID name;
        ice::Span<ice::gfx::GfxGraphStage const> stages;
    };

    struct GfxGraph
    {
    public:
        virtual ~GfxGraph() noexcept = default;

        //! \returns A resource-id representing the framebuffer object / image.
        //! \note This ID represents a proper swapchain immage every frame.
        virtual auto get_framebuffer() const noexcept -> ice::gfx::GfxResource = 0;

        //! \brief Creates or returns an existing resource-id of the type for the given name.
        //! \note Resources created this way are automatically handled by the graph-runtime.
        //! \returns Resource-id of the given resource.
        virtual auto get_resource(
            ice::StringID_Arg name,
            ice::gfx::GfxResourceType type
        ) noexcept -> ice::gfx::GfxResource = 0;

        //! \brief Adds a new pass to the graph.
        virtual bool add_pass(ice::gfx::GfxGraphPass const&) noexcept = 0;

        //! \returns All graph passes of the graph.
        virtual auto passes() const noexcept -> ice::Span<ice::gfx::GfxGraphPass const> = 0;
    };

    auto create_graph(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<GfxGraph>;

} // namespace ice::gfx
