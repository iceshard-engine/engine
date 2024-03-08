/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/gfx/gfx_graph.hxx>
#include <ice/container/array.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/task.hxx>

namespace ice::gfx
{

    // struct GfxGraphRuntimeStageList
    // {
    //     virtual ~GfxGraphRuntimeStageList() noexcept = default;
    // };

    // auto create_graph_runtime_stage_list(
    //     ice::Allocator& alloc
    // ) noexcept -> ice::UniquePtr<GfxGraphRuntimeStageList>;

    struct GfxGraphRuntime
    {
        virtual ~GfxGraphRuntime() noexcept = default;

        virtual auto renderpass() const noexcept -> ice::render::Renderpass = 0;

        virtual bool prepare(
            ice::gfx::GfxStages& stages,
            ice::gfx::GfxStageRegistry const& stage_registry,
            ice::Array<ice::Task<>>& out_tasks
        ) noexcept = 0;

        virtual bool execute(
            ice::EngineFrame const& frame,
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
