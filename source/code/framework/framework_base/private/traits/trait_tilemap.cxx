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
    {
    }

    void IceWorldTrait_TileMap::load_tilemap(
        ice::TileMap const& tilemap
    ) noexcept
    {
        IPT_ZONE_SCOPED_NAMED("[Trait] TileMap :: Load Tilemap");

        ice::TileMapInstance instance{ ice::addressof(tilemap) };
        instance.fixture_count = tilemap.fixture_count;
        instance.fixture_ids = nullptr;

        ice::pod::array::push_back(_tilemaps, instance);
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

        for (ice::TileMapInstance const& instance : _tilemaps)
        {
            _allocator.destroy(instance.fixture_ids);
        }

        ice::pod::array::clear(_tilemaps);
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
            ice::TileMapInstance& tilemap_info = _tilemaps[0];

            if (tilemap_info.fixture_count > 0 && tilemap_info.fixture_ids == nullptr)
            {
                tilemap_info.fixture_ids = reinterpret_cast<ice::PhysicsID*>(_allocator.allocate(tilemap_info.fixture_count * sizeof(ice::PhysicsID)));

                ice::PhysicsID* fixture_id = tilemap_info.fixture_ids;

                ice::TileMap const& tilemap = *tilemap_info.tilemap;
                ice::TileTerrain const* terrain = tilemap.terrain;

                for (ice::u32 layer_idx = 0; layer_idx < tilemap.layer_count; ++layer_idx)
                {
                    ice::TileLayer const& layer = tilemap.layers[layer_idx];
                    ice::Tile const* layer_tiles = tilemap.tiles + layer.tile_offset;

                    for (ice::u32 tile_idx = 0; tile_idx < layer.tile_count; ++tile_idx)
                    {
                        for (ice::u32 terrain_idx = 0; terrain_idx < tilemap.terrain_count; ++terrain_idx)
                        {
                            if (layer_tiles->tile_id == terrain[terrain_idx].tile_id)
                            {
                                ice::f32 const tile_x = layer_tiles->offset & 0x0000'ffff;
                                ice::f32 const tile_y = (layer_tiles->offset & 0xffff'0000) >> 16;

                                *fixture_id = _physics.create_static_body(
                                    ice::vec2f{ tile_x * tilemap.tile_size.x, tile_y * tilemap.tile_size.y } / Constant_PixelsInMeter,
                                    ice::PhysicsShape::Box,
                                    tilemap.tile_size / Constant_PixelsInMeter
                                );
                                fixture_id += 1;
                                break;
                            }
                        }

                        layer_tiles += 1;
                    }
                }
            }

            ice::IceTileMap_RenderInfo* render_info = frame.create_named_object<ice::IceTileMap_RenderInfo>("tilemap.render-info"_sid);
            render_info->tilemap = tilemap_info.tilemap;
            render_info->tilesize = tilemap_info.tilemap->tile_size;

            runner.execute_task(
                detail::task_prepare_rendering_data(frame, runner, *tilemap_info.tilemap, *render_info),
                EngineContext::LogicFrame
            );
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
