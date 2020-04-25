#pragma once
#include <iceshard/renderer/render_types.hxx>

namespace iceshard::renderer
{

    //! \brief Pipeline layouts available when creating render pipelines.
    //! \todo Provide a description for the given layouts.
    enum class RenderPipelineLayout : uint32_t
    {
        //! \brief Default rendering pipeline allowing to draw colored meshes.
        //! * Each vertice consists of a position (vec3) and a color (vec3)
        //! * Each instance requires a model (mat4).
        Default,
        PostProcess,
        DebugUI,
    };

} // namespace iceshard::renderer
