/// Copyright 2026 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/devui_widgets.hxx>
#include "widgets/devui_colorpicker_oklch.hxx"

bool ImGui::ColorPickerOkLCH(
    ice::String label,
    ice::Color& inout_color,
    ImGui::OkLCHPickerFlags flags,
    ice::Color const* ref_color,
    ice::f32 max_chroma
) noexcept
{
    return ice::colorpicker_oklch(label, inout_color, flags, ref_color, max_chroma);
}
