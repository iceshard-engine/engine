#include "imgui_renderer.hxx"
#include <core/memory.hxx>
#include <core/math/vector.hxx>

#include <iceshard/engine.hxx>
#include <iceshard/render/render_system.hxx>
#include <iceshard/renderer/render_commands.hxx>
#include <iceshard/renderer/render_pass.hxx>
#include <iceshard/renderer/render_resources.hxx>
#include <iceshard/renderer/render_funcs.hxx>
#include <iceshard/renderer/render_system.hxx>
#include <iceshard/renderer/render_buffers.hxx>

#include <asset_system/assets/asset_shader.hxx>


namespace iceshard::debug::imgui
{

    ImGuiRenderer::ImGuiRenderer(
        core::allocator& alloc,
        ImGuiIO& io,
        asset::AssetSystem& asset_system,
        iceshard::Engine& engine
    ) noexcept
        : _allocator{ alloc }
        , _asset_system{ asset_system }
        , _render_system{ engine.render_system() }
        , _io{ io }
        , _vertice_buffers{ alloc }
    {
        using namespace iceshard::renderer;

        core::pod::array::push_back(_vertice_buffers,
            iceshard::renderer::create_buffer(api::BufferType::VertexBuffer, sizeof(ImDrawVert) * 1024 * 256)
        );

        _indice_buffer = iceshard::renderer::create_buffer(api::BufferType::IndexBuffer, sizeof(ImDrawIdx) * 1024 * 1024);

        _io.DisplaySize.x = 1280;
        _io.DisplaySize.y = 720;

        {
            using namespace core::math;

            unsigned char* pixels;
            int width, height;
            io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

            uint32_t upload_size = width * height * 4 * sizeof(char);

            iceshard::renderer::api::v1_1::data::Texture font_texture;
            font_texture.width = width;
            font_texture.height = height;
            font_texture.format = iceshard::renderer::api::TextureFormat::UnormRGBA;
            font_texture.data = pixels;

            asset::AssetData tex_data{ };
            tex_data.content = { &font_texture, sizeof(font_texture) };

            _font_texture = create_texture(
                "imgui.fonts"_sid,
                tex_data
            );

            _temp_texture_data = create_buffer(
                api::BufferType::TransferBuffer,
                upload_size
            );

            {
                auto temp_buffer_arr = core::pod::array::create_view(&_temp_buffer, 1);
                create_command_buffers(
                    api::CommandBufferType::Primary,
                    temp_buffer_arr
                );
            }

            api::DataView buffer_view;
            map_buffer(_temp_texture_data, buffer_view);
            memcpy(buffer_view.data, pixels, upload_size);
            unmap_buffer(_temp_texture_data);

            commands::begin(_temp_buffer, api::CommandBufferUsage::RunOnce);
            commands::update_texture(_temp_buffer, _font_texture, _temp_texture_data, vec2<u32>(width, height));
            commands::end(_temp_buffer);
            _render_system.submit_command_buffer_v2(_temp_buffer);
        }

        //_font_texture = detail::create_fonts_texture(alloc, _io, _render_system);
        io.Fonts->TexID = reinterpret_cast<ImTextureID>(_font_texture);

        {
            using namespace iceshard::renderer;

            RenderResource handles[1];
            handles[0].type = RenderResourceType::ResTexture2D;
            handles[0].handle.texture = _font_texture;
            handles[0].binding = 2;

            core::pod::Array<RenderResource> resources{ _allocator };
            core::pod::array::push_back(resources, handles[0]);

            _resource_set = _render_system.create_resource_set(
                "imgui_resources"_sid,
                RenderPipelineLayout::DebugUI,
                RenderResourceSetInfo{ .usage = RenderResourceSetUsage::MaterialData },
                resources
            );

            core::pod::Array<asset::AssetData> shader_assets{ _allocator };
            core::pod::array::resize(shader_assets, 2);

            _asset_system.load(asset::AssetShader{ "shaders/debug/imgui-vert" }, shader_assets[0]);
            _asset_system.load(asset::AssetShader{ "shaders/debug/imgui-frag" }, shader_assets[1]);

            _pipeline = _render_system.create_pipeline(
                "imgui_pipeline"_sid,
                RenderPipelineLayout::DebugUI,
                shader_assets
            );
        }
    }

    ImGuiRenderer::~ImGuiRenderer() noexcept
    {
        _render_system.destroy_pipeline("imgui_pipeline"_sid);
        _render_system.destroy_resource_set("imgui_resources"_sid);
    }

    void ImGuiRenderer::create_render_tasks(
        iceshard::Frame const& current,
        iceshard::renderer::api::CommandBuffer cmds,
        core::Vector<cppcoro::task<>>& task_list
    ) noexcept
    {
        task_list.push_back(draw_task(cmds, ImGui::GetDrawData()));
    }

