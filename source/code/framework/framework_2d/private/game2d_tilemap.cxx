#include <ice/game2d_tilemap.hxx>

#include <ice/engine.hxx>
#include <ice/engine_runner.hxx>
#include <ice/engine_frame.hxx>

#include <ice/world/world_portal.hxx>
#include <ice/entity/entity_storage.hxx>
#include <ice/archetype/archetype_index.hxx>
#include <ice/archetype/archetype_query.hxx>

#include <ice/gfx/gfx_stage.hxx>
#include <ice/gfx/gfx_resource_tracker.hxx>
#include <ice/gfx/gfx_device.hxx>
#include <ice/gfx/gfx_subpass.hxx>
#include <ice/gfx/gfx_frame.hxx>
#include <ice/gfx/gfx_task.hxx>

#include <ice/render/render_image.hxx>
#include <ice/render/render_device.hxx>
#include <ice/render/render_buffer.hxx>
#include <ice/render/render_resource.hxx>
#include <ice/render/render_pipeline.hxx>

#include <ice/memory/proxy_allocator.hxx>

#include <ice/asset_system.hxx>

namespace ice
{

    namespace detail
    {

        static constexpr ice::f32 Constant_TileSize = 32.f;

        static ice::vec2f Constant_TileShapeVertices[]{
            { 0.0, Constant_TileSize }, // pos
            { 0.0, 0.0 }, // uv
            { 0.0, 0.0 }, // pos
            { 0.0, 1.f }, // uv
            { Constant_TileSize, 0 }, // pos
            { 1.f, 1.f }, // uv
            { Constant_TileSize, Constant_TileSize }, // pos
            { 1.f, 0.0 }, // uv
        };

        static constexpr ice::f32 Constant_TileWidth = 16.f;
        static constexpr ice::f32 Constant_TileHeight = 16.f;

        using TileMapQuery = ice::ComponentQuery<ice::TileMapComponent const&>;

        struct IceTileSharedRenderObjects
        {
            ice::render::ResourceSetLayout resource_layout[3];
            ice::render::ResourceSet resource_set;
            ice::render::PipelineLayout pipeline_layout;
            ice::render::Pipeline pipeline;

            ice::render::Shader shaders[2];
            ice::render::ShaderStageFlags shader_stages[2]{
                ice::render::ShaderStageFlags::VertexStage,
                ice::render::ShaderStageFlags::FragmentStage
            };

            ice::render::Sampler sampler;

            ice::render::Buffer tile_shape_buffer;
        };

        struct IceTileMapRenderObjects
        {
            ice::render::Buffer material_buffer;
            ice::render::Buffer position_buffer;
            ice::render::Buffer tile_props_buffer;
            ice::render::ResourceSet material_resource;

            ice::u32 image_count;
            ice::render::ImageInfo const* image_infos[4];
            ice::render::Image images[4];
            ice::vec4f image_tile_scale[4];;
        };

        struct IceRoomDrawCall
        {
            ice::u32 instance_count;
            ice::u32 instance_base_offset;
            ice::render::Buffer instance_buffer;
        };

        class IceTileMap : public ice::gfx::GfxStage
        {
        public:
            IceTileMap(
                ice::TileMap const* tilemap_data,
                ice::detail::IceTileSharedRenderObjects const& render_shared
            ) noexcept
                : _tilemap_data{ tilemap_data }
                , _render_shared{ render_shared }
                , _render_objects{ }
                , _loaded{ false }
                , _loading{ false }
            {
            }

            ~IceTileMap() noexcept = default;

            bool loaded() const noexcept
            {
                return _loaded;
            }

            bool loading() const noexcept
            {
                return _loading;
            }

            void set_loading() noexcept
            {
                _loading = true;
            }

            void record_commands(
                ice::EngineFrame const& frame,
                ice::render::CommandBuffer cmds,
                ice::render::RenderCommands& api
            ) noexcept
            {
                api.bind_vertex_buffer(cmds, _render_shared.tile_shape_buffer, 0);
                api.bind_vertex_buffer(cmds, _render_objects.position_buffer, 1);
                api.bind_vertex_buffer(cmds, _render_objects.material_buffer, 2);

                api.bind_pipeline(cmds, _render_shared.pipeline);
                api.bind_resource_set(
                    cmds,
                    _render_shared.pipeline_layout,
                    _render_objects.material_resource,
                    1
                );
                api.bind_resource_set(
                    cmds,
                    _render_shared.pipeline_layout,
                    _render_shared.resource_set,
                    2
                );

                ice::u32 buffer_tile_offset = 0;
                for (ice::TileRoom const& room : _tilemap_data->rooms)
                {
                    api.draw(
                        cmds,
                        4, room.tiles_count,
                        0, buffer_tile_offset
                    );

                    buffer_tile_offset += room.tiles_count;
                }

            }

