#include "trait_render_ui.hxx"
#include <ice/ui_element.hxx>
#include <ice/ui_button.hxx>
#include <ice/game_render_traits.hxx>

#include <ice/engine.hxx>
#include <ice/engine_frame.hxx>
#include <ice/engine_runner.hxx>
#include <ice/world/world_portal.hxx>
#include <ice/task_thread_pool.hxx>

#include <ice/gfx/gfx_device.hxx>
#include <ice/gfx/gfx_frame.hxx>
#include <ice/gfx/gfx_resource_tracker.hxx>

#include <ice/render/render_device.hxx>
#include <ice/render/render_buffer.hxx>
#include <ice/render/render_surface.hxx>
#include <ice/render/render_resource.hxx>
#include <ice/render/render_shader.hxx>
#include <ice/render/render_pipeline.hxx>
#include <ice/render/render_image.hxx>
#include <ice/render/render_swapchain.hxx>

#include <ice/shard_container.hxx>
#include <ice/asset_storage.hxx>
#include <ice/asset.hxx>

namespace ice
{

    namespace detail
    {

        auto load_ui_shader(ice::AssetStorage& assets, ice::Data& data, ice::Utf8String name) noexcept -> ice::Task<>
        {
            ice::Asset const asset = co_await assets.request(ice::render::AssetType_Shader, name, ice::AssetState::Baked);

            bool const shader_loaded = asset_check(asset, AssetState::Baked);
            ICE_ASSERT(shader_loaded, "Shader not available!");
            if (shader_loaded)
            {
                data = asset.data;
            }
        }

    } // namespace detail

    IceWorldTrait_RenderUI::IceWorldTrait_RenderUI(
        ice::Allocator& alloc
    ) noexcept
        : _render_data{ alloc }
    {
        ice::pod::hash::reserve(_render_data, 10);
    }

    void IceWorldTrait_RenderUI::record_commands(
        ice::gfx::GfxContext const& context,
        ice::EngineFrame const& frame,
        ice::render::CommandBuffer cmds,
        ice::render::RenderCommands& api
    ) const noexcept
    {
        using ice::render::ShaderStageFlags;

        ice::vec2f const scale{
            2.f / _display_size.x,
            2.f / _display_size.y
        };
        ice::vec2f const translate{
            -1.f,
            -1.f,
        };

        api.bind_pipeline(cmds, _pipeline);
        api.push_constant(cmds, _pipeline_layout, ShaderStageFlags::VertexStage, ice::data_view(scale), 0);
        api.push_constant(cmds, _pipeline_layout, ShaderStageFlags::VertexStage, ice::data_view(translate), sizeof(scale));

        for (auto const& entry : _render_data)
        {
            if (entry.value->is_dirty == true)
            {
                return;
            }

            ice::RenderUIData const* const data = entry.value;

            api.bind_resource_set(cmds, _pipeline_layout, data->resourceset_uniform, 0);
            api.bind_vertex_buffer(cmds, data->buffer_vertices, 0);
            api.bind_vertex_buffer(cmds, data->buffer_colors, 1);

            ice::u32 const instance_count = data->draw_data->vertices.size() / 4;
            ice::u32 instance_idx = 0;

            while (instance_idx < instance_count)
            {
                api.draw(cmds, 4, 1, 4 * instance_idx, 0);
                instance_idx += 1;
            }
        }
    }

