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
        static_cast<ice::gfx::IceGfxFrame&>(_runner.frame()).add_task(_runner.task_setup_gfx_traits());
    }

    void IceGfxRunnerTrait::on_deactivate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        static_cast<ice::gfx::IceGfxFrame&>(_runner.frame()).add_task(_runner.task_cleanup_gfx_contexts());
        static_cast<ice::gfx::IceGfxFrame&>(_runner.frame()).add_task(_runner.task_cleanup_gfx_traits());
    }

    void IceGfxRunnerTrait::gfx_setup(
        ice::gfx::GfxFrame& frame,
        ice::gfx::GfxDevice& device
    ) noexcept
    {
    }

    void IceGfxRunnerTrait::gfx_cleanup(
        ice::gfx::GfxFrame& frame,
        ice::gfx::GfxDevice& device
    ) noexcept
    {
    }

} // namespace ice::gfx
