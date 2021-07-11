#include "trait_render_sprites.hxx"
#include "../trait_camera.hxx"
#include <ice/game_entity.hxx>
#include <ice/game_sprites.hxx>

#include <ice/engine.hxx>
#include <ice/engine_runner.hxx>
#include <ice/world/world_portal.hxx>
#include <ice/archetype/archetype_query.hxx>

#include <ice/gfx/gfx_device.hxx>
#include <ice/gfx/gfx_resource_tracker.hxx>
#include <ice/gfx/gfx_frame.hxx>
#include <ice/gfx/gfx_task.hxx>

#include <ice/render/render_command_buffer.hxx>
#include <ice/render/render_swapchain.hxx>
#include <ice/render/render_resource.hxx>
#include <ice/render/render_pipeline.hxx>
#include <ice/render/render_device.hxx>
#include <ice/render/render_buffer.hxx>
#include <ice/render/render_image.hxx>
#include <ice/render/render_pass.hxx>

#include <ice/resource_meta.hxx>
#include <ice/asset_system.hxx>

#include <ice/profiler.hxx>

namespace ice
{

    namespace detail
    {

        using SpriteQuery = ice::ComponentQuery<ice::Transform2DStatic const&, ice::Sprite const&, ice::SpriteTile const*>;
        static constexpr ice::StringID SpriteQueryId = "ice.trait.sprite-query"_sid;

        auto load_sprite_shader(ice::AssetSystem& assets, ice::StringID name) noexcept -> ice::Data
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

        struct SpriteInstanceInfo
        {
            ice::StringID_Hash materialid;
            ice::u32 instance_offset;
            ice::u32 next_instance;
            ice::u32 instance_count;
        };

        struct SpriteInstance
        {
            ice::vec3f position;
            ice::vec2f scale;
            ice::vec2i tile_offset;
        };

    } // namespace detail

    IceWorldTrait_RenderSprites::IceWorldTrait_RenderSprites(
        ice::Allocator& alloc
    ) noexcept
        : _sprite_materials{ alloc }
        , _vertex_offsets{ alloc }
    {
    }

    auto IceWorldTrait_RenderSprites::gfx_stage_infos() const noexcept -> ice::Span<ice::gfx::GfxStageInfo const>
    {
        static ice::StringID const dependencies[]{
            "frame.clear"_sid,
        };
        static ice::gfx::GfxStageInfo const infos[]{
            ice::gfx::GfxStageInfo
            {
                .name = "frame.render-sprites"_sid,
                .dependencies = dependencies,
                .type = ice::gfx::GfxStageType::DrawStage
            }
        };
        return infos;
    }

    auto IceWorldTrait_RenderSprites::gfx_stage_slots() const noexcept -> ice::Span<ice::gfx::GfxStageSlot const>
    {
        static ice::gfx::GfxStageSlot const slots[]{
            ice::gfx::GfxStageSlot
            {
                .name = "frame.render-sprites"_sid,
                .stage = this
            }
        };
        return slots;
    }

    void IceWorldTrait_RenderSprites::set_camera(
        ice::StringID_Arg camera_name
    ) noexcept
    {
        _render_camera = camera_name;
    }

    void IceWorldTrait_RenderSprites::on_activate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        _asset_system = ice::addressof(engine.asset_system());

        portal.storage().create_named_object<detail::SpriteQuery>(
            detail::SpriteQueryId,
            portal.allocator(),
            portal.entity_storage().archetype_index()
        );