    void IceWorldTrait_RenderUI::gfx_setup(
        ice::gfx::GfxFrame& gfx_frame,
        ice::gfx::GfxDevice& gfx_device
    ) noexcept
    {
        using namespace ice::gfx;
        using namespace ice::render;

        RenderDevice& device = gfx_device.device();
        Renderpass renderpass = ice::gfx::find_resource<Renderpass>(
            gfx_device.resource_tracker(),
            "ice.gfx.renderpass.default"_sid
        );

        _shader_stages[0] = ShaderStageFlags::VertexStage;
        _shader_stages[1] = ShaderStageFlags::FragmentStage;
        _shaders[0] = device.create_shader(ShaderInfo{ .shader_data = _shader_data[0] });
        _shaders[1] = device.create_shader(ShaderInfo{ .shader_data = _shader_data[1] });

        ResourceSetLayoutBinding const resource_bindings[]
        {
            ResourceSetLayoutBinding
            {
                .binding_index = 0,
                .resource_count = 1,
                .resource_type = ResourceType::UniformBuffer,
                .shader_stage_flags = ShaderStageFlags::VertexStage
            },
        };

        _resource_set_layout[0] = device.create_resourceset_layout(resource_bindings);

        PipelinePushConstant const push_constants[]
        {
            PipelinePushConstant
            {
                .shader_stage_flags = ShaderStageFlags::VertexStage,
                .offset = 0,
                .size = sizeof(ice::vec2f) * 2,
            }
        };
        PipelineLayoutInfo const layout_info
        {
            .push_constants = push_constants,
            .resource_layouts = _resource_set_layout,
        };

        _pipeline_layout = device.create_pipeline_layout(layout_info);

        ShaderInputAttribute attribs[]
        {
            ShaderInputAttribute
            {
                .location = 0,
                .offset = 0,
                .type = ShaderAttribType::Vec2f
            },
            ShaderInputAttribute
            {
                .location = 1,
                .offset = 8,
                .type = ShaderAttribType::Vec2f
            },
            ShaderInputAttribute
            {
                .location = 2,
                .offset = 0,
                .type = ShaderAttribType::Vec4f// _Unorm8
            },
        };

        ShaderInputBinding bindings[]
        {
            ShaderInputBinding
            {
                .binding = 0,
                .stride = sizeof(ice::vec2f),
                .instanced = false,
                .attributes = { attribs + 0, 1 }
            },
            ShaderInputBinding
            {
                .binding = 1,
                .stride = sizeof(ice::vec4f),
                .instanced = false,
                .attributes = { attribs + 2, 1 }
            },
        };

        PipelineInfo pipeline_info{
            .layout = _pipeline_layout,
            .renderpass = renderpass,
            .shaders = _shaders,
            .shaders_stages = _shader_stages,
            .shader_bindings = bindings,
            .primitive_topology = PrimitiveTopology::TriangleStrip,
            .cull_mode = CullMode::Disabled,
            .front_face = FrontFace::CounterClockWise,
            .subpass_index = 2,
            .depth_test = false
        };

        _pipeline = device.create_pipeline(pipeline_info);
    }

    void IceWorldTrait_RenderUI::gfx_cleanup(
        ice::gfx::GfxFrame& gfx_frame,
        ice::gfx::GfxDevice& gfx_device
    ) noexcept
    {
        using namespace ice::render;
        RenderDevice& device = gfx_device.device();

        for (auto const& entry : _render_data)
        {
            ice::RenderUIData* const data = entry.value;
            if (data->resourceset_uniform != ResourceSet::Invalid)
            {
                device.destroy_resourcesets({ &data->resourceset_uniform, 1 });
                device.destroy_buffer(data->buffer_uniform);
                device.destroy_buffer(data->buffer_vertices);
                device.destroy_buffer(data->buffer_colors);
            }
        }

        device.destroy_pipeline(_pipeline);
        device.destroy_pipeline_layout(_pipeline_layout);
        device.destroy_resourceset_layout(_resource_set_layout[0]);
        device.destroy_shader(_shaders[0]);
        device.destroy_shader(_shaders[1]);
    }

    void IceWorldTrait_RenderUI::gfx_update(
        ice::EngineFrame const& engine_frame,
        ice::gfx::GfxFrame& gfx_frame,
        ice::gfx::GfxDevice& gfx_device
    ) noexcept
    {
        using namespace ice::render;

        RenderDevice& render_device = gfx_device.device();

        for (auto const& entry : _render_data)
        {
            ice::RenderUIData* const data = entry.value;
            if (data->resourceset_uniform == ResourceSet::Invalid)
            {
                data->buffer_uniform = render_device.create_buffer(BufferType::Uniform, sizeof(ice::RenderUIData::Uniform));
                data->buffer_vertices = render_device.create_buffer(BufferType::Vertex, data->draw_data->vertices.size_bytes());
                data->buffer_colors = render_device.create_buffer(BufferType::Vertex, data->draw_data->colors.size_bytes());

                render_device.create_resourcesets(_resource_set_layout, { &data->resourceset_uniform, 1 });

                ResourceUpdateInfo const resource_updates[]
                {
                    ResourceUpdateInfo
                    {
                        .uniform_buffer = data->buffer_uniform
                    }
                };

                ResourceSetUpdateInfo const resourceset_updates[]
                {
                    ResourceSetUpdateInfo
                    {
                        .resource_set = data->resourceset_uniform,
                        .resource_type = ResourceType::UniformBuffer,
                        .binding_index = 0,
                        .array_element = 0,
                        .resources = resource_updates,
                    }
                };

                render_device.update_resourceset(resourceset_updates);
            }

            if (data->is_dirty)
            {
                BufferUpdateInfo const buffer_updates[]
                {
                    BufferUpdateInfo
                    {
                        .buffer = data->buffer_uniform,
                        .data = ice::data_view(data->uniform),
                        .offset = 0,
                    },
                    BufferUpdateInfo
                    {
                        .buffer = data->buffer_vertices,
                        .data = ice::data_view(data->draw_data->vertices),
                        .offset = 0,
                    },
                    BufferUpdateInfo
                    {
                        .buffer = data->buffer_colors,
                        .data = ice::data_view(data->draw_data->colors),
                        .offset = 0,
                    }
                };

                render_device.update_buffers(buffer_updates);
                data->is_dirty = false;
            }
        }

        gfx_frame.set_stage_slot(ice::Constant_GfxStage_DrawUI, this);
    }

