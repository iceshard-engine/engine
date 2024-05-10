#include <ice/devui_module.hxx>
#include <ice/devui_imgui.hxx>
#include <ice/module_query.hxx>
#include <imgui/imgui_internal.h>

namespace ice
{

} // namespace ice::api

namespace ImGui
{

    auto Detail::TempBuffer() noexcept -> char*
    {
        return ImGui::GetCurrentContext()->TempBuffer.Data;
    }

    auto Detail::TempBufferSize() noexcept -> size_t
    {
        return ImGui::GetCurrentContext()->TempBuffer.Size;
    }

    void Detail::TextEx(char const* begin, char const* end) noexcept
    {
        ImGui::TextEx(begin, end, ImGuiTextFlags_NoWidthForLargeClippedText);
    }

} // namespace ImGui
