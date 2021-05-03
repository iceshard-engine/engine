#include <ice/game2d_trait.hxx>
#include <ice/game2d_object.hxx>

#include <ice/engine.hxx>
#include <ice/engine_runner.hxx>
#include <ice/engine_frame.hxx>
#include <ice/world/world.hxx>

#include <ice/entity/entity_storage.hxx>
#include <ice/archetype/archetype_query.hxx>

#include <ice/gfx/gfx_pass.hxx>
#include <ice/gfx/gfx_frame.hxx>
#include <ice/gfx/gfx_stage.hxx>
#include <ice/gfx/gfx_device.hxx>
#include <ice/gfx/gfx_subpass.hxx>
#include <ice/gfx/gfx_resource_tracker.hxx>

#include <ice/asset.hxx>
#include <ice/asset_system.hxx>

#include <ice/render/render_device.hxx>
#include <ice/render/render_resource.hxx>
#include <ice/render/render_pipeline.hxx>
#include <ice/render/render_shader.hxx>
#include <ice/render/render_buffer.hxx>
#include <ice/render/render_image.hxx>

#include <ice/task.hxx>

namespace ice
{

    static constexpr ice::f32 Constant_TileWidth = 16;
    static constexpr ice::f32 Constant_TileHeight = 16;

    static ice::vec2f Constant_BoxShapeVertices[]{
        { 0.0, 0.0 },
        { 0.0, Constant_TileSize },
        { Constant_TileSize, Constant_TileSize },
        { Constant_TileSize, 0.0 },
    };

    static ice::vec2f Constant_BoxShapeUvs[]{
        { 3.0, 4.0 },
        { 3.0, 3.0 },
        { 4.0, 3.0 },
        { 4.0, 4.0 },
    };

    static ice::Obj2dShapeDefinition Constant_BoxShape
    {
        .shape_name = "shape.box2d"_sid,
        .vertices = Constant_BoxShapeVertices,
        .uvs = Constant_BoxShapeUvs,
        .vertex_count = ice::size(Constant_BoxShapeVertices)
    };

    auto load_texture(ice::AssetSystem& assets, ice::StringID name) noexcept -> ice::render::ImageInfo
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

    auto load_shader(ice::AssetSystem& assets, ice::StringID name) noexcept -> ice::Data
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

    struct Game2D_UpdateImage : public ice::gfx::GfxStage
    {
        Game2D_UpdateImage(
            ice::Allocator& alloc,
            ice::render::RenderDevice& device,
            ice::render::ImageInfo image_info,
            ice::render::Image image
        ) noexcept
            : _allocator{ alloc }
            , _device{ device }
            , _image_data{ }
            , _image_info{ image_info }
            , _image{ image }
        {
        }

        auto update_buffers() noexcept -> ice::Task<>
        {
            using namespace ice::render;

            ice::u32 const bytes = _image_info.width * _image_info.height * 4;

            _image_data = _device.create_buffer(BufferType::Transfer, bytes);

            BufferUpdateInfo updates[]{
                BufferUpdateInfo{ .buffer = _image_data, .data = { _image_info.data, bytes } }
            };

            _device.update_buffers(updates);
            co_return;
        }

        ~Game2D_UpdateImage() noexcept
        {
            _device.destroy_buffer(_image_data);
        }

        void record_commands(
            ice::EngineFrame const& frame,
            ice::render::CommandBuffer cmds,
            ice::render::RenderCommands& api
        ) noexcept
        {
            api.update_texture(cmds, _image, _image_data, { _image_info.width, _image_info.height });

            // We want to self-delete us!
            ice::Allocator& alloc = _allocator;
            alloc.destroy(this);
        }

    private:
        ice::Allocator& _allocator;
        ice::render::RenderDevice& _device;
        ice::render::Buffer _image_data;
        ice::render::ImageInfo _image_info;
        ice::render::Image _image;
    };

    struct Game2D_RenderObjects : public ice::gfx::GfxStage
    {
        ice::render::ResourceSetLayout resourceset_layouts[2];
        ice::render::PipelineLayout pipeline_layout;

        ice::render::Shader shaders[2];
        ice::render::ShaderStageFlags shader_stages[2]{
            ice::render::ShaderStageFlags::VertexStage,
            ice::render::ShaderStageFlags::FragmentStage
        };

        ice::render::ResourceSet resourceset;
        ice::render::Pipeline pipeline;

        ice::render::Buffer buffer_vtx[2];
        ice::render::Buffer buffer_idx;
        ice::render::Buffer buffer_inst;

