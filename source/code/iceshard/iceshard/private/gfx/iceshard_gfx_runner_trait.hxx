/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once

namespace ice::gfx
{

#if 0
    class IceGfxRunner;
    class IceGfxRunnerTrait : public ice::gfx::GfxTrait
    {
    public:
        IceGfxRunnerTrait(ice::gfx::IceGfxRunner& runner) noexcept;

        void on_activate(
            ice::Engine& engine,
            ice::EngineRunner& runner,
            ice::WorldPortal& portal
        ) noexcept override;

        void on_deactivate(
            ice::Engine& engine,
            ice::EngineRunner& runner,
            ice::WorldPortal& portal
        ) noexcept override;

        void gfx_setup(
            ice::gfx::GfxFrame& frame,
            ice::gfx::GfxDevice& device
        ) noexcept override;

        void gfx_cleanup(
            ice::gfx::GfxFrame& frame,
            ice::gfx::GfxDevice& device
        ) noexcept override;

    private:
        ice::gfx::IceGfxRunner& _runner;
    };
#endif

} // namespace ice::gfx