        runner.execute_task(
            task_create_render_objects(runner, engine.asset_system(), runner.graphics_device()),
            EngineContext::EngineRunner
        );
    }

    void IceWorldTrait_RenderSprites::on_deactivate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        runner.execute_task(
            task_destroy_render_objects(runner.graphics_device()),
            EngineContext::GraphicsFrame
        );

        for (auto const& entry : _sprite_materials)
        {
            runner.execute_task(
                task_destroy_resource_material(runner.graphics_device(), entry.value),
                EngineContext::GraphicsFrame
            );
        }

        portal.storage().destroy_named_object<detail::SpriteQuery>(detail::SpriteQueryId);

        _asset_system = nullptr;
    }

    void IceWorldTrait_RenderSprites::on_update(
        ice::EngineFrame& frame,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        IPT_ZONE_SCOPED_NAMED("[Trait] Sprites :: Update");

        ice::gfx::GfxFrame& gfx_frame = runner.graphics_frame();

        if (_resource_set_layouts[0] == ice::render::ResourceSetLayout::Invalid)
        {
            return;
        }

        detail::SpriteQuery::ResultByEntity result = portal.storage().named_object<detail::SpriteQuery>(detail::SpriteQueryId)->result_by_entity(frame.allocator(), portal.entity_storage());

        ice::u32 next_instance_idx = 0;
        ice::pod::Hash<ice::u32> instance_info_idx{ frame.allocator() };
        ice::pod::hash::reserve(instance_info_idx, 64);

        ice::Span<detail::SpriteInstanceInfo> instance_infos = frame.create_named_span<detail::SpriteInstanceInfo>("ice.sprite.instance_infos"_sid, 64);
        frame.create_named_object<ice::Span<detail::SpriteInstanceInfo>>("ice.sprite.instance_infos_span"_sid, instance_infos);

        ice::u32 valid_entity_count = 0;
        result.for_each(
            [&](ice::Transform2DStatic const& xform, ice::Sprite const& sprite, ice::SpriteTile const* sprite_tile) noexcept
            {
                ice::u64 const material_hash = ice::hash(sprite.material);

                if (ice::pod::hash::has(_sprite_materials, material_hash))
                {
                    ice::u32 const info_idx = ice::pod::hash::get(instance_info_idx, material_hash, next_instance_idx);
                    detail::SpriteInstanceInfo& instance_info = instance_infos[info_idx];

                    if (info_idx == next_instance_idx)
                    {
                        ice::pod::hash::set(instance_info_idx, material_hash, info_idx);

                        instance_info.materialid = ice::stringid_hash(sprite.material);
                        instance_info.instance_offset = std::numeric_limits<ice::u32>::max();
                        instance_info.instance_count = 0;
                        next_instance_idx += 1;
                    }

                    instance_info.instance_count += 1;
                    valid_entity_count += 1;
                }
                else
                {
                }
            }
        );


        ice::u32 current_instance_offset = 0;
        ice::Span<detail::SpriteInstance> instances = frame.create_named_span<detail::SpriteInstance>("ice.sprite.instances"_sid, valid_entity_count);

        result.for_each(
            [&](ice::Transform2DStatic const& xform, ice::Sprite const& sprite, ice::SpriteTile const* sprite_tile) noexcept
            {
                ice::u64 const material_hash = ice::hash(sprite.material);
                if (ice::pod::hash::has(instance_info_idx, material_hash) == false)
                {
                    return;
                }

                ice::vec2i tile{ 0, 0 };
                if (sprite_tile != nullptr)
                {
                    tile = { ice::i32(sprite_tile->material_tile.x), ice::i32(sprite_tile->material_tile.y) };
                    //tile = { ice::u32(sprite_tile->material_tile.x), ice::u32(sprite_tile->material_tile.y) };
                }

                ice::u32 const info_idx = ice::pod::hash::get(instance_info_idx, material_hash, std::numeric_limits<ice::u32>::max());

                detail::SpriteInstanceInfo& instance_info = instance_infos[info_idx];
                if (instance_info.instance_offset == std::numeric_limits<ice::u32>::max())
                {
                    instance_info.instance_offset = current_instance_offset;
                    instance_info.next_instance = current_instance_offset;
                    current_instance_offset += instance_info.instance_count;
                }

                instances[instance_info.next_instance] = detail::SpriteInstance
                {
                    .position = xform.position,
                    .scale = xform.scale,
                    .tile_offset = tile
                };
                instance_info.next_instance += 1;
            }
        );

        result.for_each(
            [&](ice::Transform2DStatic const& xform, ice::Sprite const& sprite, ice::SpriteTile const* sprite_tile) noexcept
            {
                ice::u64 const material_hash = ice::hash(sprite.material);
                if (ice::pod::hash::has(_sprite_materials, material_hash) == false)
                {
                    runner.execute_task(
                        task_load_resource_material(sprite.material, runner, runner.graphics_device()),
                        EngineContext::EngineRunner
                    );
                }
            }
        );

        ice::render::Buffer* camera_buffer = frame.named_object<ice::render::Buffer>(_render_camera);
        if (camera_buffer != nullptr)
        {
            if (_render_camera_buffer != *camera_buffer)
            {
                _render_camera_buffer = *camera_buffer;
            }

            runner.execute_task(
                task_update_resource_camera(runner.graphics_device()),
                EngineContext::GraphicsFrame
            );

            runner.execute_task(
                task_update_resource_data(runner.graphics_device(), instances),
                EngineContext::GraphicsFrame
            );

            gfx_frame.set_stage_slots(gfx_stage_slots());
        }
    }

    void IceWorldTrait_RenderSprites::record_commands(
        ice::EngineFrame const& frame,
        ice::render::CommandBuffer cmds,
        ice::render::RenderCommands& api
    ) const noexcept
    {
        IPT_ZONE_SCOPED_NAMED("[Trait] Sprites :: Graphics Commands");

        ice::Span<detail::SpriteInstanceInfo> const* const instances = frame.named_object<ice::Span<detail::SpriteInstanceInfo>>("ice.sprite.instance_infos_span"_sid);
        if (instances != nullptr)
        {
            api.bind_pipeline(cmds, _pipeline);
            api.bind_resource_set(cmds, _pipeline_layout, _resource_sets[0], 0);
            api.bind_vertex_buffer(cmds, _vertex_buffer, 0);
            api.bind_vertex_buffer(cmds, _instance_buffer, 1);

            for (detail::SpriteInstanceInfo const& instance : *instances)
            {
                detail::RenderData_Sprite const& sprite_render_data = ice::pod::hash::get(_sprite_materials, ice::hash(instance.materialid), {});
                if (sprite_render_data.material[0] == ice::render::Image::Invalid)
                {
                    continue;
                }

                api.bind_resource_set(
                    cmds,
                    _pipeline_layout,
                    sprite_render_data.sprite_resource[0],
                    1
                );
                api.draw(
                    cmds,
                    sprite_render_data.shape_vertices,
                    instance.instance_count,
                    sprite_render_data.shape_offset,
                    instance.instance_offset
                );
            }
        }
    }

    auto IceWorldTrait_RenderSprites::task_create_render_objects(
        ice::EngineRunner& runner,
        ice::AssetSystem& asset_system,
        ice::gfx::GfxDevice& gfx_device
    ) noexcept -> ice::Task<>
    {
        using namespace ice::gfx;
        using namespace ice::render;

        Data vtx_shader_data = ice::detail::load_sprite_shader(asset_system, "/shaders/game2d/sprite-vtx"_sid);
        Data pix_shader_data = ice::detail::load_sprite_shader(asset_system, "/shaders/game2d/sprite-pix"_sid);

        ice::render::Renderpass renderpass = Renderpass::Invalid;
        while(renderpass == Renderpass::Invalid)
        {
            ice::EngineFrame& frame = co_await runner.schedule_next_frame();

            co_await frame.schedule_frame_end();

            ice::render::Renderpass* candidate_renderpass = frame.named_object<ice::render::Renderpass>("ice.gfx.renderpass"_sid);
            if (candidate_renderpass != nullptr)
            {
                renderpass = *candidate_renderpass;
            }
        }

        co_await runner.graphics_frame().frame_start();

        RenderDevice& device = gfx_device.device();

        _shader_stages[0] = ShaderStageFlags::VertexStage;
        _shader_stages[1] = ShaderStageFlags::FragmentStage;
        _shaders[0] = device.create_shader(ShaderInfo{ .shader_data = vtx_shader_data });
        _shaders[1] = device.create_shader(ShaderInfo{ .shader_data = pix_shader_data });

        SamplerInfo sampler_info{
            .min_filter = SamplerFilter::Nearest,
            .mag_filter = SamplerFilter::Nearest,
            .address_mode = {
                .u = SamplerAddressMode::ClampToEdge,
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
                .resource_count = 1,
                .resource_type = ResourceType::SampledImage,
                .shader_stage_flags = ShaderStageFlags::FragmentStage
            },
            ResourceSetLayoutBinding
            {
                .binding_index = 3,
                .resource_count = 1,
                .resource_type = ResourceType::UniformBuffer,
                .shader_stage_flags = ShaderStageFlags::FragmentStage
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
                .type = ShaderAttribType::Vec3f
            },
            ShaderInputAttribute{
                .location = 3,
                .offset = 12,
                .type = ShaderAttribType::Vec2f
            },
            ShaderInputAttribute{
                .location = 4,
                .offset = 20,
                .type = ShaderAttribType::Vec1i
            },
            ShaderInputAttribute{
                .location = 5,
                .offset = 24,
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
                .stride = 28,
                .instanced = true,
                .attributes = { attribs + 2, 4 }
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
        co_return;
    }

    auto IceWorldTrait_RenderSprites::task_destroy_render_objects(
        ice::gfx::GfxDevice& gfx_device
    ) noexcept -> ice::Task<>
    {
        using namespace ice::gfx;
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
        co_return;
    }

    auto IceWorldTrait_RenderSprites::task_update_resource_camera(
        ice::gfx::GfxDevice& gfx_device
    ) noexcept -> ice::Task<>
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
        co_return;
    }

    auto IceWorldTrait_RenderSprites::task_update_resource_data(
        ice::gfx::GfxDevice& gfx_device,
        ice::Span<detail::SpriteInstance> instances
    ) noexcept -> ice::Task<>
    {
        using namespace render;
        RenderDevice& device = gfx_device.device();

        BufferUpdateInfo buffer_updates[]{
            BufferUpdateInfo
            {
                .buffer = _instance_buffer,
                .data = ice::data_view(instances),
                .offset = 0
            }
        };

        device.update_buffers(buffer_updates);
        co_return;
    }

    auto IceWorldTrait_RenderSprites::task_load_resource_material(
        ice::StringID_Arg material_name,
        ice::EngineRunner& runner,
        ice::gfx::GfxDevice& gfx_device
    ) noexcept -> ice::Task<>
    {
        using namespace ice::render;

        if (ice::pod::hash::has(_sprite_materials, ice::hash(material_name)))
        {
            co_return;
        }

        Asset image_asset = _asset_system->request(AssetType::Texture, material_name);

        Data image_asset_data;
        Metadata image_metadata;
        if (asset_data(image_asset, image_asset_data) != AssetStatus::Loaded)
        {
            co_return;
        }

        if (asset_metadata(image_asset, image_metadata) != AssetStatus::Loaded)
        {
            co_return;
        }

        ice::i32 tile_width;
        ice::i32 tile_height;

        bool meta_valid = true;
        meta_valid &= meta_read_int32(image_metadata, "tileset.tile.width"_sid, tile_width);
        meta_valid &= meta_read_int32(image_metadata, "tileset.tile.height"_sid, tile_height);

        if (meta_valid == false)
        {
            ICE_LOG(
                ice::LogSeverity::Error, ice::LogTag::Engine,
                "The asset {} does not provide `tile` specific metadata!",
                ice::stringid_hint(material_name)
            );
            co_return;
        }

        ice::u64 const tile_mesh_id = (static_cast<ice::u64>(tile_width) << 32) | tile_height;
        bool const has_vertex_offsets = ice::pod::hash::has(_vertex_offsets, tile_mesh_id) == false;

        ice::u32 const vertex_offset = ice::pod::hash::get(
            _vertex_offsets,
            tile_mesh_id,
            ice::pod::array::size(_vertex_offsets._data) * 4
        );

        if (has_vertex_offsets == false)
        {
            ice::pod::hash::set(_vertex_offsets, tile_mesh_id, vertex_offset);
        }

        // Set a dummy value so we can check for it and skip loading the same asset more than once.
        ice::pod::hash::set(_sprite_materials, ice::hash(material_name), { });

        ImageInfo const* image_data = reinterpret_cast<ImageInfo const*>(image_asset_data.location);
        ice::u32 const image_data_size = image_data->width * image_data->height * 4;

        ice::detail::RenderData_Sprite sprite_data{ };
        sprite_data.shape_offset = vertex_offset;
        sprite_data.shape_vertices = 4;
        sprite_data.material_scale = ice::vec2f{
            ice::f32(tile_width) / ice::f32(image_data->width),
            ice::f32(tile_height) / ice::f32(image_data->height),
        };

        ice::EngineFrame& frame = runner.current_frame();
        ice::gfx::GfxFrame& gfx_frame = runner.graphics_frame();

        ice::vec4f vertices[4];

        co_await gfx_frame.frame_start();

        RenderDevice& device = gfx_device.device();
        ice::render::Buffer const data_buffer = device.create_buffer(
            ice::render::BufferType::Transfer,
            image_data_size
        );

        sprite_data.material[0] = device.create_image(*image_data, { });
        sprite_data.material_tileinfo[0] = device.create_buffer(BufferType::Uniform, sizeof(sprite_data.material_scale));
        device.create_resourcesets({ _resource_set_layouts + 1, 1 }, sprite_data.sprite_resource);

        ice::u32 update_count = 2;
        ice::render::BufferUpdateInfo updates[3]{
            ice::render::BufferUpdateInfo
            {
                .buffer = sprite_data.material_tileinfo[0],
                .data = {
                    .location = ice::addressof(sprite_data.material_scale),
                    .size = sizeof(sprite_data.material_scale),
                    .alignment = 4
                }
            },
            ice::render::BufferUpdateInfo
            {
                .buffer = data_buffer,
                .data = {
                    .location = image_data->data,
                    .size = image_data_size,
                    .alignment = 4
                }
            }
        };

        if (has_vertex_offsets)
        {
            vertices[0] = { 0.f, ice::f32(tile_height), 0.f, 0.f };
            vertices[1] = { 0.f, 0.f, 0.f, 1.f };
            vertices[2] = { ice::f32(tile_width), 0.f, 1.f, 1.f };
            vertices[3] = { ice::f32(tile_width), ice::f32(tile_height), 1.f, 0.f };

            updates[2].buffer = _vertex_buffer;
            updates[2].data = ice::data_view(vertices);
            updates[2].offset = vertex_offset * sizeof(ice::vec4f);
            update_count += 1;
        }

        device.update_buffers({ updates, update_count });

        // Await command recording stage
        //  Here we have access to a command buffer where we can record commands.
        //  These commands will be later executed on the graphics thread.
        ice::gfx::GfxTaskCommands& cmds = co_await gfx_frame.frame_commands("default"_sid);

        cmds.update_texture(
            sprite_data.material[0],
            data_buffer,
            { image_data->width, image_data->height }
        );

        // Await end of graphics frame.
        //  Here we know that all commands have been executed
        //  and temporary objects can be destroyed.
        co_await gfx_frame.frame_end();
        device.destroy_buffer(data_buffer);

        co_await runner.schedule_next_frame();

        runner.execute_task(
            task_update_resource_material(runner, gfx_device, material_name, sprite_data),
            EngineContext::EngineRunner
        );
        co_return;
    }

    auto IceWorldTrait_RenderSprites::task_update_resource_material(
        ice::EngineRunner& runner,
        ice::gfx::GfxDevice& gfx_device,
        ice::StringID material_name,
        detail::RenderData_Sprite sprite_data
    ) noexcept -> ice::Task<>
    {
        using namespace ice::render;

        co_await runner.graphics_frame().frame_start();

        RenderDevice& device = gfx_device.device();

        ResourceUpdateInfo res_updates[]{
            ResourceUpdateInfo
            {
                .image = sprite_data.material[0]
            },
            ResourceUpdateInfo
            {
                .uniform_buffer = {
                    .buffer = sprite_data.material_tileinfo[0],
                    .offset = 0,
                    .size = sizeof(sprite_data.material_scale)
                }
            }
        };

        ResourceSetUpdateInfo set_updates[]{
            ResourceSetUpdateInfo
            {
                .resource_set = sprite_data.sprite_resource[0],
                .resource_type = ResourceType::SampledImage,
                .binding_index = 2,
                .array_element = 0,
                .resources = { res_updates + 0, 1 },
            },
            ResourceSetUpdateInfo
            {
                .resource_set = sprite_data.sprite_resource[0],
                .resource_type = ResourceType::UniformBuffer,
                .binding_index = 3,
                .array_element = 0,
                .resources = { res_updates + 1, 1 },
            }
        };

        device.update_resourceset(set_updates);

        co_await runner.schedule_next_frame();

        ice::pod::hash::set(_sprite_materials, ice::hash(material_name), sprite_data);
        co_return;
    }

    auto IceWorldTrait_RenderSprites::task_destroy_resource_material(
        ice::gfx::GfxDevice& gfx_device,
        ice::detail::RenderData_Sprite sprite_data
    ) noexcept -> ice::Task<>
    {
        using namespace ice::render;
        RenderDevice& device = gfx_device.device();

        device.destroy_image(sprite_data.material[0]);
        device.destroy_buffer(sprite_data.material_tileinfo[0]);
        device.destroy_resourcesets(sprite_data.sprite_resource);
        co_return;
    }

} // namespace ice
