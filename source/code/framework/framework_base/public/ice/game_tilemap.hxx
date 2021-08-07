#pragma once
#include <ice/span.hxx>
#include <ice/stringid.hxx>
#include <ice/unique_ptr.hxx>
#include <ice/world/world_trait.hxx>

namespace ice
{

    enum class TileID : ice::u32
    {
        Invalid = 0xffff'ffff
    };

    struct Tile
    {
        ice::vec2f position;
        ice::TileID tile_id;
    };

    inline auto make_tileid(ice::u8 tilemap_idx, ice::u32 tile_x, ice::u32 tile_y) noexcept -> ice::TileID
    {
        ice::u32 result = 0x0000'3fff & tile_y;
        result <<= 14;
        result |= 0x0000'3fff & tile_x;
        result <<= 4;
        result |= 0x0000'000f & tilemap_idx;
        return static_cast<ice::TileID>(result);
    }

    struct TilePhysics
    {
        ice::f32 mass = 0.f;
    };

    struct TileRoom
    {
        ice::StringID name;
        ice::vec2f world_offset{ 0.f };
        ice::Span<ice::Tile const> tiles;
        ice::Span<ice::TilePhysics const> tiles_physics;
    };

    struct TileMap
    {
        ice::StringID name;
        ice::StringID tilesets[4];
        ice::Span<ice::TileRoom const> rooms;
    };

    class WorldTrait_Physics2D;
    class WorldTrait_TileMap : public ice::WorldTrait
    {
    public:
        virtual void set_tilesize(ice::vec2f tile_size) noexcept = 0;

        virtual void load_tilemap(ice::TileMap const& tilemap) noexcept = 0;
    };

    auto create_tilemap_trait(
        ice::Allocator& alloc,
        ice::WorldTrait_Physics2D& trait_physics
    ) noexcept -> ice::UniquePtr<ice::WorldTrait_TileMap>;

} // namespace ice
