/// Copyright 2025 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/heap_string.hxx>
#include <ice/assert_core.hxx>
#include <ice/colors.hxx>
#include <ice/math.hxx>

#ifdef IM_ASSERT
#undef IM_ASSERT
#endif

#define IM_ASSERT(cond) ICE_ASSERT_CORE(cond)
#include <imgui/imgui.h>
#include <fmt/core.h>
#undef assert

namespace ImGui
{

    // Helpers
    namespace Compat
    {

        inline constexpr auto Vec2(ice::vec2f val) noexcept -> ImVec2 { return { val.v[0][0], val.v[0][1] }; };
        inline constexpr auto Vec4(ice::vec4f val) noexcept -> ImVec4 { return { val.v[0][0], val.v[0][1], val.v[0][2], val.v[0][3] }; };

        inline constexpr auto Color(ice::Color color) noexcept -> ImColor
        {
            ice::color::SRGB const srgb = color.gammut_corrected(ice::ColorSpace::SRGB, 0.0f).to_lrgb().to_srgb();
            return ImGui::ColorConvertFloat4ToU32({ srgb.red, srgb.green, srgb.blue, srgb.alpha });
        }

    } // Details

    inline bool Begin(ice::String name, bool* inout_open = nullptr, ImGuiWindowFlags flags = 0) noexcept
    {
        return ImGui::Begin(name.begin(), inout_open, flags);
    }

    inline bool BeginListBox(ice::String label, ice::vec2f size = {}) noexcept
    {
        return ImGui::BeginListBox(label.begin(), Compat::Vec2(size));
    }

    inline void TextUnformatted(ice::String text) noexcept
    {
        ImGui::TextUnformatted(text.begin(), text.end());
    }

    inline bool Selectable(
        ice::String label,
        bool selected = false,
        ImGuiSelectableFlags flags = 0,
        ice::vec2f size = {}
    ) noexcept
    {
        return ImGui::Selectable(label.begin(), selected, flags, Compat::Vec2(size));
    }

    // Extensions

    namespace Detail
    {

        auto TempBuffer() noexcept -> char*;
        auto TempBufferSize() noexcept -> size_t;
        void TextEx(char const* begin, char const* end) noexcept;

    } // namespace Detail

    inline auto ToColor(ice::Color color) noexcept -> ImU32
    {
        return Compat::Color(color);
    }

    constexpr auto ToVec2(ice::vec4f pos) noexcept -> ImVec2
    {
        return { pos.x, pos.y };
    }

    template<typename... Args>
    inline void TextT(fmt::format_string<Args...> format, Args&&... args) noexcept
    {
        fmt::format_to_n_result<char*> result = fmt::format_to_n(
            Detail::TempBuffer(),
            Detail::TempBufferSize(),
            format,
            ice::forward<Args>(args)...
        );
        Detail::TextEx(Detail::TempBuffer(), result.out);
    }

    template<typename... Args>
    inline void TextColoredT(ice::Color col, fmt::format_string<Args...> format, Args&&... args) noexcept
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ToColor(col));
        TextT(format, ice::forward<Args>(args)...);
        ImGui::PopStyleColor();
    }

    template<typename... Args>
    inline void TextRightT(fmt::format_string<Args...> format, Args&&... args) noexcept
    {
        fmt::format_to_n_result<char*> result = fmt::format_to_n(
            Detail::TempBuffer(),
            Detail::TempBufferSize(),
            format,
            ice::forward<Args>(args)...
        );

        ImVec2 const avail = ImGui::GetContentRegionAvail();
        ImVec2 const text_size = ImGui::CalcTextSize(Detail::TempBuffer(), result.out);
        float const cursor_pos = ImGui::GetCursorPosX();
        float const new_cursor_pos = (cursor_pos + avail.x) - (text_size.x);

        ImGui::SetCursorPosX(new_cursor_pos);
        Detail::TextEx(Detail::TempBuffer(), result.out);
    }

    template<typename... Args>
    inline void TextRightColoredT(ice::Color col, fmt::format_string<Args...> format, Args&&... args) noexcept
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ToColor(col));
        TextRightT(format, ice::forward<Args>(args)...);
        ImGui::PopStyleColor();
    }

    bool InputText(
        ice::String label,
        ice::HeapString<>& out_string,
        ImGuiInputTextFlags flags = ImGuiInputTextFlags_None
    ) noexcept;

    bool BeginLargeButton(
        std::string_view label,
        int& inout_status,
        ImVec2 const& size_arg = {0,0},
        ImGuiButtonFlags flags = 0
    ) noexcept;

    void EndLargeButton() noexcept;

    inline void SetCursorScreenPos2(ice::vec2f pos) noexcept
    {
        ImGui::SetCursorScreenPos(Compat::Vec2(pos));
    }

} // namespace ImGui