    auto ImGuiRenderer::draw_task(
        iceshard::renderer::api::CommandBuffer cb,
        ImDrawData* draw_data
    ) noexcept -> cppcoro::task<>
    {
        // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
        int fb_width = static_cast<int>(_io.DisplaySize.x * _io.DisplayFramebufferScale.x);
        int fb_height = static_cast<int>(_io.DisplaySize.y * _io.DisplayFramebufferScale.y);
        if (fb_width == 0 || fb_height == 0)
        {
            co_return;
        }

        //auto draw_area_ext = render_area();

        float scale[2];
        scale[0] = 2.0f / fb_width;
        scale[1] = 2.0f / fb_height;
        float translate[2];
        translate[0] = -1.0f; // -1.0f - width * scale[0];
        translate[1] = -1.0f; //-1.0f - height * scale[1];

        using iceshard::renderer::RenderPassStage;
        using iceshard::renderer::api::Buffer;
        using iceshard::renderer::api::DataView;
        using namespace iceshard::renderer::commands;

        push_constants(cb, _pipeline, { scale, sizeof(scale) });
        push_constants(cb, _pipeline, { translate, sizeof(translate) }, sizeof(scale));
        bind_pipeline(cb, _pipeline);
        bind_resource_set(cb, _resource_set);
        bind_vertex_buffers(cb, _vertice_buffers);
        bind_index_buffer(cb, _indice_buffer);
        set_viewport(cb, (uint32_t)_io.DisplaySize.x, (uint32_t)_io.DisplaySize.y);
        set_scissor(cb, (uint32_t)_io.DisplaySize.x, (uint32_t)_io.DisplaySize.y);

        // Upload vertex/index data into a single contiguous GPU buffer
        {
            Buffer buff[] = { _indice_buffer, _vertice_buffers[0] };
            DataView buff_view[2];

            iceshard::renderer::api::render_module_api->buffer_array_map_data_func(buff, buff_view, 2);

            for (int n = 0; n < draw_data->CmdListsCount; n++)
            {
                const ImDrawList* cmd_list = draw_data->CmdLists[n];

                IS_ASSERT(buff_view[0].size > cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx), "how?");
                IS_ASSERT(buff_view[1].size > cmd_list->VtxBuffer.Size * sizeof(ImDrawVert), "how?");
                buff_view[0].size -= cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx);
                buff_view[1].size -= cmd_list->VtxBuffer.Size * sizeof(ImDrawVert);

                memcpy(buff_view[0].data, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
                memcpy(buff_view[1].data, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
                buff_view[0].data = core::memory::utils::pointer_add(buff_view[0].data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
                buff_view[1].data = core::memory::utils::pointer_add(buff_view[1].data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
            }

            iceshard::renderer::api::render_module_api->buffer_array_unmap_data_func(buff, 2);
        }

        ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
        ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

        uint32_t vtx_buffer_offset = 0;
        uint32_t idx_buffer_offset = 0;
        for (int32_t i = 0; i < draw_data->CmdListsCount; i++)
        {
            ImDrawList const* cmd_list = draw_data->CmdLists[i];
            for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
            {
                ImDrawCmd const* pcmd = &cmd_list->CmdBuffer[cmd_i];
                if (pcmd->UserCallback)
                {
                    pcmd->UserCallback(cmd_list, pcmd);
                }
                else
                {
                    ImVec4 clip_rect;
                    clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
                    clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
                    clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
                    clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;



                    if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)
                    {
                        // Negative offsets are illegal for vkCmdSetScissor
                        if (clip_rect.x < 0.0f)
                            clip_rect.x = 0.0f;
                        if (clip_rect.y < 0.0f)
                            clip_rect.y = 0.0f;

                        // Apply scissor/clipping rectangle
                        core::math::vec4i scissor;
                        scissor.x = (int32_t)(clip_rect.x);
                        scissor.y = (int32_t)(clip_rect.y);
                        scissor.z = (uint32_t)(clip_rect.z - clip_rect.x);
                        scissor.w = (uint32_t)(clip_rect.w - clip_rect.y);

                        set_scissor(cb,
                            scissor.x,
                            scissor.y,
                            scissor.z,
                            scissor.w
                        );

                        // Draw
                        draw_indexed(cb, pcmd->ElemCount, 1, pcmd->IdxOffset + idx_buffer_offset, pcmd->VtxOffset + vtx_buffer_offset, 0);
                    }
                }
            }

            vtx_buffer_offset += cmd_list->VtxBuffer.Size;
            idx_buffer_offset += cmd_list->IdxBuffer.Size;
        }

        co_return;
    }

} // namespace debugui::imgui