            void create_render_data(
                ice::AssetSystem& asset_system,
                ice::gfx::GfxDevice& gfx_device,
                ice::render::ResourceSetLayout image_resource_layout
            ) noexcept
            {
                using namespace ice::render;

                ice::render::RenderDevice& device = gfx_device.device();

                //ice::u32 total_material_variation_count = 0;
                //ice::u32 last_material = std::numeric_limits<ice::u32>::max();

                _render_objects.image_count = ice::size(_tilemap_data->material_groups);
                device.create_resourcesets({ &image_resource_layout, 1 }, { &_render_objects.material_resource, 1 });

                for (ice::u32 image_idx = 0; image_idx < _render_objects.image_count; ++image_idx)
                {
                    ice::TileMaterialGroup const& material_group = _tilemap_data->material_groups[image_idx];

                    ice::Asset image_asset = asset_system.request(AssetType::Texture, material_group.image_asset);
                    ICE_ASSERT(image_asset != Asset::Invalid, "Image asset {} does not exist", ice::stringid_hint(material_group.image_asset));

                    ice::Data image_data;
                    ice::AssetStatus status = ice::asset_data(image_asset, image_data);
                    ICE_ASSERT(status == AssetStatus::Loaded, "Counldn't load image asset {}", ice::stringid_hint(material_group.image_asset));

                    ice::render::ImageInfo const* asset_image_info = reinterpret_cast<ice::render::ImageInfo const*>(image_data.location);

                    ImageInfo image_info = *asset_image_info;
                    image_info.usage = ImageUsageFlags::TransferDst | ImageUsageFlags::Sampled;

                    _render_objects.image_infos[image_idx] = asset_image_info;
                    _render_objects.images[image_idx] = device.create_image(image_info, {});
                    _render_objects.image_tile_scale[image_idx] = {
                        Constant_TileWidth / static_cast<ice::f32>(asset_image_info->width),
                        Constant_TileHeight / static_cast<ice::f32>(asset_image_info->height),
                        0.f, 0.f
                    };
                }

                ice::u32 total_tile_count = 0;
                for (ice::TileRoom const& room : _tilemap_data->rooms)
                {
                    total_tile_count += room.tiles_count;
                }

                ice::u32 const instance_position_buffer_size =
                    sizeof(ice::vec2f) * total_tile_count;
                ice::u32 const instance_material_buffer_size =
                    sizeof(ice::TileMaterial) * total_tile_count;

                _render_objects.position_buffer = device.create_buffer(BufferType::Vertex, instance_position_buffer_size);
                _render_objects.material_buffer = device.create_buffer(BufferType::Vertex, instance_material_buffer_size);
                _render_objects.tile_props_buffer = device.create_buffer(BufferType::Uniform, sizeof(ice::vec4f) * 4);
            }

            void destroy_render_data(
                ice::gfx::GfxDevice& gfx_device
            ) noexcept
            {
                using namespace ice::render;

                ice::render::RenderDevice& device = gfx_device.device();
                for (ice::u32 image_idx = 0; image_idx < _render_objects.image_count; ++image_idx)
                {
                    device.destroy_image(_render_objects.images[image_idx]);
                }

                device.destroy_buffer(_render_objects.tile_props_buffer);
                device.destroy_buffer(_render_objects.position_buffer);
                device.destroy_buffer(_render_objects.material_buffer);

                device.destroy_resourcesets({ &_render_objects.material_resource, 1 });

                _loaded = false;
            }

