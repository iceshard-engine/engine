#include "trait_tilemap.hxx"
#include "render/trait_render_tilemap.hxx"

#include <ice/engine_frame.hxx>

namespace ice
{

    IceWorldTrait_TileMap::IceWorldTrait_TileMap(
        ice::Allocator& alloc,
        ice::WorldTrait_Physics2D& trait_physics
    ) noexcept
        : _physics{ trait_physics }
        , _tilemaps{ alloc }
    {
    }

    void IceWorldTrait_TileMap::set_tilesize(ice::vec2f tile_size) noexcept
    {
        _tilesize = tile_size;
    }

    void IceWorldTrait_TileMap::prepare_tilemap(
        ice::TileMap const& tilemap
    ) noexcept
    {
        ice::pod::array::push_back(_tilemaps, ice::addressof(tilemap));
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
    }

    void IceWorldTrait_TileMap::on_update(
        ice::EngineFrame& frame,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        if (ice::pod::array::any(_tilemaps))
        {
            ice::TileMap const* tilemap = ice::pod::array::front(_tilemaps);

            if (_tilesize.x > 0 && _tilesize.y > 0)
            {
                ice::IceTileMap_RenderInfo* render_info = frame.create_named_object<ice::IceTileMap_RenderInfo>("tilemap.render-info"_sid);
                render_info->tilemap = tilemap;
                render_info->tilesize = _tilesize;
                render_info->tilerooms[0].visible = true;
                render_info->tilerooms[0].room_index = 0;
                render_info->tilerooms[0].room_offset = { 0, 0 };
            }

            //ice::Span<ice::Tile> frame_tiles = frame.create_named_span<ice::Tile>("tilemap.tiles"_sid, ice::size(tilemap->rooms[0].tiles));
            //frame.create_named_object<ice::Span<ice::Tile>>("tilemap.tiles.span"_sid, frame_tiles);
            //frame.create_named_object<ice::TileMap const*>("tilemap.tilemap"_sid, tilemap);

            //ice::memcpy(
            //    frame_tiles.data(),
            //    tilemap->rooms[0].tiles.data(),
            //    tilemap->rooms[0].tiles.size_bytes()
            //);
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
