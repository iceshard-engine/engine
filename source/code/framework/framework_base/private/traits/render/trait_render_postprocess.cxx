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

    auto IceWorldTrait_RenderPostProcess::gfx_stage_infos() const noexcept -> ice::Span<ice::gfx::GfxStageInfo const>
    {
        static ice::StringID const dependencies[]
        {
            "frame.clear"_sid,
            "frame.render-sprites"_sid,
        };
        static ice::gfx::GfxStageInfo const infos[]
        {
            ice::gfx::GfxStageInfo
            {
                .name = "frame.render-postprocess"_sid,
                .dependencies = dependencies,
                .type = gfx::GfxStageType::DrawStage
            }
        };
        return infos;
    }

    auto IceWorldTrait_RenderPostProcess::gfx_stage_slots() const noexcept -> ice::Span<ice::gfx::GfxStageSlot const>
    {
        static ice::gfx::GfxStageSlot const slots[]
        {
            ice::gfx::GfxStageSlot
            {
                .name = "frame.render-postprocess"_sid,
                .stage = this
            }
        };
        return { slots, _slot_count };
    }

    void IceWorldTrait_RenderPostProcess::on_activate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        runner.execute_task(
            task_create_render_objects(engine.asset_system(), runner, runner.graphics_device()),
            EngineContext::EngineRunner
        );
    }

    void IceWorldTrait_RenderPostProcess::on_deactivate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        runner.execute_task(
            task_destroy_render_objects(runner.graphics_device()),
            EngineContext::GraphicsFrame
        );
    }

    void IceWorldTrait_RenderPostProcess::on_update(
        ice::EngineFrame& frame,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        IPT_ZONE_SCOPED_NAMED("[Trait] PostProcess :: Update");

        runner.graphics_frame().set_stage_slots(gfx_stage_slots());
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

    auto IceWorldTrait_RenderPostProcess::task_create_render_objects(
        ice::AssetSystem& asset_system,
        ice::EngineRunner& runner,
        ice::gfx::GfxDevice& gfx_device
    ) noexcept -> ice::Task<>
    {
        using namespace ice::gfx;
        using namespace ice::render;

        Data vtx_shader_data = ice::detail::load_postprocess_shader(asset_system, "/shaders/debug/pp-vert"_sid);
        Data pix_shader_data = ice::detail::load_postprocess_shader(asset_system, "/shaders/debug/pp-frag"_sid);

        ice::render::Renderpass renderpass = Renderpass::Invalid;
        while (renderpass == Renderpass::Invalid)
        {
            ice::EngineFrame& frame = co_await runner.schedule_next_frame();

            co_await frame.schedule_frame_end();

            ice::render::Renderpass* candidate_renderpass = frame.named_object<ice::render::Renderpass>("ice.gfx.renderpass"_sid);
            if (candidate_renderpass != nullptr)
            {
                renderpass = *candidate_renderpass;
            }
        }

        co_await runner.schedule_current_frame();
        co_await runner.graphics_frame().frame_start();

        RenderDevice& device = gfx_device.device();

        _shader_stages[0] = ShaderStageFlags::VertexStage;
        _shader_stages[1] = ShaderStageFlags::FragmentStage;
        _shaders[0] = device.create_shader(ShaderInfo{ .shader_data = vtx_shader_data });
        _shaders[1] = device.create_shader(ShaderInfo{ .shader_data = pix_shader_data });

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

        ResourceUpdateInfo const resource_updates[]
        {
            ResourceUpdateInfo{ .sampler = _sampler, },
            ResourceUpdateInfo{ .image = ice::gfx::find_resource<ice::render::Image>(gfx_device.resource_tracker(), "ice.gfx.attachment.image.color"_sid) }
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

        co_await runner.schedule_next_frame();
        _slot_count = 1;
    }

    auto IceWorldTrait_RenderPostProcess::task_destroy_render_objects(
        ice::gfx::GfxDevice& gfx_device
    ) noexcept -> ice::Task<>
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

        co_return;
    }

} // namespace ice
