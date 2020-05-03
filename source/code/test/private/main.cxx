#include <core/memory.hxx>
#include <core/allocators/proxy_allocator.hxx>
#include <core/string.hxx>
#include <core/stack_string.hxx>
#include <core/string_view.hxx>
#include <core/pod/array.hxx>
#include <core/collections.hxx>
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
#include <asset_system/asset_module.hxx>

#include <fmt/format.h>
#include <application/application.hxx>

#include <iceshard/module.hxx>
#include <iceshard/engine.hxx>
#include <iceshard/frame.hxx>
#include <iceshard/world/world.hxx>
#include <iceshard/entity/entity_index.hxx>
#include <iceshard/entity/entity_command_buffer.hxx>
#include <iceshard/component/component_system.hxx>
#include <iceshard/component/component_block.hxx>
#include <iceshard/component/component_block_operation.hxx>
#include <iceshard/component/component_block_allocator.hxx>
#include <iceshard/component/component_archetype_index.hxx>
#include <iceshard/component/component_archetype_iterator.hxx>
#include <iceshard/renderer/render_pipeline.hxx>
#include <iceshard/renderer/render_resources.hxx>
#include <iceshard/renderer/render_pass.hxx>
#include <iceshard/renderer/render_model.hxx>

#include <iceshard/ecs/model.hxx>
#include <iceshard/ecs/transform.hxx>
#include <iceshard/ecs/light.hxx>
#include <iceshard/ecs/camera.hxx>

#include <debugui/debugui_module.hxx>
#include <debugui/debugui.hxx>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include <imgui/imgui.h>


struct DebugName
{
    static constexpr auto identifier = "isc.debug_name"_sid;

    core::StackString<32> name;
};

struct Position
{
    static constexpr auto identifier = "isc.position"_sid;

    glm::vec3 pos;
};

struct Transform
{
    static constexpr auto identifier = "isc.transform"_sid;

    glm::mat4 xform;
};

class PostProcessSystem
{
    using Buffer = iceshard::renderer::Buffer;
    using BufferType = iceshard::renderer::api::BufferType;
    using DataView = iceshard::renderer::api::DataView;

    struct Vertice
    {
        glm::vec2 pos;
        glm::vec2 uv;
    };

public:
    PostProcessSystem(
        core::allocator& alloc,
        render::RenderSystem& render_system
    ) noexcept
        : _allocator{ alloc }
        , _render_system{ render_system  }
    {
        _buffers[0] = _render_system.create_buffer(BufferType::IndexBuffer, 1024);
        _buffers[1] = _render_system.create_buffer(BufferType::VertexBuffer, 1024);

        DataView views[2];

        iceshard::renderer::api::render_api_instance->buffer_array_map_data(_buffers, views, core::size(_buffers));
        memcpy(views[0].data, PostProcessIndices, sizeof(PostProcessIndices));
        memcpy(views[1].data, PostProcessVertices, sizeof(PostProcessVertices));
        iceshard::renderer::api::render_api_instance->buffer_array_unmap_data(_buffers, core::size(_buffers));
    }

    void update(iceshard::renderer::CommandBuffer cb) noexcept
    {
        iceshard::renderer::commands::bind_index_buffer(cb, _buffers[0]);

        core::pod::Array<Buffer> buffer_views{ core::memory::globals::null_allocator() };
        core::pod::array::create_view(buffer_views, &_buffers[1], 1);

        iceshard::renderer::commands::bind_vertex_buffers(cb, buffer_views);
        iceshard::renderer::commands::draw_indexed(cb, 3, 1, 0, 0, 0);
    }

private:
    core::allocator& _allocator;
    render::RenderSystem& _render_system;

    iceshard::renderer::Buffer _buffers[2];

    static Vertice PostProcessVertices[3];
    static uint16_t PostProcessIndices[3];
};

PostProcessSystem::Vertice PostProcessSystem::PostProcessVertices[3] = {
    { { -1.0f, -1.0f }, { 0.0f, 0.0f, } },
    { { 3.0f, -1.0f }, { 2.0f, 0.0f, } },
    { { -1.0f, 3.0f }, { 0.0f, 2.0f, } },
};

uint16_t PostProcessSystem::PostProcessIndices[3] = {
    0, 1, 2,
};

class DebugNameUI : public debugui::DebugUI
{
public:
    DebugNameUI(
        debugui::debugui_context_handle context,
        core::allocator& alloc,
        iceshard::ecs::ArchetypeIndex* archetype_index
    ) noexcept
        : debugui::DebugUI{ context }
        , _allocator{ alloc }
        , _arch_index{ *archetype_index }
    { }

