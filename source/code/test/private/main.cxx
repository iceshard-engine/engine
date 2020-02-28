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
#include <iceshard/component/component_block.hxx>
#include <iceshard/component/component_block_operation.hxx>
#include <iceshard/component/component_block_allocator.hxx>
#include <iceshard/renderer/render_pipeline.hxx>
#include <iceshard/renderer/render_resources.hxx>
#include <iceshard/renderer/render_pass.hxx>
#include <iceshard/component/component_archetype.hxx>

#include <debugui/debugui_module.hxx>
#include <debugui/debugui.hxx>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include <imgui/imgui.h>

class DebugNameComponent : public iceshard::ComponentSystem
{
public:
    struct Name
    {
        char buff[32];
    };

    struct Data
    {
        iceshard::ComponentBlock* _block;

        Name* debug_name;
    };

    struct Instance
    {
        uint64_t instance;
        uint32_t block;
        uint32_t index;
    };

    DebugNameComponent(core::allocator& alloc, iceshard::ComponentBlockAllocator* block_alloc) noexcept
        : _instances{ alloc }
        , _data_blocks{ alloc }
        , _raw_block_allocator{ block_alloc }
    {
        Data data;
        data._block = _raw_block_allocator->alloc_block();
        iceshard::component_block_prepare(data._block, data.debug_name);

        core::pod::array::push_back(_data_blocks, data);
    }

    ~DebugNameComponent() noexcept
    {
        for (auto const& block : _data_blocks)
        {
            _raw_block_allocator->release_block(block._block);
        }
    }

    //! \brief The name of the component system.
    auto name() const noexcept -> core::stringid_type override
    {
        return "debug-name"_sid;
    }

    auto lookup(iceshard::Entity, core::stringid_arg_type) const noexcept -> iceshard::ComponentInstance override
    {
        return iceshard::ComponentInstance{ 0 };
    }

    void create(iceshard::Entity entity, core::stringid_arg_type name) noexcept override
    {
        create_internal(entity, name);
    }

    //! \brief Creates a new component instance for the given entity and name.
    auto create_internal(iceshard::Entity entity, core::stringid_arg_type name) noexcept -> Instance
    {
        using namespace core::pod;

        auto entity_hash = core::hash(entity);

        Instance i;
        i.instance = core::hash(name);
        i.block = core::pod::array::size(_data_blocks) - 1;

        // This will ensure we call this function at least twice (if we dont have enough space for the next entity)
        while(iceshard::component_block_insert(_data_blocks[i.block]._block, 1, i.index) == 0)
        {
            Data data;
            data._block = _raw_block_allocator->alloc_block();
            iceshard::component_block_prepare(data._block, data.debug_name);
            core::pod::array::push_back(_data_blocks, data);

            i.block += 1;
        }

        memset(_data_blocks[i.block].debug_name[i.index].buff, '\0', sizeof(Name));

        core::pod::multi_hash::insert(_instances, entity_hash, i);
        fmt::print(stdout, "Entity : added debug-name : {}\n", name);

        return i;
    }

    //! \brief Creates a new component instance for the given entity and name.
    void remove([[maybe_unused]] iceshard::Entity entity, core::stringid_arg_type name) noexcept override
    {
        using namespace core::pod;

        auto entity_hash = core::hash(entity);
        auto name_hash = core::hash(name);

        auto* it = core::pod::multi_hash::find_first(_instances, entity_hash);
        while (it != nullptr && it->value.instance != name_hash)
        {
            it = core::pod::multi_hash::find_next(_instances, it);
        }

        if (it != nullptr)
        {
            Instance i = it->value;
            Data& d = _data_blocks[i.block];

            d._block->_entity_count -= 1;
            if (i.index != d._block->_entity_count)
            {
                d.debug_name[i.index] = d.debug_name[d._block->_entity_count];
            }

            core::pod::multi_hash::remove(_instances, it);
            fmt::print(stdout, "Entity : removed debug-name : {}\n", name);
        }
    }

    void set_debug_name(Instance i, std::string_view str) noexcept
    {
        if (core::pod::array::size(_data_blocks) <= i.block)
        {
            return;
        }

        Data const& block = _data_blocks[i.block];
        IS_ASSERT(
            block._block->_entity_count > i.index,
            "Instance index out-of-bounds in component data block! block:{}/{} index:{}/{}",
            i.block, core::pod::array::size(_data_blocks),
            i.index, block._block->_entity_count
        );

        memcpy(block.debug_name[i.index].buff, str.data(), std::min(str.size(), 31llu));
    }

private:
    core::pod::Hash<Instance> _instances;
    core::pod::Array<Data> _data_blocks;

    iceshard::ComponentBlock* _raw_block = nullptr;
    iceshard::ComponentBlockAllocator* _raw_block_allocator;

public:
    class DebugComponentDebugUI;
};

auto debug_component_factory(core::allocator& alloc, void* userdata) noexcept -> iceshard::ComponentSystem*
{
    return alloc.make<DebugNameComponent>(alloc, reinterpret_cast<iceshard::ComponentBlockAllocator*>(userdata));
}

class DebugNameComponent::DebugComponentDebugUI : public debugui::DebugUI
{
public:
    DebugComponentDebugUI(debugui::debugui_context_handle context, DebugNameComponent* debug_component) noexcept
        : debugui::DebugUI{ context }
        , _debug_component{ debug_component }
    { }

