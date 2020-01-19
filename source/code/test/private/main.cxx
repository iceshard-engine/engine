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

    void toggle_show(bool value) noexcept
    {
        _menu_visible = _menu_visible ^ value;
    }

    bool quit_message() noexcept
    {
        return _quit;
    }

    void on_draw() noexcept override
    {
        if (_menu_visible)
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
        render_system->add_named_vertex_descriptor_set(render::descriptor_set::Color);
        render_system->add_named_vertex_descriptor_set(render::descriptor_set::Model);

        // Debug UI module
        core::memory::unique_pointer<debugui::DebugUIModule> debugui_module{ nullptr, { alloc } };
        auto* debugui_module_location = resource_system.find(URN{ "imgui_driver.dll" });
        if (debugui_module_location != nullptr)
        {
            debugui_module = debugui::load_module(alloc, debugui_module_location->location().path, *engine_instance->input_system(), *asset_system, *render_system);
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

        debugui::DebugUIContext& debugui_context = debugui_module->context();
        MainDebugUI main_debug_menu{ debugui_context.context_handle() };

        auto& io = ImGui::GetIO();

        fmt::print("IceShard engine revision: {}\n", engine_instance->revision());

        // Create a test world
        engine_instance->world_manager()->create_world("test-world"_sid);

        bool quit = false;
        while (quit == false)
        {
            core::message::filter<input::message::AppExit>(engine_instance->current_frame().messages(), [&quit](auto const&) noexcept
                {
                    quit = true;
                });

            core::message::for_each(engine_instance->current_frame().messages(), [&](core::Message const& msg) noexcept
                {
                    using input::KeyboardMod;
                    using namespace input::message;

                    if (msg.header.type == KeyboardKeyDown::message_type)
                    {
                        auto const& data = *reinterpret_cast<KeyboardKeyDown const*>(msg.data._data);
                        io.KeysDown[static_cast<uint32_t>(data.key)] = true;

                        main_debug_menu.toggle_show(data.key == input::KeyboardKey::BackQuote);
                        quit = io.KeyCtrl && data.key == input::KeyboardKey::KeyW;
                    }
                    else if (msg.header.type == KeyboardKeyUp::message_type)
                    {
                        auto const& data = *reinterpret_cast<KeyboardKeyUp const*>(msg.data._data);
                        io.KeysDown[static_cast<uint32_t>(data.key)] = false;
                    }
                    else if (msg.header.type == KeyboardModChanged::message_type)
                    {
                        auto const& data = *reinterpret_cast<KeyboardModChanged const*>(msg.data._data);
                        if (has_flag(data.mod, KeyboardMod::ShiftAny))
                        {
                            io.KeyShift = data.pressed;
                        }
                        if (has_flag(data.mod, KeyboardMod::CtrlAny))
                        {
                            io.KeyCtrl = data.pressed;
                        }
                        if (has_flag(data.mod, KeyboardMod::AltAny))
                        {
                            io.KeyAlt = data.pressed;
                        }
                        if (has_flag(data.mod, KeyboardMod::GuiAny))
                        {
                            io.KeySuper = data.pressed;
                        }
                    }
                    else if (msg.header.type == KeyboardTextInput::message_type)
                    {
                        auto const& data = *reinterpret_cast<KeyboardTextInput const*>(msg.data._data);
                        io.AddInputCharactersUTF8(data.text);
                    }
                });

            core::message::for_each(engine_instance->current_frame().messages(), [&](core::Message const& msg) noexcept
                {
                    if (msg.header.type == input::message::MouseMotion::message_type)
                    {
                        auto const& data = *reinterpret_cast<input::message::MouseMotion const*>(msg.data._data);
                        io.MousePos = { (float)data.pos.x, (float)data.pos.y };
                    }
                    else if (msg.header.type == input::message::MouseButtonDown::message_type)
                    {
                        auto const& data = *reinterpret_cast<input::message::MouseButtonDown const*>(msg.data._data);
                        io.MousePos = { (float)data.pos.x, (float)data.pos.y };

                        io.MouseDown[0] = data.button == input::MouseButton::Left; // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
                        io.MouseDown[1] = data.button == input::MouseButton::Right;
                        io.MouseDown[2] = data.button == input::MouseButton::Middle;
                    }
                    else if (msg.header.type == input::message::MouseButtonUp::message_type)
                    {
                        auto const& data = *reinterpret_cast<input::message::MouseButtonUp const*>(msg.data._data);
                        io.MousePos = { (float)data.pos.x, (float)data.pos.y };

                        if (io.MouseDown[0])
                        {
                            io.MouseDown[0] = data.button != input::MouseButton::Left; // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
                        }
                        if (io.MouseDown[1])
                        {
                            io.MouseDown[1] = data.button != input::MouseButton::Right; // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
                        }
                        if (io.MouseDown[2])
                        {
                            io.MouseDown[2] = data.button != input::MouseButton::Middle; // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
                        }
                    }
                    else if (msg.header.type == input::message::MouseWheel::message_type)
                    {
                        auto const& data = *reinterpret_cast<input::message::MouseWheel const*>(msg.data._data);
                        if (data.dy > 0)
                        {
                            io.MouseWheel += 1.0f;
                        }
                        else if (data.dy < 0)
                        {
                            io.MouseWheel -= 1.0f;
                        }
                        else if (data.dx > 0)
                        {
                            io.MouseWheelH += 1.0f;
                        }
                        else if (data.dx < 0)
                        {
                            io.MouseWheelH -= 1.0f;
                        }
                    }
                });

            debugui_context.begin_frame();
            main_debug_menu.on_draw();
            quit |= main_debug_menu.quit_message();
            debugui_context.end_frame();

            engine_instance->next_frame();
        }

        ImGui::EndFrame();

        engine_instance->world_manager()->destroy_world("test-world"_sid);

        debugui_module = nullptr;
    }

    return 0;
}
