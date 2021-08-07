#include "trait_render_tilemap.hxx"
#include "../trait_camera.hxx"

#include <ice/game_tilemap.hxx>

#include <ice/engine.hxx>
#include <ice/engine_frame.hxx>
#include <ice/engine_runner.hxx>
#include <ice/world/world_portal.hxx>

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
#include <ice/asset_system.hxx>
#include <ice/asset.hxx>

#include <ice/profiler.hxx>

namespace ice
{

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
            ice::Data image_data[4];
            ice::Metadata image_metadata[4];
        };

        auto load_tilemap_shader(ice::AssetSystem& assets, ice::StringID name) noexcept -> ice::Data
        {
            Data result;
            Asset const shader_asset = assets.request(ice::AssetType::Shader, name);
            if (shader_asset != Asset::Invalid)
            {
                Data temp;
                if (ice::asset_data(shader_asset, temp) == AssetStatus::Loaded)
                {
                    result = *reinterpret_cast<ice::Data const*>(temp.location);
                }
            }

            return result;
        }

        auto task_load_resource_material(
            ice::gfx::GfxFrame& gfx_frame,
            ice::gfx::GfxDevice& gfx_device,
            ice::render::ResourceSetLayout layout,
            TileMap_LoadImageOperation const& operation
        ) noexcept -> ice::Task<>;

    } // namespace detail

    IceWorldTrait_RenderTilemap::IceWorldTrait_RenderTilemap(
        ice::Allocator& alloc,
        ice::StringID_Arg stage_name
    ) noexcept
        : _allocator{ alloc }
        , _stage_name{ stage_name }
        , _render_cache{ _allocator }
    {
        ice::pod::hash::reserve(_render_cache, 10);
    }

    void IceWorldTrait_RenderTilemap::on_activate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        _asset_system = ice::addressof(engine.asset_system());
        _shader_data[0] = detail::load_tilemap_shader(*_asset_system, Tilemap_VtxShader);
        _shader_data[1] = detail::load_tilemap_shader(*_asset_system, Tilemap_PixShader);
    }

    void IceWorldTrait_RenderTilemap::on_deactivate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        for (auto const& entry : _render_cache)
        {
            portal.allocator().destroy(entry.value);
        }
        ice::pod::hash::clear(_render_cache);

        _asset_system = nullptr;
    }

    void IceWorldTrait_RenderTilemap::on_update(
        ice::EngineFrame& frame,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        IPT_ZONE_SCOPED_NAMED("[GfxTrait] TileMap :: Update");

        ice::IceTileMap_RenderInfo const* tilemap_render = frame.named_object<ice::IceTileMap_RenderInfo>("tilemap.render-info"_sid);
        if (tilemap_render != nullptr)
        {
            ice::TileMap const* tilemap = tilemap_render->tilemap;
            ice::IceTileMap_RenderCache* render_cache = ice::pod::hash::get(_render_cache, ice::hash(tilemap->name), nullptr);

            if (render_cache != nullptr)
            {
                detail::TileMap_DrawOperation* operation = frame.create_named_object<detail::TileMap_DrawOperation>("tilemap_render.draw_operation"_sid);
                operation->tilemap = tilemap;
                operation->render_info = tilemap_render;
                operation->render_cache = render_cache;
            }
            else
            {

                render_cache = portal.allocator().make<ice::IceTileMap_RenderCache>();
                render_cache->tileset_resourceset[0] = ice::render::ResourceSet::Invalid;

                ice::pod::hash::set(
                    _render_cache,
                    ice::hash(tilemap->name),
                    render_cache
                );

                detail::TileMap_LoadImageOperation operation{ };
                operation.tilemap = tilemap;
                operation.render_cache = render_cache;
                operation.tile_render_size = tilemap_render->tilesize;

                for (ice::u32 idx = 0; idx < ice::size(tilemap->tilesets); ++idx)
                {
                    if (tilemap->tilesets[idx] != ice::stringid_invalid)
                    {
                        Asset tileset_asset = _asset_system->request(AssetType::Texture, tilemap->tilesets[idx]);

                        Metadata& tileset_meta = operation.image_metadata[operation.image_count];
                        if (asset_metadata(tileset_asset, tileset_meta) != AssetStatus::Loaded)
                        {
                            continue;
                        }

                        ice::i32 tileset_type = -1;

                        bool const meta_valid = meta_read_int32(tileset_meta, "tileset.type"_sid, tileset_type);
                        if (meta_valid == false || tileset_type != 0)
                        {
                            continue;
                        }

                        if (asset_data(tileset_asset, operation.image_data[operation.image_count]) != AssetStatus::Loaded)
                        {
                            continue;
                        }

                        operation.image_count += 1;
                    }
                }

                frame.create_named_object<detail::TileMap_LoadImageOperation>("tilemap_render.load_operation"_sid, operation);
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
                .resource_type = ResourceType::Sampler,
                .shader_stage_flags = ShaderStageFlags::FragmentStage
            },
            ResourceSetLayoutBinding
            {
                .binding_index = 2,
                .resource_count = 4,
                .resource_type = ResourceType::SampledImage,
                .shader_stage_flags = ShaderStageFlags::FragmentStage
            },
            ResourceSetLayoutBinding
            {
                .binding_index = 3,
                .resource_count = 4,
                .resource_type = ResourceType::UniformBuffer,
                .shader_stage_flags = ShaderStageFlags::VertexStage | ShaderStageFlags::FragmentStage
            },
        };

        _resource_set_layouts[0] = device.create_resourceset_layout({ resource_bindings + 0, 2 });
        _resource_set_layouts[1] = device.create_resourceset_layout({ resource_bindings + 2, 2 });
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
                .offset = 0,
                .type = ShaderAttribType::Vec2f
            },
            ShaderInputAttribute{
                .location = 3,
                .offset = 8,
                .type = ShaderAttribType::Vec1i
            },
        };

        ShaderInputBinding bindings[]{
            ShaderInputBinding{
                .binding = 0,
                .stride = 16,
                .instanced = false,
                .attributes = { attribs + 0, 2 }
            },
            ShaderInputBinding{
                .binding = 1,
                .stride = 12,
                .instanced = true,
                .attributes = { attribs + 2, 2 }
            },
        };

        PipelineInfo pipeline_info{
            .layout = _pipeline_layout,
            .renderpass = renderpass,
            .shaders = _shaders,
            .shaders_stages = _shader_stages,
            .shader_bindings = bindings,
            .primitive_topology = PrimitiveTopology::TriangleFan,
            .cull_mode = CullMode::Disabled,
            .front_face = FrontFace::CounterClockWise,
            .subpass_index = 1,
            .depth_test = true
        };

        _pipeline = device.create_pipeline(pipeline_info);

        _vertex_buffer = device.create_buffer(BufferType::Vertex, 1024 * 1024 * 1);
        _instance_buffer = device.create_buffer(BufferType::Vertex, 1024 * 1024 * 4);
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
        device.destroy_pipeline(_pipeline);
        device.destroy_pipeline_layout(_pipeline_layout);
        device.destroy_sampler(_sampler);
        device.destroy_shader(_shaders[1]);
        device.destroy_shader(_shaders[0]);
        device.destroy_resourcesets(_resource_sets);
        device.destroy_resourceset_layout(_resource_set_layouts[1]);
        device.destroy_resourceset_layout(_resource_set_layouts[0]);

        for (auto const& entry : _render_cache)
        {
            ice::IceTileMap_RenderCache const& render_cache = *entry.value;

            device.destroy_resourcesets(render_cache.tileset_resourceset);
            for (ice::u32 idx = 0; idx < render_cache.image_count; ++idx)
            {
                device.destroy_image(render_cache.tileset_images[idx]);
                device.destroy_buffer(render_cache.tileset_properties[idx]);
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

        {
            IPT_ZONE_SCOPED_NAMED("[GfxTrait] TileMap :: Update Camera");

            for (ice::Shard const& shard : engine_frame.shards())
            {
                ice::StringID_Hash camera_name;
                if (shard == ice::Shard_SetDefaultCamera && ice::shard_inspect(shard, camera_name))
                {
                    _render_camera = ice::StringID{ camera_name };
                }
            }

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

        ice::detail::TileMap_LoadImageOperation const* const load_operation = engine_frame.named_object<ice::detail::TileMap_LoadImageOperation>(
            "tilemap_render.load_operation"_sid
        );

        if (load_operation != nullptr && load_operation->image_count > 0)
        {
            co_await ice::detail::task_load_resource_material(gfx_frame, gfx_device, _resource_set_layouts[1], *load_operation);
        }
        else
        {
            ice::detail::TileMap_DrawOperation const* const draw_operation = engine_frame.named_object<ice::detail::TileMap_DrawOperation>(
                "tilemap_render.draw_operation"_sid
            );

            if (draw_operation != nullptr)
            {
                if (draw_operation->tilemap != _last_tilemap)
                {
                    _last_tilemap = draw_operation->tilemap;
                    update_resource_tilemap(gfx_device, *draw_operation->render_info);
                }

                gfx_frame.set_stage_slot(_stage_name, this);
            }
        }
    }

    void IceWorldTrait_RenderTilemap::record_commands(
        ice::gfx::GfxContext const& context,
        ice::EngineFrame const& engine_frame,
        ice::render::CommandBuffer cmds,
        ice::render::RenderCommands& api
    ) const noexcept
    {
        IPT_ZONE_SCOPED_NAMED("[GfxTrait] TileMap :: Record commands");

        detail::TileMap_DrawOperation const* const draw_operation = engine_frame.named_object<detail::TileMap_DrawOperation>(
            "tilemap_render.draw_operation"_sid
        );

        ice::TileMap const* tilemap = draw_operation->tilemap;
        ice::Span<ice::TileRoom const> tilemap_rooms = tilemap->rooms;

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
        for (ice::IceTileRoom_RenderInfo const& room : draw_operation->render_info->tilerooms)
        {
            if (room.visible)
            {
                ice::u32 const instance_count = ice::size(room.tiles);

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

        ice::vec4f vertices[4];
        vertices[0] = { 0.f, ice::f32(tile_height) * 1, 0.f, 0.f };
        vertices[1] = { 0.f, 0.f, 0.f, 1.f };
        vertices[2] = { ice::f32(tile_width) * 1, 0.f, 1.f, 1.f };
        vertices[3] = { ice::f32(tile_width) * 1, ice::f32(tile_height) * 1, 1.f, 0.f };

        ice::u32 update_count = 1;
        ice::render::BufferUpdateInfo updates[6]{
            ice::render::BufferUpdateInfo
            {
                .buffer = _vertex_buffer,
                .data = ice::data_view(vertices)
            }
        };

        ice::Span<ice::TileRoom const> tilemap_rooms = tilemap_info.tilemap->rooms;

        ice::u32 instance_offset = 0;
        for (ice::IceTileRoom_RenderInfo const& room : tilemap_info.tilerooms)
        {
            if (room.visible)
            {
                ice::u32 const instance_byte_offset = sizeof(Tile) * instance_offset;

                instance_offset += ice::size(room.tiles);
                updates[update_count] = ice::render::BufferUpdateInfo
                {
                    .buffer = _instance_buffer,
                    .data = ice::data_view(room.tiles),
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
                .uniform_buffer = {
                    .buffer = _render_camera_buffer,
                    .offset = 0,
                    .size = sizeof(ice::TraitCameraRenderData)
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
                .resource_type = ResourceType::Sampler,
                .binding_index = 1,
                .array_element = 0,
                .resources = { res_updates + 1, 1 },
            }
        };

        device.update_resourceset(set_updates);
    }

    auto detail::task_load_resource_material(
        ice::gfx::GfxFrame& gfx_frame,
        ice::gfx::GfxDevice& gfx_device,
        ice::render::ResourceSetLayout resourceset_layout,
        TileMap_LoadImageOperation const& operation
    ) noexcept -> ice::Task<>
    {
        using namespace ice::render;

        ice::u32 const image_count = operation.image_count;
        operation.render_cache->image_count = image_count;

        ice::vec2u image_extent[4];
        ice::render::Buffer image_data_buffer[4];
        ice::detail::TileSet_ShaderData_Properties properties[4];

        for (ice::u32 idx = 0; idx < image_count; ++idx)
        {

            ice::i32 tile_width;
            ice::i32 tile_height;

            bool meta_valid = true;
            meta_valid &= meta_read_int32(operation.image_metadata[idx], "tileset.tile.width"_sid, tile_width);
            meta_valid &= meta_read_int32(operation.image_metadata[idx], "tileset.tile.height"_sid, tile_height);

            ICE_ASSERT(
                meta_valid == true,
                "The tileset asset does not provide `tile` specific metadata!"
            );

            ImageInfo const* image_data = reinterpret_cast<ImageInfo const*>(operation.image_data[idx].location);
            image_extent[idx] = { image_data->width, image_data->height };

            ice::vec2f const tileset_size = { ice::f32(image_data->width), ice::f32(image_data->height) };
            ice::vec2f const tile_size = { ice::f32(tile_width), ice::f32(tile_height) };

            properties[idx].tile_size = operation.tile_render_size;
            properties[idx].tile_scale = { tile_size.x / tileset_size.x, tile_size.y / tileset_size.y };
        }

        co_await gfx_frame.frame_begin();

        RenderDevice& device = gfx_device.device();

        device.create_resourcesets({ &resourceset_layout, 1 }, operation.render_cache->tileset_resourceset);

        ice::u32 buffer_update_count = 0;
        ice::render::BufferUpdateInfo buffer_updates[8]{ };

        for (ice::u32 idx = 0; idx < image_count; ++idx)
        {
            ImageInfo const* image_data = reinterpret_cast<ImageInfo const*>(operation.image_data[idx].location);
            ice::u32 const image_data_size = image_data->width * image_data->height * 4;

            operation.render_cache->tileset_images[idx] = device.create_image(*image_data, { });
            operation.render_cache->tileset_properties[idx] = device.create_buffer(BufferType::Uniform, sizeof(TileSet_ShaderData_Properties));
            image_data_buffer[idx] = device.create_buffer(ice::render::BufferType::Transfer, image_data_size);

            buffer_updates[buffer_update_count] = ice::render::BufferUpdateInfo
            {
                .buffer = operation.render_cache->tileset_properties[idx],
                .data = ice::data_view(properties[idx])
            };
            buffer_updates[buffer_update_count + 1] = ice::render::BufferUpdateInfo
            {
                .buffer = image_data_buffer[idx],
                .data = {
                    .location = image_data->data,
                    .size = image_data_size,
                    .alignment = 4
                }
            };

            buffer_update_count += 2;
        }

        device.update_buffers({ buffer_updates, buffer_update_count });

        ice::u32 resource_update_count = 0;
        ResourceUpdateInfo resource_updates[8]{ };

        for (ice::u32 idx = 0; idx < 4; ++idx)
        {
            Image image = operation.render_cache->tileset_images[idx];
            if (idx >= image_count)
            {
                image = operation.render_cache->tileset_images[image_count - 1];
            }

            resource_updates[resource_update_count] = ResourceUpdateInfo
            {
                .image = image
            };
            resource_update_count += 1;
        }

        for (ice::u32 idx = 0; idx < 4; ++idx)
        {
            ice::render::Buffer buffer = operation.render_cache->tileset_properties[idx];
            if (idx >= image_count)
            {
                buffer = operation.render_cache->tileset_properties[image_count -1];
            }

            resource_updates[resource_update_count] = ResourceUpdateInfo
            {
                .uniform_buffer = {
                    .buffer = buffer,
                    .offset = 0,
                    .size = sizeof(TileSet_ShaderData_Properties)
                }
            };
            resource_update_count += 1;
        }

        ResourceSetUpdateInfo set_updates[]{
            ResourceSetUpdateInfo
            {
                .resource_set = operation.render_cache->tileset_resourceset[0],
                .resource_type = ResourceType::SampledImage,
                .binding_index = 2,
                .array_element = 0,
                .resources = { resource_updates + 0, 4 },
            },
            ResourceSetUpdateInfo
            {
                .resource_set = operation.render_cache->tileset_resourceset[0],
                .resource_type = ResourceType::UniformBuffer,
                .binding_index = 3,
                .array_element = 0,
                .resources = { resource_updates + 4, 4 },
            }
        };

        device.update_resourceset(set_updates);

        struct : public ice::gfx::GfxFrameStage
        {
            void record_commands(
                ice::EngineFrame const& frame,
                ice::render::CommandBuffer cmds,
                ice::render::RenderCommands& api
            ) const noexcept override
            {
                ImageBarrier barriers[4]{ };

                for (ice::u32 idx = 0; idx < image_count; ++idx)
                {
                    barriers[idx].image = image[idx];
                    barriers[idx].source_layout = ImageLayout::Undefined;
                    barriers[idx].destination_layout = ImageLayout::TransferDstOptimal;
                    barriers[idx].source_access = AccessFlags::None;
                    barriers[idx].destination_access = AccessFlags::TransferWrite;
                }

                api.pipeline_image_barrier(
                    cmds,
                    PipelineStage::TopOfPipe,
                    PipelineStage::Transfer,
                    { barriers, image_count }
                );

                for (ice::u32 idx = 0; idx < image_count; ++idx)
                {
                    api.update_texture(
                        cmds,
                        image[idx],
                        image_data[idx],
                        image_size[idx]
                    );
                }

                for (ice::u32 idx = 0; idx < image_count; ++idx)
                {
                    barriers[idx].image = image[idx];
                    barriers[idx].source_layout = ImageLayout::TransferDstOptimal;
                    barriers[idx].destination_layout = ImageLayout::ShaderReadOnly;
                    barriers[idx].source_access = AccessFlags::TransferWrite;
                    barriers[idx].destination_access = AccessFlags::ShaderRead;
                }

                api.pipeline_image_barrier(
                    cmds,
                    PipelineStage::Transfer,
                    PipelineStage::FramentShader,
                    { barriers, image_count }
                );
            }

            ice::u32 image_count;
            ice::render::Image* image;
            ice::render::Buffer* image_data;
            ice::vec2u* image_size;
        } frame_stage;

        frame_stage.image_count = image_count;
        frame_stage.image = operation.render_cache->tileset_images;
        frame_stage.image_data = image_data_buffer;
        frame_stage.image_size = image_extent;

        // Await command recording stage
        //  Here we have access to a command buffer where we can record commands.
        //  These commands will be later executed on the graphics thread.
        co_await gfx_frame.frame_commands(&frame_stage);

        // Await end of graphics frame.
        //  Here we know that all commands have been executed
        //  and temporary objects can be destroyed.
        co_await gfx_frame.frame_end();

        for (ice::u32 idx = 0; idx < image_count; ++idx)
        {
            device.destroy_buffer(image_data_buffer[idx]);
        }
        co_return;
    }


} // namespace ice