    void end_frame() noexcept override
    {
        static bool shown = true;
        if (ImGui::Begin("Debug Names", &shown))
        {
            ImGui::Separator();
            for (auto const& block : _debug_component->_data_blocks)
            {
                for (uint32_t idx = 0; idx < block._block->_entity_count; ++idx)
                {
                    ImGui::Text("Debug name: %s", block.debug_name[idx].buff);
                }
            }
            ImGui::End();
        }
    }

private:
    DebugNameComponent* _debug_component;
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

        // Create a test world
        auto* world = engine_instance->world_manager()->create_world("test-world"_sid);
        world->add_component_system("debug-name-1"_sid, debug_component_factory, world->service_provider()->component_block_allocator());

        [[maybe_unused]]
        iceshard::ComponentArchetypeIndex arch_index{ alloc, world->service_provider()->component_block_allocator() };

        auto e0 = engine_instance->entity_manager()->create();
        arch_index.add_component(e0, "b"_sid, 4, 4);
        auto e1 = engine_instance->entity_manager()->create();
        arch_index.add_component(e1, "b"_sid, 4, 4);

        auto e = world->entity();
        arch_index.add_component(e, "a"_sid, 4, 4);
        arch_index.add_component(e, "b"_sid, 4, 4);
        arch_index.add_component(e, "c"_sid, 4, 4);

        {
            core::pod::Array<core::stringid_type> components{ alloc };
            core::pod::Array<uint32_t> offsets{ alloc };
            core::pod::Array<uint32_t> counts{ alloc };

            core::pod::Array<iceshard::ComponentBlock*> blocks{ alloc };

            core::pod::array::clear(components);
            core::pod::array::push_back(components, "isc.entity"_sid);
            core::pod::array::push_back(components, "b"_sid);

            arch_index.query_components_instances(components, offsets, blocks, counts);

            uint32_t const offset_stride = core::pod::array::size(components);
            uint32_t offset_base_idx = 0;

            core::pod::Array<void*> pointers{ alloc };
            core::pod::array::resize(pointers, offset_stride);

            uint32_t block_base_index = 0;
            for (uint32_t block_count : counts)
            {
                for (uint32_t block_idx = 0; block_idx < block_count; ++block_idx)
                {
                    iceshard::ComponentBlock* const block = blocks[block_base_index + block_idx];

                    for (uint32_t offset_idx = 0; offset_idx < offset_stride; ++offset_idx)
                    {
                        pointers[offset_idx] = core::memory::utils::pointer_add(block, offsets[offset_base_idx + offset_idx]);
                        fmt::print("{} => {}\n", offset_idx, fmt::ptr(pointers[offset_idx]));
                    }

                    static uint32_t global = 330;
                    for (uint32_t cidx = 0; cidx < block->_entity_count; ++cidx)
                    {
                        *(reinterpret_cast<uint32_t*>(pointers[1]) + cidx) = global++;
                    }
                }

                block_base_index += block_count;
                offset_base_idx += offset_stride;
            }


            arch_index.add_component(e0, "c"_sid, 4, 4);

            core::pod::array::clear(offsets);
            core::pod::array::clear(components);
            core::pod::array::push_back(components, "isc.entity"_sid);
            core::pod::array::push_back(components, "b"_sid);
            iceshard::ComponentBlock* block = nullptr;

            {
                arch_index.query_components_entity(e0, components, offsets, block);

                iceshard::Entity* ep = reinterpret_cast<iceshard::Entity*>(core::memory::utils::pointer_add(block, offsets[0]));
                uint32_t* eval = reinterpret_cast<uint32_t*>(core::memory::utils::pointer_add(block, offsets[1]));
                fmt::print("entity: {} (saved: {})\nval: {}\n", e0, *ep, *eval);
            }

            core::pod::array::clear(offsets);
            {
                arch_index.query_components_entity(e1, components, offsets, block);

                iceshard::Entity* ep = reinterpret_cast<iceshard::Entity*>(core::memory::utils::pointer_add(block, offsets[0]));
                uint32_t* eval = reinterpret_cast<uint32_t*>(core::memory::utils::pointer_add(block, offsets[1]));
                fmt::print("entity: {} (saved: {})\nval: {}\n", e1, *ep, *eval);
            }
        }


        auto* world_service = world->service_provider();
        auto* debug_component = (DebugNameComponent*) world_service->component_system("debug-name-1"_sid);

        auto i0 = debug_component->create_internal(world->entity(), "world-name-0"_sid);
        auto i1 = debug_component->create_internal(world->entity(), "world-name-1"_sid);
        auto i2 = debug_component->create_internal(world->entity(), "world-name-2"_sid);
        auto i3 = debug_component->create_internal(world->entity(), "world-name-3"_sid);
        debug_component->set_debug_name(i0, "asd-0");
        debug_component->set_debug_name(i1, "asd-1");
        debug_component->set_debug_name(i2, "asd-2");
        debug_component->set_debug_name(i3, "asd-3");
        debug_component->remove(world->entity(), "world-name-0"_sid);
        debug_component->remove(world->entity(), "world-name-3"_sid);

        // Debug UI module
        core::memory::unique_pointer<DebugNameComponent::DebugComponentDebugUI> debug_component_ui{ nullptr, { alloc } };
        if constexpr (core::build::is_release == false)
        {
            debug_component_ui = core::memory::make_unique<DebugNameComponent::DebugComponentDebugUI>(alloc,
                debugui_module->context().context_handle(),
                debug_component
            );
            debugui_module->context().register_ui(debug_component_ui.get());
        }

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
        render_system->destroy_pipeline("pp.3d"_sid);
        render_system->destroy_resource_set("pp.3d"_sid);

        debugui_module = nullptr;
    }

    return 0;
}
