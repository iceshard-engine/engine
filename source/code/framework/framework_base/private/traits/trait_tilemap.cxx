#include "trait_tilemap.hxx"
#include "render/trait_render_tilemap.hxx"

#include <ice/engine_frame.hxx>
#include <ice/engine_runner.hxx>
#include <ice/task_thread_pool.hxx>

#include <ice/profiler.hxx>

namespace ice
{

    namespace detail
    {

        //struct TileMapData
        //{
        //    ice::TileMap_v2* const tilemap_copy;
        //    ice::Memory tilemap_memory;
        //};

        //auto tilemap_copy(ice::Allocator& alloc, ice::TileMap_v2 const& source_tilemap) noexcept -> TileMapData
        //{
        //    ice::u32 total_size = alignof(TileRoom) + alignof(Tile);
        //    total_size += sizeof(ice::TileMap);
        //    total_size += sizeof(ice::TileRoom) * ice::size(source_tilemap.rooms);

        //    ice::u32 const room_count = ice::size(source_tilemap.rooms);

        //    for (ice::TileRoom const& room : source_tilemap.rooms)
        //    {
        //        total_size += sizeof(ice::Tile) * ice::size(room.tiles);
        //    }

        //    void* data = alloc.allocate(total_size, alignof(TileMap));

        //    TileMapData result{ .tilemap_copy = reinterpret_cast<ice::TileMap*>(data) };
        //    result.tilemap_copy->name = source_tilemap.name;
        //    result.tilemap_memory = ice::memory_block(data, total_size, alignof(TileMap));

        //    ice::memcpy(result.tilemap_copy->tilesets, source_tilemap.tilesets, sizeof(source_tilemap.tilesets));

        //    //ice::u32 room_count = ice::size(source_tilemap.rooms);
        //    TileRoom* tileroom_copy = reinterpret_cast<TileRoom*>(
        //        ice::memory::ptr_align_forward(
        //            result.tilemap_copy + 1,
        //            alignof(TileRoom)
        //        )
        //    );
        //    result.tilemap_copy->rooms = { tileroom_copy, room_count };

        //    for (ice::TileRoom const& room : source_tilemap.rooms)
        //    {
        //        tileroom_copy->name = room.name;
        //        tileroom_copy->world_offset = room.world_offset;
        //        tileroom_copy += 1;
        //    }

        //    ice::Tile* tiles = reinterpret_cast<ice::Tile*>(
        //        ice::memory::ptr_align_forward(
        //            tileroom_copy,
        //            alignof(Tile)
        //        )
        //    );
        //    tileroom_copy -= room_count;

        //    for (ice::TileRoom const& room : source_tilemap.rooms)
        //    {
        //        ice::u32 const tile_count = ice::size(room.tiles);

        //        ice::memcpy(tiles, room.tiles.data(), room.tiles.size_bytes());
        //        tileroom_copy->tiles = { tiles, tile_count };
        //        tileroom_copy->tiles_physics = { };

        //        tiles += tile_count;
        //        tileroom_copy += 1;
        //    }

        //    return result;
        //}

        auto task_prepare_rendering_data(
            ice::EngineFrame& frame,
            ice::EngineRunner& runner,
            ice::TileMap const& tilemap,
            ice::IceTileMap_RenderInfo& render_info
        ) noexcept -> ice::Task<>
        {
            ice::u32 const layer_count = tilemap.layer_count;
            ice::pod::Array<ice::Span<ice::Tile>> layer_tiles{ frame.allocator() };

            {
                IPT_ZONE_SCOPED_NAMED("[Trait] TileMap :: Prepare render memory");

                ice::pod::array::resize(layer_tiles, layer_count);

                for (ice::u32 idx = 0; idx < layer_count; ++idx)
                {
                    ice::TileLayer const& layer = tilemap.layers[idx];
                    ice::IceTileLayer_RenderInfo& layer_render_info = render_info.layers[idx];

                    layer_tiles[idx] = frame.create_named_span<ice::Tile>(layer.name, layer.tile_count);
                    layer_render_info.tiles = layer_tiles[idx];
                    layer_render_info.visible = true;
                }
            }

            co_await runner.thread_pool();

            {
                IPT_ZONE_SCOPED_NAMED("[Trait] TileMap :: Prepare render data");

                for (ice::u32 idx = 0; idx < layer_count; ++idx)
                {
                    ice::TileLayer const& layer = tilemap.layers[idx];
                    ice::Tile const* tiles = tilemap.tiles + layer.tile_offset;
                    ice::IceTileLayer_RenderInfo& layer_render_info = render_info.layers[idx];

                    ice::memcpy(layer_tiles[idx].data(), tiles, layer.tile_count * sizeof(ice::Tile));

                    //for (ice::Tile_v2& tile : tiles[idx])
                    //{
                    //    tile.position = tile.position + room.world_offset;
                    //}
                }
            }
        }

    }

    IceWorldTrait_TileMap::IceWorldTrait_TileMap(
        ice::Allocator& alloc,
        ice::WorldTrait_Physics2D& trait_physics
    ) noexcept
        : _allocator{ alloc }
        , _physics{ trait_physics }
        , _tilemaps{ _allocator }
        //, _tilemap_memory{ _allocator }
    {
    }

    void IceWorldTrait_TileMap::set_tilesize(ice::vec2f tile_size) noexcept
    {
        _tilesize = tile_size;
    }

    void IceWorldTrait_TileMap::load_tilemap(
        ice::TileMap const& tilemap
    ) noexcept
    {
        IPT_ZONE_SCOPED_NAMED("[Trait] TileMap :: Load Tilemap");

        //detail::TileMapData tilemap_data = detail::tilemap_copy(_allocator, tilemap);

        ice::pod::array::push_back(_tilemaps, ice::addressof(tilemap));
        //ice::pod::array::push_back(_tilemaps, tilemap_data.tilemap_copy);
        //ice::pod::array::push_back(_tilemap_memory, tilemap_data.tilemap_memory);
    }

    void IceWorldTrait_TileMap::on_activate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
    }

    void IceWorldTrait_TileMap::on_deactivate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        IPT_ZONE_SCOPED_NAMED("[Trait] TileMap :: Deactivate");

        //for (ice::Memory const& memory : _tilemap_memory)
        //{
        //    _allocator.destroy(memory.location);
        //}

        ice::pod::array::clear(_tilemaps);
        //ice::pod::array::clear(_tilemap_memory);
    }

    void IceWorldTrait_TileMap::on_update(
        ice::EngineFrame& frame,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        IPT_ZONE_SCOPED_NAMED("[Trait] TileMap :: Update");

        if (ice::pod::array::any(_tilemaps))
        {
            ice::TileMap const* tilemap = ice::pod::array::front(_tilemaps);

            if (_tilesize.x > 0 && _tilesize.y > 0)
            {
                ice::IceTileMap_RenderInfo* render_info = frame.create_named_object<ice::IceTileMap_RenderInfo>("tilemap.render-info"_sid);
                render_info->tilemap = tilemap;
                render_info->tilesize = _tilesize;

                runner.execute_task(
                    detail::task_prepare_rendering_data(frame, runner, *tilemap, *render_info),
                    EngineContext::LogicFrame
                );
            }
        }
    }

    auto create_tilemap_trait(
        ice::Allocator& alloc,
        ice::WorldTrait_Physics2D& trait_physics
    ) noexcept -> ice::UniquePtr<ice::WorldTrait_TileMap>
    {
        return ice::make_unique<ice::WorldTrait_TileMap, ice::IceWorldTrait_TileMap>(alloc, alloc, trait_physics);
    }

} // namespace ice
