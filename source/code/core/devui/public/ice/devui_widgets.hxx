/// Copyright 2026 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/colors.hxx>
#include <ice/string.hxx>
#include <ice/devui_imgui.hxx>

namespace ImGui
{

    enum class OkLCHPickerFlags : ice::u8
    {
        None = 0,
        NoAlpha = 1 << 0,
        // Exclusive flags
        Chroma_NoClip = 1 << 1,
        Chroma_ClipToMax = 1 << 2,
        Chroma_ClipGreedy = 1 << 3, // TODO: No effect
        All = NoAlpha | Chroma_NoClip | Chroma_ClipToMax | Chroma_ClipGreedy,
    };

    bool ColorPickerOkLCH(
        ice::String label,
        ice::Color& inout_color,
        ImGui::OkLCHPickerFlags flags = OkLCHPickerFlags::None,
        ice::Color const* ref_color = nullptr,
        ice::f32 max_chroma = 0.32f // TODO: Convert to HDR'ish flag. (P3/RC2020 colorspace flags?)
    ) noexcept;

} // namespace ImGui
