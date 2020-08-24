#include "ice_rs_postprocess.hxx"
#include "../ice_render_pass.hxx"

#include <core/pod/array.hxx>
#include <core/allocators/stack_allocator.hxx>

#include <iceshard/renderer/render_funcs.hxx>
#include <iceshard/renderer/render_commands.hxx>
#include <iceshard/renderer/render_pipeline.hxx>
#include <iceshard/renderer/render_resources.hxx>
#include <iceshard/renderer/render_pass.hxx>
#include <iceshard/renderer/render_buffers.hxx>
#include <iceshard/engine.hxx>
#include <iceshard/frame.hxx>
#include <iceshard/math.hxx>

#include <asset_system/asset_system.hxx>
#include <asset_system/assets/asset_shader.hxx>

#include <input_system/message/window.hxx>

namespace iceshard
{
    IceRS_PostProcess::IceRS_PostProcess(core::allocator& alloc) noexcept
        : IceRenderStage{ alloc }
    {
    }

    void IceRS_PostProcess::on_prepare(
        iceshard::Engine& engine,
        iceshard::RenderPass& render_pass
    ) noexcept
    {
        using asset::AssetShader;
        using namespace iceshard::renderer;

        asset::AssetData shader_assets[2];

        auto& asset_system = engine.asset_system();
        asset_system.load(asset::AssetShader{ "shaders/debug/pp-vert" }, shader_assets[0]);
        asset_system.load(asset::AssetShader{ "shaders/debug/pp-frag" }, shader_assets[1]);

        auto& render_system = engine.render_system();
        _pipeline = render_system.create_pipeline(
            PostProcessPipelineName,
            RenderPipelineLayout::PostProcess,
            core::pod::array::create_view(shader_assets)
        );

        RenderResource resources[] = {
            {
                .binding = 2,
                .type = RenderResourceType::ResTexture2D,
                .handle = {.texture = iceshard::renderer::api::Texture::Attachment0 },
            }
        };

        _resources = render_system.create_resource_set(
            PostProcessResourceSetName,
            RenderPipelineLayout::PostProcess,
            core::pod::array::create_view(resources)
        );

        static ism::vec2f vertices[] = {
            { -1.0f, -1.0f }, { 0.0f, 0.0f, },
            { 3.0f, -1.0f }, { 2.0f, 0.0f, },
            { -1.0f, 3.0f }, { 0.0f, 2.0f, },
        };
        static ism::u16 indices[] = {
            0, 1, 2
        };

        _buffers[0] = create_buffer(api::BufferType::VertexBuffer, 256);
        _buffers[1] = create_buffer(api::BufferType::IndexBuffer, 256);

        api::DataView data_views[2];

        auto buffers_arr = core::pod::array::create_view(_buffers);
        auto data_views_arr = core::pod::array::create_view(data_views);

        map_buffers(
            buffers_arr,
            data_views_arr
        );

        memcpy(data_views[0].data, vertices, sizeof(vertices));
        memcpy(data_views[1].data, indices, sizeof(indices));

        unmap_buffers(
            buffers_arr
        );

        auto cmds_view = core::pod::array::create_view(&_command_buffer, 1);
        create_command_buffers(
            api::CommandBufferType::Secondary,
            cmds_view
        );

    }

    void IceRS_PostProcess::on_cleanup(
        iceshard::Engine& engine,
        iceshard::RenderPass& render_pass
    ) noexcept
    {
        auto& render_system = engine.render_system();
        render_system.destroy_pipeline(PostProcessPipelineName);
        render_system.destroy_resource_set(PostProcessResourceSetName);
    }

    void IceRS_PostProcess::on_execute(
        iceshard::Frame& current,
        iceshard::RenderPass& render_pass
    ) noexcept
    {
        using namespace iceshard::renderer;
        namespace api = iceshard::renderer::api;
        namespace cmd = iceshard::renderer::commands;

        RenderResource resources[] = {
            {
                .binding = 2,
                .type = RenderResourceType::ResTexture2D,
                .handle = {.texture = iceshard::renderer::api::Texture::Attachment0 },
            }
        };

        render_pass.render_system().update_resource_set(
            PostProcessResourceSetName,
            core::pod::array::create_view(resources)
        );

        static ism::vec2u viewport{ 1280, 720 };
        core::message::filter<::input::message::WindowSizeChanged>(current.messages(), [](::input::message::WindowSizeChanged const& msg) noexcept
            {
                viewport.x = msg.width;
                viewport.y = msg.height;
            });

        cmd::begin(_command_buffer, render_pass.handle(), 2);
        cmd::bind_pipeline(_command_buffer, _pipeline);
        cmd::bind_resource_set(_command_buffer, _resources);
        {
            cmd::set_viewport(_command_buffer, viewport.x, viewport.y);
            cmd::set_scissor(_command_buffer, viewport.x, viewport.y);
        }
        {
            core::memory::stack_allocator<128> temp_alloc;
            core::pod::Array<api::Buffer> buffers{ temp_alloc };
            core::pod::array::push_back(buffers, _buffers[0]);

            cmd::bind_vertex_buffers(_command_buffer, buffers);
            cmd::bind_index_buffer(_command_buffer, _buffers[1]);
            cmd::draw_indexed(_command_buffer, 3, 1, 0, 0, 0);
        }
        cmd::end(_command_buffer);

        auto cb = render_pass.command_buffer();
        cmd::next_subpass(cb, api::RenderSubPass::CommandBuffers);
        cmd::execute_commands(cb, 1, (api::CommandBuffer*) &_command_buffer);
    }

} // namespace iceshard
