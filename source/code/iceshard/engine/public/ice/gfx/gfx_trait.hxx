#pragma once
#include <ice/world/world_trait.hxx>

namespace ice::gfx
{

    class GfxDevice;
    class GfxContext;
    class GfxFrame;

    class GfxTrait : public ice::WorldTrait
    {
    public:
        virtual void gfx_context_setup(
            ice::gfx::GfxDevice& device,
            ice::gfx::GfxContext& context
        ) noexcept = 0;

        virtual void gfx_context_cleanup(
            ice::gfx::GfxDevice& device,
            ice::gfx::GfxContext& context
        ) noexcept = 0;

        virtual void gfx_update(
            ice::gfx::GfxDevice& device,
            ice::gfx::GfxContext& context,
            ice::gfx::GfxFrame& frame
        ) noexcept = 0;
    };

} // namespace ice::gfx
