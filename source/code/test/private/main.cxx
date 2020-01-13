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

        asset::AssetData shader_data;
        if (asset_system->load(asset::AssetShader{ "shaders/debug/test-vert" }, shader_data) == asset::AssetStatus::Loaded)
        {
            render_system->load_shader(shader_data);
        }
        if (asset_system->load(asset::AssetShader{ "shaders/debug/test-frag" }, shader_data) == asset::AssetStatus::Loaded)
        {
            render_system->load_shader(shader_data);
        }

        render::api::VertexBuffer vtx_buffer[2]{};

        asset::AssetData mesh_data;
        if (asset_system->load(asset::Asset{ "mesh/test/box", asset::AssetType::Mesh }, mesh_data) == asset::AssetStatus::Loaded)
        {
            vtx_buffer[0] = render_system->create_vertex_buffer(mesh_data.content._size);

            render::api::BufferDataView buffer_data_view;
            render::api::render_api_instance->vertex_buffer_map_data(vtx_buffer[0], buffer_data_view);
            IS_ASSERT(buffer_data_view.data_size >= mesh_data.content._size, "Render buffer not big enoguht! Ugh!");

            std::memcpy(buffer_data_view.data_pointer, mesh_data.content._data, mesh_data.content._size);
            render::api::render_api_instance->vertex_buffer_unmap_data(vtx_buffer[0]);
        }

        {
            static const glm::mat4 instances[] = {
                glm::mat4{ 1.0f },
                glm::translate(glm::mat4{ 1.0f }, glm::vec3{ 1.0f, 3.0f, 1.0f }),
                glm::translate(glm::mat4{ 1.0f }, glm::vec3{ 3.0f, 1.0f, 1.0f }),
                glm::translate(glm::mat4{ 1.0f }, glm::vec3{ 1.0f, 1.0f, 3.0f }),
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

        render_system->create_uniform_descriptor_sets(sizeof(MVP));
        auto render_pipeline = render_system->create_pipeline(render::pipeline::DefaultPieline);

        fmt::print("IceShard engine revision: {}\n", engine_instance->revision());

        // Create a test world
        engine_instance->world_manager()->create_world(core::cexpr::stringid("test-world"));

        bool quit = false;
        while (quit == false)
        {
            core::message::filter<input::message::AppExit>(engine_instance->current_frame().messages(), [&quit](auto const&) noexcept
                {
                    quit = true;
                });

            core::message::filter<input::message::MouseMotion>(engine_instance->current_frame().messages(), [&](input::message::MouseMotion const& msg) noexcept
                {
                    static auto last_pos = msg.pos.x;

                    auto new_view = glm::lookAt(
                        cam_pos, // Camera is at (-5,3,-10), in World Space
                        glm::vec3(0, 0, 0),      // and looks at the origin
                        glm::vec3(0, -1, 0)      // Head is up (set to 0,-1,0 to look upside-down)
                    );

                    static float deg = 0.0f;
                    deg += static_cast<float>(last_pos - msg.pos.x);
                    new_view = glm::rotate(new_view, glm::radians(deg), glm::vec3{ 0.f, 1.f, 0.f });
                    last_pos = msg.pos.x;

                    if (deg >= 360.0f)
                        deg = 0.0f;

                    MVP = clip * projection * new_view;

                    render::api::BufferDataView data_view;
                    render::api::render_api_instance->uniform_buffer_map_data(uniform_buffer, data_view);
                    IS_ASSERT(data_view.data_size >= sizeof(MVP), "Insufficient buffer size!");

                    memcpy(data_view.data_pointer, &MVP, sizeof(MVP));
                    render::api::render_api_instance->uniform_buffer_unmap_data(uniform_buffer);
                });

            engine_instance->add_task([](iceshard::Engine& e, render::api::RenderPipeline pipeline, render::api::VertexBuffer vtx_buffer[2]) noexcept -> cppcoro::task<>
                {
                    auto command_buffer = e.render_system()->command_buffer();
                    auto descriptor_sets = e.render_system()->descriptor_sets();

                    render::cmd::begin(command_buffer);
                    render::cmd::begin_renderpass(command_buffer);
                    render::cmd::bind_render_pipeline(command_buffer, pipeline);
                    render::cmd::bind_descriptor_sets(command_buffer, descriptor_sets);
                    render::cmd::bind_vertex_buffers(command_buffer, vtx_buffer[0], vtx_buffer[1]);
                    render::cmd::set_viewport(command_buffer, 1280, 720);
                    render::cmd::set_scissor(command_buffer, 1280, 720);
                    render::cmd::draw(command_buffer, 12 * 3, 4);
                    render::cmd::end_renderpass(command_buffer);
                    render::cmd::end(command_buffer);

                    co_return;
                }(*engine_instance, render_pipeline, vtx_buffer));

            engine_instance->next_frame();
        }

        engine_instance->world_manager()->destroy_world(core::cexpr::stringid("test-world"));
    }

    return 0;
}
