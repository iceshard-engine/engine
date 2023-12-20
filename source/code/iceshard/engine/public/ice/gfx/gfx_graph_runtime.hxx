/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/gfx/gfx_graph.hxx>
#include <ice/mem_unique_ptr.hxx>

namespace ice::gfx
{

    struct GfxGraphRuntime
    {
        virtual ~GfxGraphRuntime() noexcept = default;

        virtual auto renderpass() const noexcept -> ice::render::Renderpass = 0;

        virtual bool execute(
            ice::EngineFrame const& frame,
            ice::gfx::GfxStageRegistry const& stage_registry,
            ice::render::RenderFence& fence
        ) noexcept = 0;
    };

    auto create_graph_runtime(
        ice::Allocator& alloc,
        ice::gfx::GfxDevice& device,
        ice::gfx::GfxGraph const& base_definition,
        ice::gfx::GfxGraph const* dynamic_definition = nullptr
    ) noexcept -> ice::UniquePtr<GfxGraphRuntime>;

} // namespace ice::gfx
