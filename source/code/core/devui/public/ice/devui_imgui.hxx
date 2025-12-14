/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/string/string.hxx>
#include <ice/assert_core.hxx>
#include <ice/color.hxx>

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

    inline bool Begin(ice::String name, bool* inout_open = nullptr, ImGuiWindowFlags flags = 0) noexcept
    {
        return ImGui::Begin(ice::string::begin(name), inout_open, flags);
    }

    inline bool BeginListBox(ice::String label, ice::vec2f size = {}) noexcept
    {
        return ImGui::BeginListBox(ice::string::begin(label), ImVec2{ size.x, size.y });
    }

    inline void TextUnformatted(ice::String text) noexcept
    {
        ImGui::TextUnformatted(ice::string::begin(text), ice::string::end(text));
    }

    inline bool Selectable(
        ice::String label,
        bool selected = false,
        ImGuiSelectableFlags flags = 0,
        ice::vec2f size = {}
    ) noexcept
    {
        return ImGui::Selectable(ice::begin(label), selected, flags, ImVec2{ size.x, size.y });
    }

    // Extensions

    namespace Detail
    {

        auto TempBuffer() noexcept -> char*;
        auto TempBufferSize() noexcept -> size_t;
        void TextEx(char const* begin, char const* end) noexcept;

    } // namespace Detail

    constexpr auto ToColor(ice::Color<ice::u8> color) noexcept -> ImU32
    {
        return ImU32{ ice::u32(color.a) << 24 | ice::u32(color.b) << 16 | ice::u32(color.g) << 8 | ice::u32(color.r) };
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
    inline void TextColoredT(ice::Color<ice::u8> col, fmt::format_string<Args...> format, Args&&... args) noexcept
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
    inline void TextRightColoredT(ice::Color<ice::u8> col, fmt::format_string<Args...> format, Args&&... args) noexcept
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

} // namespace ImGui
