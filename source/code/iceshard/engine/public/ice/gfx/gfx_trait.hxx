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
    };

} // namespace ice::gfx