            auto update_images(ice::render::RenderDevice& device) noexcept -> ice::Task<>
            {
                using ice::render::ResourceUpdateInfo;
                using ice::render::ResourceSetUpdateInfo;

                ResourceUpdateInfo resource_updates[5]{
                    ResourceUpdateInfo{.image = _render_objects.images[0] },
                    ResourceUpdateInfo{.image = _render_objects.images[1] },
                    ResourceUpdateInfo{.image = _render_objects.images[2] },
                    ResourceUpdateInfo{.image = _render_objects.images[3] },
                    ResourceUpdateInfo{
                        .uniform_buffer =
                        {
                            .buffer = _render_objects.tile_props_buffer,
                            .offset = 0,
                            .size = sizeof(ice::vec4f) * 4
                        },
                    }
                };

                ResourceSetUpdateInfo resourceset_update[2]{
                    ResourceSetUpdateInfo
                    {
                        .resource_set = _render_objects.material_resource,
                        .resource_type = ice::render::ResourceType::SampledImage,
                        .binding_index = 1,
                        .array_element = 0,
                        .resources = { resource_updates, _render_objects.image_count },
                    },
                    ResourceSetUpdateInfo
                    {
                        .resource_set = _render_objects.material_resource,
                        .resource_type = ice::render::ResourceType::UniformBuffer,
                        .binding_index = 0,
                        .array_element = 0,
                        .resources = { resource_updates + 4, 1 },
                    }
                };

                device.update_resourceset(resourceset_update);

                co_await update_buffers(device);
            }

            auto update_buffers(ice::render::RenderDevice& device) noexcept -> ice::Task<>
            {
                using ice::render::BufferUpdateInfo;

                static ice::u32 const RequiredBufferSize = ice::size(_tilemap_data->rooms) * 2 * sizeof(BufferUpdateInfo);
                ICE_ASSERT(RequiredBufferSize < 512, "Buffer to small!");

                ice::memory::StackAllocator_512 stack_alloc;
                ice::pod::Array<BufferUpdateInfo> buffer_updates{ stack_alloc };
                ice::pod::array::reserve(buffer_updates, ice::size(_tilemap_data->rooms) * 2 + 1);

                ice::u32 total_tile_count = 0;
                for (ice::TileRoom const& room : _tilemap_data->rooms)
                {
                    ice::pod::array::push_back(
                        buffer_updates,
                        BufferUpdateInfo
                        {
                            .buffer = _render_objects.position_buffer,
                            .data = ice::data_view(room.tiles_position),
                            .offset = total_tile_count * sizeof(ice::vec2f)
                        }
                    );
                    ice::pod::array::push_back(
                        buffer_updates,
                        BufferUpdateInfo
                        {
                            .buffer = _render_objects.material_buffer,
                            .data = ice::data_view(room.tiles_material),
                            .offset = total_tile_count * sizeof(ice::TileMaterial)
                        }
                    );

                    total_tile_count += room.tiles_count;
                }

                ice::pod::array::push_back(
                    buffer_updates,
                    BufferUpdateInfo
                    {
                        .buffer = _render_objects.tile_props_buffer,
                        .data = ice::data_view(_render_objects.image_tile_scale),
                        .offset = 0
                    }
                );

                device.update_buffers(buffer_updates);
                _loaded = true;
                _loading = false;
                co_return;
            }

            void update_images(
                ice::gfx::GfxDevice& device,
                ice::gfx::GfxFrame& frame
            ) noexcept
            {
                auto image_task = [](
                    ice::gfx::GfxDevice& device,
                    ice::gfx::GfxFrame& frame,
                    ice::render::Image image,
                    ice::render::ImageInfo image_info
                ) -> ice::Task<>
                {
                    ice::gfx::GfxTaskLoadImage const task_info{
                        .image = image,
                        .image_info = image_info
                    };

                    co_await ice::gfx::load_image_data_task(
                        device,
                        frame,
                        task_info
                    );
                };

                for (ice::u32 image_idx = 0; image_idx < _render_objects.image_count; ++image_idx)
                {
                    frame.execute_task(
                        image_task(device, frame, _render_objects.images[image_idx], *_render_objects.image_infos[image_idx])
                    );
                }
            }

        private:
            ice::TileMap const* const _tilemap_data;
            ice::detail::IceTileSharedRenderObjects const& _render_shared;
            ice::detail::IceTileMapRenderObjects _render_objects;

            bool _loaded;
            bool _loading;
        };


        auto load_tile_texture(ice::AssetSystem& assets, ice::StringID name) noexcept -> ice::render::ImageInfo
        {
            ice::render::ImageInfo result{ };
            Asset const shader_asset = assets.request(ice::AssetType::Texture, name);
            if (shader_asset != Asset::Invalid)
            {
                Data temp;
                if (ice::asset_data(shader_asset, temp) == AssetStatus::Loaded)
                {
                    result = *reinterpret_cast<ice::render::ImageInfo const*>(temp.location);
                }
            }

            return result;
        }

