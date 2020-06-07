#include "ice_rs_clear.hxx"

#include <iceshard/engine.hxx>
#include <iceshard/service_provider.hxx>

namespace iceshard
{
    IceRS_Clear::IceRS_Clear(core::allocator& alloc) noexcept
        : IceRenderStage{ alloc }
    {
    }

    void IceRS_Clear::on_prepare(
        iceshard::Engine& engine,
        iceshard::RenderPass& render_pass
    ) noexcept
    {
    }

    void IceRS_Clear::on_cleanup(
        iceshard::Engine& engine,
        iceshard::RenderPass& render_pass
    ) noexcept
    {
    }

} // namespace iceshard
