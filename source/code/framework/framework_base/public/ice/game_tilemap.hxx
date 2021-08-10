#pragma once
#include <ice/span.hxx>
#include <ice/stringid.hxx>
#include <ice/unique_ptr.hxx>
#include <ice/world/world_trait.hxx>

namespace ice
{

    enum class Asset : ice::u64;

    // #todo rename to TileSetID as thats what it is.
    enum class TileID : ice::u32
    {
        Invalid = 0xffff'ffff
    };

    struct Tile
    {
        ice::u32 offset;
        ice::TileID tile_id;
    };

    struct TileLayer
    {
        ice::StringID name;
        ice::vec2u size;
        ice::u32 tile_count;
        ice::u32 tile_offset;
    };

    struct TileSet
    {
        ice::Asset asset;
        ice::vec2f element_size;
    };

    struct TileMap
    {
        ice::vec2f tile_size;

        ice::u32 tileset_count;
        ice::u32 layer_count;

        ice::TileSet const* tilesets;
        ice::TileLayer const* layers;
        ice::Tile const* tiles;
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

    class ModuleRegister;
    void register_asset_modules(
        ice::Allocator& alloc,
        ice::ModuleRegister& registry
    ) noexcept;

    auto make_tileid(
        ice::u8 tileset_idx,
        ice::u8 tile_flip,
        ice::u16 tile_x,
        ice::u16 tile_y
    ) noexcept -> ice::TileID;

} // namespace ice
