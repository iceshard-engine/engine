#include "imgui_renderer.hxx"
#include <core/memory.hxx>
#include <render_system/render_commands.hxx>

#include <glm/glm.hpp>

namespace debugui::imgui
{

    namespace detail
    {

        auto create_fonts_texture(core::allocator& alloc, ImGuiIO& io, render::RenderSystem& render_system) noexcept -> render::api::Texture
        {
            unsigned char* pixels;
            int width, height;
            io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

            [[maybe_unused]]
            uint32_t upload_size = width * height * 4 * sizeof(char);

            resource::ResourceMeta meta{ alloc };
            resource::set_meta_int32(meta, "texture.extents.width"_sid, width);
            resource::set_meta_int32(meta, "texture.extents.height"_sid, height);

            asset::AssetData tex_data;
            tex_data.metadata = resource::create_meta_view(meta);
            tex_data.content = { pixels, upload_size };
            return render_system.load_texture(std::move(tex_data));
        }

    } // namespace detail

    ImGuiRenderer::ImGuiRenderer(core::allocator& alloc, ImGuiIO& io, asset::AssetSystem& asset_system, render::RenderSystem& render_system) noexcept
        : _allocator{ alloc }
        , _asset_system{ asset_system }
        , _render_system{ render_system }
        , _io{ io }
        , _vertice_buffers{ alloc }
    {
        _render_system.initialize_render_interface(&render::api::render_api_instance);

        core::pod::array::push_back(_vertice_buffers, _render_system.create_vertex_buffer(sizeof(ImDrawVert) * 1024));
        _indice_buffer = _render_system.create_vertex_buffer(sizeof(ImDrawIdx) * 1024 * 256);

        _render_system.add_named_vertex_descriptor_set(render::descriptor_set::ImGui);

        asset::AssetData shader_data;
        if (_asset_system.load(asset::AssetShader{ "shaders/debug/imgui-vert" }, shader_data) == asset::AssetStatus::Loaded)
        {
            _render_system.load_shader(shader_data);
        }
        if (_asset_system.load(asset::AssetShader{ "shaders/debug/imgui-frag" }, shader_data) == asset::AssetStatus::Loaded)
        {
            _render_system.load_shader(shader_data);
        }

        _io.DisplaySize.x = 1280;
        _io.DisplaySize.y = 720;
        _font_texture = detail::create_fonts_texture(alloc, _io, _render_system);
        io.Fonts->TexID = reinterpret_cast<ImTextureID>(_font_texture);

        _render_system.create_imgui_descriptor_sets();
        _pipeline = _render_system.create_pipeline(render::pipeline::ImGuiPipeline);
    }

    ImGuiRenderer::~ImGuiRenderer() noexcept
    {
    }

    void ImGuiRenderer::Draw(ImDrawData* draw_data) noexcept
    {
        // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
        int fb_width = static_cast<int>(_io.DisplaySize.x * _io.DisplayFramebufferScale.x);
        int fb_height = static_cast<int>(_io.DisplaySize.y * _io.DisplayFramebufferScale.y);
        if (fb_width == 0 || fb_height == 0)
        {
            return;
        }

        auto command_buffer = _render_system.command_buffer();
        auto descriptor_sets = _render_system.descriptor_sets();

        render::cmd::begin(command_buffer);
        render::cmd::begin_renderpass(command_buffer);
        render::cmd::bind_render_pipeline(command_buffer, _pipeline);
        render::cmd::bind_descriptor_sets(command_buffer, descriptor_sets);
        render::cmd::bind_vertex_buffers(command_buffer, _vertice_buffers);
        render::cmd::bind_index_buffers(command_buffer, _indice_buffer);
        render::cmd::set_viewport(command_buffer, 1280, 720);
        render::cmd::set_scissor(command_buffer, 1280, 720);

        constexpr bool test_draw = false;

        // Upload vertex/index data into a single contiguous GPU buffer
        {
            render::api::BufferDataView vtx_buff, idx_buff;
            render::api::render_api_instance->vertex_buffer_map_data(_vertice_buffers[0], vtx_buff);
            render::api::render_api_instance->vertex_buffer_map_data(_indice_buffer, idx_buff);

            if constexpr (test_draw)
            {
                uint16_t indices[] = {
                    0, 1, 2,
                    0, 2, 3,
                };

                ImDrawVert vertexes[] = {
                    ImDrawVert{ { 0.0f, 0.0f }, { 0.0f, 0.0f }, 0xff0000ff },
                    ImDrawVert{ { 0.0f, 500.f }, { 0.0f, 0.1f }, 0x00ff00ff },
                    ImDrawVert{ { 500.f, 500.f }, { 0.1f, 0.1f }, 0x0000ffff },
                    ImDrawVert{ { 500.f, 0.0f }, { 0.1f, 0.0f }, 0x0f0f0fff },
                };

                memcpy(idx_buff.data_pointer, indices, sizeof(indices));
                memcpy(vtx_buff.data_pointer, vertexes, sizeof(vertexes));
            }
            else
            {
                for (int n = 0; n < draw_data->CmdListsCount; n++)
                {
                    const ImDrawList* cmd_list = draw_data->CmdLists[n];

                    IS_ASSERT(vtx_buff.data_size > cmd_list->VtxBuffer.Size * sizeof(ImDrawVert), "how?");
                    IS_ASSERT(idx_buff.data_size > cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx), "how?");

                    memcpy(vtx_buff.data_pointer, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
                    memcpy(idx_buff.data_pointer, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
                    vtx_buff.data_pointer = core::memory::utils::pointer_add(vtx_buff.data_pointer, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
                    idx_buff.data_pointer = core::memory::utils::pointer_add(idx_buff.data_pointer, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
                }
            }

            render::api::render_api_instance->vertex_buffer_unmap_data(_indice_buffer);
            render::api::render_api_instance->vertex_buffer_unmap_data(_vertice_buffers[0]);
        }

        if constexpr (test_draw)
        {
            //render::cmd::draw_indexed(command_buffer, 6, 1, 0, 0, 0);
        }
        else
        {
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
                            glm::ivec4 scissor;
                            scissor.x = (int32_t)(clip_rect.x);
                            scissor.y = (int32_t)(clip_rect.y);
                            scissor.z = (uint32_t)(clip_rect.z - clip_rect.x);
                            scissor.w = (uint32_t)(clip_rect.w - clip_rect.y);

                            render::cmd::set_scissor(command_buffer,
                                scissor.x,
                                scissor.y,
                                scissor.z,
                                scissor.w
                            );

                            // Draw
                            render::cmd::draw_indexed(command_buffer, pcmd->ElemCount, 1, pcmd->IdxOffset + idx_buffer_offset, pcmd->VtxOffset + vtx_buffer_offset, 0);
                        }
                    }
                }

                vtx_buffer_offset += cmd_list->VtxBuffer.Size;
                idx_buffer_offset += cmd_list->IdxBuffer.Size;
            }
        }

        //render::cmd::draw(command_buffer, 12 * 3, 4);
        render::cmd::end_renderpass(command_buffer);
        render::cmd::end(command_buffer);
    }

} // namespace debugui::imgui