        auto load_tile_shader(ice::AssetSystem& assets, ice::StringID name) noexcept -> ice::Data
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

    } // namespace detail

    class IceTileMap2D : public ice::TileMap2DTrait
    {
    public:
        IceTileMap2D(
            ice::Allocator& alloc,
            ice::AssetSystem& assets
        ) noexcept;
        ~IceTileMap2D() noexcept override;

        auto load_tilemap(
            ice::StringID_Arg map_name
        ) noexcept -> ice::Task<ice::TileMap const*> override;

        void load_tilemap(
            ice::StringID_Arg name,
            ice::TileMap const& tilemap
        ) noexcept override;

        auto unload_tilemap(
            ice::StringID_Arg map_name
        ) noexcept -> ice::Task<> override;

        auto move_entity(
            ice::Entity entity,
            ice::StringID_Arg target_portal
        ) noexcept -> ice::Task<> override;

        void on_activate(
            ice::Engine& engine,
            ice::EngineRunner& runner,
            ice::WorldPortal& portal
        ) noexcept override;

        void on_deactivate(
            ice::Engine& engine,
            ice::EngineRunner& runner,
            ice::WorldPortal& portal
        ) noexcept override;

        void on_update(
            ice::EngineFrame& frame,
            ice::EngineRunner& runner,
            ice::WorldPortal& portal
        ) noexcept override;

    protected:
        auto update_buffers(ice::render::RenderDevice& device) noexcept -> ice::Task<void>
        {
            using namespace ice::render;

            ResourceUpdateInfo res[1]{
                ResourceUpdateInfo{.sampler = _render_shared.sampler }
            };
            ResourceSetUpdateInfo resu[1]{
                ResourceSetUpdateInfo{
                    .resource_set = _render_shared.resource_set,
                    .resource_type = ResourceType::Sampler,
                    .binding_index = 0,
                    .array_element = 0,
                    .resources = res
                }
            };
            device.update_resourceset(resu);

            BufferUpdateInfo updates[]{
                BufferUpdateInfo{.buffer = _render_shared.tile_shape_buffer, .data = ice::data_view(detail::Constant_TileShapeVertices) },
            };

            device.update_buffers(updates);
            co_return;
        }

    private:
        ice::memory::ProxyAllocator _allocator;
        ice::AssetSystem& _assets;

        ice::pod::Hash<detail::IceTileMap*> _loaded_tilemaps;
        detail::IceTileSharedRenderObjects _render_shared;
    };

    IceTileMap2D::IceTileMap2D(
        ice::Allocator& alloc,
        ice::AssetSystem& assets
    ) noexcept
        : _allocator{ alloc, "tile-map-alloc" }
        , _assets{ assets }
        , _loaded_tilemaps{ _allocator }
    {
        ice::pod::hash::reserve(_loaded_tilemaps, 10);
    }

    IceTileMap2D::~IceTileMap2D() noexcept
    {
        for (auto const& entry : _loaded_tilemaps)
        {
            _allocator.destroy(entry.value);
        }
    }

    void IceTileMap2D::load_tilemap(
        ice::StringID_Arg name,
        ice::TileMap const& tilemap
    ) noexcept
    {
        detail::IceTileMap* tilemap_internal = _allocator.make<detail::IceTileMap>(&tilemap, _render_shared);


        ICE_ASSERT(
            ice::pod::hash::has(_loaded_tilemaps, ice::hash(name)) == false,
            "Tilemap {} already loaded!",
            ice::stringid_hint(name)
        );

        ice::pod::hash::set(
            _loaded_tilemaps,
            ice::hash(name),
            tilemap_internal
        );
    }

