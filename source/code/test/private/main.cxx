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

#include <debugui/debugui_module.hxx>
#include <debugui/debugui.hxx>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include <imgui/imgui.h>


struct Entity
{
    static constexpr auto identifier = "isc.entity"_sid;

    iceshard::Entity e;
};

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

struct Camera
{
    static constexpr auto identifier = "isc.camera"_sid;

    float rotation;
};

struct Transform
{
    static constexpr auto identifier = "isc.transform"_sid;

    glm::mat4 xform;
};

class StaticMeshRenderer
{
    using Buffer = iceshard::renderer::api::Buffer;
    using BufferType = iceshard::renderer::api::BufferType;
    using DataView = iceshard::renderer::api::DataView;

    struct Vertice
    {
        glm::vec3 pos;
        glm::vec3 color;
    };

public:
    StaticMeshRenderer(
        core::allocator& alloc,
        iceshard::ecs::ArchetypeIndex& index,
        render::RenderSystem& render_system
    ) noexcept
        : _allocator{ alloc }
        , _index{ index }
        , _xform_query{ _allocator }
        , _render_system{ render_system }
        , _vertices{ _allocator }
        , _indices{ _allocator }
    {
        initialize();
    }

    void initialize() noexcept
    {
        core::pod::array::push_back(_vertices, { { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } });
        core::pod::array::push_back(_vertices, { { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } });
        core::pod::array::push_back(_vertices, { { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } });
        core::pod::array::push_back(_vertices, { { 1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 1.0f } });

        static uint16_t indice_array[] = {
                0, 1, 2,
                1, 3, 2,
                2, 1, 0,
                2, 3, 1,
        };

        for (auto indice : indice_array)
        {
            core::pod::array::push_back(_indices, indice);
        }

        _buffers[0] = _render_system.create_buffer(BufferType::VertexBuffer, 1024);
        _buffers[1] = _render_system.create_buffer(BufferType::VertexBuffer, 1024);
        _buffers[2] = _render_system.create_buffer(BufferType::IndexBuffer, 1024);

        {
            Buffer mapped_buffers[] = {
                _buffers[0],
                _buffers[2],
            };
            DataView mapped_buffer_views[core::size(mapped_buffers)];

            iceshard::renderer::api::render_api_instance->buffer_array_map_data(
                mapped_buffers,
                mapped_buffer_views,
                core::size(mapped_buffers)
            );

            memcpy(mapped_buffer_views[0].data, core::pod::array::begin(_vertices), core::pod::array::size(_vertices) * sizeof(Vertice));
            memcpy(mapped_buffer_views[1].data, core::pod::array::begin(_indices), core::pod::array::size(_indices) * sizeof(uint16_t));

            iceshard::renderer::api::render_api_instance->buffer_array_unmap_data(
                mapped_buffers,
                core::size(mapped_buffers)
            );
        }
    }

    void update(iceshard::renderer::CommandBuffer cb) noexcept
    {
        {
            DataView mapped_buffer_view;

            iceshard::renderer::api::render_api_instance->buffer_array_map_data(
                &_buffers[1],
                &mapped_buffer_view,
                1
            );

            glm::mat4* it = reinterpret_cast<glm::mat4*>(mapped_buffer_view.data);

            uint32_t model_count = 0;
            iceshard::ecs::for_each_entity(
                iceshard::ecs::query_index(_xform_query, _index),
                [&](Transform* tform) noexcept
                {
                    *it = tform->xform;
                    model_count += 1;
                    it += 1;
                }
            );

            iceshard::renderer::api::render_api_instance->buffer_array_unmap_data(
                &_buffers[1],
                1
            );

            core::pod::Array<Buffer> buffer_view{ core::memory::globals::null_allocator() };
            core::pod::array::create_view(buffer_view, _buffers, 2);

            using namespace iceshard::renderer::commands;

            bind_index_buffer(cb, _buffers[2]);
            bind_vertex_buffers(cb, buffer_view);
            draw_indexed(cb, 12, model_count, 0, 0, 0);
        }
    }

private:
    core::allocator& _allocator;
    iceshard::ecs::ArchetypeIndex& _index;
    iceshard::ecs::ComponentQuery<Transform*> _xform_query;
    render::RenderSystem& _render_system;

