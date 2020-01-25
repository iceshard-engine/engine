#pragma once
#include <core/cexpr/stringid.hxx>
#include <iceshard/renderer/render_types.hxx>

namespace iceshard::renderer
{

    enum class RenderPassFeatures : uint32_t
    {
        None = 0x0,
        PostProcess = 0x1,
        DebugUI = 0x80,
    };

    enum class RenderPassStage : uint32_t
    {
        Geometry,
        PostProcess,
        DebugUI,
    };

} // namespace iceshard::renderer