    auto IceTileMap2D::load_tilemap(
        ice::StringID_Arg map_name
    ) noexcept -> ice::Task<ice::TileMap const*>
    {
        ice::Asset tilemap_asset = _assets.request(ice::AssetType::TileMap, map_name);
        if (tilemap_asset == Asset::Invalid)
        {
            ICE_LOG(
                ice::LogSeverity::Error, ice::LogTag::Engine,
                "Failed to access tile map asset [{}]",
                ice::stringid_hint(map_name)
            );
            co_return nullptr;
        }

        ice::Data tilemap_data;
        ice::AssetStatus tilemap_status = ice::asset_data(tilemap_asset, tilemap_data);
        ICE_ASSERT(
            tilemap_status == AssetStatus::Loaded,
            "Failed to load tilemap asset [{}]",
            ice::stringid_hint(map_name)
        );

        ice::TileMap const* const tilemap = reinterpret_cast<ice::TileMap const*>(tilemap_data.location);
        detail::IceTileMap* tilemap_internal = _allocator.make<detail::IceTileMap>(tilemap, _render_shared);


        ICE_ASSERT(
            ice::pod::hash::has(_loaded_tilemaps, ice::hash(map_name)) == false,
            "Tilemap {} already loaded!",
            ice::stringid_hint(map_name)
        );

        ice::pod::hash::set(
            _loaded_tilemaps,
            ice::hash(map_name),
            tilemap_internal
        );

        co_return tilemap;
    }

    auto IceTileMap2D::unload_tilemap(
        ice::StringID_Arg map_name
    ) noexcept -> ice::Task<>
    {
        ICE_ASSERT(
            ice::pod::hash::has(_loaded_tilemaps, ice::hash(map_name)) == true,
            "Tilemap {} not loaded!",
            ice::stringid_hint(map_name)
        );

        detail::IceTileMap* tilemap = ice::pod::hash::get(
            _loaded_tilemaps,
            ice::hash(map_name),
            nullptr
        );
        _allocator.destroy(tilemap);

        ice::pod::hash::remove(
            _loaded_tilemaps,
            ice::hash(map_name)
        );
        co_return;
    }

    auto IceTileMap2D::move_entity(
        ice::Entity entity,
        ice::StringID_Arg target_portal
    ) noexcept -> ice::Task<>
    {
        co_return;
    }

    void IceTileMap2D::on_activate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        portal.storage().create_named_object<detail::TileMapQuery>(
            "ice.tilemap.query"_sid,
            _allocator,
            portal.entity_storage().archetype_index()
        );

        using namespace ice::gfx;
        using namespace ice::render;

        GfxResourceTracker& gfxres = runner.graphics_device().resource_tracker();
        RenderDevice& device = runner.graphics_device().device();

        Data vtx_shader_data = ice::detail::load_tile_shader(engine.asset_system(), "/shaders/game2d/tiled-vtx"_sid);
        Data pix_shader_data = ice::detail::load_tile_shader(engine.asset_system(), "/shaders/game2d/tiled-pix"_sid);

        ResourceSetLayoutBinding resourceset_binding[]{
            ResourceSetLayoutBinding{
                .binding_index = 0,
                .resource_count = 1,
                .resource_type = ResourceType::UniformBuffer,
                .shader_stage_flags = ShaderStageFlags::FragmentStage
            },
            ResourceSetLayoutBinding{
                .binding_index = 1,
                .resource_count = 4,
                .resource_type = ResourceType::SampledImage,
                .shader_stage_flags = ShaderStageFlags::FragmentStage
            },
            ResourceSetLayoutBinding{
                .binding_index = 0,
                .resource_count = 1,
                .resource_type = ResourceType::Sampler,
                .shader_stage_flags = ShaderStageFlags::FragmentStage
            },
        };

        _render_shared.resource_layout[0] = find_resource<ResourceSetLayout>(gfxres, GfxSubpass_Primitives::ResName_ResourceLayout);
        _render_shared.resource_layout[1] = device.create_resourceset_layout({ resourceset_binding + 0, 2 });
        _render_shared.resource_layout[2] = device.create_resourceset_layout({ resourceset_binding + 2, 1 });

        device.create_resourcesets({ _render_shared.resource_layout + 2, 1 }, { &_render_shared.resource_set, 1 });

        PipelineLayoutInfo pipeline_layout_info{
            .push_constants = { },
            .resource_layouts = _render_shared.resource_layout
        };

        _render_shared.pipeline_layout = device.create_pipeline_layout(pipeline_layout_info);

        _render_shared.shaders[0] = device.create_shader(ShaderInfo{ .shader_data = vtx_shader_data });
        _render_shared.shaders[1] = device.create_shader(ShaderInfo{ .shader_data = pix_shader_data });