        ice::render::Image textures;
        ice::render::Sampler sampler;

        struct TilemapProps
        {
            ice::f32 width_scale;
            ice::f32 height_scale;
        } tilemap_props;

        ice::render::Buffer tilemap_buffer;

        void record_commands(
            ice::EngineFrame const& frame,
            ice::render::CommandBuffer cmds,
            ice::render::RenderCommands& api
        ) noexcept
        {
            ice::u32 const instance_count = *frame.named_object<ice::u32>("game2d.level.instance_count"_sid);
            if (instance_count > 0)
            {
                api.bind_resource_set(cmds, pipeline_layout, resourceset, 1);
                api.bind_vertex_buffer(cmds, buffer_vtx[0], 0);
                api.bind_vertex_buffer(cmds, buffer_vtx[1], 1);
                api.bind_vertex_buffer(cmds, buffer_inst, 2);
                api.bind_pipeline(cmds, pipeline);
                api.draw(cmds, 4, instance_count, 0, 0);
            }
        }
    };

    class IceGame2DTrait : public ice::Game2DTrait
    {
    public:
        using ObjectQuery = ice::ComponentQuery<ice::Obj2dShape&, ice::Obj2dTransform&>;

        IceGame2DTrait(ice::Allocator& alloc) noexcept;
        ~IceGame2DTrait() noexcept override = default;

        void on_activate(
            ice::Engine& engine,
            ice::EngineRunner& runner,
            ice::World& world
        ) noexcept override
        {
            using namespace ice::gfx;
            using namespace ice::render;

            world.data_storage().create_named_object<ObjectQuery>(
                "game2d.level.query"_sid,
                _allocator, world.entity_storage().archetype_index()
            );

            GfxResourceTracker& gfxres = runner.graphics_device().resource_tracker();
            RenderDevice& device = runner.graphics_device().device();

            Data vtx_shader_data = load_shader(engine.asset_system(), "/shaders/game2d/tiled-vtx"_sid);
            Data pix_shader_data = load_shader(engine.asset_system(), "/shaders/game2d/tiled-pix"_sid);

            ResourceSetLayoutBinding resourceset_binding[]{
                ResourceSetLayoutBinding{
                    .binding_index = 0,
                    .resource_count = 1,
                    .resource_type = ResourceType::SampledImage,
                    .shader_stage_flags = ShaderStageFlags::FragmentStage
                },
                ResourceSetLayoutBinding{
                    .binding_index = 1,
                    .resource_count = 1,
                    .resource_type = ResourceType::Sampler,
                    .shader_stage_flags = ShaderStageFlags::FragmentStage
                },
                ResourceSetLayoutBinding{
                    .binding_index = 2,
                    .resource_count = 1,
                    .resource_type = ResourceType::UniformBuffer,
                    .shader_stage_flags = ShaderStageFlags::FragmentStage
                },
            };

            _render.resourceset_layouts[0] = find_resource<ResourceSetLayout>(gfxres, GfxSubpass_Primitives::ResName_ResourceLayout);
            _render.resourceset_layouts[1] = device.create_resourceset_layout({ resourceset_binding + 0, 3 });

            device.create_resourcesets({ _render.resourceset_layouts + 1, 1 }, { &_render.resourceset, 1 });

            PipelineLayoutInfo pipeline_layout_info{
                .push_constants = { },
                .resource_layouts = _render.resourceset_layouts
            };

            _render.pipeline_layout = device.create_pipeline_layout(pipeline_layout_info);

            _render.shaders[0] = device.create_shader(ShaderInfo{ .shader_data = vtx_shader_data });
            _render.shaders[1] = device.create_shader(ShaderInfo{ .shader_data = pix_shader_data });

            _render.buffer_vtx[0] = device.create_buffer(BufferType::Vertex, 1024 * 16);
            _render.buffer_vtx[1] = device.create_buffer(BufferType::Vertex, 1024 * 16);
            _render.buffer_inst = device.create_buffer(BufferType::Vertex, 1024 * 16);
            _render.buffer_idx = device.create_buffer(BufferType::Index, 1024 * 4);
            _render.tilemap_buffer = device.create_buffer(BufferType::Uniform, sizeof(_render.tilemap_props));

            ImageInfo tiles_texture_info = load_texture(engine.asset_system(), "/cotm/tileset_a"_sid);
            _render.textures = device.create_image(tiles_texture_info, { });

            _render.tilemap_props.width_scale = Constant_TileWidth / tiles_texture_info.width;
            _render.tilemap_props.height_scale = Constant_TileHeight / tiles_texture_info.height;

            SamplerInfo sampler_info{
                .min_filter = SamplerFilter::Linear,
                .mag_filter = SamplerFilter::Linear,
                .address_mode = {
                    .u = SamplerAddressMode::Repeat,
                    .v = SamplerAddressMode::Repeat,
                    .w = SamplerAddressMode::Repeat,
                },
                .mip_map_mode = SamplerMipMapMode::Linear,
            };

            _render.sampler = device.create_sampler(sampler_info);

            ShaderInputAttribute attribs[]{
                ShaderInputAttribute{
                    .location = 0,
                    .offset = 0,
                    .type = ShaderAttribType::Vec2f
                },
                ShaderInputAttribute{
                    .location = 1,
                    .offset = 0,
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
                    .type = ShaderAttribType::Vec1f
                },
            };

            ShaderInputBinding bindings[]{
                ShaderInputBinding{
                    .binding = 0,
                    .stride = 8,
                    .instanced = false,
                    .attributes = { attribs + 0, 1 }
                },
                ShaderInputBinding{
                    .binding = 1,
                    .stride = 8,
                    .instanced = false,
                    .attributes = { attribs + 1, 1 }
                },
                ShaderInputBinding{
                    .binding = 2,
                    .stride = 16,
                    .instanced = true,
                    .attributes = { attribs + 2, 2 }
                },
            };

            PipelineInfo pipeline_info{
                .layout = _render.pipeline_layout,
                .renderpass = find_resource<Renderpass>(gfxres, "renderpass.default"_sid),
                .shaders = _render.shaders,
                .shaders_stages = _render.shader_stages,
                .shader_bindings = bindings,
                .primitive_topology = PrimitiveTopology::TriangleFan,
                .cull_mode = CullMode::Disabled,
                .front_face = FrontFace::CounterClockWise,
                .subpass_index = 1,
                .depth_test = true
            };

            _render.pipeline = device.create_pipeline(pipeline_info);

            Game2D_UpdateImage* tiles_update_stage = _allocator.make<Game2D_UpdateImage>(_allocator, device, tiles_texture_info, _render.textures);

            runner.graphics_device().aquire_pass("pass.default"_sid).add_update_stage(
                tiles_update_stage
            );

            runner.graphics_frame().execute_task(update_buffers(device));
            runner.graphics_frame().execute_task(tiles_update_stage->update_buffers());
            runner.graphics_frame().execute_task(update_textures(runner.graphics_device().device()));
        }

