#pragma once
#include <core/cexpr/stringid.hxx>

namespace iceshard::renderer
{

    enum class RenderPassType : uint32_t
    {
        Forward,
        ForwardPostProcess,
    };

} // namespace iceshard::renderer
