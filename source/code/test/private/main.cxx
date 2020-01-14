#include <core/memory.hxx>
#include <core/allocators/proxy_allocator.hxx>
#include <core/string.hxx>
#include <core/stack_string.hxx>
#include <core/string_view.hxx>
#include <core/pod/array.hxx>
#include <core/datetime/datetime.hxx>

#include <core/cexpr/stringid.hxx>
#include <core/scope_exit.hxx>
#include <core/debug/profiler.hxx>
#include <core/pod/hash.hxx>

#include <resource/uri.hxx>
#include <resource/resource_system.hxx>
#include <resource/modules/dynlib_module.hxx>
#include <resource/modules/filesystem_module.hxx>

#include <core/message/buffer.hxx>
#include <core/message/operations.hxx>

#include <input_system/module.hxx>
#include <input_system/message/app.hxx>
#include <input_system/message/mouse.hxx>

#include <render_system/render_commands.hxx>
#include <render_system/render_vertex_descriptor.hxx>
#include <render_system/render_pipeline.hxx>

#include <asset_system/asset_system.hxx>
#include <asset_system/assets/asset_config.hxx>
#include <asset_system/assets/asset_shader.hxx>
#include <asset_system/assets/asset_mesh.hxx>

#include <fmt/format.h>
#include <application/application.hxx>

#include <iceshard/module.hxx>
#include <iceshard/engine.hxx>
#include <iceshard/frame.hxx>
#include <iceshard/world/world.hxx>
#include <iceshard/entity/entity_index.hxx>
#include <iceshard/entity/entity_command_buffer.hxx>
#include <iceshard/component/component_system.hxx>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include <imgui/imgui.h>

