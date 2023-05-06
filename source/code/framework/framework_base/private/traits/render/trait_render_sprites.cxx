/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "trait_render_sprites.hxx"
#include "../trait_camera.hxx"
#include <ice/game_entity.hxx>
#include <ice/game_sprites.hxx>

#include <ice/engine.hxx>
#include <ice/engine_runner.hxx>
#include <ice/world/world_portal.hxx>
#include <ice/world/world_trait_archive.hxx>
#include <ice/ecs/ecs_query.hxx>
#include <ice/ecs/ecs_entity_storage.hxx>

#include <ice/gfx/gfx_device.hxx>
#include <ice/gfx/gfx_resource_tracker.hxx>
#include <ice/gfx/gfx_frame.hxx>

#include <ice/render/render_command_buffer.hxx>
#include <ice/render/render_swapchain.hxx>
#include <ice/render/render_resource.hxx>
#include <ice/render/render_pipeline.hxx>
#include <ice/render/render_device.hxx>
#include <ice/render/render_buffer.hxx>
#include <ice/render/render_image.hxx>
#include <ice/render/render_pass.hxx>

#include <ice/resource_meta.hxx>
#include <ice/asset_storage.hxx>
#include <ice/asset.hxx>
#include <ice/task.hxx>
#include <ice/task_utils.hxx>

#include <ice/profiler.hxx>
#include <ice/hash.hxx>

namespace ice
{

    namespace detail
    {

        using SpriteQuery = ice::ecs::QueryDefinition<ice::Transform2DStatic const*, ice::Transform2DDynamic const*, ice::Sprite const&, ice::SpriteTile const*>;
        static constexpr ice::StringID SpriteQueryId = "ice.trait.sprite-query"_sid;

        auto load_sprites_shader(ice::AssetStorage& assets, ice::String name) noexcept -> ice::Task<ice::Data>
        {
            ice::Asset asset = assets.bind(ice::render::AssetType_Shader, name);
            co_return co_await asset[AssetState::Baked];
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

    void IceWorldTrait_RenderSprites::gfx_setup(
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
    }

    void IceWorldTrait_RenderSprites::gfx_cleanup(
        ice::gfx::GfxFrame& gfx_frame,
        ice::gfx::GfxDevice& gfx_device
    ) noexcept
    {
        using namespace ice::gfx;
        using namespace ice::render;

        for (ice::detail::RenderData_Sprite const& entry : _sprite_materials)
        {
            destroy_resource_material(gfx_device, entry);
        }

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
    }

    void IceWorldTrait_RenderSprites::gfx_update(
        ice::EngineFrame const& engine_frame,
        ice::gfx::GfxFrame& gfx_frame,
        ice::gfx::GfxDevice& gfx_device
    ) noexcept
    {
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

        if (_render_camera_buffer != ice::render::Buffer::Invalid)
        {
            ice::Span<detail::SpriteInstance> const* instances = engine_frame.storage().named_object<ice::Span<detail::SpriteInstance>>("ice.sprite.instances_span"_sid);
            update_resource_data(gfx_device, *instances);

            gfx_frame.set_stage_slot(ice::Constant_GfxStage_DrawSprites, this);
        }
    }

    void IceWorldTrait_RenderSprites::on_activate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        _asset_system = ice::addressof(engine.asset_storage());

        _shader_data[0] = ice::wait_for(ice::detail::load_sprites_shader(*_asset_system, "shaders/game2d/sprite-vtx"));
        _shader_data[1] = ice::wait_for(ice::detail::load_sprites_shader(*_asset_system, "shaders/game2d/sprite-pix"));

        portal.storage().create_named_object<detail::SpriteQuery::Query>(
            detail::SpriteQueryId,
            portal.entity_storage().create_query(portal.allocator(), detail::SpriteQuery{})
        );
    }

    void IceWorldTrait_RenderSprites::on_deactivate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        portal.storage().destroy_named_object<detail::SpriteQuery::Query>(detail::SpriteQueryId);

