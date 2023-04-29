/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "trait_render_debug.hxx"
#include "../trait_camera.hxx"

#include <ice/game_render_traits.hxx>
#include <ice/task.hxx>
#include <ice/task_sync_wait.hxx>
#include <ice/engine.hxx>
#include <ice/world/world_trait_archive.hxx>

#include <ice/gfx/gfx_device.hxx>
#include <ice/gfx/gfx_frame.hxx>
#include <ice/gfx/gfx_resource_tracker.hxx>

#include <ice/render/render_device.hxx>
#include <ice/render/render_pass.hxx>
#include <ice/render/render_pipeline.hxx>
#include <ice/render/render_resource.hxx>
#include <ice/render/render_shader.hxx>
#include <ice/render/render_buffer.hxx>

#include <ice/asset_storage.hxx>
#include <ice/assert.hxx>

namespace ice
{

    namespace detail
    {

        struct DebugInstance
        {
            ice::vec4f color;
        };

        auto load_debug_shader(ice::AssetStorage& assets, ice::String name) noexcept -> ice::Task<ice::Data>
        {
            ice::Asset const asset = assets.bind(ice::render::AssetType_Shader, name, AssetState::Baked);
            if (asset_check(asset, AssetState::Baked))
            {
                co_return asset.data;
            }
            co_return co_await assets.request(asset, AssetState::Baked);
        }

    } // namespace detail

    void IceWorldTrait_RenderDebug::on_activate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        ice::AssetStorage& asset_system = engine.asset_storage();

