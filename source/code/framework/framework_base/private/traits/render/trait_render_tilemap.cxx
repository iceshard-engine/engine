/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "trait_render_tilemap.hxx"
#include "../trait_camera.hxx"

#include <ice/game_tilemap.hxx>

#include <ice/engine.hxx>
#include <ice/engine_frame.hxx>
#include <ice/engine_runner.hxx>
#include <ice/task.hxx>
#include <ice/task_utils.hxx>
#include <ice/world/world_trait_archive.hxx>

#include <ice/gfx/gfx_device.hxx>
#include <ice/gfx/gfx_frame.hxx>
#include <ice/gfx/gfx_resource_tracker.hxx>

#include <ice/render/render_device.hxx>
#include <ice/render/render_pipeline.hxx>
#include <ice/render/render_resource.hxx>
#include <ice/render/render_buffer.hxx>
#include <ice/render/render_shader.hxx>
#include <ice/render/render_image.hxx>
#include <ice/render/render_pass.hxx>

#include <ice/resource_meta.hxx>
#include <ice/asset_storage.hxx>
#include <ice/asset.hxx>

#include <ice/profiler.hxx>

namespace ice
{

#if 0
    namespace detail
    {
        struct TileSet_ShaderData_Properties
        {
            ice::vec2f tile_scale;
            ice::vec2f tile_size;
        };

        struct TileMap_DrawOperation
        {
            ice::TileMap const* tilemap;
            ice::IceTileMap_RenderInfo const* render_info;
            ice::IceTileMap_RenderCache const* render_cache;
        };

        struct TileMap_LoadImageOperation
        {
            ice::TileMap const* tilemap;
            ice::IceTileMap_RenderCache* render_cache;

            ice::vec2f tile_render_size;

            ice::u32 image_count;
            ice::render::Image image_handle[4];
            ice::Metadata image_metadata[4];
        };

        auto load_tilemap_shader(ice::AssetStorage& assets, ice::String name) noexcept -> ice::Task<ice::Data>
        {
            ice::Asset asset = assets.bind(ice::render::AssetType_Shader, name);
            co_return co_await asset[AssetState::Baked];
        }

