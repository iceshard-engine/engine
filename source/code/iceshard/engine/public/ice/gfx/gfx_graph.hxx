/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/gfx/gfx_types.hxx>
#include <ice/gfx/gfx_graph_resource.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/span.hxx>

namespace ice::gfx
{

    struct GfxGraphStage
    {
        ice::StringID name;
        ice::Span<ice::gfx::GfxResource const> inputs;
        ice::Span<ice::gfx::GfxResource const> outputs;
        ice::Span<ice::gfx::GfxResource const> depth_stencil;
    };

    struct GfxGraphPass
    {
        ice::StringID name;
        ice::Span<ice::gfx::GfxGraphStage const> stages;
    };

    struct GfxGraph
    {
    public:
        virtual ~GfxGraph() noexcept = default;

        virtual auto get_framebuffer() const noexcept -> ice::gfx::GfxResource = 0;

        virtual auto get_resource(
            ice::StringID_Arg name,
            ice::gfx::GfxResourceType type
        ) noexcept -> ice::gfx::GfxResource = 0;

        virtual bool add_pass(ice::gfx::GfxGraphPass const&) noexcept = 0;

        virtual auto passes() const noexcept -> ice::Span<ice::gfx::GfxGraphPass const> = 0;
    };

    auto create_graph(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<GfxGraph>;

} // namespace ice::gfx