        _render_shared.tile_shape_buffer = device.create_buffer(BufferType::Vertex, 128 * 16);

        ImageInfo tiles_texture_info = ice::detail::load_tile_texture(engine.asset_system(), "/cotm/tileset_a"_sid);

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

        _render_shared.sampler = device.create_sampler(sampler_info);

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
                .offset = 0,
                .type = ShaderAttribType::Vec1i
            },
            //ShaderInputAttribute{
            //    .location = 4,
            //    .offset = 4,
            //    .type = ShaderAttribType::Vec1i
            //},
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
                .stride = 8,
                .instanced = true,
                .attributes = { attribs + 2, 1 }
            },
            ShaderInputBinding{
                .binding = 2,
                .stride = 4,
                .instanced = true,
                .attributes = { attribs + 3, 1 }
            },
        };

        PipelineInfo pipeline_info{
            .layout = _render_shared.pipeline_layout,
            .renderpass = find_resource<Renderpass>(gfxres, "renderpass.default"_sid),
            .shaders = _render_shared.shaders,
            .shaders_stages = _render_shared.shader_stages,
            .shader_bindings = bindings,
            .primitive_topology = PrimitiveTopology::TriangleFan,
            .cull_mode = CullMode::Disabled,
            .front_face = FrontFace::CounterClockWise,
            .subpass_index = 1,
            .depth_test = true
        };

        _render_shared.pipeline = device.create_pipeline(pipeline_info);

        runner.graphics_frame().execute_task(update_buffers(device));
    }

    void IceTileMap2D::on_deactivate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        using namespace ice::gfx;
        using namespace ice::render;

        RenderDevice& device = runner.graphics_device().device();

        for (auto const& entry : _loaded_tilemaps)
        {
            if (entry.value->loaded())
            {
                entry.value->destroy_render_data(runner.graphics_device());
            }
        }

        device.destroy_pipeline(_render_shared.pipeline);

        device.destroy_buffer(_render_shared.tile_shape_buffer);

        device.destroy_shader(_render_shared.shaders[1]);
        device.destroy_shader(_render_shared.shaders[0]);

        device.destroy_sampler(_render_shared.sampler);

        device.destroy_resourcesets({ &_render_shared.resource_set, 1 });

        device.destroy_pipeline_layout(_render_shared.pipeline_layout);
        device.destroy_resourceset_layout(_render_shared.resource_layout[2]);
        device.destroy_resourceset_layout(_render_shared.resource_layout[1]);

        portal.storage().destroy_named_object<ice::detail::TileMapQuery>(
            "ice.tilemap.query"_sid
        );
    }

    void IceTileMap2D::on_update(
        ice::EngineFrame& frame,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        detail::TileMapQuery const* const query = portal.storage().named_object<detail::TileMapQuery>("ice.tilemap.query"_sid);
        detail::TileMapQuery::ResultByEntity query_result = query->result_by_entity(_allocator, portal.entity_storage());

        query_result.for_each(
            [&, this](ice::TileMapComponent const& cmp_tilemap) noexcept
            {
                detail::IceTileMap* const tilemap = ice::pod::hash::get(_loaded_tilemaps, ice::hash(cmp_tilemap.name), nullptr);
                if (tilemap != nullptr)
                {
                    if (tilemap->loaded() == false)
                    {
                        if (tilemap->loading() == false)
                        {
                            tilemap->set_loading();
                            tilemap->create_render_data(_assets, runner.graphics_device(), _render_shared.resource_layout[1]);
                            runner.graphics_frame().execute_task(
                                tilemap->update_images(runner.graphics_device().device())
                            );
                            tilemap->update_images(runner.graphics_device(), runner.graphics_frame());
                        }
                        // wait
                    }
                    else
                    {
                        static ice::StringID deps[]{
                            "test.stage.clear"_sid,
                            "camera.update_view"_sid,
                        };

                        runner.graphics_device().aquire_pass("pass.default"_sid).add_stage(
                            "game2d.level"_sid, deps, tilemap
                        );
                    }
                }
            }
        );
    }

    auto create_tilemap_trait(
        ice::Allocator& alloc,
        ice::AssetSystem& assets
    ) noexcept -> ice::UniquePtr<TileMap2DTrait>
    {
        return ice::make_unique<ice::TileMap2DTrait, ice::IceTileMap2D>(alloc, alloc, assets);
    }

} // namespace ice