    void end_frame() noexcept override
    {
        iceshard::ecs::ComponentQuery<iceshard::component::Entity*, DebugName*> debug_name_query{ _allocator };
        iceshard::ecs::ComponentQuery<iceshard::component::Camera*> camera_query{ _allocator };

        if (ImGui::Begin("Debug names"))
        {
            iceshard::ecs::for_each_entity(
                iceshard::ecs::query_index(
                    debug_name_query,
                    _arch_index
                ),
                [](iceshard::component::Entity* e, DebugName* debug_name) noexcept
                {
                    ImGui::Text("%s <%llu>", core::string::begin(debug_name->name), core::hash(e->e));
                }
            );
        }
        ImGui::End();

        if (ImGui::Begin("Camera"))
        {
            iceshard::ecs::for_each_entity(
                iceshard::ecs::query_index(
                    camera_query,
                    _arch_index
                ),
                [](iceshard::component::Camera* cam) noexcept
                {
                    ImGui::Text("pos[ %0.3f, %.3f, %0.3f ] / front[ %0.3f, %.3f, %0.3f ]"
                        , cam->position.x, cam->position.y, cam->position.z
                        , cam->front.x, cam->front.y, cam->front.z
                    );
                }
            );
        }
        ImGui::End();
    }

private:
    core::allocator& _allocator;
    iceshard::ecs::ArchetypeIndex& _arch_index;
};

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
        iceshard::Engine* engine_instance = engine_module->engine();

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

        auto* assimp_module_location = resource_system.find(URN{ "asset_module.dll" });
        auto assimp_module = iceshard::load_asset_module(alloc, assimp_module_location->location().path, *asset_system);

        asset_system->add_resolver(asset::default_resolver_mesh(alloc));
        asset_system->add_resolver(asset::default_resolver_shader(alloc));
        asset_system->add_loader(asset::AssetType::Mesh, asset::default_loader_mesh(alloc));
        asset_system->add_loader(asset::AssetType::Shader, asset::default_loader_shader(alloc));
        asset_system->update();
        resource_system.flush_messages();

        // Prepare the render system
        auto* render_system = engine_instance->render_system();

        using iceshard::renderer::RenderResource;
        using iceshard::renderer::RenderResourceType;

        core::pod::Array<RenderResource> resources{ alloc };
        core::pod::array::resize(resources, 1);

        resources[0].type = RenderResourceType::ResTexture2D;
        resources[0].handle.texture = iceshard::renderer::api::Texture::Attachment0;
        resources[0].binding = 2;

        [[maybe_unused]]
        auto pp_resource_set = render_system->create_resource_set(
            "pp.3d"_sid,
            iceshard::renderer::RenderPipelineLayout::PostProcess,
            resources
        );

        core::pod::Array<asset::AssetData> shader_assets{ alloc };
        core::pod::array::resize(shader_assets, 2);

        asset_system->load(asset::AssetShader{ "shaders/debug/test-vert" }, shader_assets[0]);
        asset_system->load(asset::AssetShader{ "shaders/debug/test-frag" }, shader_assets[1]);

        auto pipeline = render_system->create_pipeline(
            "test.3d"_sid,
            iceshard::renderer::RenderPipelineLayout::Default,
            shader_assets
        );

        asset_system->load(asset::AssetShader{ "shaders/debug/pp-vert" }, shader_assets[0]);
        asset_system->load(asset::AssetShader{ "shaders/debug/pp-frag" }, shader_assets[1]);

        [[maybe_unused]]
        auto pp_pipeline = render_system->create_pipeline(
            "pp.3d"_sid,
            iceshard::renderer::RenderPipelineLayout::PostProcess,
            shader_assets
        );

        // Debug UI module
        core::memory::unique_pointer<debugui::DebugUIModule> debugui_module{ nullptr, { alloc } };
        core::memory::unique_pointer<DebugNameUI> debugname_ui{ nullptr, { alloc } };

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

        fmt::print("IceShard engine revision: {}\n", engine_instance->revision());

        glm::uvec2 viewport{ 1280, 720 };

        [[maybe_unused]]
        iceshard::World* world = engine_instance->world_manager()->create_world("test-world"_sid);

        auto arch_idx = world->service_provider()->archetype_index();
        arch_idx->add_component(world->entity(), DebugName{ "Test World" });

        if (debugui_module)
        {
            debugname_ui = core::memory::make_unique<DebugNameUI>(alloc,
                debugui_module->context_handle(),
                alloc,
                arch_idx
            );
            debugui_module->context().register_ui(debugname_ui.get());
        }

        PostProcessSystem post_process{ alloc, *render_system };


        core::pod::Array<iceshard::Entity> entities{ alloc };
        engine_instance->entity_manager()->create_many(10, entities);

        namespace ism = core::math;

        auto pos = ism::translate(
            ism::scale(
                ism::identity<ism::mat4>(),
                ism::vec3{ 10.0f, 0.01f, 10.0f }
            ),
            ism::vec3{ 0.0f, -1.0f, 0.0f }
        );

        arch_idx->add_component(entities[0], iceshard::component::Transform{ pos });
        arch_idx->add_component(entities[0], iceshard::component::ModelMaterial{ ism::vec4 { 0.8f, 0.8f, 0.8f, 1.0f } });
        arch_idx->add_component(entities[0], iceshard::component::ModelName{ "mesh/box/box2"_sid });

        pos = ism::translate(
            ism::identity<ism::mat4>(),
            ism::vec3{ 0.0f, 0.0f, 0.0f }
        );

        arch_idx->add_component(entities[4], iceshard::component::ModelName{ "mesh/box/dbox"_sid });
        arch_idx->add_component(entities[4], iceshard::component::Transform{ pos });
        arch_idx->add_component(entities[4], iceshard::component::ModelMaterial{ ism::vec4 { 0.3f, 0.6f, 0.2f, 1.0f } });

        pos = ism::translate(
            ism::identity<ism::mat4>(),
            ism::vec3{ 3.0f, 0.0f, 0.0f }
        );

        arch_idx->add_component(entities[5], iceshard::component::ModelName{ "mesh/box/box2"_sid });
        arch_idx->add_component(entities[5], iceshard::component::Transform{ pos });
        arch_idx->add_component(entities[5], iceshard::component::ModelMaterial{ ism::vec4 { 0.8f, 0.3f, 0.2f, 1.0f } });

        pos = ism::scale(
            ism::translate(
                ism::identity<ism::mat4>(),
                ism::vec3{ 0.5f, 0.5f, 5.0f }
            ),
            ism::vec3{ 0.04f, 0.04f, 0.04f }
        );

        arch_idx->add_component(entities[9], iceshard::component::Light{ { 0.5f, 0.5f, 5.0f } });
        arch_idx->add_component(entities[9], iceshard::component::ModelName{ "mesh/box/box2"_sid });
        arch_idx->add_component(entities[9], iceshard::component::Transform{ pos });
        arch_idx->add_component(entities[9], iceshard::component::ModelMaterial{ ism::vec4 { 0.8, 0.8, 0.8, 1.0f } });
        arch_idx->add_component(entities[9], DebugName{ "Light" });


        iceshard::Entity camera_entity = engine_instance->entity_manager()->create();
        arch_idx->add_component(camera_entity,
            iceshard::component::Camera{
                .position = { 0.0f, 0.0f, 7.0f },
                .front = { 0.0f, 0.0f, -1.0f },
                .fovy = 45.0f,
                .yaw = -90.f,
                .pitch = 0.0f,
            }
        );


        iceshard::ecs::ComponentQuery<iceshard::component::Light*, iceshard::component::Transform*> light_query{ alloc };

        static float angle = 2.0f;

        bool quit = false;
        while (quit == false)
        {
            auto& frame = engine_instance->current_frame();

            //iceshard::ecs::for_each_entity(
            //    iceshard::ecs::query_entity(light_query, *arch_idx, entities[4]),
            //    [](iceshard::component::Light* l, iceshard::component::Transform* xform) noexcept
            //    {
            //        //angle += 0.02f;
            //        if (angle >= 360.f)
            //        {
            //            angle = 0.f;
            //        }
            //        auto rotation_mat = glm::rotate(glm::radians(angle), glm::vec3{ 0.0f, 1.0f, 0.0f });
            //        glm::vec3 pos = { l->position.x, l->position.y, l->position.z };
            //        pos = glm::vec4(pos, 1.0f) * rotation_mat;

            //        memcpy(std::addressof(l->position), &pos, sizeof(pos));

            //        auto mat_xform = glm::scale(
            //            glm::translate(pos),
            //            glm::vec3{ 0.04, 0.08, 0.04 }
            //        );
            //        memcpy(std::addressof(xform->xform), &mat_xform, sizeof(mat_xform));
            //    }
            //);

            core::message::filter<input::message::AppExit>(engine_instance->current_frame().messages(), [&quit](auto const&) noexcept
                {
                    quit = true;
                });

            engine_instance->add_task([](
                    iceshard::Engine* engine_instance,
                    iceshard::renderer::RenderSystem* render_system,
                    glm::uvec2 viewport,
                    PostProcessSystem* post_process,
                    debugui::DebugUIModule* debugui_module,
                    iceshard::renderer::Pipeline pp_pipeline,
                    iceshard::renderer::ResourceSet pp_resource_set
                ) noexcept -> cppcoro::task<>
                {
                    {
                        using iceshard::renderer::RenderPassStage;
                        using namespace iceshard::renderer::commands;

                        auto cb = render_system->acquire_command_buffer(RenderPassStage::PostProcess);
                        bind_pipeline(cb, pp_pipeline);
                        bind_resource_set(cb, pp_resource_set);
                        set_viewport(cb, viewport.x, viewport.y);
                        set_scissor(cb, viewport.x, viewport.y);
                        post_process->update(cb);
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

                    co_return;
                }(
                    engine_instance,
                    render_system,
                    viewport,
                    &post_process,
                    debugui_module.get(),
                    pp_pipeline,
                    pp_resource_set
                    ));

            engine_instance->next_frame();

            core::message::filter<input::message::WindowSizeChanged>(frame.messages(), [&viewport](auto const& msg) noexcept
                {
                    fmt::print("Window size changed to: {}x{}\n", msg.width, msg.height);
                    viewport = { msg.width, msg.height };
                });
        }

        engine_instance->world_manager()->destroy_world("test-world"_sid);

        render_system->destroy_pipeline("test.3d"_sid);
        render_system->destroy_resource_set("test.3d"_sid);
        render_system->destroy_pipeline("pp.3d"_sid);
        render_system->destroy_resource_set("pp.3d"_sid);

        debugui_module = nullptr;
    }

    return 0;
}
