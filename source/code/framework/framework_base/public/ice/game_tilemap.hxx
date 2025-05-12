/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/math.hxx>
#include <ice/span.hxx>
#include <ice/shard.hxx>
#include <ice/stringid.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/world/world_trait.hxx>
#include <ice/asset_category.hxx>

namespace ice
{

    static constexpr ice::Shard Shard_LoadTileMap = "action/tilemap/load`ice::String const*"_shard;

    static constexpr ice::AssetCategory AssetCategory_TileMap = ice::make_asset_category("ice/framework/tile-map");

    static constexpr ice::StringID Constant_TraitName_Tilemap
        = "ice.base-framework.trait-tilemap"_sid;

    enum class TileSetID : ice::u32
    {
        Invalid = 0xffff'ffff
    };

    struct Tile
    {
        ice::u32 offset;
        ice::TileSetID tile_id;
    };

    struct TileCollision
    {
        ice::TileSetID tile_id;
        ice::u32 object_offset;
    };

    struct TileObject
    {
        ice::u32 vertex_offset;
        ice::u32 vertex_count;
    };

    struct TileLayer
    {
        ice::StringID name;
        ice::vec2u size;
        ice::u32 tile_offset;
        ice::u32 tile_count;
    };

    struct TileSet
    {
        ice::String asset;
        ice::vec2f element_size;
    };

    struct TileMap
    {
        ice::vec2f tile_size;

        ice::u32 tileset_count;
        ice::u32 layer_count;
        ice::u32 objects_count;
        ice::u32 tile_collision_count;
        ice::u32 map_collision_count;

        ice::TileSet const* tilesets;
        ice::TileLayer const* layers;
        ice::Tile const* tiles;
        ice::TileObject const* objects;
        ice::TileCollision const* tile_collisions;
        ice::vec2f const* object_vertices;
    };

    class WorldTrait_Physics2D;
#if 0
    class WorldTrait_TileMap : public ice::WorldTrait
    {
    public:
        virtual void load_tilemap(ice::String tilemap) noexcept = 0;
    };
#endif


    auto make_tileset_id(
        ice::u8 tileset_idx,
        ice::u8 tile_flip,
        ice::u16 tile_x,
        ice::u16 tile_y
    ) noexcept -> ice::TileSetID;

} // namespace ice
