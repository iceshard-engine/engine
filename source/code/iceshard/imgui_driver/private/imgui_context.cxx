#include "imgui_context.hxx"
#include <core/allocator.hxx>
#include <input_system/keyboard.hxx>

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

    DebugUIContext_ImGui::DebugUIContext_ImGui(
        core::allocator& alloc,
        iceshard::Engine& engine
    ) noexcept
        : _allocator{ alloc }
        , _imgui_context{ detail::create_imgui_context() }
        , _imgui_inputs{ nullptr, { _allocator } }
        , _imgui_renderer{ nullptr, { _allocator } }
        , _debugui_objects{ _allocator }
    {
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

        core::pod::array::reserve(_debugui_objects, 16);

        ImGui::NewFrame();
    }

    DebugUIContext_ImGui::~DebugUIContext_ImGui() noexcept
    {
        _imgui_renderer = nullptr;
        _imgui_inputs = nullptr;
        detail::release_debugui_context(_imgui_context);
    }

    void DebugUIContext_ImGui::register_ui(DebugUI* ui_object) noexcept
    {
        core::pod::array::push_back(_debugui_objects, ui_object);
    }

    void DebugUIContext_ImGui::update(core::MessageBuffer const& messages) noexcept
    {
        _imgui_inputs->update(messages);
        std::for_each(core::pod::begin(_debugui_objects), core::pod::end(_debugui_objects), [&messages](auto* object) noexcept
            {
                object->update(messages);
            });

    }

    void DebugUIContext_ImGui::update(Frame& frame, Frame const&) noexcept
    {
        update(frame.messages());

        ImGui::NewFrame();
        std::for_each(core::pod::begin(_debugui_objects), core::pod::end(_debugui_objects), [](auto* object) noexcept
            {
                object->begin_frame();
            });
    }

    void DebugUIContext_ImGui::end_frame(Frame& frame, Frame const&) noexcept
    {
        std::for_each(core::pod::begin(_debugui_objects), core::pod::end(_debugui_objects), [](auto* object) noexcept
            {
                object->end_frame();
            });

        frame.add_task([]() noexcept -> cppcoro::task<>
            {
                ImGui::EndFrame();
                ImGui::Render();
                co_return;
            }());
    }

    auto DebugUIContext_ImGui::render_task_factory() noexcept -> RenderStageTaskFactory*
    {
        return _imgui_renderer.get();
    }

} // namespace debugui::imgui

extern "C"
{
    __declspec(dllexport) auto create_debugui(
        core::allocator& alloc,
        iceshard::Engine& engine
    ) -> iceshard::debug::imgui::DebugUIContext_ImGui*
    {
        return alloc.make<iceshard::debug::imgui::DebugUIContext_ImGui>(
            alloc,
            engine
        );
    }

    __declspec(dllexport) void release_debugui(
        core::allocator& alloc,
        iceshard::debug::imgui::DebugUIContext_ImGui* driver
    )
    {
         alloc.destroy(driver);
    }
}
