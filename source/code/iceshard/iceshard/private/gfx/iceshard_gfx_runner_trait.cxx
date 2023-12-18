/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "iceshard_gfx_runner_trait.hxx"
#include "iceshard_gfx_runner.hxx"
#include "iceshard_gfx_frame.hxx"

#include <ice/gfx/gfx_frame.hxx>

namespace ice::gfx
{

#if 0
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
        _runner.setup_traits();
    }

    void IceGfxRunnerTrait::on_deactivate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        _runner.cleanup_traits();
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
#endif

} // namespace ice::gfx
