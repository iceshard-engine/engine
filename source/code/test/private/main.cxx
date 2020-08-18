#include <core/memory.hxx>
#include <core/allocators/proxy_allocator.hxx>
#include <core/string.hxx>
#include <core/stack_string.hxx>
#include <core/string_view.hxx>
#include <core/pod/array.hxx>
#include <core/collections.hxx>
#include <core/datetime/datetime.hxx>
#include <core/platform/windows.hxx>

#include <core/cexpr/stringid.hxx>
#include <core/scope_exit.hxx>
#include <core/debug/profiler.hxx>
#include <core/pod/hash.hxx>
#include <core/pod/algorithm.hxx>
#include <core/clock.hxx>

#include <resource/uri.hxx>
#include <resource/resource_system.hxx>
#include <resource/modules/dynlib_module.hxx>
#include <resource/modules/filesystem_module.hxx>

#include <core/message/buffer.hxx>
#include <core/message/operations.hxx>

#include <input_system/module.hxx>
#include <input_system/message/app.hxx>
#include <input_system/message/keyboard.hxx>
#include <input_system/message/window.hxx>

#include <asset_system/asset_system.hxx>
#include <asset_system/assets/asset_config.hxx>
#include <asset_system/assets/asset_shader.hxx>
#include <asset_system/assets/asset_mesh.hxx>
#include <asset_system/asset_module.hxx>

#include <fmt/format.h>
#include <application/application.hxx>

#include <iceshard/math.hxx>
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
#include <iceshard/execution.hxx>

#include <iceshard/renderer/render_pipeline.hxx>
#include <iceshard/renderer/render_resources.hxx>
#include <iceshard/renderer/render_pass.hxx>
#include <iceshard/renderer/render_model.hxx>
#include <iceshard/renderer/render_funcs.hxx>
#include <iceshard/renderer/render_commands.hxx>

#include <iceshard/input/input_mouse.hxx>
#include <iceshard/input/input_keyboard.hxx>
#include <iceshard/input/input_controller.hxx>

#include <iceshard/ecs/model.hxx>
#include <iceshard/ecs/transform.hxx>
#include <iceshard/ecs/light.hxx>
#include <iceshard/ecs/camera.hxx>
#include <iceshard/math.hxx>

#include <iceshard/debug/debug_system.hxx>
#include <iceshard/debug/debug_window.hxx>
#include <iceshard/debug/debug_module.hxx>

#include <imgui/imgui.h>

struct DebugName
{
    static constexpr auto identifier = "isc.debug_name"_sid;

    core::StackString<32> name;

    DebugName(core::StringView str) noexcept
        : name{ str }
    { }
};

class DebugNameUI : public iceshard::debug::DebugWindow, public iceshard::debug::DebugModule
{
public:
    DebugNameUI(
        core::allocator& alloc,
        iceshard::ecs::ArchetypeIndex* archetype_index
    ) noexcept
        : iceshard::debug::DebugWindow{ }
        , _allocator{ alloc }
        , _arch_index{ *archetype_index }
    { }

    void on_initialize(iceshard::debug::DebugSystem& ds) noexcept
    {
        ds.register_window("debug-name-ui"_sid, *this);
    }

