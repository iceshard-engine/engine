#pragma once
#include <ice/unique_ptr.hxx>
#include <ice/world/world_trait.hxx>
#include <ice/gfx/gfx_types.hxx>
#include <ice/game2d_trait.hxx>

namespace ice
{

    static constexpr ice::f32 Constant_TileSize = 64.f;

    class Game2DTrait : public ice::WorldTrait
    {
    public:
        virtual ~Game2DTrait() noexcept override = default;
    };

    auto create_game2d_trait(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::Game2DTrait>;

} // namespace ice