    void ice::IceWorldTrait_RenderUI::on_activate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        runner.execute_task(
            detail::load_ui_shader(engine.asset_storage(), _shader_data[0], u8"shaders/ui/ui-vert"),
            EngineContext::LogicFrame
        );
        runner.execute_task(
            detail::load_ui_shader(engine.asset_storage(), _shader_data[1], u8"shaders/ui/ui-frag"),
            EngineContext::LogicFrame
        );

        ice::vec2u const size = runner.graphics_device().swapchain().extent();
        _display_size = ice::vec2f{ (ice::f32)size.x, (ice::f32)size.y };
    }

    void IceWorldTrait_RenderUI::on_deactivate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        for (auto const& entry : _render_data)
        {
            portal.allocator().deallocate(entry.value);
        }
    }

    void IceWorldTrait_RenderUI::on_update(
        ice::EngineFrame& frame,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        ice::shards::inspect_each<ice::RenderUIRequest const*>(
            frame.shards(),
            ice::Shard_RenderUIData,
            [&, this](ice::RenderUIRequest const* render_request)
            {
                ice::RenderUIData* data = ice::pod::hash::get(_render_data, render_request->id, nullptr);
                if (data == nullptr)
                {
                    ice::RenderUIData* data = portal.allocator().make<ice::RenderUIData>();
                    ice::pod::hash::set(_render_data, render_request->id, data);

                    data->id = render_request->id;
                    data->buffer_uniform = ice::render::Buffer::Invalid;
                    data->buffer_vertices = ice::render::Buffer::Invalid;
                    data->buffer_colors = ice::render::Buffer::Invalid;
                    data->resourceset_uniform = ice::render::ResourceSet::Invalid;
                    data->is_dirty = true;

                    portal.execute(
                        create_render_data(portal.allocator(), runner, *render_request, *data)
                    );
                }
                else
                {
                    data->draw_data = render_request->draw_data;
                    data->is_dirty = true;
                }
            }
        );

        ice::vec2i window_size{ };
        if (ice::shards::inspect_last(frame.shards(), ice::platform::Shard_WindowSizeChanged, window_size))
        {
            _display_size = ice::vec2f{ (ice::f32)window_size.x, (ice::f32)window_size.y };
        }
    }

    auto IceWorldTrait_RenderUI::create_render_data(
        ice::Allocator& alloc,
        ice::EngineRunner& runner,
        ice::RenderUIRequest const& ui_request,
        ice::RenderUIData& render_data
    ) noexcept -> ice::Task<>
    {
        //co_await runner.thread_pool();

        render_data.uniform.position = {}; // ui_request.position;
        render_data.uniform.scale = ice::vec2f{ 1.f };

        //co_await runner.schedule_next_frame();

        render_data.draw_data = ui_request.draw_data;
        co_return;
    }

    void register_trait_render_ui(ice::WorldTraitArchive& archive) noexcept
    {
        static constexpr ice::StringID deps[]{
            ice::Constant_TraitName_RenderGlyphs
        };

        archive.register_trait(
            ice::Constant_TraitName_RenderUI,
            ice::WorldTraitDescription
            {
                .factory = ice::detail::generic_trait_factory<IceWorldTrait_RenderUI>,
                .optional_dependencies = deps
            }
        );
    }

} // namespace ice
