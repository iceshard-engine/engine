#include <ice/devui_imgui.hxx>
#include <imgui/imgui_internal.h>

namespace ice::detail
{

#if 0
    auto mul_imvec4(ImVec4 val, ice::arr3f right) noexcept -> ImVec4
    {
        return { val.x * right.x, val.y * right.y, val.z * right.z, val.w };
    }

    auto add_imvec4(ImVec4 val, ice::arr3f right) noexcept -> ImVec4
    {
        return { val.x + right.x, val.y + right.y, val.z + right.z, val.w };
    }
#endif

} // namespace ice

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

    bool BeginLargeButton(std::string_view label, int& inout_status, ImVec2 const& size_arg, ImGuiButtonFlags flags) noexcept
    {
        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiContext& g = *GImGui;
        ImGuiStyle const& style = g.Style;
        ImGuiID const id = window->GetID(label.data());

#if 0
        if (inout_status == 0) ImGui::PushStyleColor(ImGuiCol_ChildBg, style.Colors[ImGuiCol_WindowBg]);
        else if (inout_status < 4) ImGui::PushStyleColor(ImGuiCol_ChildBg, style.Colors[ImGuiCol_ButtonActive]);
        else ImGui::PushStyleColor(ImGuiCol_ChildBg, ice::detail::add_imvec4(style.Colors[ImGuiCol_WindowBg], ice::arr3f{0.1f}));
#else
        if (inout_status == 0) ImGui::PushStyleColor(ImGuiCol_ChildBg, style.Colors[ImGuiCol_Button]);
        else if (inout_status < 4) ImGui::PushStyleColor(ImGuiCol_ChildBg, style.Colors[ImGuiCol_ButtonActive]);
        else ImGui::PushStyleColor(ImGuiCol_ChildBg, style.Colors[ImGuiCol_ButtonHovered]);
#endif

        if (ImGui::BeginChild(id, size_arg, ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY))
        {
            ImGui::TextUnformatted(label.data());

            if (ImGui::IsWindowHovered())
            {
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                {
                    inout_status = 1;
                }
                else if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                {
                    inout_status = 3;
                }
                else
                {
                    inout_status = 4;
                }
            }
            else
            {
                inout_status = 0;
            }
            return true;
        }
        inout_status = 0;
        return false;
    }

    void EndLargeButton() noexcept
    {
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

} // namespace ImGui