    core::pod::Array<Vertice> _vertices;
    core::pod::Array<uint16_t> _indices;

    Buffer _buffers[3];
};

class CameraManager
{
public:
    CameraManager(
        core::allocator& alloc,
        iceshard::ecs::ArchetypeIndex& index
    ) noexcept
        : _allocator{ alloc }
        , _index{ index }
        , _camera_query{ _allocator }
    { }

    void set_default_camera(iceshard::Entity entity) noexcept
    {
        _current_camera = entity;
    }

    void update([[maybe_unused]] core::MessageBuffer const& messages, iceshard::renderer::Buffer uniform_buffer) noexcept
    {
        static float rotation_delta = 0.0f;
        core::message::filter<input::message::MouseMotion>(messages, [&](input::message::MouseMotion const& motion) noexcept
            {
                int const ddx = 1280 / 2;
                int const dx = motion.pos.x / ddx;

                if (dx < 1.0)
                {
                    rotation_delta = 3.0f;
                }
                else
                {
                    rotation_delta = -3.0f;
                }
            });

        iceshard::ecs::for_each_entity(
            iceshard::ecs::query_entity(
                _camera_query, _index, _current_camera
            ),
            [&](Position* pos, Camera* camera) noexcept
            {
                glm::mat4 view = glm::lookAt(
                    pos->pos,
                    glm::vec3(0, 0, 0),
                    glm::vec3(0, -1, 0)
                );

                camera->rotation += rotation_delta;
                view = glm::rotate(view, glm::radians(camera->rotation), glm::vec3{ 0.f, 1.0f, 0.f });

                glm::mat4 mvp = CameraManager::Clip * CameraManager::Projection * view;

                iceshard::renderer::api::DataView data_view;
                iceshard::renderer::api::render_api_instance->buffer_array_map_data(&uniform_buffer, &data_view, 1);
                memcpy(data_view.data, &mvp, sizeof(mvp));
                iceshard::renderer::api::render_api_instance->buffer_array_unmap_data(&uniform_buffer, 1);
            }
        );
    }

private:
    core::allocator& _allocator;
    iceshard::ecs::ArchetypeIndex& _index;
    iceshard::ecs::ComponentQuery<Position*, Camera*> _camera_query;

    iceshard::Entity _current_camera;

    static glm::mat4 const Projection;
    static glm::mat4 const Clip;
};


