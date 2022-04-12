#include "asset_tilemap.hxx"

#include <ice/game_tilemap.hxx>
#include <ice/memory/pointer_arithmetic.hxx>
#include <ice/asset_storage.hxx>

namespace ice
{


    bool asset_tilemap_loader(
        void*,
        ice::Allocator& alloc,
        ice::AssetStorage& asset_storage,
        ice::Metadata const& meta,
        ice::Data data,
        ice::Memory& out_data
    ) noexcept
    {
        ice::TileMap const* const asset_tilemap = reinterpret_cast<ice::TileMap const*>(data.location);

        ice::u32 required_size = sizeof(ice::TileMap) + alignof(ice::TileSet);
        required_size += sizeof(ice::TileSet) * asset_tilemap->tileset_count;

        void* load_memory = alloc.allocate(required_size, alignof(ice::TileMap));

        ice::TileMap* const tilemap = reinterpret_cast<ice::TileMap*>(load_memory);
        *tilemap = *asset_tilemap;

        tilemap->tilesets = reinterpret_cast<ice::TileSet const*>(
            ice::memory::ptr_add(data.location, static_cast<ice::u32>(reinterpret_cast<ice::uptr>(tilemap->tilesets)))
        );
        tilemap->layers = reinterpret_cast<ice::TileLayer const*>(
            ice::memory::ptr_add(data.location, static_cast<ice::u32>(reinterpret_cast<ice::uptr>(tilemap->layers)))
        );
        tilemap->tiles = reinterpret_cast<ice::Tile const*>(
            ice::memory::ptr_add(data.location, static_cast<ice::u32>(reinterpret_cast<ice::uptr>(tilemap->tiles)))
        );
        tilemap->objects = reinterpret_cast<ice::TileObject const*>(
            ice::memory::ptr_add(data.location, static_cast<ice::u32>(reinterpret_cast<ice::uptr>(tilemap->objects)))
        );
        tilemap->tile_collisions = reinterpret_cast<ice::TileCollision const*>(
            ice::memory::ptr_add(data.location, static_cast<ice::u32>(reinterpret_cast<ice::uptr>(tilemap->tile_collisions)))
        );
        tilemap->object_vertices = reinterpret_cast<ice::vec2f const*>(
            ice::memory::ptr_add(data.location, static_cast<ice::u32>(reinterpret_cast<ice::uptr>(tilemap->object_vertices)))
        );

        ice::u32 const tileset_asset_names_offset = static_cast<ice::u32>(
            reinterpret_cast<ice::uptr>(asset_tilemap->tilesets)
            + sizeof(ice::TileSet) * tilemap->tileset_count
        );

        ice::TileSet* resolved_tilesets = reinterpret_cast<ice::TileSet*>(ice::memory::ptr_align_forward(tilemap + 1, alignof(ice::TileSet)));
        for (ice::u32 idx = 0; idx < tilemap->tileset_count; ++idx)
        {
            ice::u32 const* asset_name_loc = reinterpret_cast<ice::u32 const*>(&tilemap->tilesets[idx].asset);
            ice::u32 const offset = asset_name_loc[0];
            ice::u32 const lenght = asset_name_loc[1];

            resolved_tilesets[idx] = tilemap->tilesets[idx];
            resolved_tilesets[idx].asset = ice::Utf8String{
                reinterpret_cast<ice::c8utf const*>(ice::memory::ptr_add(asset_tilemap, tileset_asset_names_offset + offset)),
                lenght
            };
        }

        tilemap->tilesets = resolved_tilesets;

        out_data.location = tilemap;
        out_data.size = sizeof(ice::TileMap);
        out_data.alignment = alignof(ice::TileMap);
        return true;
    }

} // namespace ice
