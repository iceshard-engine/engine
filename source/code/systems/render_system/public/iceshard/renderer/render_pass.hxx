#pragma once
#include <core/cexpr/stringid.hxx>

namespace iceshard::renderer
{

    enum class RenderPassType
    {
        ForwardDebug,
        ForwardPostProcess,
    };

    struct RenderPass
    {
        RenderPassType type;
    };

    enum class RenderPassHandle : uintptr_t
    {
        Invalid = 0x0
    };

} // namespace iceshard::renderer
