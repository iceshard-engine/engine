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
#include <input_system/message/keyboard.hxx>
#include <input_system/message/window.hxx>

#include <render_system/render_commands.hxx>

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
#include <iceshard/renderer/render_pipeline.hxx>
#include <iceshard/renderer/render_resources.hxx>
#include <iceshard/renderer/render_pass.hxx>

#include <debugui/debugui_module.hxx>
#include <debugui/debugui.hxx>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include <imgui/imgui.h>

class MainDebugUI : public debugui::DebugUI
{
public:
    MainDebugUI(debugui::debugui_context_handle context) noexcept
        : debugui::DebugUI{ context }
    { }

    bool quit_message() noexcept
    {
        return _quit;
    }

    void end_frame() noexcept override
    {
        static bool demo_window_visible = false;
        if (demo_window_visible)
        {
            ImGui::ShowDemoWindow(&demo_window_visible);
        }

        ImGui::BeginMainMenuBar();
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Demo window", nullptr, &demo_window_visible))
            {
                demo_window_visible = demo_window_visible ? true : false;
            }
            if (ImGui::MenuItem("Close", "Ctrl+W"))
            {
                _quit = true;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

private:
    bool _quit = false;
    bool _menu_visible = false;
};

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

        static auto projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);
        static auto cam_pos = glm::vec3(0.0f, 0.0f, -10.0f);
        static auto clip = glm::mat4(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, -1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.5f, 0.0f,
            0.0f, 0.0f, 0.5f, 1.0f
        );

        glm::mat4 MVP{ 1 };

        [[maybe_unused]]
        auto uniform_buffer = render_system->create_buffer(iceshard::renderer::api::BufferType::UniformBuffer, sizeof(MVP));

        static float deg = 0.0f;
        {
            auto new_view = glm::lookAt(
                cam_pos, // Camera is at (-5,3,-10), in World Space
                glm::vec3(0, 0, 0),      // and looks at the origin
                glm::vec3(0, -1, 0)      // Head is up (set to 0,-1,0 to look upside-down)
            );

            deg += 3.0f;
            new_view = glm::rotate(new_view, glm::radians(deg), glm::vec3{ 0.f, 1.f, 0.f });

            if (deg >= 360.0f)
                deg = 0.0f;

            MVP = clip * projection * new_view;

            iceshard::renderer::api::DataView data_view;
            iceshard::renderer::api::render_api_instance->buffer_array_map_data(&uniform_buffer, &data_view, 1);
            memcpy(data_view.data, &MVP, sizeof(MVP));
            iceshard::renderer::api::render_api_instance->buffer_array_unmap_data(&uniform_buffer, 1);
        }

        using iceshard::renderer::RenderResource;
        using iceshard::renderer::RenderResourceType;

        core::pod::Array<RenderResource> resources{ alloc };
        core::pod::array::resize(resources, 1);
        resources[0].type = RenderResourceType::ResUniform;
        resources[0].handle.uniform.buffer = uniform_buffer;
        resources[0].handle.uniform.offset = 0;
        resources[0].handle.uniform.range = sizeof(MVP);
        resources[0].binding = 0;

        [[maybe_unused]]
        auto resource_set = render_system->create_resource_set(
            "test.3d"_sid,
            iceshard::renderer::RenderPipelineLayout::Default,
            resources
        );

        core::pod::Array<asset::AssetData> shader_assets{ alloc };
        core::pod::array::resize(shader_assets, 2);

        asset_system->load(asset::AssetShader{ "shaders/debug/test-vert" }, shader_assets[0]);
        asset_system->load(asset::AssetShader{ "shaders/debug/test-frag" }, shader_assets[1]);

        [[maybe_unused]]
        auto pipeline = render_system->create_pipeline(
            "test.3d"_sid,
            iceshard::renderer::RenderPipelineLayout::Default,
            shader_assets
        );

        // Debug UI module
        core::memory::unique_pointer<debugui::DebugUIModule> debugui_module{ nullptr, { alloc } };

        if constexpr (core::build::is_release == false)
        {
            auto* debugui_module_location = resource_system.find(URN{ "imgui_driver.dll" });
            if (debugui_module_location != nullptr)
            {
                debugui_module = debugui::load_module(alloc, debugui_module_location->location().path, *engine_instance->input_system(), *asset_system, *render_system);
                engine_module->load_debugui(debugui_module->context());
            }
        }