        _shader_data[0] = ice::sync_wait(ice::detail::load_debug_shader(asset_system, "shaders/debug/debug-vert"));
        _shader_data[1] = ice::sync_wait(ice::detail::load_debug_shader(asset_system, "shaders/debug/debug-frag"));
    }

    void IceWorldTrait_RenderDebug::gfx_setup(
        ice::gfx::GfxFrame& gfx_frame,
        ice::gfx::GfxDevice& gfx_device
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
                .type = ShaderAttribType::Vec3f
            },
            ShaderInputAttribute
            {
                .location = 1,
                .offset = 0,
                .type = ShaderAttribType::Vec4f_Unorm8
            },
        };
        ShaderInputBinding const shader_bindings[]
        {
            ShaderInputBinding
            {
                .binding = 0,
                .stride = 12,
                .instanced = false,
                .attributes = { shader_attribs + 0, 1 }
            },
            ShaderInputBinding
            {
                .binding = 1,
                .stride = 4,
                .instanced = false,
                .attributes = { shader_attribs + 1, 1 }
            }
        };

        PipelineInfo const pipeline_info
        {
            .layout = _layout,
            .renderpass = renderpass,
            .shaders = _shaders,
            .shaders_stages = _shader_stages,
            .shader_bindings = shader_bindings,
            .primitive_topology = PrimitiveTopology::LineStrip,
            .front_face = FrontFace::CounterClockWise,
            .subpass_index = 2,
            .depth_test = false,
        };
        _pipeline = device.create_pipeline(pipeline_info);

        _vertices = device.create_buffer(BufferType::Vertex, 1024 * 96);
        _colors = device.create_buffer(BufferType::Vertex, 1024 * 32);
    }

    void IceWorldTrait_RenderDebug::gfx_cleanup(
        ice::gfx::GfxFrame& gfx_frame,
        ice::gfx::GfxDevice& gfx_device
    ) noexcept
    {
        using namespace ice::render;
        RenderDevice& device = gfx_device.device();

        device.destroy_buffer(_vertices);
        device.destroy_buffer(_colors);
        device.destroy_shader(_shaders[0]);
        device.destroy_shader(_shaders[1]);
        device.destroy_pipeline(_pipeline);
        device.destroy_pipeline_layout(_layout);
        device.destroy_resourcesets({ &_resource_set, 1 });
        device.destroy_resourceset_layout(_resource_layout);
    }

    void IceWorldTrait_RenderDebug::gfx_update(
        ice::EngineFrame const& engine_frame,
        ice::gfx::GfxFrame& gfx_frame,
        ice::gfx::GfxDevice& gfx_device
    ) noexcept
    {

        ice::u32 buffer_offset = 0;
        ice::u32 color_buffer_offset = 0;

        ice::shards::inspect_each<ice::DebugDrawCommandList const*>(
            engine_frame.shards(),
            ice::Shard_DebugDrawCommand,
            [&](ice::DebugDrawCommandList const* draw_list) noexcept
            {
                ice::Span<ice::DebugDrawCommand const> commands{ draw_list->list, draw_list->list_size };
                for (ice::DebugDrawCommand const& command : commands)
                {
                    Data vertex_data = ice::Data{ command.vertex_list, command.vertex_count * sizeof(ice::vec3f) };
                    Data color_data = ice::Data{ command.vertex_color_list, command.vertex_count * sizeof(ice::vec1u) };

                    ice::render::BufferUpdateInfo updates[2]{
                        ice::render::BufferUpdateInfo
                        {
                            .buffer = _vertices,
                            .data = vertex_data,
                            .offset = buffer_offset,
                        },
                        ice::render::BufferUpdateInfo
                        {
                            .buffer = _colors,
                            .data = color_data,
                            .offset = color_buffer_offset,
                        }
                    };

                    buffer_offset += ice::u32(vertex_data.size.value);
                    color_buffer_offset += ice::u32(color_data.size.value);

                    gfx_device.device().update_buffers(updates);
                }
            }
        );

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
                using namespace render;

                _render_camera_buffer = camera_buffer;

                RenderDevice& device = gfx_device.device();

                ResourceUpdateInfo res_updates[]{
                    ResourceUpdateInfo
                    {
                        .uniform_buffer = ResourceBufferInfo{
                            .buffer = _render_camera_buffer,
                            .offset = 0,
                            .size = sizeof(ice::TraitCameraRenderData)
                        }
                    }
                };

                ResourceSetUpdateInfo set_updates[]{
                    ResourceSetUpdateInfo
                    {
                        .resource_set = _resource_set,
                        .resource_type = ResourceType::UniformBuffer,
                        .binding_index = 0,
                        .array_element = 0,
                        .resources = { res_updates + 0, 1 },
                    },
                };

                device.update_resourceset(set_updates);
            }
        }

        gfx_frame.set_stage_slot(ice::Constant_GfxStage_DrawDebug, this);
    }

    void IceWorldTrait_RenderDebug::record_commands(
        ice::gfx::GfxContext const& context,
        ice::EngineFrame const& engine_frame,
        ice::render::CommandBuffer cmds,
        ice::render::RenderCommands& api
    ) const noexcept
    {
        api.bind_resource_set(cmds, _layout, _resource_set, 0);
        api.bind_vertex_buffer(cmds, _vertices, 0);
        api.bind_vertex_buffer(cmds, _colors, 1);
        api.bind_pipeline(cmds, _pipeline);

        ice::shards::inspect_each<ice::DebugDrawCommandList const*>(
            engine_frame.shards(),
            ice::Shard_DebugDrawCommand,
            [&](ice::DebugDrawCommandList const* draw_list) noexcept
            {
                ice::u32 vertex_offset = 0;

                ice::Span<ice::DebugDrawCommand const> commands{ draw_list->list, draw_list->list_size };
                for (ice::DebugDrawCommand const& command : commands)
                {
                    api.draw(
                        cmds,
                        command.vertex_count,
                        1,
                        vertex_offset,
                        0
                    );

                    vertex_offset += command.vertex_count;
                }
            }
        );
    }

    void register_trait_render_debug(
        ice::WorldTraitArchive& archive
    ) noexcept
    {
        static constexpr ice::StringID trait_dependencies[]{
            Constant_TraitName_RenderPostprocess,
        };

        archive.register_trait(
            ice::Constant_TraitName_RenderDebug,
            ice::WorldTraitDescription
            {
                .factory = ice::detail::generic_trait_factory<IceWorldTrait_RenderDebug>,
                .required_dependencies = trait_dependencies
            }
        );
    }

} // namespace ice
