#include "trait_render_postprocess.hxx"

#include <ice/engine.hxx>

#include <ice/render/render_device.hxx>
#include <ice/render/render_pass.hxx>
#include <ice/render/render_resource.hxx>
#include <ice/render/render_pipeline.hxx>
#include <ice/render/render_buffer.hxx>
#include <ice/render/render_image.hxx>

#include <ice/gfx/gfx_resource_tracker.hxx>
#include <ice/gfx/gfx_stage.hxx>
#include <ice/gfx/gfx_frame.hxx>
#include <ice/gfx/gfx_pass.hxx>

#include <ice/asset_system.hxx>
#include <ice/span_filter.hxx>
#include <ice/profiler.hxx>

namespace ice
{

    namespace detail
    {

        auto load_postprocess_shader(ice::AssetSystem& assets, ice::StringID_Arg name) noexcept -> ice::Data
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

    IceWorldTrait_RenderPostProcess::IceWorldTrait_RenderPostProcess(ice::StringID_Arg stage_name) noexcept
        : _stage_name{ stage_name }
    {
    }

    void IceWorldTrait_RenderPostProcess::gfx_context_setup(
        ice::gfx::GfxDevice& gfx_device,
        ice::gfx::GfxContext& context
    ) noexcept
    {
        using namespace ice::gfx;
        using namespace ice::render;

        Renderpass renderpass = ice::gfx::find_resource<Renderpass>(gfx_device.resource_tracker(), "ice.gfx.renderpass.default"_sid);
        ICE_ASSERT(
            renderpass != Renderpass::Invalid,
            "Trait cannot properly setup render objects due to a missing renderpass object!"
        );

        RenderDevice& device = gfx_device.device();

        _shader_stages[0] = ShaderStageFlags::VertexStage;
        _shader_stages[1] = ShaderStageFlags::FragmentStage;
        _shaders[0] = device.create_shader(ShaderInfo{ .shader_data = _shader_data[0] });
        _shaders[1] = device.create_shader(ShaderInfo{ .shader_data = _shader_data[1] });

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

        _sampler = device.create_sampler(sampler_info);

        ResourceSetLayoutBinding const resource_bindings[]
        {
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
                .resource_type = ResourceType::InputAttachment,
                .shader_stage_flags = ShaderStageFlags::FragmentStage
            },
        };
        _resource_layout = device.create_resourceset_layout(resource_bindings);
        device.create_resourcesets({ &_resource_layout, 1 }, { &_resource_set, 1 });

        PipelineLayoutInfo const layout_info
        {
            .push_constants = { },
            .resource_layouts = { &_resource_layout, 1 },
        };
        _layout = device.create_pipeline_layout(layout_info);

        ShaderInputAttribute const shader_attribs[]
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
        };
        ShaderInputBinding const shader_bindings[]
        {
            ShaderInputBinding
            {
                .binding = 0,
                .stride = 16,
                .instanced = false,
                .attributes = shader_attribs
            }
        };

        PipelineInfo const pipeline_info
        {
            .layout = _layout,
            .renderpass = renderpass,
            .shaders = _shaders,
            .shaders_stages = _shader_stages,
            .shader_bindings = shader_bindings,
            .subpass_index = 2,
        };
        _pipeline = device.create_pipeline(pipeline_info);

        ice::vec4f const vertices[]
        {
            ice::vec4f{ -1.f, -1.f, 0.f, 0.f },
            ice::vec4f{ -1.f, 3.f, 0.f, 2.f },
            ice::vec4f{ 3.f, -1.f, 2.f, 0.f },
        };

        _vertices = device.create_buffer(BufferType::Vertex, sizeof(vertices));

        BufferUpdateInfo buffer_updates[]
        {
            BufferUpdateInfo
            {
                .buffer = _vertices,
                .data = ice::data_view(vertices),
                .offset = 0
            }
        };
        device.update_buffers(buffer_updates);

        update_resources(gfx_device);
    }

    void IceWorldTrait_RenderPostProcess::gfx_context_cleanup(
        ice::gfx::GfxDevice& gfx_device,
        ice::gfx::GfxContext& context
    ) noexcept
    {
        using namespace ice::render;
        RenderDevice& device = gfx_device.device();

        device.destroy_buffer(_vertices);
        device.destroy_sampler(_sampler);
        device.destroy_shader(_shaders[0]);
        device.destroy_shader(_shaders[1]);
        device.destroy_pipeline(_pipeline);
        device.destroy_pipeline_layout(_layout);
        device.destroy_resourcesets({ &_resource_set, 1 });
        device.destroy_resourceset_layout(_resource_layout);
    }

    void IceWorldTrait_RenderPostProcess::gfx_update(
        ice::EngineFrame const& engine_frame,
        ice::gfx::GfxDevice& device,
        ice::gfx::GfxContext& context,
        ice::gfx::GfxFrame& frame
    ) noexcept
    {
        IPT_ZONE_SCOPED_NAMED("[Trait] PostProcess :: Update");

        for (ice::Shard const& shard : ice::filter_span(engine_frame.shards(), ice::any_of<ice::platform::Shard_WindowSizeChanged>))
        {
            update_resources(device);
        }

        frame.set_stage_slot(_stage_name, this);
    }

    void IceWorldTrait_RenderPostProcess::on_activate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        ice::AssetSystem& asset_system = engine.asset_system();

        _shader_data[0] = ice::detail::load_postprocess_shader(asset_system, "/shaders/debug/pp-vert"_sid);
        _shader_data[1] = ice::detail::load_postprocess_shader(asset_system, "/shaders/debug/pp-frag"_sid);
    }

    void IceWorldTrait_RenderPostProcess::record_commands(
        ice::EngineFrame const& frame,
        ice::render::CommandBuffer cmds,
        ice::render::RenderCommands& api
    ) const noexcept
    {
        IPT_ZONE_SCOPED_NAMED("[Trait] PostProcess :: Graphics Commands");

        api.next_subpass(cmds, ice::render::SubPassContents::Inline);
        api.bind_pipeline(cmds, _pipeline);
        api.bind_resource_set(cmds, _layout, _resource_set, 0);
        api.bind_vertex_buffer(cmds, _vertices, 0);
        api.draw(cmds, 3, 1, 0, 0);
    }

    void IceWorldTrait_RenderPostProcess::update_resources(ice::gfx::GfxDevice& gfx_device) noexcept
    {
        using namespace ice::render;

        RenderDevice& device = gfx_device.device();

        ResourceUpdateInfo const resource_updates[]
        {
            ResourceUpdateInfo{.sampler = _sampler, },
            ResourceUpdateInfo{.image = ice::gfx::find_resource<ice::render::Image>(gfx_device.resource_tracker(), "ice.gfx.attachment.image.color"_sid) }
        };
        ResourceSetUpdateInfo const set_updates[]
        {
            ResourceSetUpdateInfo
            {
                .resource_set = _resource_set,
                .resource_type = ResourceType::Sampler,
                .binding_index = 1,
                .array_element = 0,
                .resources = { resource_updates + 0, 1 },
            },
            ResourceSetUpdateInfo
            {
                .resource_set = _resource_set,
                .resource_type = ResourceType::InputAttachment,
                .binding_index = 2,
                .array_element = 0,
                .resources = { resource_updates + 1, 1 },
            },
        };

        device.update_resourceset(set_updates);
    }

} // namespace ice
