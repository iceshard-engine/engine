#include "iceshard_gfx_runner_trait.hxx"
#include "iceshard_gfx_runner.hxx"
#include "iceshard_gfx_frame.hxx"

#include <ice/gfx/gfx_frame.hxx>

namespace ice::gfx
{

    IceGfxRunnerTrait::IceGfxRunnerTrait(ice::gfx::IceGfxRunner& runner) noexcept
        : _runner{ runner }
    {
    }

    void IceGfxRunnerTrait::on_activate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        static_cast<ice::gfx::IceGfxFrame&>(_runner.frame()).execute_task(_runner.task_setup_gfx_traits());
    }

    void IceGfxRunnerTrait::on_deactivate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        static_cast<ice::gfx::IceGfxFrame&>(_runner.frame()).execute_task(_runner.task_cleanup_gfx_traits());
    }

    void IceGfxRunnerTrait::gfx_context_setup(
        ice::gfx::GfxDevice& device,
        ice::gfx::GfxContext& context
    ) noexcept
    {
    }

    void IceGfxRunnerTrait::gfx_context_cleanup(
        ice::gfx::GfxDevice& device,
        ice::gfx::GfxContext& context
    ) noexcept
    {
    }

} // namespace ice::gfx
