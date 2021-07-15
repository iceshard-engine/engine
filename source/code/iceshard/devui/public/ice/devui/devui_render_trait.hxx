#pragma once
#include <ice/gfx/gfx_pass.hxx>
#include <ice/world/world_trait.hxx>

namespace ice::devui
{

    class DevUITrait : public ice::WorldTrait
    {
    public:
        virtual auto gfx_stage_info() const noexcept -> ice::gfx::GfxStageInfo = 0;
        virtual auto gfx_stage_slot() const noexcept -> ice::gfx::GfxStageSlot = 0;
    };

} // namespace ice::devui