        _asset_system = nullptr;
    }

    void IceWorldTrait_RenderSprites::on_update(
        ice::EngineFrame& frame,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        IPT_ZONE_SCOPED_NAMED("[Trait] Sprites :: Update");

        detail::SpriteQuery::Query const& query = *portal.storage().named_object<detail::SpriteQuery::Query>(detail::SpriteQueryId);

        ice::u32 next_instance_idx = 0;
        ice::HashMap<ice::u32> instance_info_idx{ frame.allocator() };
        ice::hashmap::reserve(instance_info_idx, 64);

        ice::Span<detail::SpriteInstanceInfo> instance_infos = frame.storage().create_named_span<detail::SpriteInstanceInfo>("ice.sprite.instance_infos"_sid, 64);

        ice::u32 valid_entity_count = 0;
        ice::ecs::query::for_each_entity(
            query,
            [&](
                ice::Transform2DStatic const* xform,
                ice::Transform2DDynamic const* dyn_xform,
                ice::Sprite const& sprite,
                ice::SpriteTile const* sprite_tile
            ) noexcept
            {
                if (xform == nullptr && dyn_xform == nullptr)
                {
                    return;
                }

                ice::u64 const material_hash = ice::hash(stringid(sprite.material));

                if (ice::hashmap::has(_sprite_materials, material_hash))
                {
                    ice::u32 const info_idx = ice::hashmap::get(instance_info_idx, material_hash, next_instance_idx);
                    detail::SpriteInstanceInfo& instance_info = instance_infos[info_idx];

                    if (info_idx == next_instance_idx)
                    {
                        ice::hashmap::set(instance_info_idx, material_hash, info_idx);

                        instance_info.materialid = ice::stringid_hash(ice::stringid(sprite.material));
                        instance_info.instance_offset = std::numeric_limits<ice::u32>::max();
                        instance_info.instance_count = 0;
                        next_instance_idx += 1;
                    }

                    instance_info.instance_count += 1;
                    valid_entity_count += 1;
                }
            }
        );

        frame.storage().create_named_object<ice::Span<detail::SpriteInstanceInfo>>(
            "ice.sprite.instance_infos_span"_sid,
            ice::span::subspan(instance_infos, 0, next_instance_idx)
        );


        ice::u32 current_instance_offset = 0;
        ice::Span<detail::SpriteInstance> instances = frame.storage().create_named_span<detail::SpriteInstance>("ice.sprite.instances"_sid, valid_entity_count);

        frame.storage().create_named_object<ice::Span<detail::SpriteInstance>>("ice.sprite.instances_span"_sid, instances);

        ice::ecs::query::for_each_entity(
            query,
            [&](ice::Transform2DStatic const* xform, ice::Transform2DDynamic const* dyn_xform, ice::Sprite const& sprite, ice::SpriteTile const* sprite_tile) noexcept
            {
                ice::u64 const material_hash = ice::hash(ice::stringid(sprite.material));
                if (ice::hashmap::has(instance_info_idx, material_hash) == false)
                {
                    return;
                }

                ice::vec2i tile{ 0, 0 };
                if (sprite_tile != nullptr)
                {
                    tile = { ice::i32(sprite_tile->material_tile.x), ice::i32(sprite_tile->material_tile.y) };
                }

                ice::u32 const info_idx = ice::hashmap::get(instance_info_idx, material_hash, std::numeric_limits<ice::u32>::max());

                detail::SpriteInstanceInfo& instance_info = instance_infos[info_idx];
                if (instance_info.instance_offset == std::numeric_limits<ice::u32>::max())
                {
                    instance_info.instance_offset = current_instance_offset;
                    instance_info.next_instance = current_instance_offset;
                    current_instance_offset += instance_info.instance_count;
                }

                if (dyn_xform != nullptr)
                {
                    instances[instance_info.next_instance] = detail::SpriteInstance
                    {
                        .position = dyn_xform->position,
                        .scale = dyn_xform->scale,
                        .tile_offset = tile
                    };
                    instance_info.next_instance += 1;
                }
                else if (xform != nullptr)
                {
                    instances[instance_info.next_instance] = detail::SpriteInstance
                    {
                        .position = xform->position,
                        .scale = xform->scale,
                        .tile_offset = tile
                    };
                    instance_info.next_instance += 1;
                }
            }
        );

        ice::ecs::query::for_each_entity(
            query,
            [&](ice::Transform2DStatic const*, ice::Transform2DDynamic const*, ice::Sprite const& sprite, ice::SpriteTile const* sprite_tile) noexcept
            {
                ice::u64 const material_hash = ice::hash(sprite.material);
                if (ice::hashmap::has(_sprite_materials, material_hash) == false)
                {
                    runner.execute_task(
                        task_load_resource_material(sprite.material, runner, runner.graphics_device()),
                        EngineContext::EngineRunner
                    );
                }
            }
        );
    }

