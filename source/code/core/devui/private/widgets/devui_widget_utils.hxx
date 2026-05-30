/// Copyright 2026 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/colors.hxx>
#include <ice/devui_imgui.hxx>

namespace ImGui
{

    void PushID(ice::String strid) noexcept;

    constexpr auto Srgb(ice::Color color, bool& is_fallback) noexcept -> ice::color::SRGB
    {
        ice::color::LRGB const lrgb = color.to_lrgb();
        ice::color::LRGB const corrected_lrgb = color.gammut_corrected(ice::ColorSpace::SRGB, 0.0f).to_lrgb();
        //ice::color::LRGB const corrected_lrgb = color.to_lrgb().gammut_clipped();
        if (lrgb.red != corrected_lrgb.red || lrgb.green != corrected_lrgb.green || lrgb.blue != corrected_lrgb.blue)
        {
            is_fallback = true;
            return corrected_lrgb.to_srgb();
        }
        else
        {
            is_fallback = false;
            return lrgb.to_srgb();
        }
    }

    constexpr auto Srgb(ice::Color color) noexcept -> ice::color::SRGB
    {
        bool unused;
        return Srgb(color, unused);
    }

    inline auto SrgbU32(ice::Color color, bool& is_fallback) noexcept -> ImU32
    {
        ice::color::SRGB const rgb = Srgb(color, is_fallback);
        return ImColor{ rgb.red, rgb.green, rgb.blue, rgb.alpha };
    }

    inline auto SrgbU32(ice::Color color) noexcept -> ImU32
    {
        bool unused;
        return SrgbU32(color, unused);
    }

    constexpr auto Vec2(ImVec2 imvec) noexcept -> ice::vec2f
    {
        return { imvec.x, imvec.y };
    }
    constexpr auto Vec2(ice::vec2f imvec) noexcept -> ImVec2
    {
        return { imvec.v[0][0], imvec.v[0][1] };
    }
    constexpr auto Vec4(ice::Color color) noexcept -> ImVec4
    {
        return { color.lightness, color.chroma, color.hue.raw_value(), color.alpha};
    }
    constexpr auto ColorFromVec4(ImVec4 color) noexcept -> ice::Color
    {
        return { color.x, color.y, ice::deg32{ color.z }, color.w };
    }

    void RenderArrowsForVerticalBar2(ImDrawList* draw_list, ImVec2 pos, ImVec2 half_sz, float bar_w, float alpha) noexcept;

} // namespace ImGui