        auto task_update_tilemap_images(
            ice::EngineRunner& runner,
            ice::render::ResourceSetLayout resourceset_layout,
            TileMap_LoadImageOperation operation
        ) noexcept -> ice::Task<>
        {
            ice::TileMap const& tilemap = *operation.tilemap;
            ice::IceTileMap_RenderCache& cache = *operation.render_cache;

            ice::vec2u image_extent[4];
            ice::detail::TileSet_ShaderData_Properties properties[4];

            for (ice::u32 idx = 0; idx < tilemap.tileset_count; ++idx)
            {
                ice::String const asset_name = tilemap.tilesets[idx].asset;

                Asset image_asset = runner.asset_storage().bind(ice::render::AssetType_Texture2D, asset_name);
                Data image_data = co_await image_asset[AssetState::Loaded];
                //ICE_ASSERT(asset_check(image_data, AssetState::Loaded), "Shader not available!");
                ice::render::ImageInfo const* image_info = reinterpret_cast<ice::render::ImageInfo const*>(image_data.location);

                ice::Metadata const& metadata = image_asset.metadata();

                ice::i32 tile_width;
                ice::i32 tile_height;
                bool meta_valid = true;
                meta_valid &= meta_read_int32(metadata, "tileset.tile.width"_sid, tile_width);
                meta_valid &= meta_read_int32(metadata, "tileset.tile.height"_sid, tile_height);

                if (meta_valid == false)
                {
                    ICE_LOG(
                        ice::LogSeverity::Error, ice::LogTag::Engine,
                        "The asset {} does not provide `tile` specific metadata!",
                        asset_name
                    );
                    co_return;
                }

                image_extent[idx] = { image_info->width, image_info->height };

                ice::vec2f const tileset_size = { ice::f32(image_info->width), ice::f32(image_info->height) };
                ice::vec2f const tile_size = { ice::f32(tile_width), ice::f32(tile_height) };

                properties[idx].tile_size = operation.tile_render_size;
                properties[idx].tile_scale = { tile_size.x / tileset_size.x, tile_size.y / tileset_size.y };

                image_data = co_await image_asset[AssetState::Runtime];
                cache.tileset_images[idx] = *reinterpret_cast<ice::render::Image const*>(image_data.location);
                cache.image_count += 1;
            }

            co_await runner.stage_next_frame();
            co_await runner.graphics_frame().frame_begin();

            ice::gfx::GfxDevice& gfx_device = runner.graphics_device();
            ice::render::RenderDevice& device = gfx_device.device();

            ice::render::ResourceSet final_resourceset;
            device.create_resourcesets({ &resourceset_layout, 1 }, { &final_resourceset, 1 });

            ice::u32 buffer_update_count = 0;
            ice::render::BufferUpdateInfo buffer_updates[4]{ };

            for (ice::u32 idx = 0; idx < cache.image_count; ++idx)
            {
                cache.tileset_properties[idx] = device.create_buffer(ice::render::BufferType::Uniform, sizeof(TileSet_ShaderData_Properties));

                buffer_updates[buffer_update_count] = ice::render::BufferUpdateInfo
                {
                    .buffer = cache.tileset_properties[idx],
                    .data = ice::data_view(properties[idx])
                };

                buffer_update_count += 1;
            }

            device.update_buffers({ buffer_updates, buffer_update_count });

            ice::u32 resource_update_count = 0;
            ice::render::ResourceUpdateInfo resource_updates[8]{ };

            for (ice::u32 idx = 0; idx < 4; ++idx)
            {
                ice::render::Image image = cache.tileset_images[idx];
                if (idx >= cache.image_count)
                {
                    image = cache.tileset_images[cache.image_count - 1];
                }

                resource_updates[resource_update_count] = ice::render::ResourceUpdateInfo
                {
                    .image = image
                };
                resource_update_count += 1;
            }

            for (ice::u32 idx = 0; idx < 4; ++idx)
            {
                ice::render::Buffer buffer = cache.tileset_properties[idx];
                if (idx >= cache.image_count)
                {
                    buffer = cache.tileset_properties[cache.image_count - 1];
                }

                resource_updates[resource_update_count] = ice::render::ResourceUpdateInfo
                {
                    .uniform_buffer = ice::render::ResourceBufferInfo{
                        .buffer = buffer,
                        .offset = 0,
                        .size = sizeof(TileSet_ShaderData_Properties)
                    }
                };
                resource_update_count += 1;
            }

            ice::render::ResourceSetUpdateInfo set_updates[]{
                ice::render::ResourceSetUpdateInfo
                {
                    .resource_set = final_resourceset,
                    .resource_type = ice::render::ResourceType::SampledImage,
                    .binding_index = 3,
                    .array_element = 0,
                    .resources = { resource_updates + 0, 4 },
                },
                ice::render::ResourceSetUpdateInfo
                {
                    .resource_set = final_resourceset,
                    .resource_type = ice::render::ResourceType::UniformBuffer,
                    .binding_index = 4,
                    .array_element = 0,
                    .resources = { resource_updates + 4, 4 },
                }
            };

            device.update_resourceset(set_updates);

            co_await runner.graphics_frame().frame_end();

            cache.tileset_resourceset[0] = final_resourceset;
            co_return;
        }

    } // namespace detail

    IceWorldTrait_RenderTilemap::IceWorldTrait_RenderTilemap(
        ice::Allocator& alloc
    ) noexcept
        : _allocator{ alloc }
        , _render_cache{ _allocator }
    {
        ice::hashmap::reserve(_render_cache, 10);
    }

