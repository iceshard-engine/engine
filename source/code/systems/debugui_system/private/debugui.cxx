#include <debugui/debugui.hxx>
#include <core/debug/assert.hxx>
#include <imgui/imgui.h>

namespace debugui
{

    namespace detail
    {

        auto get_imgui_context(debugui_context_handle handle) noexcept -> ImGuiContext*
        {
            return reinterpret_cast<ImGuiContext*>(handle);
        }

    } // namespace detail

    DebugUI::DebugUI(debugui_context_handle context_handle) noexcept
    {
        ImGuiContext* const context = detail::get_imgui_context(context_handle);
        bool const is_same_context = ImGui::GetCurrentContext() == context;
        bool const is_null_context = ImGui::GetCurrentContext() == nullptr;

        IS_ASSERT(is_same_context || is_null_context, "Unexpected ImGui context value!");
        ImGui::SetCurrentContext(context);
    }

    DebugUI::~DebugUI() noexcept = default;

} // namespace debugui

