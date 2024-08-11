/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include "webgpu_utils.hxx"
#include <ice/render/render_pipeline.hxx>
#include <ice/assert.hxx>

namespace ice::render::webgpu
{

    struct WebGPUPipeline
    {
        static auto handle(WGPURenderPipeline native) noexcept
        {
            return static_cast<ice::render::Pipeline>(reinterpret_cast<uintptr_t>(native));
        }

        static auto native(ice::render::Pipeline handle) noexcept
        {
            return reinterpret_cast<WGPURenderPipeline>(static_cast<uintptr_t>(handle));
        }

        // Layout
        static auto handle(WGPUPipelineLayout native) noexcept
        {
            return static_cast<ice::render::PipelineLayout>(reinterpret_cast<uintptr_t>(native));
        }

        static auto native(ice::render::PipelineLayout handle) noexcept
        {
            return reinterpret_cast<WGPUPipelineLayout>(static_cast<uintptr_t>(handle));
        }
    };

    inline auto primitive_topology(ice::render::PrimitiveTopology topology) noexcept
    {
        switch(topology)
        {
        case PrimitiveTopology::LineStrip: return WGPUPrimitiveTopology_LineStrip;
        case PrimitiveTopology::LineStripWithAdjency: return WGPUPrimitiveTopology_LineList;
        case PrimitiveTopology::TriangleList: return WGPUPrimitiveTopology_TriangleList;
        case PrimitiveTopology::TriangleStrip: return WGPUPrimitiveTopology_TriangleStrip;
        case PrimitiveTopology::TriangleFan: // [[fallthrough]]
        case PrimitiveTopology::PatchList: return WGPUPrimitiveTopology_Undefined;
        }
    }

} // namespace ice::render::webgpu
