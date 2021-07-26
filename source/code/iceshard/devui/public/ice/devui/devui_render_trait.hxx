#pragma once
#include <ice/gfx/gfx_pass.hxx>
#include <ice/gfx/gfx_trait.hxx>
#include <ice/world/world_trait.hxx>

namespace ice::devui
{

    class DevUITrait : public ice::gfx::GfxTrait
    {
    public:
        virtual auto gfx_stage_name() const noexcept -> ice::StringID = 0;
    };

} // namespace ice::devui
