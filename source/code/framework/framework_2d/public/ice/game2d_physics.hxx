#pragma once
#include <ice/game2d_tilemap.hxx>
#include <ice/world/world_trait.hxx>
#include <ice/unique_ptr.hxx>
#include <ice/allocator.hxx>

namespace ice
{

    struct Clock;

    class AssetSystem;

    class Physics2DTrait : public ice::WorldTrait
    {
    public:
        virtual ~Physics2DTrait() noexcept override = default;

        virtual void load_tilemap_room(
            ice::TileRoom const& room,
            ice::Span<ice::TileMaterial const> rigid_materials
        ) noexcept = 0;
    };

    auto create_physics2d_trait(
        ice::Allocator& alloc,
        ice::Clock const& clock
    ) noexcept -> ice::UniquePtr<ice::Physics2DTrait>;

} // namespace ice
