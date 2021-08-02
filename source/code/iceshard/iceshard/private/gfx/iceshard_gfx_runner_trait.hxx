#pragma once
#include <ice/gfx/gfx_trait.hxx>

namespace ice::gfx
{

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

} // namespace ice::gfx
