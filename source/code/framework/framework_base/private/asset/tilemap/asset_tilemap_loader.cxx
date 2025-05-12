/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "asset_tilemap.hxx"

#include <ice/game_tilemap.hxx>
#include <ice/asset_storage.hxx>

namespace ice
{


    auto asset_tilemap_loader(
        void*,
        ice::Allocator& alloc,
        ice::AssetStorage& asset_storage,
        ice::Config const& meta,
        ice::Data data,
        ice::Memory& out_data
    ) noexcept -> ice::Task<bool>
    {
        ice::TileMap const* const asset_tilemap = reinterpret_cast<ice::TileMap const*>(data.location);

        ice::meminfo mi_tilemap = ice::meminfo_of<ice::TileMap>;
        mi_tilemap += ice::meminfo_of<ice::TileSet> * asset_tilemap->tileset_count;

        out_data = alloc.allocate(mi_tilemap);

        ice::TileMap* const tilemap = reinterpret_cast<ice::TileMap*>(out_data.location);
        *tilemap = *asset_tilemap;

        tilemap->tilesets = reinterpret_cast<ice::TileSet const*>(
            ice::ptr_add(data.location, std::bit_cast<ice::usize>(tilemap->tilesets))
        );
        tilemap->layers = reinterpret_cast<ice::TileLayer const*>(
            ice::ptr_add(data.location, std::bit_cast<ice::usize>(tilemap->layers))
        );
        tilemap->tiles = reinterpret_cast<ice::Tile const*>(
            ice::ptr_add(data.location, std::bit_cast<ice::usize>(tilemap->tiles))
        );
        tilemap->objects = reinterpret_cast<ice::TileObject const*>(
            ice::ptr_add(data.location, std::bit_cast<ice::usize>(tilemap->objects))
        );
        tilemap->tile_collisions = reinterpret_cast<ice::TileCollision const*>(
            ice::ptr_add(data.location, std::bit_cast<ice::usize>(tilemap->tile_collisions))
        );
        tilemap->object_vertices = reinterpret_cast<ice::vec2f const*>(
            ice::ptr_add(data.location, std::bit_cast<ice::usize>(tilemap->object_vertices))
        );

        ice::u32 const tileset_asset_names_offset = static_cast<ice::u32>(
            reinterpret_cast<ice::uptr>(asset_tilemap->tilesets)
            + sizeof(ice::TileSet) * tilemap->tileset_count
        );

        ice::TileSet* resolved_tilesets = reinterpret_cast<ice::TileSet*>(ice::align_to(tilemap + 1, ice::align_of<ice::TileSet>).value);
        for (ice::u32 idx = 0; idx < tilemap->tileset_count; ++idx)
        {
            ice::u32 const* asset_name_loc = reinterpret_cast<ice::u32 const*>(&tilemap->tilesets[idx].asset);
            ice::u32 const offset = asset_name_loc[0];
            ice::u32 const lenght = asset_name_loc[1];

            resolved_tilesets[idx] = tilemap->tilesets[idx];
            resolved_tilesets[idx].asset = ice::String{
                reinterpret_cast<char const*>(ice::ptr_add(asset_tilemap, { tileset_asset_names_offset + offset })),
                lenght
            };
        }

        tilemap->tilesets = resolved_tilesets;
        co_return true;
    }

} // namespace ice