int game_main(core::allocator& alloc, resource::ResourceSystem& resource_system)
{
    using resource::URN;
    using resource::URI;

    resource_system.mount(URI{ resource::scheme_file, "../source/data/config.json" });

    auto* engine_module_location = resource_system.find(URN{ "iceshard.dll" });
    IS_ASSERT(engine_module_location != nullptr, "Missing engine module!");

    if (auto engine_module = iceshard::load_engine_module(alloc, engine_module_location->location().path, resource_system))
    {
        auto* engine_instance = engine_module->engine();

        // Default file system mount points
        resource_system.flush_messages();
        resource_system.mount(URI{ resource::scheme_file, "mount.isr" });
        resource_system.mount(URI{ resource::scheme_directory, "../source/data" });

        // Check for an user defined mounting file
        if (auto* mount_resource = resource_system.find(URI{ resource::scheme_file, "mount.isr" }))
        {
            fmt::print("Custom mount resource found: {}\n", mount_resource->location());
        }

        // Prepare the asset system
        auto* asset_system = engine_instance->asset_system();
        asset_system->add_resolver(asset::default_resolver_mesh(alloc));
        asset_system->add_resolver(asset::default_resolver_shader(alloc));
        asset_system->add_loader(asset::AssetType::Mesh, asset::default_loader_mesh(alloc));
        asset_system->add_loader(asset::AssetType::Shader, asset::default_loader_shader(alloc));
        asset_system->update();
        resource_system.flush_messages();

        // Prepare the render system
        auto* render_system = engine_instance->render_system();
        render_system->add_named_vertex_descriptor_set(render::descriptor_set::Color);
        render_system->add_named_vertex_descriptor_set(render::descriptor_set::Model);
        render_system->add_named_vertex_descriptor_set(render::descriptor_set::ImGui);

        asset::AssetData shader_data;
        if (asset_system->load(asset::AssetShader{ "shaders/debug/imgui-vert" }, shader_data) == asset::AssetStatus::Loaded)
        {
            render_system->load_shader(shader_data);
        }
        if (asset_system->load(asset::AssetShader{ "shaders/debug/imgui-frag" }, shader_data) == asset::AssetStatus::Loaded)
        {
            render_system->load_shader(shader_data);
        }

        render::api::VertexBuffer vtx_buffer[2]{};
        render::api::VertexBuffer idx_buffer{};

        asset::AssetData mesh_data;
        if (asset_system->load(asset::Asset{ "mesh/test/box", asset::AssetType::Mesh }, mesh_data) == asset::AssetStatus::Loaded)
        {
            vtx_buffer[0] = render_system->create_vertex_buffer(1024 * 128 * sizeof(float));

            render::api::BufferDataView buffer_data_view;
            render::api::render_api_instance->vertex_buffer_map_data(vtx_buffer[0], buffer_data_view);
            IS_ASSERT(buffer_data_view.data_size >= mesh_data.content._size, "Render buffer not big enoguht! Ugh!");

            std::memcpy(buffer_data_view.data_pointer, mesh_data.content._data, mesh_data.content._size);
            render::api::render_api_instance->vertex_buffer_unmap_data(vtx_buffer[0]);
        }

        {
            static const glm::mat4 instances[] = {
                glm::mat4{ 1.0f },
            };

            vtx_buffer[1] = render_system->create_vertex_buffer(sizeof(instances));

            render::api::BufferDataView buffer_data_view;
            render::api::render_api_instance->vertex_buffer_map_data(vtx_buffer[1], buffer_data_view);
            IS_ASSERT(buffer_data_view.data_size >= sizeof(instances), "Render buffer not big enoguht! Ugh!");

            std::memcpy(buffer_data_view.data_pointer, &instances, sizeof(instances));
            render::api::render_api_instance->vertex_buffer_unmap_data(vtx_buffer[1]);
        }

        static auto projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);
        static auto cam_pos = glm::vec3(-5, 3, -10);
        static auto clip = glm::mat4(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, -1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.5f, 0.0f,
            0.0f, 0.0f, 0.5f, 1.0f
        );

        glm::mat4 MVP{ 1 };
        auto uniform_buffer = render_system->create_uniform_buffer(sizeof(MVP));
        idx_buffer = render_system->create_vertex_buffer(1024 * 128 * sizeof(uint16_t));


        {
            auto new_view = glm::lookAt(
                cam_pos, // Camera is at (-5,3,-10), in World Space
                glm::vec3(0, 0, 0),      // and looks at the origin
                glm::vec3(0, -1, 0)      // Head is up (set to 0,-1,0 to look upside-down)
            );

            static float deg = 0.0f;
            deg += 3.0f;
            new_view = glm::rotate(new_view, glm::radians(deg), glm::vec3{ 0.f, 1.f, 0.f });

            if (deg >= 360.0f)
                deg = 0.0f;

            MVP = clip * projection * new_view;

            render::api::BufferDataView data_view;
            render::api::render_api_instance->uniform_buffer_map_data(uniform_buffer, data_view);
            IS_ASSERT(data_view.data_size >= sizeof(MVP), "Insufficient buffer size!");

            memcpy(data_view.data_pointer, &MVP, sizeof(MVP));
            render::api::render_api_instance->uniform_buffer_unmap_data(uniform_buffer);
        }

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize.x = 1280;
        io.DisplaySize.y = 720;

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
        render::api::Texture tex = render_system->load_texture(std::move(tex_data));

        io.Fonts->TexID = reinterpret_cast<ImTextureID>(tex);

        render_system->create_uniform_descriptor_sets(sizeof(MVP));
        auto render_pipeline = render_system->create_pipeline(render::pipeline::ImGuiPipeline);

        fmt::print("IceShard engine revision: {}\n", engine_instance->revision());
        ImGui::NewFrame();

        // Create a test world
        engine_instance->world_manager()->create_world(core::cexpr::stringid("test-world"));

        bool quit = false;
        while (quit == false)
        {
            core::message::filter<input::message::AppExit>(engine_instance->current_frame().messages(), [&quit](auto const&) noexcept
                {
                    quit = true;
                });

            core::message::filter<input::message::MouseMotion>(engine_instance->current_frame().messages(), [&]([[maybe_unused]] input::message::MouseMotion const& msg) noexcept
                {
                    //static auto last_pos = msg.pos.x;

                    //auto new_view = glm::lookAt(
                    //    cam_pos, // Camera is at (-5,3,-10), in World Space
                    //    glm::vec3(0, 0, 0),      // and looks at the origin
                    //    glm::vec3(0, -1, 0)      // Head is up (set to 0,-1,0 to look upside-down)
                    //);

                    //static float deg = 0.0f;
                    //deg += static_cast<float>(last_pos - msg.pos.x);
                    //new_view = glm::rotate(new_view, glm::radians(deg), glm::vec3{ 0.f, 1.f, 0.f });
                    //last_pos = msg.pos.x;

                    //if (deg >= 360.0f)
                    //    deg = 0.0f;

                    auto ortho_projection = glm::ortho(0.f, io.DisplaySize.x, io.DisplaySize.y, 0.f);

                    MVP = ortho_projection;// clip * projection * new_view;

                    render::api::BufferDataView data_view;
                    render::api::render_api_instance->uniform_buffer_map_data(uniform_buffer, data_view);
                    IS_ASSERT(data_view.data_size >= sizeof(MVP), "Insufficient buffer size!");

                    memcpy(data_view.data_pointer, &MVP, sizeof(MVP));
                    render::api::render_api_instance->uniform_buffer_unmap_data(uniform_buffer);
                });

            static bool demo = true;
            ImGui::ShowDemoWindow(&demo);

            ImGui::ShowMetricsWindow();

            ImGui::EndFrame();
            ImGui::Render();


            engine_instance->add_task(
                [](iceshard::Engine& e, render::api::RenderPipeline pipeline, render::api::VertexBuffer idx_buffer, render::api::VertexBuffer vtx_buffer[2]) noexcept -> cppcoro::task<>
                {
                    ImGuiIO& io = ImGui::GetIO();

                    sizeof(ImDrawVert);

                    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
                    int fb_width = static_cast<int>(io.DisplaySize.x * io.DisplayFramebufferScale.x);
                    int fb_height = static_cast<int>(io.DisplaySize.y * io.DisplayFramebufferScale.y);
                    if (fb_width == 0 || fb_height == 0)
                    {
                        return;
                    }

                    auto command_buffer = e.render_system()->command_buffer();
                    auto descriptor_sets = e.render_system()->descriptor_sets();

                    render::cmd::begin(command_buffer);
                    render::cmd::begin_renderpass(command_buffer);
                    render::cmd::bind_render_pipeline(command_buffer, pipeline);
                    render::cmd::bind_descriptor_sets(command_buffer, descriptor_sets);
                    render::cmd::bind_vertex_buffers(command_buffer, vtx_buffer[0], vtx_buffer[1]);
                    render::cmd::bind_index_buffers(command_buffer, idx_buffer);
                    render::cmd::set_viewport(command_buffer, 1280, 720);
                    render::cmd::set_scissor(command_buffer, 1280, 720);

                    constexpr bool test_draw = false;

                    [[maybe_unused]]
                    auto* draw_data = ImGui::GetDrawData();

                    // Upload vertex/index data into a single contiguous GPU buffer
                    {
                        render::api::BufferDataView vtx_buff, idx_buff;
                        render::api::render_api_instance->vertex_buffer_map_data(vtx_buffer[0], vtx_buff);
                        render::api::render_api_instance->vertex_buffer_map_data(idx_buffer, idx_buff);

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

                        render::api::render_api_instance->vertex_buffer_unmap_data(idx_buffer);
                        render::api::render_api_instance->vertex_buffer_unmap_data(vtx_buffer[0]);
                    }

                    if constexpr (test_draw)
                    {
                        render::cmd::draw_indexed(command_buffer, 6, 1, 0, 0, 0);
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

                    co_return;
                }(*engine_instance, render_pipeline, idx_buffer, vtx_buffer));

            engine_instance->next_frame();
            ImGui::NewFrame();
        }

        ImGui::EndFrame();

        engine_instance->world_manager()->destroy_world(core::cexpr::stringid("test-world"));

        ImGui::DestroyContext();
    }

    return 0;
}