        using iceshard::renderer::api::Buffer;
        using iceshard::renderer::api::BufferType;
        using iceshard::renderer::api::DataView;

        core::pod::Array<Buffer> buffs{ alloc };
        core::pod::array::push_back(buffs, render_system->create_buffer(BufferType::VertexBuffer, 1024));
        core::pod::array::push_back(buffs, render_system->create_buffer(BufferType::VertexBuffer, 1024));
        auto idx = render_system->create_buffer(BufferType::IndexBuffer, 1024);

        uint32_t indice_count = 0;
        {
            struct IsVertice {
                glm::vec3 pos;
                glm::vec3 color;
            } vertices[] = {
                { { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
                { { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
                { { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
                { { 1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 1.0f } },
            };
            uint16_t indices[] = {
                0, 1, 2,
                1, 3, 2,
                2, 1, 0,
                2, 3, 1,
            };
            indice_count = core::size(indices);
            glm::mat4 model = glm::translate(glm::mat4{ 1 }, glm::vec3{ -0.5f, 0.0f, 0.0f });

            Buffer bfs[] = {
                buffs[0], buffs[1], idx
            };
            DataView views[core::size(bfs)];

            iceshard::renderer::api::render_api_instance->buffer_array_map_data(bfs, views, core::size(bfs));
            memcpy(views[0].data, vertices, sizeof(vertices));
            memcpy(views[1].data, &model, sizeof(model));
            memcpy(views[2].data, indices, sizeof(indices));
            iceshard::renderer::api::render_api_instance->buffer_array_unmap_data(bfs, core::size(bfs));
        }


        fmt::print("IceShard engine revision: {}\n", engine_instance->revision());

        // Create a test world
        engine_instance->world_manager()->create_world("test-world"_sid);

        glm::uvec2 viewport{ 1280, 720 };

        bool quit = false;
        while (quit == false)
        {
            core::message::filter<input::message::AppExit>(engine_instance->current_frame().messages(), [&quit](auto const&) noexcept
                {
                    quit = true;
                });

            {
                using iceshard::renderer::RenderPassStage;
                using namespace iceshard::renderer::commands;

                auto cb = render_system->acquire_command_buffer(RenderPassStage::Geometry);
                bind_pipeline(cb, pipeline);
                bind_resource_set(cb, resource_set);
                bind_index_buffer(cb, idx);
                bind_vertex_buffers(cb, buffs);
                set_viewport(cb, viewport.x, viewport.y);
                set_scissor(cb, viewport.x, viewport.y);
                draw_indexed(cb, indice_count, 1, 0, 0, 0);
                render_system->submit_command_buffer(cb);
            }

            if constexpr (core::build::is_release == false)
            {
                if (debugui_module)
                {
                    auto& debugui_context = debugui_module->context();
                    debugui_context.update(engine_instance->current_frame().messages());
                    debugui_context.begin_frame();
                    debugui_context.end_frame();
                }
            }

            {
                auto new_view = glm::lookAt(
                    cam_pos, // Camera is at (-5,3,-10), in World Space
                    glm::vec3(0, 0, 0),      // and looks at the origin
                    glm::vec3(0, -1, 0)      // Head is up (set to 0,-1,0 to look upside-down)
                );

                deg += 3.0f;
                new_view = glm::rotate(new_view, glm::radians(deg), glm::vec3{ 0.f, 1.f, 0.f });

                if (deg >= 360.0f)
                    deg = 0.0f;

                MVP = clip * projection * new_view;

                iceshard::renderer::api::DataView data_view;
                iceshard::renderer::api::render_api_instance->buffer_array_map_data(&uniform_buffer, &data_view, 1);
                memcpy(data_view.data, &MVP, sizeof(MVP));
                iceshard::renderer::api::render_api_instance->buffer_array_unmap_data(&uniform_buffer, 1);
            }

            engine_instance->next_frame();

            core::message::filter<input::message::WindowSizeChanged>(engine_instance->current_frame().messages(), [&viewport](auto const& msg) noexcept
                {
                    fmt::print("Window size changed to: {}x{}\n", msg.width, msg.height);
                    viewport = { msg.width, msg.height };
                });
        }

        engine_instance->world_manager()->destroy_world("test-world"_sid);

        render_system->destroy_pipeline("test.3d"_sid);
        render_system->destroy_resource_set("test.3d"_sid);

        debugui_module = nullptr;
    }

    return 0;
}