    void on_deinitialize(iceshard::debug::DebugSystem& ds) noexcept
    {
        ds.unregister_window("debug-name-ui"_sid);
    }

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

int game_main(core::allocator& alloc, resource::ResourceSystem& resource_system)
{
    using resource::URN;
    using resource::URI;

    resource_system.mount(URI{ resource::scheme_file, "../source/data/config.json" });

    auto* engine_module_location = resource_system.find(URN{ "iceshard.dll" });
    IS_ASSERT(engine_module_location != nullptr, "Missing engine module!");

    if (auto engine_module = iceshard::load_engine_module(alloc, engine_module_location->location().path))
    {
        // Default file system mount points
        resource_system.flush_messages();
        resource_system.mount(URI{ resource::scheme_file, "mount.isr" });
        resource_system.mount(URI{ resource::scheme_directory, "../source/data" });

        // Check for an user defined mounting file
        if (auto* mount_resource = resource_system.find(URI{ resource::scheme_file, "mount.isr" }))
        {
            fmt::print("Custom mount resource found: {}\n", mount_resource->location());
        }

        auto engine_instance = engine_module->create_instance(alloc, resource_system);

        // Prepare the asset system
        auto& asset_system = engine_instance->asset_system();

        auto* assimp_module_location = resource_system.find(URN{ "asset_module.dll" });
        auto assimp_module = iceshard::load_asset_module(alloc, assimp_module_location->location().path, asset_system);
        if (assimp_module == nullptr)
        {
            fmt::print("ERROR: Couldn't properly load `asset_module.dll`!\n");
        }

        asset_system.add_resolver(asset::default_resolver_mesh(alloc));
        asset_system.add_resolver(asset::default_resolver_shader(alloc));
        asset_system.add_loader(asset::AssetType::Mesh, asset::default_loader_mesh(alloc));
        asset_system.add_loader(asset::AssetType::Shader, asset::default_loader_shader(alloc));
        asset_system.update();
        resource_system.flush_messages();

        // Prepare the render system
        auto& render_system = engine_instance->render_system();

        using iceshard::renderer::RenderResource;
        using iceshard::renderer::RenderResourceType;

        // Debug UI module
        core::memory::unique_pointer<iceshard::debug::DebugSystemModule> debugui_module{ nullptr, { alloc } };
        core::memory::unique_pointer<iceshard::debug::DebugModule> engine_debugui{ nullptr, { alloc } };
        core::memory::unique_pointer<DebugNameUI> debugname_ui{ nullptr, { alloc } };

        if constexpr (core::build::is_release == false)
        {
            auto* debugui_module_location = resource_system.find(URN{ "debug_module.dll" });
            if (debugui_module_location != nullptr)
            {
                debugui_module = iceshard::debug::load_debug_system_module(
                    alloc,
                    debugui_module_location->location().path,
                    *engine_instance
                );

                engine_debugui = iceshard::debug::load_debug_module_from_handle(
                    alloc, engine_module->native_handle(), debugui_module->debug_system()
                );
            }
        }

        fmt::print("IceShard engine revision: {}\n", engine_instance->revision());

        [[maybe_unused]]
        iceshard::World* world = engine_instance->world_manager().create_world("test-world"_sid);

        auto arch_idx = world->service_provider()->archetype_index();
        arch_idx->add_component(world->entity(), DebugName{ "Test World" });

        if (debugui_module)
        {
            debugname_ui = core::memory::make_unique<DebugNameUI>(alloc,
                alloc,
                arch_idx
            );

            debugui_module->debug_system().register_module(*debugname_ui);
        }

        core::pod::Array<iceshard::Entity> entities{ alloc };
        engine_instance->entity_manager().create_many(10, entities);

        namespace ism = core::math;

        auto pos = ism::translate(
            ism::scale(ism::vec3f{ 10.0f, 0.01f, 10.0f }),
            ism::vec3f{ 0.0f, -1.0f, 0.0f }
        );

        arch_idx->add_component(entities[0], iceshard::component::Transform{ pos });
        arch_idx->add_component(entities[0], iceshard::component::ModelMaterial{ ism::vec4f{ 0.8f, 0.8f, 0.8f, 1.0f } });
        arch_idx->add_component(entities[0], iceshard::component::ModelName{ "mesh/box/box"_sid });

        namespace ism = core::math;
        {
            ism::vec3f cube_positions[] = {
                ism::vec3f{ 0.0f,  0.0f,  0.0f },
                ism::vec3f{ 2.0f,  5.0f, -15.0f },
                ism::vec3f{ -1.5f, -2.2f, -2.5f },
                ism::vec3f{ -3.8f, -2.0f, -12.3f },
                ism::vec3f{ 2.4f, -0.4f, -3.5f },
                ism::vec3f{ -1.7f,  3.0f, -7.5f },
                ism::vec3f{ 1.3f, -2.0f, -2.5f },
                ism::vec3f{ 1.5f,  2.0f, -2.5f },
                ism::vec3f{ 1.5f,  0.2f, -1.5f },
                ism::vec3f{ -1.3f,  1.0f, -1.5f },
            };


            core::pod::Array<iceshard::Entity> cube_entities{ alloc };
            engine_instance->entity_manager().create_many(core::size(cube_positions), cube_entities);

            uint32_t i = 0;
            for (auto const cube_pos : cube_positions)
            {
                static float angle = 0.0f;
                auto model = ism::translate(cube_pos);
                model = ism::scale(model, { 0.5, 0.5, 0.5 });
                model = ism::rotate(model, ism::radians(angle), { 1.f, 0.3f, 0.5f });

                arch_idx->add_component(cube_entities[i], iceshard::component::ModelName{ "mesh/box/box"_sid });
                arch_idx->add_component(cube_entities[i], iceshard::component::Transform{ model });
                arch_idx->add_component(cube_entities[i], iceshard::component::ModelMaterial{
                    ism::vec4f { 0.2, 0.8, 0.4, 1.0 } // model.v[0][1], model.v[1][2], model.v[2][0], 1.0f }
                });

                angle += 20.f;
                i += 1;
            }
        }

        pos = ism::scale(
            ism::translate(ism::vec3f{ 0.5f, 0.5f, 3.0f }),
            ism::vec3f{ 0.04f, 0.04f, 0.04f }
        );

        arch_idx->add_component(entities[8], iceshard::component::Light{ { 0.5f, 0.5f, 3.0f } });
        arch_idx->add_component(entities[8], iceshard::component::ModelName{ "mesh/box/box"_sid });
        arch_idx->add_component(entities[8], iceshard::component::Transform{ pos });
        arch_idx->add_component(entities[8], iceshard::component::ModelMaterial{ ism::vec4f{ 0.8, 0.8, 0.8, 1.0f } });
        arch_idx->add_component(entities[8], DebugName{ "Light" });

        pos = ism::scale(
            ism::translate(ism::vec3f{ 0.5f, 0.5f, 5.0f }),
            ism::vec3f{ 0.04f, 0.04f, 0.04f }
        );

        arch_idx->add_component(entities[9], iceshard::component::Light{ { 0.5f, 0.5f, 5.0f } });
        arch_idx->add_component(entities[9], iceshard::component::ModelName{ "mesh/box/box"_sid });
        arch_idx->add_component(entities[9], iceshard::component::Transform{ pos });
        arch_idx->add_component(entities[9], iceshard::component::ModelMaterial{ ism::vec4f{ 0.8, 0.8, 0.8, 1.0f } });
        arch_idx->add_component(entities[9], DebugName{ "Light" });



        iceshard::Entity camera_entity = engine_instance->entity_manager().create();
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

        static float angles[]{
            2.0f,
            3.0f,
        };

        uint32_t current_angle = 0;
        auto next_angle = [&]() -> float
        {
            current_angle += 1;
            if (current_angle == core::size(angles))
            {
                current_angle = 0;
            }
            return angles[current_angle];
        };


        using iceshard::input::InputID;
        using iceshard::input::InputEvent;
        using iceshard::input::DeviceType;
        using iceshard::input::KeyboardKey;
        using iceshard::input::ControllerInput;
        using iceshard::input::create_inputid;

        auto execution_instance = engine_instance->execution_instance();

        auto light_clock = core::clock::create_clock(execution_instance->engine_clock(), 10.0f);

        bool quit = false;
        while (quit == false)
        {
            auto& frame = execution_instance->current_frame();
            core::clock::update(light_clock);

            for (auto const& input : frame.input_events())
            {
                switch (input.identifier)
                {
                case create_inputid(DeviceType::Controller, ControllerInput::ButtonA):
                case create_inputid(DeviceType::Controller, ControllerInput::ButtonB):
                case create_inputid(DeviceType::Controller, ControllerInput::ButtonX):
                case create_inputid(DeviceType::Controller, ControllerInput::ButtonY):
                    if (input.value.button.state.pressed)
                    {
                        //fmt::print("{} pressed\n", input.identifier);
                    }
                    if (input.value.button.state.clicked)
                    {
                        fmt::print("{} clicked\n", input.identifier);
                    }
                    if (input.value.button.state.repeat > 0)
                    {
                        fmt::print("{} repeated {}\n", input.identifier, input.value.button.state.repeat);
                    }
                    if (input.value.button.state.hold)
                    {
                        fmt::print("{} hold\n", input.identifier);
                    }
                    if (input.value.button.state.released)
                    {
                        fmt::print("{} released\n", input.identifier);
                    }
                default:
                    break;
                }
            }

            using ControllerInput = iceshard::input::ControllerInput;

            iceshard::ecs::for_each_entity(
                iceshard::ecs::query_index(light_query, *arch_idx),
                [&next_angle, &light_clock](iceshard::component::Light* l, iceshard::component::Transform* xform) noexcept
                {
                    ism::mat4 rot_mat = ism::rotate(ism::radians(next_angle() * core::clock::elapsed(light_clock)), ism::vec3f{ 0.0f, 1.0f, 0.0f });
                    ism::vec4f pos = rot_mat * ism::vec4(l->position, 1.0f);

                    l->position = vec3(pos);

                    xform->xform = ism::scale(
                        ism::translate(l->position),
                        ism::vec3f{ 0.04, 0.08, 0.04 }
                    );
                }
            );

            core::message::filter<input::message::AppExit>(frame.messages(), [&quit](auto const&) noexcept
                {
                    quit = true;
                });

            execution_instance->next_frame();
        }

        engine_instance->world_manager().destroy_world("test-world"_sid);

        engine_debugui = nullptr;
        debugui_module = nullptr;
    }

    return 0;
}