    void IceWorldTrait_RenderTilemap::on_activate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        _asset_system = ice::addressof(engine.asset_storage());
        _shader_data[0] = ice::wait_for(detail::load_tilemap_shader(*_asset_system, Tilemap_VtxShader));
        _shader_data[1] = ice::wait_for(detail::load_tilemap_shader(*_asset_system, Tilemap_PixShader));
    }

    void IceWorldTrait_RenderTilemap::on_deactivate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        for (IceTileMap_RenderCache* cache_ptr : _render_cache)
        {
            portal.allocator().destroy(cache_ptr);
        }
        ice::hashmap::clear(_render_cache);

        _asset_system = nullptr;
    }

    void IceWorldTrait_RenderTilemap::on_update(
        ice::EngineFrame& frame,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        IPT_ZONE_SCOPED_NAMED("[GfxTrait] TileMap :: Update");

        ice::IceTileMap_RenderInfo const* tilemap_render = frame.storage().named_object<ice::IceTileMap_RenderInfo>("tilemap.render-info"_sid);
        if (tilemap_render != nullptr)
        {
            ice::TileMap const* tilemap = tilemap_render->tilemap;
            ice::IceTileMap_RenderCache* render_cache = ice::hashmap::get(_render_cache, ice::hash_from_ptr(tilemap), nullptr);

            if (render_cache != nullptr)
            {
                if (render_cache->tileset_resourceset[0] != ice::render::ResourceSet::Invalid)
                {
                    detail::TileMap_DrawOperation* operation = frame.storage().create_named_object<detail::TileMap_DrawOperation>("tilemap_render.draw_operation"_sid);
                    operation->tilemap = tilemap;
                    operation->render_info = tilemap_render;
                    operation->render_cache = render_cache;
                }
            }
            else
            {

                render_cache = portal.allocator().create<ice::IceTileMap_RenderCache>();
                render_cache->tileset_resourceset[0] = ice::render::ResourceSet::Invalid;

                ice::hashmap::set(
                    _render_cache,
                    ice::hash_from_ptr(tilemap),
                    render_cache
                );

                detail::TileMap_LoadImageOperation operation{ };
                operation.tilemap = tilemap;
                operation.render_cache = render_cache;
                operation.tile_render_size = tilemap_render->tilesize;

                portal.execute(ice::detail::task_update_tilemap_images(runner, _resource_set_layouts[1], operation));
            }
        }
    }

    void IceWorldTrait_RenderTilemap::gfx_setup(
        ice::gfx::GfxFrame& gfx_frame,
        ice::gfx::GfxDevice& gfx_device
    ) noexcept
    {

        using namespace ice::gfx;
        using namespace ice::render;

        Renderpass renderpass = ice::gfx::find_resource<Renderpass>(gfx_device.resource_tracker(), "ice.gfx.renderpass.default"_sid);
        RenderDevice& device = gfx_device.device();

        _shader_stages[0] = ShaderStageFlags::VertexStage;
        _shader_stages[1] = ShaderStageFlags::FragmentStage;
        _shaders[0] = device.create_shader(ShaderInfo{ .shader_data = _shader_data[0] });
        _shaders[1] = device.create_shader(ShaderInfo{ .shader_data = _shader_data[1] });

        SamplerInfo sampler_info{
            .min_filter = SamplerFilter::Nearest,
            .mag_filter = SamplerFilter::Nearest,
            .address_mode = {
                .u = SamplerAddressMode::RepeatMirrored,
                .v = SamplerAddressMode::ClampToEdge,
                .w = SamplerAddressMode::ClampToEdge,
            },
            .mip_map_mode = SamplerMipMapMode::Nearest,
        };

        _sampler = device.create_sampler(sampler_info);

        ResourceSetLayoutBinding const resource_bindings[]{
            ResourceSetLayoutBinding
            {
                .binding_index = 0,
                .resource_count = 1,
                .resource_type = ResourceType::UniformBuffer,
                .shader_stage_flags = ShaderStageFlags::VertexStage
            },
            ResourceSetLayoutBinding
            {
                .binding_index = 1,
                .resource_count = 1,
                .resource_type = ResourceType::UniformBuffer,
                .shader_stage_flags = ShaderStageFlags::VertexStage
            },
            ResourceSetLayoutBinding
            {
                .binding_index = 2,
                .resource_count = 1,
                .resource_type = ResourceType::Sampler,
                .shader_stage_flags = ShaderStageFlags::FragmentStage
            },
            ResourceSetLayoutBinding
            {
                .binding_index = 3,
                .resource_count = 4,
                .resource_type = ResourceType::SampledImage,
                .shader_stage_flags = ShaderStageFlags::FragmentStage
            },
            ResourceSetLayoutBinding
            {
                .binding_index = 4,
                .resource_count = 4,
                .resource_type = ResourceType::UniformBuffer,
                .shader_stage_flags = ShaderStageFlags::VertexStage | ShaderStageFlags::FragmentStage
            },
        };

        _resource_set_layouts[0] = device.create_resourceset_layout({ resource_bindings + 0, 3 });
        _resource_set_layouts[1] = device.create_resourceset_layout({ resource_bindings + 3, 2 });
        device.create_resourcesets({ _resource_set_layouts + 0, 1 }, _resource_sets);

        PipelineLayoutInfo const layout_info
        {
            .push_constants = { },
            .resource_layouts = _resource_set_layouts,
        };

        _pipeline_layout = device.create_pipeline_layout(layout_info);


        ShaderInputAttribute attribs[]{
            ShaderInputAttribute{
                .location = 0,
                .offset = 0,
                .type = ShaderAttribType::Vec2f
            },
            ShaderInputAttribute{
                .location = 1,
                .offset = 8,
                .type = ShaderAttribType::Vec2f
            },
            ShaderInputAttribute{
                .location = 2,
                .offset = 16,
                .type = ShaderAttribType::Vec1u
            },
            ShaderInputAttribute{
                .location = 3,
                .offset = 0,
                .type = ShaderAttribType::Vec1u
            },
            ShaderInputAttribute{
                .location = 4,
                .offset = 4,
                .type = ShaderAttribType::Vec1u
            },
        };

        ShaderInputBinding bindings[]{
            ShaderInputBinding{
                .binding = 0,
                .stride = 20,
                .instanced = false,
                .attributes = { attribs + 0, 3 }
            },
            ShaderInputBinding{
                .binding = 1,
                .stride = 8,
                .instanced = true,
                .attributes = { attribs + 3, 2 }
            },
        };

        PipelineInfo pipeline_info{
            .layout = _pipeline_layout,
            .renderpass = renderpass,
            .shaders = _shaders,
            .shaders_stages = _shader_stages,
            .shader_bindings = bindings,
            .primitive_topology = PrimitiveTopology::TriangleFan,
            .cull_mode = CullMode::BackFace,
            .front_face = FrontFace::CounterClockWise,
            .subpass_index = 1,
            .depth_test = false
        };

        _pipeline = device.create_pipeline(pipeline_info);

        ice::vec2f flip_vertex_operations[32]{
            ice::vec2f{ 0.f }, // No Flip
            ice::vec2f{ 0.f },
            ice::vec2f{ 0.f },
            ice::vec2f{ 0.f },

            ice::vec2f{ 0.f, 1.f }, // Flip along X axis
            ice::vec2f{ 0.f, -1.f },
            ice::vec2f{ 0.f, -1.f },
            ice::vec2f{ 0.f, 1.f },

            ice::vec2f{ 1.f, 0.f }, // Flip along Y axis
            ice::vec2f{ 1.f, 0.f },
            ice::vec2f{ -1.f, 0.f },
            ice::vec2f{ -1.f, 0.f },

            ice::vec2f{ 1.f, 1.f },  // Flip along X and Y axis
            ice::vec2f{ 1.f, -1.f },
            ice::vec2f{ -1.f, -1.f },
            ice::vec2f{ -1.f, 1.f },

            ice::vec2f{ 0.f, 0.f },  // Diagonal flip (left rotation)
            ice::vec2f{ 1.f, -1.f },
            ice::vec2f{ 0.f, 0.f },
            ice::vec2f{ -1.f, 1.f },

            ice::vec2f{ 1.f, 0.f },  // Diagonal flip (left rotation) + flip along X axis
            ice::vec2f{ 0.f, -1.f },
            ice::vec2f{ -1.f, 0.f },
            ice::vec2f{ 0.f, 1.f },

            ice::vec2f{ 0.f, 1.f },  // Diagonal flip (left rotation) + flip along Y axis
            ice::vec2f{ 1.f, 0.f },
            ice::vec2f{ 0.f, -1.f },
            ice::vec2f{ -1.f, 0.f },

            ice::vec2f{ 1.f, 1.f },  // Diagonal flip (left rotation) + flip along X and Y axis
            ice::vec2f{ 0.f, 0.f },
            ice::vec2f{ -1.f, -1.f },
            ice::vec2f{ 0.f, 0.f },
        };

        _tile_flip_buffer = device.create_buffer(BufferType::Uniform, sizeof(flip_vertex_operations));

        BufferUpdateInfo updates[]{
            BufferUpdateInfo
            {
                .buffer = _tile_flip_buffer,
                .data = ice::data_view(flip_vertex_operations),
                .offset = 0
            }
        };

        device.update_buffers(updates);


        _vertex_buffer = device.create_buffer(BufferType::Vertex, 1024 * 1024 * 2);
        _instance_buffer = device.create_buffer(BufferType::Vertex, 1024 * 1024 * 1);
    }

    void IceWorldTrait_RenderTilemap::gfx_cleanup(
        ice::gfx::GfxFrame& gfx_frame,
        ice::gfx::GfxDevice& gfx_device
    ) noexcept
    {
        using namespace ice::render;

        RenderDevice& device = gfx_device.device();
        device.destroy_buffer(_instance_buffer);
        device.destroy_buffer(_vertex_buffer);
        device.destroy_buffer(_tile_flip_buffer);
        device.destroy_pipeline(_pipeline);
        device.destroy_pipeline_layout(_pipeline_layout);
        device.destroy_sampler(_sampler);
        device.destroy_shader(_shaders[1]);
        device.destroy_shader(_shaders[0]);
        device.destroy_resourcesets(_resource_sets);
        device.destroy_resourceset_layout(_resource_set_layouts[1]);
        device.destroy_resourceset_layout(_resource_set_layouts[0]);

        for (ice::IceTileMap_RenderCache const* render_cache : _render_cache)
        {
            device.destroy_resourcesets(render_cache->tileset_resourceset);
            for (ice::u32 idx = 0; idx < render_cache->image_count; ++idx)
            {
                device.destroy_buffer(render_cache->tileset_properties[idx]);
            }
        }
    }

    auto IceWorldTrait_RenderTilemap::task_gfx_update(
        ice::EngineFrame const& engine_frame,
        ice::gfx::GfxFrame& gfx_frame,
        ice::gfx::GfxDevice& gfx_device
    ) noexcept -> ice::Task<>
    {
        using namespace ice::render;

        IPT_ZONE_SCOPED_NAMED("[GfxTrait] TileMap :: Update Camera");

        ice::StringID_Hash camera_name = ice::StringID_Invalid;
        if (ice::shards::inspect_last(engine_frame.shards(), ice::Shard_SetDefaultCamera, camera_name))
        {
            _render_camera = ice::StringID{ camera_name };
        }

        if (camera_name != ice::StringID_Invalid)
        {
            ice::render::Buffer const camera_buffer = ice::gfx::find_resource<ice::render::Buffer>(
                gfx_device.resource_tracker(),
                _render_camera
            );

            if (_render_camera_buffer != camera_buffer && camera_buffer != ice::render::Buffer::Invalid)
            {
                _render_camera_buffer = camera_buffer;
                update_resource_camera(gfx_device);
            }
        }

        ice::detail::TileMap_DrawOperation const* const draw_operation = engine_frame.storage().named_object<ice::detail::TileMap_DrawOperation>(
            "tilemap_render.draw_operation"_sid
        );

        if (draw_operation != nullptr)
        {
            if (draw_operation->tilemap != _last_tilemap)
            {
                _last_tilemap = draw_operation->tilemap;
                update_resource_tilemap(gfx_device, *draw_operation->render_info);
            }

            gfx_frame.set_stage_slot(ice::Constant_GfxStage_DrawTilemap, this);
        }

        co_return;
    }

    void IceWorldTrait_RenderTilemap::record_commands(
        ice::gfx::GfxContext const& context,
        ice::EngineFrame const& engine_frame,
        ice::render::CommandBuffer cmds,
        ice::render::RenderCommands& api
    ) const noexcept
    {
        IPT_ZONE_SCOPED_NAMED("[GfxTrait] TileMap :: Record commands");

        detail::TileMap_DrawOperation const* const draw_operation = engine_frame.storage().named_object<detail::TileMap_DrawOperation>(
            "tilemap_render.draw_operation"_sid
        );

        api.bind_pipeline(cmds, _pipeline);
        api.bind_resource_set(cmds, _pipeline_layout, _resource_sets[0], 0);
        api.bind_vertex_buffer(cmds, _vertex_buffer, 0);
        api.bind_vertex_buffer(cmds, _instance_buffer, 1);

        api.bind_resource_set(
            cmds,
            _pipeline_layout,
            draw_operation->render_cache->tileset_resourceset[0],
            1
        );

        ice::u32 instance_offset = 0;
        for (ice::IceTileLayer_RenderInfo const& layer : draw_operation->render_info->layers)
        {
            if (layer.visible)
            {
                ice::u32 const instance_count = ice::count(layer.tiles);

                api.draw(
                    cmds,
                    4,
                    instance_count,
                    0,
                    instance_offset
                );

                instance_offset += instance_count;
            }
        }
    }

    void IceWorldTrait_RenderTilemap::update_resource_tilemap(
        ice::gfx::GfxDevice& gfx_device,
        ice::IceTileMap_RenderInfo const& tilemap_info
    ) noexcept
    {
        ice::f32 tile_height = tilemap_info.tilesize.x;
        ice::f32 tile_width = tilemap_info.tilesize.y;

        struct Vertex
        {
            ice::vec4f pos_and_uv;
            ice::vec1u index;
        };

        Vertex vertices[4];
        vertices[0] = Vertex{ .pos_and_uv = { 0.f, ice::f32(tile_height) * 1, 0.f, 0.f }, .index = vec1u{ 0 } };
        vertices[1] = Vertex{ .pos_and_uv = { 0.f, 0.f, 0.f, 1.f }, .index = vec1u{ 1 } };
        vertices[2] = Vertex{ .pos_and_uv = { ice::f32(tile_width) * 1, 0.f, 1.f, 1.f }, .index = vec1u{ 2 } };
        vertices[3] = Vertex{ .pos_and_uv = { ice::f32(tile_width) * 1, ice::f32(tile_height) * 1, 1.f, 0.f }, .index = vec1u{ 3 } };

        ice::u32 update_count = 1;
        ice::render::BufferUpdateInfo updates[6]{
            ice::render::BufferUpdateInfo
            {
                .buffer = _vertex_buffer,
                .data = ice::data_view(vertices)
            }
        };

        ice::u32 instance_offset = 0;
        for (ice::IceTileLayer_RenderInfo const& layer : tilemap_info.layers)
        {
            if (layer.visible)
            {
                ice::u32 const instance_byte_offset = sizeof(Tile) * instance_offset;

                instance_offset += ice::count(layer.tiles);
                updates[update_count] = ice::render::BufferUpdateInfo
                {
                    .buffer = _instance_buffer,
                    .data = ice::data_view(layer.tiles),
                    .offset = instance_byte_offset,
                };
                update_count += 1;
            }
        }

        gfx_device.device().update_buffers({ updates, update_count });
    }

    void IceWorldTrait_RenderTilemap::update_resource_camera(
        ice::gfx::GfxDevice& gfx_device
    ) noexcept
    {
        using namespace render;
        RenderDevice& device = gfx_device.device();

        ResourceUpdateInfo res_updates[]{
            ResourceUpdateInfo
            {
                .uniform_buffer = ResourceBufferInfo{
                    .buffer = _render_camera_buffer,
                    .offset = 0,
                    .size = sizeof(ice::TraitCameraRenderData)
                }
            },
            ResourceUpdateInfo
            {
                .uniform_buffer = ResourceBufferInfo{
                    .buffer = _tile_flip_buffer,
                    .offset = 0,
                    .size = sizeof(ice::vec2f) * 32
                }
            },
            ResourceUpdateInfo
            {
                .sampler = _sampler
            },
        };

        ResourceSetUpdateInfo set_updates[]{
            ResourceSetUpdateInfo
            {
                .resource_set = _resource_sets[0],
                .resource_type = ResourceType::UniformBuffer,
                .binding_index = 0,
                .array_element = 0,
                .resources = { res_updates + 0, 1 },
            },
            ResourceSetUpdateInfo
            {
                .resource_set = _resource_sets[0],
                .resource_type = ResourceType::UniformBuffer,
                .binding_index = 1,
                .array_element = 0,
                .resources = { res_updates + 1, 1 },
            },
            ResourceSetUpdateInfo
            {
                .resource_set = _resource_sets[0],
                .resource_type = ResourceType::Sampler,
                .binding_index = 2,
                .array_element = 0,
                .resources = { res_updates + 2, 1 },
            }
        };

        device.update_resourceset(set_updates);
    }

    void register_trait_render_tilemap(
        ice::WorldTraitArchive& archive
    ) noexcept
    {
        static constexpr ice::StringID trait_dependencies[]{
            Constant_TraitName_RenderClear,
        };

        archive.register_trait(
            ice::Constant_TraitName_RenderTilemap,
            ice::WorldTraitDescription
            {
                .factory = ice::detail::generic_trait_factory<IceWorldTrait_RenderTilemap>,
                .required_dependencies = trait_dependencies
            }
        );
    }
#endif

} // namespace ice