    void IceWorldTrait_RenderSprites::record_commands(
        ice::gfx::GfxContext const& context,
        ice::EngineFrame const& frame,
        ice::render::CommandBuffer cmds,
        ice::render::RenderCommands& api
    ) const noexcept
    {
        IPT_ZONE_SCOPED_NAMED("[Trait] Sprites :: Graphics Commands");

        ice::Span<detail::SpriteInstanceInfo> const* const instances = frame.storage().named_object<ice::Span<detail::SpriteInstanceInfo>>("ice.sprite.instance_infos_span"_sid);
        if (instances != nullptr)
        {
            api.bind_pipeline(cmds, _pipeline);
            api.bind_resource_set(cmds, _pipeline_layout, _resource_sets[0], 0);
            api.bind_vertex_buffer(cmds, _vertex_buffer, 0);
            api.bind_vertex_buffer(cmds, _instance_buffer, 1);

            for (detail::SpriteInstanceInfo const& instance : *instances)
            {
                static detail::RenderData_Sprite no_data{ .material = { ice::render::Image::Invalid } };
                detail::RenderData_Sprite const& sprite_render_data = ice::hashmap::get(_sprite_materials, ice::hash(instance.materialid), no_data);
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

    void IceWorldTrait_RenderSprites::update_resource_camera(
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

    void IceWorldTrait_RenderSprites::update_resource_data(
        ice::gfx::GfxDevice& gfx_device,
        ice::Span<detail::SpriteInstance> instances
    ) noexcept
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
    }

    void IceWorldTrait_RenderSprites::destroy_resource_material(
        ice::gfx::GfxDevice& gfx_device,
        ice::detail::RenderData_Sprite const& sprite_data
    ) noexcept
    {
        using namespace ice::render;
        RenderDevice& device = gfx_device.device();

        device.destroy_buffer(sprite_data.material_tileinfo[0]);
        device.destroy_resourcesets(sprite_data.sprite_resource);
    }

    auto IceWorldTrait_RenderSprites::task_load_resource_material(
        ice::String material_name,
        ice::EngineRunner& runner,
        ice::gfx::GfxDevice& gfx_device
    ) noexcept -> ice::Task<>
    {
        using namespace ice::render;

        if (ice::hashmap::has(_sprite_materials, ice::hash(ice::stringid(material_name))))
        {
            co_return;
        }

        // Set a dummy value so we can check for it and skip loading the same asset more than once.
        ice::hashmap::set(_sprite_materials, ice::hash(ice::stringid(material_name)), { });

        Asset image_asset = runner.asset_storage().bind(ice::render::AssetType_Texture2D, material_name);
        Data image_data = co_await image_asset[AssetState::Loaded];
        //ICE_ASSERT(asset_check(image_data, AssetState::Loaded), "Image not available!");

        ice::Metadata const& metadata = image_asset.metadata();
        ImageInfo const* image_info = reinterpret_cast<ImageInfo const*>(image_data.location);

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
                /*ice::stringid_hint(material_name)*/ "<unsupported_string_value>"
            );
            co_return;
        }

        image_data = co_await image_asset[AssetState::Runtime];
        ICE_ASSERT(image_data.location != nullptr, "Image not available!");

        ice::u64 const tile_mesh_id = (static_cast<ice::u64>(tile_width) << 32) | tile_height;
        bool const has_vertex_offsets = ice::hashmap::has(_vertex_offsets, tile_mesh_id) == false;

        ice::u32 const vertex_offset = ice::hashmap::get(
            _vertex_offsets,
            tile_mesh_id,
            _vertex_offsets._count * 4
        );

        if (has_vertex_offsets == false)
        {
            ice::hashmap::set(_vertex_offsets, tile_mesh_id, vertex_offset);
        }

        ice::detail::RenderData_Sprite sprite_data{ };
        sprite_data.shape_offset = vertex_offset;
        sprite_data.shape_vertices = 4;
        sprite_data.material_scale = ice::vec2f{
            ice::f32(tile_width) / ice::f32(image_info->width),
            ice::f32(tile_height) / ice::f32(image_info->height),
        };

        ice::gfx::GfxFrame& gfx_frame = runner.graphics_frame();

        ice::vec4f vertices[4];

        co_await gfx_frame.frame_begin();

        RenderDevice& device = gfx_device.device();

        sprite_data.material[0] = *reinterpret_cast<ice::render::Image const*>(image_data.location);
        sprite_data.material_tileinfo[0] = device.create_buffer(BufferType::Uniform, sizeof(sprite_data.material_scale));
        device.create_resourcesets({ _resource_set_layouts + 1, 1 }, sprite_data.sprite_resource);

        ice::u32 update_count = 1;
        ice::render::BufferUpdateInfo updates[2]{
            ice::render::BufferUpdateInfo
            {
                .buffer = sprite_data.material_tileinfo[0],
                .data = {
                    .location = ice::addressof(sprite_data.material_scale),
                    .size = { sizeof(sprite_data.material_scale) },
                    .alignment = ice::ualign::b_4
                }
            }
        };

        if (has_vertex_offsets)
        {
            vertices[0] = { 0.f, ice::f32(tile_height) * 1, 0.f, 0.f };
            vertices[1] = { 0.f, 0.f, 0.f, 1.f };
            vertices[2] = { ice::f32(tile_width) * 1, 0.f, 1.f, 1.f };
            vertices[3] = { ice::f32(tile_width) * 1, ice::f32(tile_height) * 1, 1.f, 0.f };

            updates[1].buffer = _vertex_buffer;
            updates[1].data = ice::data_view(vertices);
            updates[1].offset = vertex_offset * sizeof(ice::vec4f);
            update_count += 1;
        }

        device.update_buffers({ updates, update_count });

        co_await runner.schedule_next_frame();

        runner.execute_task(
            task_update_resource_material(runner, gfx_device, ice::stringid(material_name), sprite_data),
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

        co_await runner.graphics_frame().frame_begin();

        RenderDevice& device = gfx_device.device();

        ResourceUpdateInfo res_updates[]{
            ResourceUpdateInfo
            {
                .image = sprite_data.material[0]
            },
            ResourceUpdateInfo
            {
                .uniform_buffer = ResourceBufferInfo{
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

        ice::hashmap::set(_sprite_materials, ice::hash(material_name), sprite_data);
        co_return;
    }

    void register_trait_render_sprites(
        ice::WorldTraitArchive& archive
    ) noexcept
    {
        static constexpr ice::StringID trait_dependencies[]{
            Constant_TraitName_RenderClear,
        };

        archive.register_trait(
            ice::Constant_TraitName_RenderSprites,
            ice::WorldTraitDescription
            {
                .factory = ice::detail::generic_trait_factory<IceWorldTrait_RenderSprites>,
                .required_dependencies = trait_dependencies
            }
        );
    }

} // namespace ice
