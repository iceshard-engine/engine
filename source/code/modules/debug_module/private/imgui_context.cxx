#include "imgui_context.hxx"
#include <core/allocator.hxx>

#include <iceshard/engine.hxx>
#include <iceshard/frame.hxx>

namespace iceshard::debug::imgui
{

    namespace detail
    {

        auto create_imgui_context() noexcept -> ImGuiContext*
        {
            auto* current_context = ImGui::GetCurrentContext();
            if (nullptr == current_context)
            {
                current_context = ImGui::CreateContext();
            }
            return current_context;
        }

        void release_debugui_context(ImGuiContext* context) noexcept
        {
            ImGui::DestroyContext(context);
        }

    } // namespace detail


    ImGuiDebugSystem::ImGuiDebugSystem(core::allocator& alloc, iceshard::Engine& engine) noexcept
        : _allocator{ alloc }
        , _imgui_inputs{ nullptr, { _allocator } }
        , _imgui_renderer{ nullptr, { _allocator } }
        , _debug_windows{ _allocator }
    {
        core::pod::hash::reserve(_debug_windows, 64);

        _imgui_inputs = core::memory::make_unique<ImGuiInputs>(
            _allocator,
            ImGui::GetIO(),
            engine.input_system()
        );

        _imgui_renderer = core::memory::make_unique<ImGuiRenderer>(
            _allocator,
            _allocator,
            ImGui::GetIO(),
            engine.asset_system(),
            engine
        );
    }

    ImGuiDebugSystem::~ImGuiDebugSystem() noexcept
    {
        _imgui_renderer = nullptr;
        _imgui_inputs = nullptr;
    }

    void ImGuiDebugSystem::update(Frame& frame, Frame const&) noexcept
    {
        auto const& messages = frame.messages();
        _imgui_inputs->update(frame.input_queue());
        _imgui_inputs->update(messages);

        for (auto const& entry : _debug_windows)
        {
            entry.value->update(frame);
        }

        ImGui::NewFrame();

        for (auto const& entry : _debug_windows)
        {
            entry.value->begin_frame();
        }
    }

    void ImGuiDebugSystem::end_frame(Frame& frame, Frame const&) noexcept
    {
        for (auto const& entry : _debug_windows)
        {
            entry.value->end_frame();
        }

        frame.add_task([]() noexcept -> cppcoro::task<>
            {
                ImGui::EndFrame();
                ImGui::Render();
                co_return;
            }());
    }

    auto ImGuiDebugSystem::render_task_factory() noexcept -> RenderStageTaskFactory*
    {
        return _imgui_renderer.get();
    }

    void ImGuiDebugSystem::add_window(core::stringid_arg_type name, DebugWindow* window) noexcept
    {
        if (auto name_hash = core::hash(name); core::pod::hash::has(_debug_windows, name_hash) == false)
        {
            core::pod::hash::set(_debug_windows, name_hash, window);
        }
    }

    void ImGuiDebugSystem::remove_window(core::stringid_arg_type name) noexcept
    {
        if (auto name_hash = core::hash(name); core::pod::hash::has(_debug_windows, name_hash))
        {
            auto* window = core::pod::hash::get(_debug_windows, name_hash, nullptr);
            core::pod::hash::remove(_debug_windows, name_hash);
        }
    }

    ImGuiModule_DebugSystem::ImGuiModule_DebugSystem(
        core::allocator& alloc,
        iceshard::Engine& engine
    ) noexcept
        : _allocator{ alloc }
        , _engine{ engine }
        , _debug_system{ nullptr, { alloc } }
        , _imgui_context{ detail::create_imgui_context() }
    {
        _debug_system = core::memory::make_unique<ImGuiDebugSystem>(_allocator, _allocator, engine);

        ImGui::NewFrame();

        _engine.services().add_system(
            "isc.system.debug-imgui"_sid,
            _debug_system.get()
        );
    }

    ImGuiModule_DebugSystem::~ImGuiModule_DebugSystem() noexcept
    {
        _engine.services().remove_system(
            "isc.system.debug-imgui"_sid
        );

        _debug_system = nullptr;
        detail::release_debugui_context(_imgui_context);
    }

    void ImGuiModule_DebugSystem::register_module(DebugModule& module) noexcept
    {
        module.on_register(_imgui_handle);
        module.on_initialize(*this);
    }

    void ImGuiModule_DebugSystem::unregister_module(DebugModule& module) noexcept
    {
        module.on_deinitialize(*this);
    }

    void ImGuiModule_DebugSystem::register_window(core::stringid_arg_type name, DebugWindow& window) noexcept
    {
        _debug_system->add_window(name, &window);
    }

    void ImGuiModule_DebugSystem::unregister_window(core::stringid_arg_type name) noexcept
    {
        _debug_system->remove_window(name);
    }

} // namespace debugui::imgui

extern "C"
{
    __declspec(dllexport) auto create_debug_system(
        core::allocator& alloc,
        iceshard::Engine& engine
    ) -> iceshard::debug::DebugSystem*
    {
        return alloc.make<iceshard::debug::imgui::ImGuiModule_DebugSystem>(alloc, engine);
    }

    __declspec(dllexport) void release_debug_system(
        core::allocator& alloc,
        iceshard::debug::DebugSystem* debug_system
    )
    {
         alloc.destroy(debug_system);
    }
}
