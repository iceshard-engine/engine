#pragma once
#include <iceshard/renderer/render_types.hxx>

namespace iceshard::renderer
{

    //! \brief Pipeline layouts available when creating render pipelines.
    //! \todo Provide a description for the given layouts.
    enum class RenderPipelineLayout : uint32_t
    {
        Default,
        PostProcess,
        DebugUI,
    };

} // namespace iceshard::renderer