        void on_deactivate(
            ice::Engine& engine,
            ice::EngineRunner& runner,
            ice::World& world
        ) noexcept override
        {
            using namespace ice::gfx;
            using namespace ice::render;

            RenderDevice& device = runner.graphics_device().device();

            device.destroy_pipeline(_render.pipeline);

            device.destroy_buffer(_render.buffer_inst);
            device.destroy_buffer(_render.buffer_idx);
            device.destroy_buffer(_render.buffer_vtx[1]);
            device.destroy_buffer(_render.buffer_vtx[0]);
            device.destroy_buffer(_render.tilemap_buffer);

            device.destroy_shader(_render.shaders[1]);
            device.destroy_shader(_render.shaders[0]);

            device.destroy_image(_render.textures);
            device.destroy_sampler(_render.sampler);

            device.destroy_resourcesets({ &_render.resourceset, 1 });

            device.destroy_pipeline_layout(_render.pipeline_layout);
            device.destroy_resourceset_layout(_render.resourceset_layouts[1]);

            world.data_storage().destroy_named_object<ObjectQuery>("game2d.level.query"_sid);
        }

        void on_update(
            ice::EngineFrame& frame,
            ice::EngineRunner& runner,
            ice::World& world
        ) noexcept override
        {
            static ice::StringID deps[]{
                "test.stage.clear"_sid,
                "camera.update_view"_sid,
            };

            ObjectQuery& query = *world.data_storage().named_object<ObjectQuery>("game2d.level.query"_sid);
            ObjectQuery::ResultByEntity result = query.result_by_entity(frame.allocator(), world.entity_storage());

            Span<Obj2dTransform> xform_span = frame.create_named_span<Obj2dTransform>("game2d.level.instances"_sid, result.entity_count());
            *frame.create_named_object<ice::u32>("game2d.level.instance_count"_sid) = result.entity_count();

            ice::u32 current_xform = 0;
            result.for_each([&](Obj2dShape& shape, Obj2dTransform& xform) noexcept
                {
                    xform_span[current_xform] = xform;
                    current_xform += 1;
                });

            runner.graphics_frame().execute_task(
                update_instances(runner.graphics_device().device(), xform_span)
            );

            if (_render.textures != ice::render::Image::Invalid)
            {
                runner.graphics_device().aquire_pass("pass.default"_sid).add_stage(
                    "game2d.level"_sid, deps, &_render
                );
            }
        }

