/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/world/world_trait.hxx>

namespace ice::gfx
{

    class GfxDevice;
    class GfxFrame;

    class GfxTrait : public ice::WorldTrait
    {
    public:
        virtual void gfx_setup(
            ice::gfx::GfxFrame& gfx_frame,
            ice::gfx::GfxDevice& gfx_device
        ) noexcept { }

        virtual void gfx_cleanup(
            ice::gfx::GfxFrame& gfx_frame,
            ice::gfx::GfxDevice& gfx_device
        ) noexcept { }

        virtual void gfx_update(
            ice::EngineFrame const& engine_frame,
            ice::gfx::GfxFrame& gfx_frame,
            ice::gfx::GfxDevice& gfx_device
        ) noexcept { }

        virtual auto task_gfx_update(
            ice::EngineFrame const& engine_frame,
            ice::gfx::GfxFrame& gfx_frame,
            ice::gfx::GfxDevice& gfx_device
        ) noexcept -> ice::Task<>
        {
            gfx_update(engine_frame, gfx_frame, gfx_device);
            co_return;
        }
    };

} // namespace ice::gfx