glm::mat4 const CameraManager::Projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);
glm::mat4 const CameraManager::Clip = glm::mat4(
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, -1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.5f, 0.0f,
    0.0f, 0.0f, 0.5f, 1.0f
);

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
        iceshard::ecs::ComponentQuery<Entity*, DebugName*> debug_name_query{ _allocator };

        if (ImGui::Begin("Debug names"))
        {
            iceshard::ecs::for_each_entity(
                iceshard::ecs::query_index(
                    debug_name_query,
                    _arch_index
                ),
                [](Entity* e, DebugName* debug_name) noexcept
                {
                    ImGui::Text("%s <%llu>", core::string::begin(debug_name->name), core::hash(e->e));
                }
            );

            ImGui::End();
        }
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
        asset_system->add_resolver(asset::default_resolver_mesh(alloc));
        asset_system->add_resolver(asset::default_resolver_shader(alloc));
        asset_system->add_loader(asset::AssetType::Mesh, asset::default_loader_mesh(alloc));
        asset_system->add_loader(asset::AssetType::Shader, asset::default_loader_shader(alloc));
        asset_system->update();
        resource_system.flush_messages();

        // Prepare the render system
        auto* render_system = engine_instance->render_system();

        [[maybe_unused]]
        auto uniform_buffer = render_system->create_buffer(iceshard::renderer::api::BufferType::UniformBuffer, sizeof(glm::mat4));

        using iceshard::renderer::RenderResource;
        using iceshard::renderer::RenderResourceType;

        core::pod::Array<RenderResource> resources{ alloc };
        core::pod::array::resize(resources, 1);
        resources[0].type = RenderResourceType::ResUniform;
        resources[0].handle.uniform.buffer = uniform_buffer;
        resources[0].handle.uniform.offset = 0;
        resources[0].handle.uniform.range = sizeof(glm::mat4);
        resources[0].binding = 0;

        [[maybe_unused]]
        auto resource_set = render_system->create_resource_set(
            "test.3d"_sid,
            iceshard::renderer::RenderPipelineLayout::Default,
            resources
        );

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

        auto pp_buff = render_system->create_buffer(BufferType::VertexBuffer, 1024);
        core::pod::Array<Buffer> pp_buffs{ alloc };
        core::pod::array::push_back(pp_buffs, pp_buff);

        auto pp_idx = render_system->create_buffer(BufferType::IndexBuffer, 1024);
        {
            struct PpVertice {
                glm::vec2 pos;
                glm::vec2 uv;
            } vertices[] = {
                { { -1.0f, -1.0f }, { 0.0f, 0.0f, } },
                { { 3.0f, -1.0f }, { 2.0f, 0.0f, } },
                { { -1.0f, 3.0f }, { 0.0f, 2.0f, } },
            };
            uint16_t indices[] = {
                0, 1, 2,
            };

            Buffer bfs[] = {
                pp_buff, pp_idx
            };
            DataView views[core::size(bfs)];

            iceshard::renderer::api::render_api_instance->buffer_array_map_data(bfs, views, core::size(bfs));
            memcpy(views[0].data, vertices, sizeof(vertices));
            memcpy(views[1].data, indices, sizeof(indices));
            iceshard::renderer::api::render_api_instance->buffer_array_unmap_data(bfs, core::size(bfs));
        }

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

        CameraManager camera_manager{ alloc, *arch_idx };
        StaticMeshRenderer static_mesh_render{ alloc, *arch_idx, *render_system };

        iceshard::Entity camera_entity = engine_instance->entity_manager()->create();
        arch_idx->add_component(camera_entity, Position{ { 0.0f, 0.0f, -10.0f } });
        arch_idx->add_component(camera_entity, Camera{ 0.0f });
        camera_manager.set_default_camera(camera_entity);

        core::pod::Array<iceshard::Entity> entities{ alloc };
        engine_instance->entity_manager()->create_many(10, entities);

        arch_idx->add_component(entities[0], Transform{ glm::translate(glm::mat4{ 1 }, glm::vec3{ 0.5f, 0.0f, -1.0f }) });
        arch_idx->add_component(entities[0], DebugName{ "Test entity 1" });
        arch_idx->add_component(entities[1], Transform{ glm::translate(glm::mat4{ 1 }, glm::vec3{ -0.5f, 1.5f, -1.0f }) });
        arch_idx->add_component(entities[1], DebugName{ "Test entity 2" });

        bool quit = false;
        while (quit == false)
        {
            auto& frame = engine_instance->current_frame();

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
                set_viewport(cb, viewport.x, viewport.y);
                set_scissor(cb, viewport.x, viewport.y);
                static_mesh_render.update(cb);
                render_system->submit_command_buffer(cb);
            }

            {
                using iceshard::renderer::RenderPassStage;
                using namespace iceshard::renderer::commands;

                auto cb = render_system->acquire_command_buffer(RenderPassStage::PostProcess);
                bind_pipeline(cb, pp_pipeline);
                bind_resource_set(cb, pp_resource_set);
                bind_index_buffer(cb, pp_idx);
                bind_vertex_buffers(cb, pp_buffs);
                set_viewport(cb, viewport.x, viewport.y);
                set_scissor(cb, viewport.x, viewport.y);
                draw_indexed(cb, 3, 1, 0, 0, 0);
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
                camera_manager.update(frame.messages(), uniform_buffer);
            }

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