        auto update_instances(ice::render::RenderDevice& device, ice::Span<Obj2dTransform> const instances) noexcept -> ice::Task<void>
        {
            using namespace ice::render;

            BufferUpdateInfo updates[]{
                BufferUpdateInfo{.buffer = _render.buffer_inst, .data = ice::data_view(instances.data(), instances.size_bytes(), 4) },
            };

            device.update_buffers(updates);
            co_return;
        }

        auto update_buffers(ice::render::RenderDevice& device) noexcept -> ice::Task<void>
        {
            using namespace ice::render;

            BufferUpdateInfo updates[]{
                BufferUpdateInfo{ .buffer = _render.buffer_vtx[0], .data = ice::data_view(Constant_BoxShape.vertices, sizeof(Constant_BoxShapeVertices), 4) },
                BufferUpdateInfo{ .buffer = _render.buffer_vtx[1], .data = ice::data_view(Constant_BoxShape.uvs, sizeof(Constant_BoxShapeVertices), 4) },
                BufferUpdateInfo{ .buffer = _render.tilemap_buffer, .data = ice::data_view(_render.tilemap_props) },
            };

            device.update_buffers(updates);
            co_return;
        }

        auto update_textures(ice::render::RenderDevice& device) noexcept -> ice::Task<void>
        {
            using namespace ice::render;

            ResourceUpdateInfo resources[]{
                ResourceUpdateInfo{
                    .image = _render.textures
                },
                ResourceUpdateInfo{
                    .sampler = _render.sampler
                },
                ResourceUpdateInfo{
                    .uniform_buffer = { .buffer = _render.tilemap_buffer, .offset = 0, .size = sizeof(_render.tilemap_props) }
                }
            };

            ResourceSetUpdateInfo updates[]{
                ResourceSetUpdateInfo{
                    .resource_set = _render.resourceset,
                    .resource_type = ResourceType::SampledImage,
                    .binding_index = 0,
                    .array_element = 0,
                    .resources = { resources + 0, 1 },
                },
                ResourceSetUpdateInfo{
                    .resource_set = _render.resourceset,
                    .resource_type = ResourceType::Sampler,
                    .binding_index = 1,
                    .array_element = 0,
                    .resources = { resources + 1, 1 },
                },
                ResourceSetUpdateInfo{
                    .resource_set = _render.resourceset,
                    .resource_type = ResourceType::UniformBuffer,
                    .binding_index = 2,
                    .array_element = 0,
                    .resources = { resources + 2, 1 },
                },
            };

            device.update_resourceset(updates);
            co_return;
        }

    private:
        ice::Allocator& _allocator;
        ice::Game2D_RenderObjects _render;
        ice::gfx::GfxPass* _target_pass;

        ObjectQuery* _object_query;

        ice::pod::Hash<ice::Obj2dShapeDefinition> _shape_definitions;

        struct ShapeDraw
        {
            ice::u32 offset;
            ice::u32 vertices;
        };

        ice::pod::Hash<ShapeDraw> _shape_draws;
    };

    IceGame2DTrait::IceGame2DTrait(ice::Allocator& alloc) noexcept
        : _allocator{ alloc }
        , _render{ }
        , _shape_definitions{ _allocator }
        , _shape_draws{ _allocator }
    {
        ice::pod::hash::reserve(_shape_definitions, 10);
        ice::pod::hash::reserve(_shape_draws, 10);

        ice::pod::hash::set(
            _shape_definitions,
            ice::hash(Constant_BoxShape.shape_name),
            Constant_BoxShape
        );
        ice::pod::hash::set(
            _shape_draws,
            ice::hash(Constant_BoxShape.shape_name),
            ShapeDraw{ .offset = 0, .vertices = 4 }
        );
    }

    auto create_game2d_trait(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::Game2DTrait>
    {
        return ice::make_unique<ice::Game2DTrait, ice::IceGame2DTrait>(alloc, alloc);
    }

} // namespace ice
