/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "trait_tilemap.hxx"
#include "render/trait_render_tilemap.hxx"

#include <ice/engine_frame.hxx>
#include <ice/engine_runner.hxx>
#include <ice/world/world_trait_archive.hxx>
#include <ice/task.hxx>
#include <ice/asset_storage.hxx>
#include <ice/shard_payloads.hxx>

#include <ice/profiler.hxx>

namespace ice
{

#if 0
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
            ice::Array<ice::Span<ice::Tile>> layer_tiles{ frame.allocator() };

            {
                IPT_ZONE_SCOPED_NAMED("[Trait] TileMap :: Prepare render memory");

                ice::array::resize(layer_tiles, layer_count);

                for (ice::u32 idx = 0; idx < layer_count; ++idx)
                {
                    ice::TileLayer const& layer = tilemap.layers[idx];
                    ice::IceTileLayer_RenderInfo& layer_render_info = render_info.layers[idx];

                    layer_tiles[idx] = frame.storage().create_named_span<ice::Tile>(layer.name, layer.tile_count);
                    layer_render_info.tiles = layer_tiles[idx];
                    layer_render_info.visible = true;
                }
            }

            co_await runner.task_scheduler();

            {
                IPT_ZONE_SCOPED_NAMED("[Trait] TileMap :: Prepare render data");

                for (ice::u32 idx = 0; idx < layer_count; ++idx)
                {
                    ice::TileLayer const& layer = tilemap.layers[idx];
                    ice::Tile const* tiles = tilemap.tiles + layer.tile_offset;

                    ice::memcpy(ice::begin(layer_tiles[idx]), tiles, layer.tile_count * sizeof(ice::Tile));
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
        ice::String tilemap
    ) noexcept
    {
        IPT_ZONE_SCOPED_NAMED("[Trait] TileMap :: Load Tilemap");

        _requested_tilemap = tilemap;
    }

    auto IceWorldTrait_TileMap::load_tilemap_task(
        ice::String tilemap_name,
        ice::EngineRunner& runner
    ) noexcept -> ice::Task<>
    {
        ice::Asset result = runner.asset_storage().bind(ice::AssetCategory_TileMap, tilemap_name);
        ice::Data map_data = co_await result[AssetState::Loaded];
        if (map_data.location == nullptr)
        {
            ICE_LOG(
                ice::LogSeverity::Error, ice::LogTag::Game,
                "Failed to load tilemap asset!"
            );
            co_return;
        }

        ice::TileMap const* tilemap_ptr = reinterpret_cast<ice::TileMap const*>(map_data.location);
        ice::TileMapInstance tilemap_info{ .tilemap = tilemap_ptr };
        tilemap_info.physics_ids = _allocator.allocate<ice::PhysicsID>(tilemap_ptr->map_collision_count + 1);

        co_await runner.stage_next_frame();

        ice::TileMap const& tilemap = *tilemap_info.tilemap;
        ice::Span<ice::TileCollision const> tile_collisions = { tilemap.tile_collisions, tilemap.tile_collision_count };

        ice::vec2f temp_vertices[20];
        ice::PhysicsID* physics_ids = tilemap_info.physics_ids;
        for (ice::u32 layer_idx = 0; layer_idx < tilemap.layer_count; ++layer_idx)
        {
            ice::TileLayer const& layer = tilemap.layers[layer_idx];
            ice::Tile const* tiles = tilemap.tiles + layer.tile_offset;

            for (ice::u32 idx = 0; idx < layer.tile_count; ++idx)
            {
                ice::u32 const tile_x = (tiles[idx].offset & 0x0000'ffff) >> 0;
                ice::u32 const tile_y = (tiles[idx].offset & 0xffff'0000) >> 16;

                for (ice::TileCollision const& collision : tile_collisions)
                {
                    if (collision.tile_id == tiles[idx].tile_id)
                    {
                        ice::TileObject const* const object = tilemap.objects + collision.object_offset;

                        if (object->vertex_count > 0)
                        {
                            ice::memcpy(temp_vertices, tilemap.object_vertices + object->vertex_offset, sizeof(ice::vec2f) * object->vertex_count);

                            for (ice::u32 vtx = 0; vtx < object->vertex_count; ++vtx)
                            {
                                temp_vertices[vtx] = temp_vertices[vtx] / Constant_PixelsInMeter;
                            }

                            *physics_ids = _physics.create_static_body(
                                ice::vec2f{ tile_x * tilemap.tile_size.x, tile_y * tilemap.tile_size.y } / Constant_PixelsInMeter,
                                object->vertex_count,
                                temp_vertices
                            );
                            physics_ids += 1;
                        }
                    }
                }
            }
        }

        ice::array::push_back(_tilemaps, tilemap_info);

        ICE_ASSERT((physics_ids - tilemap_info.physics_ids) <= tilemap.map_collision_count, "Invalid number of collision objects created!");
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
            _allocator.deallocate(
                {
                    .location = instance.physics_ids,
                    .size = ice::size_of<PhysicsID> * instance.tilemap->map_collision_count,
                    .alignment = align_of<PhysicsID>
                }
            );
        }

        ice::array::clear(_tilemaps);
    }

    void IceWorldTrait_TileMap::on_update(
        ice::EngineFrame& frame,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        IPT_ZONE_SCOPED_NAMED("[Trait] TileMap :: Update");

        ice::String const* name;
        if (ice::shards::inspect_first(runner.previous_frame().shards(), ice::Shard_LoadTileMap, name))
        {
            portal.execute(load_tilemap_task(*name, runner));
        }

        if (ice::array::any(_tilemaps))
        {
            ice::TileMapInstance& tilemap_info = _tilemaps[0];

            ice::IceTileMap_RenderInfo* render_info = frame.storage().create_named_object<ice::IceTileMap_RenderInfo>("tilemap.render-info"_sid);
            render_info->tilemap = tilemap_info.tilemap;
            render_info->tilesize = tilemap_info.tilemap->tile_size;

            runner.execute_task(
                detail::task_prepare_rendering_data(frame, runner, *tilemap_info.tilemap, *render_info),
                EngineContext::LogicFrame
            );
        }
    }

    auto trait_tilemap_factory(
        [[maybe_unused]] void* userdata,
        ice::Allocator& alloc,
        ice::WorldTraitTracker const& trait_tracker
    ) noexcept -> ice::WorldTrait*
    {
        ice::WorldTrait_Physics2D* phx_trait = static_cast<ice::WorldTrait_Physics2D*>(
            trait_tracker.find_trait(ice::Constant_TraitName_PhysicsBox2D)
        );
        return alloc.create<IceWorldTrait_TileMap>(alloc, *phx_trait);
    }

    void register_trait_tilemap(ice::WorldTraitArchive& archive) noexcept
    {
        static constexpr ice::StringID trait_dependencies[]{
            ice::Constant_TraitName_PhysicsBox2D
        };

        archive.register_trait(
            ice::Constant_TraitName_Tilemap,
            ice::WorldTraitDescription
            {
                .factory = trait_tilemap_factory,
                .required_dependencies = trait_dependencies
            }
        );
    }
#endif

} // namespace ice
