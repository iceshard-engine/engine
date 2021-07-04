#pragma once
#include <ice/stringid.hxx>
#include <ice/unique_ptr.hxx>
#include <ice/world/world_trait.hxx>
#include <ice/gfx/gfx_stage.hxx>

namespace ice
{

    class GameWorldTrait_Render : public ice::WorldTrait
    {
    public:
        virtual auto gfx_stage_infos() const noexcept -> ice::Span<ice::gfx::GfxStageInfo const> = 0;
        virtual auto gfx_stage_slots() const noexcept -> ice::Span<ice::gfx::GfxStageSlot const> = 0;
    };

    auto create_trait_render_gfx(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::GameWorldTrait_Render>;

    auto create_trait_render_clear(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::GameWorldTrait_Render>;

    auto create_trait_render_finish(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::GameWorldTrait_Render>;

} // namespace ice
